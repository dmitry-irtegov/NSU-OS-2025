const std = @import("std");
const types = @import("types.zig");
const net = @import("net.zig");
const proxy_client = @import("proxy_client.zig");
const proxy_fetch = @import("proxy_fetch.zig");
const cache = @import("cache.zig");

const c = @cImport({
    @cInclude("poll.h");
});

const WorkerState = struct {
    clients: [types.MAX_CLIENTS]types.Client,
    fetches: [types.MAX_FETCHES]types.Fetch,
    cache_state: *cache.Cache,
};

fn init_state(state: *WorkerState) void {
    for (state.clients[0..]) |*clt| {
        clt.* = types.Client{ .used = false, .fd = -1, .state = .READING_REQUEST, .req_buf = undefined, .req_len = 0, .fetch_idx = -1, .waiter_next = -1, .cache_idx = -1, .resp_data = null, .resp_len = 0, .resp_off = 0, .owns_resp = false };
    }
    for (state.fetches[0..]) |*f| {
        f.* = types.Fetch{ .used = false, .fd = -1, .state = .FETCH_CONNECTING, .cache_idx = -1, .waiter_head = -1, .upstream_host = null, .upstream_port = 80, .request_data = null, .request_len = 0, .request_sent = 0 };
    }
}

fn worker_loop(listen_fd: i32, state: *WorkerState, worker_id: usize) void {
    var pollfds = std.heap.c_allocator.alloc(c.struct_pollfd, 1 + types.MAX_CLIENTS + types.MAX_FETCHES) catch return;
    defer std.heap.c_allocator.free(pollfds);

    _ = std.debug.print("[proxy] worker {d} started\n", .{worker_id});

    while (true) {
        var client_revents = [_]c_short{0} ** types.MAX_CLIENTS;
        var fetch_revents = [_]c_short{0} ** types.MAX_FETCHES;

        var nfds: usize = 0;
        pollfds[nfds].fd = listen_fd;
        pollfds[nfds].events = c.POLLIN;
        pollfds[nfds].revents = 0;
        nfds += 1;

        for (0..state.clients.len) |i| {
            const cl = &state.clients[i];
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

        for (0..state.fetches.len) |fidx| {
            const f = &state.fetches[fidx];
            if (!f.used) continue;
            pollfds[nfds].fd = f.fd;
            pollfds[nfds].events = if (f.state == .FETCH_READING_RESPONSE) c.POLLIN else c.POLLOUT;
            pollfds[nfds].revents = 0;
            nfds += 1;
        }

        const rc = c.poll(pollfds.ptr, @intCast(nfds), 1000);
        if (rc < 0) continue;

        var idx: usize = 1;
        for (0..state.clients.len) |cidx| {
            if (!state.clients[cidx].used) continue;
            client_revents[cidx] = pollfds[idx].revents;
            idx += 1;
        }
        for (0..state.fetches.len) |fidx| {
            if (!state.fetches[fidx].used) continue;
            fetch_revents[fidx] = pollfds[idx].revents;
            idx += 1;
        }

        if (pollfds[0].revents & c.POLLIN != 0) {
            proxy_client.accept_new_client(listen_fd, &state.clients);
        }

        for (0..state.clients.len) |cidx| {
            proxy_client.handle_client_event(cidx, client_revents[cidx], &state.clients, &state.fetches, state.cache_state);
        }

        for (0..state.fetches.len) |fidx| {
            proxy_fetch.handle_fetch_event(fidx, fetch_revents[fidx], &state.clients, &state.fetches, state.cache_state);
        }
    }
}

pub fn run(listen_port: []const u8, worker_count: usize) void {
    var listen_fd = net.create_listener(listen_port);
    if (listen_fd < 0) {
        _ = std.debug.print("Failed to create listener on {s}\n", .{listen_port});
        return;
    }
    defer net.close_fd(&listen_fd);

    const states = std.heap.c_allocator.alloc(WorkerState, worker_count) catch {
        _ = std.debug.print("Failed to allocate worker state\n", .{});
        return;
    };
    defer std.heap.c_allocator.free(states);

    var shared_cache: cache.Cache = undefined;
    cache.init(&shared_cache);

    const threads = std.heap.c_allocator.alloc(std.Thread, worker_count) catch {
        _ = std.debug.print("Failed to allocate worker threads\n", .{});
        return;
    };
    defer std.heap.c_allocator.free(threads);

    for (0..worker_count) |idx| {
        init_state(&states[idx]);
        states[idx].cache_state = &shared_cache;
        threads[idx] = std.Thread.spawn(.{}, worker_loop, .{ listen_fd, &states[idx], idx }) catch {
            _ = std.debug.print("Failed to spawn worker {d}\n", .{idx});
            return;
        };
    }

    _ = std.debug.print("[proxy] listening on {s} with {d} worker threads\n", .{ listen_port, worker_count });
    for (0..worker_count) |idx| {
        threads[idx].join();
    }
}
