const std = @import("std");
const types = @import("types.zig");
const net = @import("net.zig");
const proxy_client = @import("proxy_client.zig");
const proxy_fetch = @import("proxy_fetch.zig");
const cache = @import("cache.zig");

const c = @cImport({
    @cInclude("poll.h");
});

pub var clients: [types.MAX_CLIENTS]types.Client = undefined;
pub var fetches: [types.MAX_FETCHES]types.Fetch = undefined;

fn init_state() void {
    for (clients[0..]) |*clt| {
        clt.* = types.Client{ .used = false, .fd = -1, .state = .READING_REQUEST, .req_buf = undefined, .req_len = 0, .fetch_idx = -1, .waiter_next = -1, .cache_idx = -1, .resp_data = null, .resp_len = 0, .resp_off = 0, .owns_resp = false };
    }
    for (fetches[0..]) |*f| {
        f.* = types.Fetch{ .used = false, .fd = -1, .state = .FETCH_CONNECTING, .cache_idx = -1, .waiter_head = -1, .upstream_host = null, .upstream_port = 80, .request_data = null, .request_len = 0, .request_sent = 0 };
    }
    cache.init();
}

pub fn run(listen_port: []const u8) void {
    init_state();
    var listen_fd = net.create_listener(listen_port);
    if (listen_fd < 0) {
        _ = std.debug.print("Failed to create listener on {s}\n", .{listen_port});
        return;
    }
    defer net.close_fd(&listen_fd);

    var pollfds = std.heap.c_allocator.alloc(c.struct_pollfd, 1 + types.MAX_CLIENTS + types.MAX_FETCHES) catch return;
    defer std.heap.c_allocator.free(pollfds);

    while (true) {
        var client_revents = [_]c_short{0} ** types.MAX_CLIENTS;
        var fetch_revents = [_]c_short{0} ** types.MAX_FETCHES;

        var nfds: usize = 0;
        pollfds[nfds].fd = listen_fd;
        pollfds[nfds].events = c.POLLIN;
        pollfds[nfds].revents = 0;
        nfds += 1;

        for (0..clients.len) |i| {
            const cl = &clients[i];
            if (!cl.used) continue;
            pollfds[nfds].fd = cl.fd;
            pollfds[nfds].events = switch (cl.state) {
                .READING_REQUEST => c.POLLIN,
                .WAITING_FOR_FETCH => c.POLLIN,
                .SENDING_RESPONSE => c.POLLOUT,
            };
            pollfds[nfds].revents = 0;
            nfds += 1;
        }

        for (0..fetches.len) |fidx| {
            const f = &fetches[fidx];
            if (!f.used) continue;
            pollfds[nfds].fd = f.fd;
            pollfds[nfds].events = if (f.state == .FETCH_READING_RESPONSE) c.POLLIN else c.POLLOUT;
            pollfds[nfds].revents = 0;
            nfds += 1;
        }

        const rc = c.poll(pollfds.ptr, @intCast(nfds), 1000);
        if (rc < 0) continue;

        var idx: usize = 1;
        for (0..clients.len) |cidx| {
            if (!clients[cidx].used) continue;
            client_revents[cidx] = pollfds[idx].revents;
            idx += 1;
        }
        for (0..fetches.len) |fidx| {
            if (!fetches[fidx].used) continue;
            fetch_revents[fidx] = pollfds[idx].revents;
            idx += 1;
        }

        if (pollfds[0].revents & c.POLLIN != 0) {
            proxy_client.accept_new_client(listen_fd, &clients);
        }

        for (0..clients.len) |cidx| {
            proxy_client.handle_client_event(cidx, client_revents[cidx], &clients, &fetches);
        }

        for (0..fetches.len) |fidx| {
            proxy_fetch.handle_fetch_event(fidx, fetch_revents[fidx], &clients, &fetches);
        }
    }
}
