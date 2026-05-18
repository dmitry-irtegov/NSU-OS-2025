const std = @import("std");

const c = @cImport({
    @cInclude("time.h");
});

pub const LISTEN_BACKLOG = 128;
pub const MAX_CACHE_ENTRIES: usize = 256;
pub const MAX_CACHE_BYTES: usize = 64 * 1024 * 1024;
pub const REQUEST_BUF_SIZE: usize = 32768;
pub const HOST_BUF_SIZE: usize = 512;

pub const CacheState = enum {
    loading,
    complete,
    failed,
};

pub const CacheEntry = struct {
    used: bool,
    state: CacheState,
    key: ?[]const u8,
    data: ?[]u8,
    len: usize,
    cap: usize,
    last_used: c.time_t,
    refcount: i32,
};

pub const allocator = std.heap.c_allocator;
