const std = @import("std");
const builtin = @import("builtin");

pub fn build(b: *std.Build) void {
    if (!std.mem.eql(u8, builtin.zig_version_string, "0.17.1"))
        @panic("p3p-cpk requires Zig 0.17.1");
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const exe = b.addExecutable(.{
        .name = "p3p-cpk",
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/main.zig"),
            .target = target,
            .optimize = optimize,
        }),
    });
    b.installArtifact(exe);
    const tests = b.addTest(.{
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/cpk.zig"),
            .target = target,
            .optimize = optimize,
        }),
    });
    b.step("test", "Test CPK tables, CRILAYLA and path sanitising").dependOn(&b.addRunArtifact(tests).step);
}
