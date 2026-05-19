const std = @import("std");

const c = @cImport({
    @cInclude("time.h");
});

pub const LISTEN_BACKLOG = 128;
pub const MAX_CLIENTS: usize = 1024;
pub const MAX_FETCHES: usize = 256;
pub const MAX_CACHE_ENTRIES: usize = 256;
pub const MAX_CACHE_BYTES: usize = 64 * 1024 * 1024;
pub const REQUEST_BUF_SIZE: usize = 32768;
pub const HOST_BUF_SIZE: usize = 512;
pub const PATH_BUF_SIZE: usize = 4096;

pub const client_state_t = enum(i32) { READING_REQUEST = 0, WAITING_FOR_FETCH = 1, SENDING_RESPONSE = 2 };
pub const fetch_state_t = enum(i32) { FETCH_CONNECTING = 0, FETCH_SENDING_REQUEST = 1, FETCH_READING_RESPONSE = 2 };

pub const CacheEntry = struct {
    used: bool,
    complete: bool,
    key: ?[]const u8,
    data: ?[]u8,
    len: usize,
    cap: usize,
    last_used: c.time_t,
    refcount: i32,
};

pub const Client = struct {
    used: bool,
    fd: i32,
    state: client_state_t,
    req_buf: [REQUEST_BUF_SIZE]u8,
    req_len: usize,
    fetch_idx: i32,
    waiter_next: i32,
    cache_idx: i32,
    resp_data: ?[]u8,
    resp_len: usize,
    resp_off: usize,
    owns_resp: bool,
};

pub const Fetch = struct {
    used: bool,
    fd: i32,
    state: fetch_state_t,
    cache_idx: i32,
    waiter_head: i32,
    upstream_host: ?[]const u8,
    upstream_port: i32,
    request_data: ?[]u8,
    request_len: usize,
    request_sent: usize,
};

pub const allocator = std.heap.c_allocator;
