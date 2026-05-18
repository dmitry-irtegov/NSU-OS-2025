const std = @import("std");
const types = @import("types.zig");
const net = @import("net.zig");
const cache = @import("cache.zig");
const io = @import("io.zig");
const http_request = @import("http_request.zig");

const c = @cImport({
    @cInclude("poll.h");
    @cInclude("sys/socket.h");
});

fn connect(req: http_request.Request) i32 {
    const fd = net.connect_nonblocking(req.upstream_host, req.upstream_port);
    if (fd < 0) return -1;

    if (!io.wait_fd(fd, c.POLLOUT)) {
        var close_fd = fd;
        io.close_fd(&close_fd);
        return -1;
    }

    var so_error: c_int = 0;
    var opt_len: c.socklen_t = @sizeOf(c_int);
    const gso = c.getsockopt(fd, c.SOL_SOCKET, c.SO_ERROR, &so_error, &opt_len);
    if (gso < 0 or so_error != 0) {
        var close_fd = fd;
        io.close_fd(&close_fd);
        return -1;
    }

    return fd;
}

pub fn fetch_to_cache(req: http_request.Request, cache_idx: i32) bool {
    var upstream_fd = connect(req);
    if (upstream_fd < 0) return false;
    defer io.close_fd(&upstream_fd);

    const upstream_req = std.fmt.allocPrint(types.allocator, "{s} {s} HTTP/1.0\r\nHost: {s}\r\nConnection: close\r\n\r\n", .{ req.method, req.target_path, req.host_header }) catch return false;
    defer types.allocator.free(upstream_req);

    if (!io.send_all(upstream_fd, upstream_req)) return false;

    while (true) {
        if (!io.wait_fd(upstream_fd, c.POLLIN)) return true;

        var buf: [4096]u8 = undefined;
        const n_raw = c.recv(upstream_fd, buf[0..].ptr, buf.len, 0);
        if (n_raw > 0) {
            const n: usize = @intCast(n_raw);
            cache.mutex.lock();
            const ok = cache.append_to_locked(cache_idx, buf[0..n]);
            cache.mutex.unlock();
            if (!ok) return false;
            continue;
        }
        if (n_raw == 0) return true;
        if (!io.is_would_block(n_raw)) return false;
    }
}
