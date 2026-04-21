const std = @import("std");
const types = @import("types.zig");
const net = @import("net.zig");
const cache = @import("cache.zig");
const io = @import("proxy_io.zig");

const c = @cImport({
    @cInclude("poll.h");
    @cInclude("unistd.h");
    @cInclude("sys/socket.h");
});

fn find_host_header(req_slice: []const u8) ?[]const u8 {
    var i: usize = 0;
    while (i + 6 < req_slice.len) {
        if (std.mem.eql(u8, req_slice[i .. i + 6], "Host: ")) {
            const j = i + 6;
            var k = j;
            while (k < req_slice.len and req_slice[k] != '\r' and req_slice[k] != '\n') k += 1;
            return req_slice[j..k];
        }
        i += 1;
    }
    return null;
}

fn prepare_fetch_for_client(cidx: usize, method: []const u8, target_path: []const u8, host_header: []const u8, clients: *[types.MAX_CLIENTS]types.Client, fetches: *[types.MAX_FETCHES]types.Fetch) void {
    const cl = &clients[cidx];
    var upstream_port: i32 = 80;
    var upstream_host = host_header;

    if (std.mem.indexOfScalar(u8, host_header, ':')) |colon_pos| {
        upstream_host = host_header[0..colon_pos];
        const port_slice = host_header[colon_pos + 1 ..];
        upstream_port = std.fmt.parseInt(i32, port_slice, 10) catch 80;
    }

    const key = std.fmt.allocPrint(types.allocator, "{s}{s}", .{ host_header, target_path }) catch null;
    if (key == null) {
        io.close_client(cidx, clients);
        return;
    }
    defer types.allocator.free(key.?);

    const cache_idx = cache.find_or_create(key.?);
    if (cache_idx < 0) {
        io.close_client(cidx, clients);
        return;
    }

    cl.cache_idx = cache_idx;

    const ce_idx: usize = @intCast(cache_idx);
    const ce = cache.cache_entries[ce_idx];
    if (ce.used and ce.complete and ce.data != null and ce.len > 0) {
        _ = std.debug.print("[proxy] cache hit host={s} path={s} bytes={d}\n", .{ host_header, target_path, ce.len });
        cl.resp_data = ce.data.?;
        cl.resp_len = ce.len;
        cl.resp_off = 0;
        cl.owns_resp = false;
        cl.state = .SENDING_RESPONSE;
        return;
    }

    _ = std.debug.print("[proxy] cache miss host={s} path={s}\n", .{ host_header, target_path });

    var existing_fetch_idx: i32 = -1;
    for (0..fetches.len) |fidx| {
        const f = &fetches[fidx];
        if (f.used and f.cache_idx == cache_idx) {
            existing_fetch_idx = @intCast(fidx);
            break;
        }
    }
    if (existing_fetch_idx >= 0) {
        _ = std.debug.print("[proxy] join inflight fetch cache_idx={d}\n", .{cache_idx});
        cl.fetch_idx = existing_fetch_idx;
        cl.state = .WAITING_FOR_FETCH;
        return;
    }

    var fetch_idx: i32 = -1;
    for (0..fetches.len) |fidx| {
        const f = &fetches[fidx];
        if (!f.used) {
            const req_copy = std.fmt.allocPrint(types.allocator, "{s} {s} HTTP/1.0\r\nHost: {s}\r\nConnection: close\r\n\r\n", .{ method, target_path, host_header }) catch null;
            if (req_copy == null) break;

            const host_copy = types.allocator.alloc(u8, upstream_host.len) catch null;
            if (host_copy == null) {
                types.allocator.free(req_copy.?);
                break;
            }
            std.mem.copyForwards(u8, host_copy.?[0..upstream_host.len], upstream_host);

            f.*.used = true;
            f.*.cache_idx = cache_idx;
            f.*.upstream_host = host_copy.?;
            f.*.upstream_port = upstream_port;
            f.*.request_data = req_copy.?;
            f.*.request_len = req_copy.?.len;
            f.*.request_sent = 0;
            f.*.waiter_head = @intCast(cidx);
            cache.acquire(cache_idx);
            _ = std.debug.print("[proxy] start fetch host={s} port={d} path={s} cache_idx={d}\n", .{ host_header, upstream_port, target_path, cache_idx });
            fetch_idx = @intCast(fidx);
            break;
        }
    }

    if (fetch_idx < 0) {
        io.close_client(cidx, clients);
        return;
    }

    cl.fetch_idx = fetch_idx;
    cl.state = .WAITING_FOR_FETCH;
    const fi: usize = @intCast(fetch_idx);
    const fd = net.connect_nonblocking(fetches[fi].upstream_host.?, fetches[fi].upstream_port);
    if (fd >= 0) {
        fetches[fi].fd = fd;
        fetches[fi].state = .FETCH_CONNECTING;
        return;
    }

    if (fetches[fi].request_data != null) {
        types.allocator.free(fetches[fi].request_data.?);
        fetches[fi].request_data = null;
    }
    if (fetches[fi].upstream_host != null) {
        types.allocator.free(fetches[fi].upstream_host.?);
        fetches[fi].upstream_host = null;
    }
    fetches[fi].used = false;
    _ = std.debug.print("Failed to connect to upstream {s}:{d}\n", .{ upstream_host, upstream_port });
    io.close_client(cidx, clients);
}

