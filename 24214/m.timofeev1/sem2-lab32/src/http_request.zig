const std = @import("std");
const types = @import("types.zig");
const io = @import("io.zig");

const c = @cImport({
    @cInclude("poll.h");
    @cInclude("sys/socket.h");
});

pub const Request = struct {
    method: []const u8,
    target_path: []const u8,
    host_header: []const u8,
    upstream_host: []const u8,
    upstream_port: i32,
};

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

fn origin_path_from_target(target: []const u8) []const u8 {
    if (std.mem.startsWith(u8, target, "http://")) {
        const rest = target["http://".len..];
        if (std.mem.indexOfScalar(u8, rest, '/')) |slash_pos| {
            return rest[slash_pos..];
        }
        return "/";
    }
    return target;
}

pub fn parse(req_buf: []const u8) ?Request {
    var line_end: usize = 0;
    while (line_end < req_buf.len and req_buf[line_end] != '\n') line_end += 1;
    const first_line = req_buf[0..line_end];

    var parts = std.mem.splitScalar(u8, first_line, ' ');
    const method = parts.next() orelse return null;
    const raw_path = parts.next() orelse return null;
    const path = origin_path_from_target(raw_path);
    const host_header = find_host_header(req_buf) orelse return null;

    var upstream_port: i32 = 80;
    var upstream_host = host_header;
    if (std.mem.indexOfScalar(u8, host_header, ':')) |colon_pos| {
        upstream_host = host_header[0..colon_pos];
        upstream_port = std.fmt.parseInt(i32, host_header[colon_pos + 1 ..], 10) catch 80;
    }

    return Request{
        .method = method,
        .target_path = path,
        .host_header = host_header,
        .upstream_host = upstream_host,
        .upstream_port = upstream_port,
    };
}

pub fn read(client_fd: i32, req_buf: *[types.REQUEST_BUF_SIZE]u8) ?usize {
    var req_len: usize = 0;
    while (req_len < req_buf.len) {
        if (!io.wait_fd(client_fd, c.POLLIN)) return null;

        var buf: [1024]u8 = undefined;
        const n_raw = c.recv(client_fd, buf[0..].ptr, buf.len, 0);
        if (n_raw <= 0) {
            if (n_raw < 0 and io.is_would_block(n_raw)) continue;
            return null;
        }

        const n: usize = @intCast(n_raw);
        if (req_len + n > req_buf.len) return null;
        std.mem.copyForwards(u8, req_buf[req_len .. req_len + n], buf[0..n]);
        req_len += n;

        if (std.mem.indexOf(u8, req_buf[0..req_len], "\r\n\r\n") != null) return req_len;
    }
    return null;
}
