const std = @import("std");

const proxy = @import("proxy.zig");

pub fn main() void {
    var args = std.process.args();
    const prog = args.next() orelse "program";
    const port_opt = args.next();
    if (port_opt == null) {
        _ = std.debug.print("Usage: {s} <listen_port>\n", .{prog});
        return;
    }
    const port = port_opt.?;
    proxy.run(port);
}
