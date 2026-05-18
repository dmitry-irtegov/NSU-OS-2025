const std = @import("std");
const types = @import("types.zig");

const c = @cImport({
    @cInclude("time.h");
});

pub var cache_entries: [types.MAX_CACHE_ENTRIES]types.CacheEntry = undefined;
pub var cache_total_bytes: usize = 0;
pub var mutex = std.Thread.Mutex{};
pub var changed = std.Thread.Condition{};

pub const LookupState = enum {
    hit,
    loading,
    created,
    failed,
};

pub const LookupResult = struct {
    idx: i32,
    state: LookupState,
};

fn now() c.time_t {
    var t: c.time_t = 0;
    _ = c.time(&t);
    return t;
}

pub fn init() void {
    cache_total_bytes = 0;
    for (cache_entries[0..]) |*e| {
        e.* = empty_entry();
    }
}

fn empty_entry() types.CacheEntry {
    return types.CacheEntry{ .used = false, .state = .failed, .key = null, .data = null, .len = 0, .cap = 0, .last_used = 0, .refcount = 0 };
}

fn free_entry(ce: *types.CacheEntry) void {
    if (ce.key) |k| std.heap.c_allocator.free(k);
    if (ce.data) |data| {
        cache_total_bytes -= ce.len;
        std.heap.c_allocator.free(data);
    }
    ce.* = empty_entry();
}

fn init_entry(ce: *types.CacheEntry, key_copy: []u8) void {
    ce.* = types.CacheEntry{
        .used = true,
        .state = .loading,
        .key = key_copy,
        .data = null,
        .len = 0,
        .cap = 0,
        .last_used = now(),
        .refcount = 1,
    };
}

pub fn lookup_or_create_locked(key: []const u8) LookupResult {
    var key_copy = std.heap.c_allocator.alloc(u8, key.len) catch return .{ .idx = -1, .state = .failed };
    std.mem.copyForwards(u8, key_copy[0..key.len], key);

    var free_idx: i32 = -1;
    var evict_idx: i32 = -1;
    var oldest_time: c.time_t = std.math.maxInt(c.time_t);

    for (0..cache_entries.len) |idx| {
        var e = &cache_entries[idx];
        if (e.used) {
            if (e.key != null and std.mem.eql(u8, e.key.?, key)) {
                if (e.state == .failed and e.refcount == 0) {
                    free_entry(e);
                    init_entry(e, key_copy);
                    return .{ .idx = @intCast(idx), .state = .created };
                }

                std.heap.c_allocator.free(key_copy);
                if (e.state != .failed) e.refcount += 1;
                e.last_used = now();
                return .{ .idx = @intCast(idx), .state = switch (e.state) {
                    .loading => .loading,
                    .complete => .hit,
                    .failed => .failed,
                } };
            }
            if (e.refcount == 0 and e.state != .loading and e.last_used < oldest_time) {
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
        return .{ .idx = -1, .state = .failed };
    }
    const ce_idx: usize = @intCast(chosen_idx);
    const ce = &cache_entries[ce_idx];

    if (ce.used) free_entry(ce);
    init_entry(ce, key_copy);
    return .{ .idx = chosen_idx, .state = .created };
}

pub fn wait_complete_locked(idx: i32) bool {
    if (idx < 0) return false;
    const ce_idx: usize = @intCast(idx);
    while (cache_entries[ce_idx].used and cache_entries[ce_idx].state == .loading) {
        changed.wait(&mutex);
    }
    return cache_entries[ce_idx].used and cache_entries[ce_idx].state == .complete;
}

pub fn append_to_locked(idx: i32, bytes: []const u8) bool {
    if (idx < 0) return false;
    const ce_idx: usize = @intCast(idx);
    var ce = &cache_entries[ce_idx];
    if (!ce.used or ce.state != .loading) return false;
    const needed = ce.len + bytes.len;
    if (needed > types.MAX_CACHE_BYTES) return false;
    if (needed > ce.cap) {
        var new_cap = if (ce.cap == 0) (bytes.len) else (ce.cap * 2);
        if (new_cap < needed) new_cap = needed;
        if (new_cap > types.MAX_CACHE_BYTES) new_cap = types.MAX_CACHE_BYTES;
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
    cache_total_bytes += bytes.len;
    return true;
}

pub fn mark_complete_locked(idx: i32) void {
    if (idx < 0) return;
    const ce_idx: usize = @intCast(idx);
    var ce = &cache_entries[ce_idx];
    if (!ce.used) return;
    ce.state = .complete;
    ce.last_used = now();
    changed.broadcast();
}

pub fn mark_failed_locked(idx: i32) void {
    if (idx < 0) return;
    const ce_idx: usize = @intCast(idx);
    var ce = &cache_entries[ce_idx];
    if (!ce.used) return;

    if (ce.data) |data| {
        cache_total_bytes -= ce.len;
        std.heap.c_allocator.free(data);
    }
    ce.data = null;
    ce.len = 0;
    ce.cap = 0;
    ce.state = .failed;
    ce.last_used = now();
    changed.broadcast();
}

pub fn release_locked(idx: i32) void {
    if (idx < 0) return;
    const ce_idx: usize = @intCast(idx);
    var ce = &cache_entries[ce_idx];
    if (!ce.used) return;
    if (ce.refcount > 0) ce.refcount -= 1;
    ce.last_used = now();
    if (ce.refcount == 0 and ce.state == .failed) free_entry(ce);
}
