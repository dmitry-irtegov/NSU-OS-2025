const std = @import("std");

const proxy = @import("proxy.zig");

pub fn main() void {
    var args = std.process.args();
    const prog = args.next() orelse "program";
    const port_opt = args.next();
    const workers_opt = args.next();
    if (port_opt == null or workers_opt == null) {
        _ = std.debug.print("Usage: {s} <listen_port> <worker_threads>\n", .{prog});
        return;
    }
    const port = port_opt.?;
    const workers = std.fmt.parseInt(usize, workers_opt.?, 10) catch {
        _ = std.debug.print("worker_threads must be a positive integer\n", .{});
        return;
    };
    if (workers == 0) {
        _ = std.debug.print("worker_threads must be greater than zero\n", .{});
        return;
    }
    proxy.run(port, workers);
}
