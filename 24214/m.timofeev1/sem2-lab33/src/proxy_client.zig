const std = @import("std");
const types = @import("types.zig");
const net = @import("net.zig");
const cache = @import("cache.zig");
const connection = @import("connection.zig");
const http_request = @import("http_request.zig");
const proxy_request = @import("proxy_request.zig");

const c = @cImport({
    @cInclude("poll.h");
    @cInclude("unistd.h");
    @cInclude("sys/socket.h");
});

fn handle_client_reading(cidx: usize, rev: c_short, clients: *[types.MAX_CLIENTS]types.Client, fetches: *[types.MAX_FETCHES]types.Fetch, cache_state: *cache.Cache) void {
    if (rev & c.POLLIN == 0) return;

    const cl = &clients[cidx];
    var buf: [1024]u8 = undefined;
    const n_raw = c.recv(cl.fd, buf[0..].ptr, buf.len, 0);
    if (n_raw <= 0) {
        if (n_raw == 0 or !connection.is_would_block(n_raw)) connection.close_client(cidx, clients, cache_state);
        return;
    }

    const n: usize = @intCast(n_raw);
    const slice = buf[0..n];
    const to_copy = if (cl.req_len + n <= cl.req_buf.len) n else cl.req_buf.len - cl.req_len;
    if (to_copy == 0) {
        connection.close_client(cidx, clients, cache_state);
        return;
    }
    std.mem.copyForwards(u8, cl.req_buf[cl.req_len .. cl.req_len + to_copy], slice[0..to_copy]);
    cl.req_len += to_copy;

    const request = http_request.parse(cl.req_buf[0..cl.req_len]) orelse return;
    proxy_request.prepare_fetch_for_client(cidx, request, clients, fetches, cache_state);
}

fn handle_client_sending(cidx: usize, rev: c_short, clients: *[types.MAX_CLIENTS]types.Client, cache_state: *cache.Cache) void {
    if (rev & c.POLLOUT == 0) return;

    const cl = &clients[cidx];
    if (cl.resp_data == null or cl.resp_off >= cl.resp_len) {
        connection.close_client(cidx, clients, cache_state);
        return;
    }

    const data = cl.resp_data.?;
    const rem = data[cl.resp_off..cl.resp_len];
    const sent_raw = c.send(cl.fd, rem.ptr, rem.len, 0);
    if (sent_raw > 0) {
        const sent: usize = @intCast(sent_raw);
        cl.resp_off += sent;
        if (cl.resp_off >= cl.resp_len) connection.close_client(cidx, clients, cache_state);
        return;
    }

    if (sent_raw < 0 and !connection.is_would_block(sent_raw)) connection.close_client(cidx, clients, cache_state);
}

pub fn accept_new_client(listen_fd: i32, clients: *[types.MAX_CLIENTS]types.Client) void {
    var free_idx: ?usize = null;
    for (0..clients.len) |cidx| {
        if (!clients[cidx].used) {
            free_idx = cidx;
            break;
        }
    }
    if (free_idx == null) return;

    const client_fd = c.accept(listen_fd, null, null);
    if (client_fd < 0) return;

    if (net.set_nonblocking(client_fd) != 0) {
        _ = c.close(client_fd);
        return;
    }

    const cl = &clients[free_idx.?];
    cl.*.used = true;
    cl.*.fd = client_fd;
    cl.*.req_len = 0;
    cl.*.state = .READING_REQUEST;
    cl.*.fetch_idx = -1;
    cl.*.waiter_next = -1;
    cl.*.cache_idx = -1;
    cl.*.resp_data = null;
    cl.*.resp_len = 0;
    cl.*.resp_off = 0;
    cl.*.owns_resp = false;
}

pub fn handle_client_event(cidx: usize, rev: c_short, clients: *[types.MAX_CLIENTS]types.Client, fetches: *[types.MAX_FETCHES]types.Fetch, cache_state: *cache.Cache) void {
    const cl = &clients[cidx];
    if (!cl.used or rev == 0) return;

    if (rev & (c.POLLERR | c.POLLHUP | c.POLLNVAL) != 0) {
        connection.close_client(cidx, clients, cache_state);
        return;
    }

    switch (cl.state) {
        .READING_REQUEST => handle_client_reading(cidx, rev, clients, fetches, cache_state),
        .SENDING_RESPONSE => handle_client_sending(cidx, rev, clients, cache_state),
        .WAITING_FOR_FETCH => {},
    }
}
