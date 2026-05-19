const std = @import("std");
const types = @import("types.zig");
const net = @import("net.zig");
const cache = @import("cache.zig");
const connection = @import("connection.zig");
const http_request = @import("http_request.zig");

pub fn prepare_fetch_for_client(cidx: usize, request: http_request.Request, clients: *[types.MAX_CLIENTS]types.Client, fetches: *[types.MAX_FETCHES]types.Fetch, cache_state: *cache.Cache) void {
    const cl = &clients[cidx];
    var upstream_port: i32 = 80;
    var upstream_host = request.host;

    if (std.mem.indexOfScalar(u8, request.host, ':')) |colon_pos| {
        upstream_host = request.host[0..colon_pos];
        const port_slice = request.host[colon_pos + 1 ..];
        upstream_port = std.fmt.parseInt(i32, port_slice, 10) catch 80;
    }

    const key = std.fmt.allocPrint(types.allocator, "{s}{s}", .{ request.host, request.path }) catch null;
    if (key == null) {
        connection.close_client(cidx, clients, cache_state);
        return;
    }
    defer types.allocator.free(key.?);

    const cache_idx = cache.find_or_create(cache_state, key.?);
    if (cache_idx < 0) {
        connection.close_client(cidx, clients, cache_state);
        return;
    }

    cl.cache_idx = cache_idx;

    if (cache.get_complete_response(cache_state, cache_idx)) |resp| {
        _ = std.debug.print("[proxy] cache hit host={s} path={s} bytes={d}\n", .{ request.host, request.path, resp.len });
        cl.resp_data = resp.data;
        cl.resp_len = resp.len;
        cl.resp_off = 0;
        cl.owns_resp = false;
        cl.state = .SENDING_RESPONSE;
        return;
    }

    _ = std.debug.print("[proxy] cache miss host={s} path={s}\n", .{ request.host, request.path });

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
            const req_copy = std.fmt.allocPrint(types.allocator, "{s} {s} HTTP/1.0\r\nHost: {s}\r\nConnection: close\r\n\r\n", .{ request.method, request.path, request.host }) catch null;
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
            cache.acquire(cache_state, cache_idx);
            _ = std.debug.print("[proxy] start fetch host={s} port={d} path={s} cache_idx={d}\n", .{ request.host, upstream_port, request.path, cache_idx });
            fetch_idx = @intCast(fidx);
            break;
        }
    }

    if (fetch_idx < 0) {
        connection.close_client(cidx, clients, cache_state);
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
    connection.close_client(cidx, clients, cache_state);
}
