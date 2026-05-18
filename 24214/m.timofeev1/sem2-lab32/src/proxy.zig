const std = @import("std");
const types = @import("types.zig");
const net = @import("net.zig");
const cache = @import("cache.zig");
const io = @import("io.zig");
const http_request = @import("http_request.zig");
const upstream = @import("upstream.zig");

const c = @cImport({
    @cInclude("poll.h");
    @cInclude("unistd.h");
    @cInclude("sys/socket.h");
});

fn send_cached_response(client_fd: i32, cache_idx: i32) void {
    const ce_idx: usize = @intCast(cache_idx);
    const ce = &cache.cache_entries[ce_idx];
    if (ce.used and ce.state == .complete and ce.data != null and ce.len > 0) {
        _ = io.send_all(client_fd, ce.data.?[0..ce.len]);
    }
}

fn send_hit_and_release(client_fd: i32, cache_idx: i32) void {
    cache.mutex.unlock();
    send_cached_response(client_fd, cache_idx);
    cache.mutex.lock();
    cache.release_locked(cache_idx);
    cache.mutex.unlock();
}

fn wait_loading_and_send(client_fd: i32, cache_idx: i32) void {
    if (cache.wait_complete_locked(cache_idx)) {
        send_hit_and_release(client_fd, cache_idx);
        return;
    }

    cache.release_locked(cache_idx);
    cache.mutex.unlock();
}

fn fetch_complete_and_send(client_fd: i32, req: http_request.Request, cache_idx: i32) void {
    cache.mutex.unlock();
    const ok = upstream.fetch_to_cache(req, cache_idx);

    cache.mutex.lock();
    if (ok) {
        cache.mark_complete_locked(cache_idx);
        send_hit_and_release(client_fd, cache_idx);
        return;
    }

    cache.mark_failed_locked(cache_idx);
    cache.release_locked(cache_idx);
    cache.mutex.unlock();
}

fn handle_connection(client_fd_arg: i32) void {
    var client_fd = client_fd_arg;
    defer io.close_fd(&client_fd);

    var req_buf: [types.REQUEST_BUF_SIZE]u8 = undefined;
    const req_len = http_request.read(client_fd, &req_buf) orelse return;
    const req = http_request.parse(req_buf[0..req_len]) orelse return;

    const key = std.fmt.allocPrint(types.allocator, "{s}{s}", .{ req.host_header, req.target_path }) catch return;
    defer types.allocator.free(key);

    cache.mutex.lock();
    const lookup = cache.lookup_or_create_locked(key);
    if (lookup.idx < 0 or lookup.state == .failed) {
        cache.mutex.unlock();
        return;
    }

    switch (lookup.state) {
        .hit => {
            _ = std.debug.print("[proxy] cache hit host={s} path={s}\n", .{ req.host_header, req.target_path });
            send_hit_and_release(client_fd, lookup.idx);
        },
        .loading => {
            _ = std.debug.print("[proxy] wait inflight fetch host={s} path={s}\n", .{ req.host_header, req.target_path });
            wait_loading_and_send(client_fd, lookup.idx);
        },
        .created => {
            _ = std.debug.print("[proxy] cache miss host={s} path={s}\n", .{ req.host_header, req.target_path });
            fetch_complete_and_send(client_fd, req, lookup.idx);
        },
        .failed => unreachable,
    }
}

fn accept_one(listen_fd: i32) void {
    const client_fd = c.accept(listen_fd, null, null);
    if (client_fd < 0) return;

    if (net.set_nonblocking(client_fd) != 0) {
        _ = c.close(client_fd);
        return;
    }

    const thread = std.Thread.spawn(.{}, handle_connection, .{client_fd}) catch {
        _ = c.close(client_fd);
        return;
    };
    thread.detach();
}

pub fn run(listen_port: []const u8) void {
    cache.init();

    var listen_fd = net.create_listener(listen_port);
    if (listen_fd < 0) {
        _ = std.debug.print("Failed to create listener on {s}\n", .{listen_port});
        return;
    }
    defer net.close_fd(&listen_fd);

    while (true) {
        if (io.wait_fd(listen_fd, c.POLLIN)) {
            accept_one(listen_fd);
        }
    }
}
