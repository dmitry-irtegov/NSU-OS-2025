const std = @import("std");

pub const Request = struct {
    method: []const u8,
    path: []const u8,
    host: []const u8,
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

pub fn parse(req_slice: []const u8) ?Request {
    if (std.mem.indexOf(u8, req_slice, "\r\n\r\n") == null) return null;

    var line_end: usize = 0;
    while (line_end < req_slice.len and req_slice[line_end] != '\n') line_end += 1;

    const first_line = req_slice[0..line_end];
    var parts = std.mem.splitScalar(u8, first_line, ' ');
    const method = parts.next() orelse return null;
    const raw_path = parts.next() orelse return null;
    const path = origin_path_from_target(raw_path);
    const host = find_host_header(req_slice) orelse return null;

    return Request{
        .method = method,
        .path = path,
        .host = host,
    };
}
