const std = @import("std");
const types = @import("types.zig");
const cache = @import("cache.zig");
const io = @import("proxy_io.zig");

const c = @cImport({
    @cInclude("poll.h");
    @cInclude("unistd.h");
    @cInclude("sys/socket.h");
});

fn finalize_fetch(fidx: usize, success: bool, clients: *[types.MAX_CLIENTS]types.Client, fetches: *[types.MAX_FETCHES]types.Fetch) void {
    var f = &fetches[fidx];
    if (!f.used) return;

    const cache_idx = f.cache_idx;
    _ = std.debug.print("[proxy] fetch done cache_idx={d} status={s}\n", .{ cache_idx, if (success) "ok" else "fail" });
    if (success and cache_idx >= 0) {
        cache.mark_complete(cache_idx);
    }

    if (f.request_data != null) {
        types.allocator.free(f.request_data.?);
        f.request_data = null;
    }
    if (f.upstream_host != null) {
        types.allocator.free(f.upstream_host.?);
        f.upstream_host = null;
    }
    if (f.fd >= 0) _ = c.close(f.fd);

    f.used = false;
    f.fd = -1;
    f.state = .FETCH_CONNECTING;
    f.cache_idx = -1;
    f.waiter_head = -1;
    f.request_len = 0;
    f.request_sent = 0;

    if (cache_idx >= 0) {
        const ce_idx: usize = @intCast(cache_idx);
        const ce = cache.cache_entries[ce_idx];
        for (0..clients.len) |cidx| {
            const cl = &clients[cidx];
            if (!cl.used) continue;
            if (cl.cache_idx != cache_idx) continue;
            if (cl.state != .WAITING_FOR_FETCH) continue;

            if (success and ce.used and ce.complete and ce.data != null and ce.len > 0) {
                cl.resp_data = ce.data.?;
                cl.resp_len = ce.len;
                cl.resp_off = 0;
                cl.owns_resp = false;
                cl.state = .SENDING_RESPONSE;
                cl.fetch_idx = -1;
            } else {
                io.close_client(cidx, clients);
            }
        }
        cache.release(cache_idx);
    }
}

pub fn handle_fetch_event(fidx: usize, rev: c_short, clients: *[types.MAX_CLIENTS]types.Client, fetches: *[types.MAX_FETCHES]types.Fetch) void {
    const f = &fetches[fidx];
    if (!f.used or f.fd < 0 or rev == 0) return;

    if (rev & (c.POLLERR | c.POLLNVAL) != 0) {
        finalize_fetch(fidx, false, clients, fetches);
        return;
    }

    if (f.state == .FETCH_CONNECTING) {
        if (rev & c.POLLOUT == 0) return;

        var so_error: c_int = 0;
        var opt_len: c.socklen_t = @sizeOf(c_int);
        const gso = c.getsockopt(f.fd, c.SOL_SOCKET, c.SO_ERROR, &so_error, &opt_len);
        if (gso < 0 or so_error != 0) {
            _ = std.debug.print("[proxy] connect failed cache_idx={d} so_error={d}\n", .{ f.cache_idx, so_error });
            finalize_fetch(fidx, false, clients, fetches);
            return;
        }

        f.state = .FETCH_SENDING_REQUEST;
    }

    if (f.state == .FETCH_SENDING_REQUEST and f.request_data != null) {
        if (rev & c.POLLOUT == 0) return;

        const req_data = f.request_data.?;
        if (f.request_sent < f.request_len) {
            const rem = req_data[f.request_sent..f.request_len];
            const sent_raw = c.send(f.fd, rem.ptr, rem.len, 0);
            if (sent_raw > 0) {
                const sent: usize = @intCast(sent_raw);
                f.request_sent += sent;
            } else if (sent_raw < 0 and !io.is_would_block(sent_raw)) {
                finalize_fetch(fidx, false, clients, fetches);
                return;
            }
            if (f.request_sent >= f.request_len) {
                f.state = .FETCH_READING_RESPONSE;
                types.allocator.free(req_data);
                f.request_data = null;
            }
        }
        return;
    }

    if (f.state != .FETCH_READING_RESPONSE) return;
    if (rev & c.POLLIN == 0) {
        if (rev & c.POLLHUP != 0) finalize_fetch(fidx, true, clients, fetches);
        return;
    }

    var buf: [4096]u8 = undefined;
    const n_raw = c.recv(f.fd, buf[0..].ptr, buf.len, 0);
    if (n_raw > 0) {
        const n: usize = @intCast(n_raw);
        if (!cache.append_to(f.cache_idx, buf[0..n])) finalize_fetch(fidx, false, clients, fetches);
        return;
    }

    if (n_raw == 0) {
        finalize_fetch(fidx, true, clients, fetches);
        return;
    }

    if (!io.is_would_block(n_raw)) finalize_fetch(fidx, false, clients, fetches);
}
