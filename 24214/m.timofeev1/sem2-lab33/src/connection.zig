const std = @import("std");
const types = @import("types.zig");
const cache = @import("cache.zig");

const c = @cImport({
    @cInclude("unistd.h");
});

pub fn is_would_block(rc: anytype) bool {
    const err = std.posix.errno(@as(isize, @intCast(rc)));
    return err == .AGAIN;
}

pub fn close_client(cidx: usize, clients: *[types.MAX_CLIENTS]types.Client, cache_state: *cache.Cache) void {
    var cl = &clients[cidx];
    if (!cl.used) return;

    if (cl.fd >= 0) _ = c.close(cl.fd);
    if (cl.cache_idx >= 0) {
        cache.release(cache_state, cl.cache_idx);
    }

    if (cl.owns_resp and cl.resp_data != null) {
        types.allocator.free(cl.resp_data.?);
    }

    cl.used = false;
    cl.fd = -1;
    cl.state = .READING_REQUEST;
    cl.req_len = 0;
    cl.fetch_idx = -1;
    cl.waiter_next = -1;
    cl.cache_idx = -1;
    cl.resp_data = null;
    cl.resp_len = 0;
    cl.resp_off = 0;
    cl.owns_resp = false;
}
