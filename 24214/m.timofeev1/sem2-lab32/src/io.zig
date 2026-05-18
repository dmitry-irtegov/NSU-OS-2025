const std = @import("std");

const c = @cImport({
    @cInclude("poll.h");
    @cInclude("unistd.h");
    @cInclude("sys/socket.h");
});

pub fn is_would_block(rc: anytype) bool {
    const err = std.posix.errno(@as(isize, @intCast(rc)));
    return err == .AGAIN;
}

pub fn close_fd(fd_ptr: *i32) void {
    if (fd_ptr.* >= 0) {
        _ = c.close(fd_ptr.*);
        fd_ptr.* = -1;
    }
}

pub fn wait_fd(fd: i32, events: c_short) bool {
    var pfd = c.struct_pollfd{ .fd = fd, .events = events, .revents = 0 };
    while (true) {
        const rc = c.poll(&pfd, 1, -1);
        if (rc < 0) continue;
        if (pfd.revents & events != 0) return true;
        if (pfd.revents & (c.POLLERR | c.POLLHUP | c.POLLNVAL) != 0) return false;
    }
}

pub fn send_all(fd: i32, data: []const u8) bool {
    var off: usize = 0;
    while (off < data.len) {
        if (!wait_fd(fd, c.POLLOUT)) return false;
        const sent_raw = c.send(fd, data[off..].ptr, data.len - off, 0);
        if (sent_raw > 0) {
            off += @intCast(sent_raw);
            continue;
        }
        if (sent_raw < 0 and is_would_block(sent_raw)) continue;
        return false;
    }
    return true;
}
