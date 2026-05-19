const std = @import("std");
const types = @import("types.zig");

const c = @cImport({
    @cInclude("time.h");
});

pub const Cache = struct {
    entries: [types.MAX_CACHE_ENTRIES]types.CacheEntry,
    total_bytes: usize,
    mutex: std.Thread.Mutex,
};

pub const Response = struct {
    data: []u8,
    len: usize,
};

fn now() c.time_t {
    var t: c.time_t = 0;
    _ = c.time(&t);
    return t;
}

pub fn init(cache: *Cache) void {
    cache.total_bytes = 0;
    cache.mutex = std.Thread.Mutex{};
    for (cache.entries[0..]) |*e| {
        e.* = types.CacheEntry{ .used = false, .complete = false, .key = null, .data = null, .len = 0, .cap = 0, .last_used = 0, .refcount = 0 };
    }
}

pub fn find_or_create(cache: *Cache, key: []const u8) i32 {
    cache.mutex.lock();
    defer cache.mutex.unlock();

    var key_copy = std.heap.c_allocator.alloc(u8, key.len) catch return -1;
    std.mem.copyForwards(u8, key_copy[0..key.len], key);

    var free_idx: i32 = -1;
    var evict_idx: i32 = -1;
    var oldest_time: c.time_t = std.math.maxInt(c.time_t);

    for (0..cache.entries.len) |idx| {
        var e = &cache.entries[idx];
        if (e.used) {
            if (e.key != null and std.mem.eql(u8, e.key.?, key)) {
                std.heap.c_allocator.free(key_copy);
                e.refcount += 1;
                e.last_used = now();
                const found_idx: i32 = @intCast(idx);
                return found_idx;
            }
            if (e.complete and e.refcount == 0 and e.last_used < oldest_time) {
                oldest_time = e.last_used;
                evict_idx = @intCast(idx);
            }
        } else if (free_idx == -1) {
            free_idx = @intCast(idx);
        }
    }

    var chosen_idx = free_idx;
    if (chosen_idx < 0) chosen_idx = evict_idx;

    if (chosen_idx < 0) {
        std.heap.c_allocator.free(key_copy);
        return -1;
    }
    const ce_idx: usize = @intCast(chosen_idx);
    var ce = &cache.entries[ce_idx];

    if (ce.used) {
        cache.total_bytes -= ce.len;
        if (ce.key) |k| std.heap.c_allocator.free(k);
        if (ce.data != null) std.heap.c_allocator.free(ce.data.?);
        ce.data = null;
        ce.len = 0;
        ce.cap = 0;
        ce.key = null;
        ce.used = false;
    }

    ce.used = true;
    ce.complete = false;
    ce.key = key_copy[0..key.len];
    ce.data = null;
    ce.len = 0;
    ce.cap = 0;
    ce.last_used = now();
    ce.refcount = 1;
    return chosen_idx;
}

pub fn append_to(cache: *Cache, idx: i32, bytes: []const u8) bool {
    cache.mutex.lock();
    defer cache.mutex.unlock();

    if (idx < 0) return false;
    const ce_idx: usize = @intCast(idx);
    var ce = &cache.entries[ce_idx];
    if (!ce.used) return false;
    const needed = ce.len + bytes.len;
    if (needed > ce.cap) {
        var new_cap = if (ce.cap == 0) (bytes.len) else (ce.cap * 2);
        if (new_cap < needed) new_cap = needed;
        var new_buf = std.heap.c_allocator.alloc(u8, new_cap) catch return false;
        if (ce.data) |old_buf| {
            if (ce.len > 0) std.mem.copyForwards(u8, new_buf[0..ce.len], old_buf[0..ce.len]);
            std.heap.c_allocator.free(old_buf);
        }
        ce.data = new_buf;
        ce.cap = new_cap;
    }
    if (bytes.len > 0) std.mem.copyForwards(u8, ce.data.?[ce.len .. ce.len + bytes.len], bytes);
    ce.len += bytes.len;
    cache.total_bytes += bytes.len;
    return true;
}

pub fn mark_complete(cache: *Cache, idx: i32) void {
    cache.mutex.lock();
    defer cache.mutex.unlock();

    if (idx < 0) return;
    const ce_idx: usize = @intCast(idx);
    var ce = &cache.entries[ce_idx];
    ce.complete = true;
    ce.last_used = now();
}

pub fn acquire(cache: *Cache, idx: i32) void {
    cache.mutex.lock();
    defer cache.mutex.unlock();

    if (idx < 0) return;
    const ce_idx: usize = @intCast(idx);
    var ce = &cache.entries[ce_idx];
    if (!ce.used) return;
    ce.refcount += 1;
    ce.last_used = now();
}

pub fn release(cache: *Cache, idx: i32) void {
    cache.mutex.lock();
    defer cache.mutex.unlock();

    if (idx < 0) return;
    const ce_idx: usize = @intCast(idx);
    var ce = &cache.entries[ce_idx];
    if (!ce.used) return;
    if (ce.refcount > 0) ce.refcount -= 1;
    ce.last_used = now();
}

pub fn get_complete_response(cache: *Cache, idx: i32) ?Response {
    cache.mutex.lock();
    defer cache.mutex.unlock();

    if (idx < 0) return null;
    const ce_idx: usize = @intCast(idx);
    const ce = &cache.entries[ce_idx];
    if (!ce.used or !ce.complete or ce.data == null or ce.len == 0) return null;
    return Response{ .data = ce.data.?, .len = ce.len };
}