fn handle_client_reading(cidx: usize, rev: c_short, clients: *[types.MAX_CLIENTS]types.Client, fetches: *[types.MAX_FETCHES]types.Fetch) void {
    if (rev & c.POLLIN == 0) return;

    const cl = &clients[cidx];
    var buf: [1024]u8 = undefined;
    const n_raw = c.recv(cl.fd, buf[0..].ptr, buf.len, 0);
    if (n_raw <= 0) {
        if (n_raw == 0 or !io.is_would_block(n_raw)) io.close_client(cidx, clients);
        return;
    }

    const n: usize = @intCast(n_raw);
    const slice = buf[0..n];
    const to_copy = if (cl.req_len + n <= cl.req_buf.len) n else cl.req_buf.len - cl.req_len;
    if (to_copy == 0) {
        io.close_client(cidx, clients);
        return;
    }
    std.mem.copyForwards(u8, cl.req_buf[cl.req_len .. cl.req_len + to_copy], slice[0..to_copy]);
    cl.req_len += to_copy;

    if (std.mem.indexOf(u8, cl.req_buf[0..cl.req_len], "\r\n\r\n") == null) return;

    var line_end: usize = 0;
    while (line_end < cl.req_len and cl.req_buf[line_end] != '\n') line_end += 1;
    const first_line = cl.req_buf[0..line_end];
    var parts = std.mem.splitScalar(u8, first_line, ' ');
    const method = parts.next();
    const path = parts.next();
    if (method == null or path == null) return;

    const req_slice = cl.req_buf[0..cl.req_len];
    const host_header = find_host_header(req_slice) orelse return;
    prepare_fetch_for_client(cidx, method.?, path.?, host_header, clients, fetches);
}

fn handle_client_sending(cidx: usize, rev: c_short, clients: *[types.MAX_CLIENTS]types.Client) void {
    if (rev & c.POLLOUT == 0) return;

    const cl = &clients[cidx];
    if (cl.resp_data == null or cl.resp_off >= cl.resp_len) {
        io.close_client(cidx, clients);
        return;
    }

    const data = cl.resp_data.?;
    const rem = data[cl.resp_off..cl.resp_len];
    const sent_raw = c.send(cl.fd, rem.ptr, rem.len, 0);
    if (sent_raw > 0) {
        const sent: usize = @intCast(sent_raw);
        cl.resp_off += sent;
        if (cl.resp_off >= cl.resp_len) io.close_client(cidx, clients);
        return;
    }

    if (sent_raw < 0 and !io.is_would_block(sent_raw)) io.close_client(cidx, clients);
}

pub fn accept_new_client(listen_fd: i32, clients: *[types.MAX_CLIENTS]types.Client) void {
    const client_fd = c.accept(listen_fd, null, null);
    if (client_fd < 0) return;

    if (net.set_nonblocking(client_fd) != 0) {
        _ = c.close(client_fd);
        return;
    }

    for (0..clients.len) |cidx| {
        const cl = &clients[cidx];
        if (!cl.used) {
            cl.*.used = true;
            cl.*.fd = client_fd;
            cl.*.req_len = 0;
            cl.*.state = .READING_REQUEST;
            cl.*.fetch_idx = -1;
            cl.*.cache_idx = -1;
            cl.*.resp_data = null;
            cl.*.resp_len = 0;
            cl.*.resp_off = 0;
            cl.*.owns_resp = false;
            return;
        }
    }

    _ = c.close(client_fd);
}

pub fn handle_client_event(cidx: usize, rev: c_short, clients: *[types.MAX_CLIENTS]types.Client, fetches: *[types.MAX_FETCHES]types.Fetch) void {
    const cl = &clients[cidx];
    if (!cl.used or rev == 0) return;

    if (rev & (c.POLLERR | c.POLLHUP | c.POLLNVAL) != 0) {
        io.close_client(cidx, clients);
        return;
    }

    switch (cl.state) {
        .READING_REQUEST => handle_client_reading(cidx, rev, clients, fetches),
        .SENDING_RESPONSE => handle_client_sending(cidx, rev, clients),
        .WAITING_FOR_FETCH => {},
    }
}
