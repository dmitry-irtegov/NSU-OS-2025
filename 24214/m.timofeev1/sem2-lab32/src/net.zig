const std = @import("std");
const types = @import("types.zig");

const c = @cImport({
    @cInclude("poll.h");
    @cInclude("fcntl.h");
    @cInclude("netdb.h");
    @cInclude("sys/socket.h");
    @cInclude("unistd.h");
    @cInclude("errno.h");
    @cInclude("string.h");
});

pub fn close_fd(fd_ptr: *i32) void {
    if (fd_ptr.* >= 0) {
        _ = c.close(fd_ptr.*);
        fd_ptr.* = -1;
    }
}

pub fn set_nonblocking(fd: i32) i32 {
    const flags = c.fcntl(fd, c.F_GETFL, @as(c_int, 0));
    if (flags < 0) return -1;
    if (c.fcntl(fd, c.F_SETFL, flags | c.O_NONBLOCK) < 0) return -1;
    return 0;
}

fn to_cstr(buf: []u8, s: []const u8) void {
    const n = if (s.len < buf.len - 1) s.len else buf.len - 1;
    std.mem.copyForwards(u8, buf[0..n], s[0..n]);
    buf[n] = 0;
}

pub fn create_listener(port: []const u8) i32 {
    var hints = std.mem.zeroes(c.addrinfo);
    hints.ai_family = c.AF_UNSPEC;
    hints.ai_socktype = c.SOCK_STREAM;
    hints.ai_flags = c.AI_PASSIVE;

    var port_buf: [32]u8 = undefined;
    to_cstr(port_buf[0..], port);

    var res: ?*c.addrinfo = null;
    const gai_rc = c.getaddrinfo(null, port_buf[0..].ptr, &hints, &res);
    if (gai_rc != 0) {
        return -1;
    }

    var listen_fd: i32 = -1;
    var it = res;
    while (it) |ai| {
        const fd = c.socket(ai.*.ai_family, ai.*.ai_socktype, ai.*.ai_protocol);
        if (fd < 0) {
            it = ai.*.ai_next;
            continue;
        }
        var yes: i32 = 1;
        _ = c.setsockopt(fd, c.SOL_SOCKET, c.SO_REUSEADDR, &yes, @sizeOf(i32));
        if (c.bind(fd, ai.*.ai_addr, ai.*.ai_addrlen) == 0 and c.listen(fd, types.LISTEN_BACKLOG) == 0) {
            if (set_nonblocking(fd) == 0) {
                listen_fd = fd;
                break;
            }
        }
        _ = c.close(fd);
        it = ai.*.ai_next;
    }
    _ = c.freeaddrinfo(res);
    return listen_fd;
}

pub fn connect_nonblocking(host: []const u8, port: i32) i32 {
    var hints = std.mem.zeroes(c.addrinfo);
    hints.ai_socktype = c.SOCK_STREAM;
    hints.ai_family = c.AF_INET;

    var port_str_buf: [16]u8 = undefined;
    const port_str = std.fmt.bufPrint(&port_str_buf, "{}", .{port}) catch return -1;
    if (port_str.len >= port_str_buf.len) return -1;
    port_str_buf[port_str.len] = 0;

    var host_buf: [types.HOST_BUF_SIZE]u8 = undefined;
    to_cstr(host_buf[0..], host);

    var res: ?*c.addrinfo = null;
    const gai_rc = c.getaddrinfo(host_buf[0..].ptr, port_str_buf[0..].ptr, &hints, &res);
    if (gai_rc != 0) return -1;

    var fd: i32 = -1;
    var it = res;
    while (it) |ai| {
        fd = c.socket(ai.*.ai_family, ai.*.ai_socktype, ai.*.ai_protocol);
        if (fd < 0) {
            it = ai.*.ai_next;
            continue;
        }

        if (set_nonblocking(fd) < 0) {
            _ = c.close(fd);
            fd = -1;
            it = ai.*.ai_next;
            continue;
        }

        const conn_rc = c.connect(fd, ai.*.ai_addr, ai.*.ai_addrlen);
        if (conn_rc == 0 or std.posix.errno(conn_rc) == .INPROGRESS) {
            break;
        }

        _ = c.close(fd);
        fd = -1;
        it = ai.*.ai_next;
    }
    _ = c.freeaddrinfo(res);
    return fd;
}
