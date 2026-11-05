const std = @import("std");
const diff = @import("diff.zig");

const usage =
    \\Usage: p3p-bindiff [--elf] [--json] [--] LEFT RIGHT
    \\       p3p-bindiff --help | --version
    \\Compare files positionally, optionally comparing ELF32 little-endian MIPS/PSP
    \\metadata. Inputs must be smaller than 256 MiB each. Ranges are [start,end).
    \\Exit: 0 identical, 1 different, 2 usage/input/output error.
    \\Only the first 256 byte ranges are listed; counts cover the entire inputs.
    \\--elf requires decrypted ELF/PRX; no decryption or relocation is performed.
    \\Sections and segments are matched by table index, not by function or name.
    \\
;

pub fn main(init: std.process.Init) void {
    const code = run(init) catch |err| {
        var buffer: [1024]u8 = undefined;
        var stderr: std.Io.File.Writer = .init(.stderr(), init.io, &buffer);
        stderr.interface.print("p3p-bindiff: {s}\n", .{@errorName(err)}) catch {};
        stderr.interface.flush() catch {};
        std.process.exit(2);
    };
    std.process.exit(code);
}

fn run(init: std.process.Init) !u8 {
    const allocator = init.gpa;
    const args = try init.minimal.args.toSlice(init.arena.allocator());
    var elf_mode = false;
    var json = false;
    var literal = false;
    var paths: [2][]const u8 = undefined;
    var count: usize = 0;
    var buffer: [8192]u8 = undefined;
    var stdout: std.Io.File.Writer = .init(.stdout(), init.io, &buffer);
    const writer = &stdout.interface;
    if (args.len == 2 and std.mem.eql(u8, args[1], "--help")) {
        try writer.writeAll(usage);
        try writer.flush();
        return 0;
    }
    if (args.len == 2 and std.mem.eql(u8, args[1], "--version")) {
        try writer.writeAll("p3p-bindiff 0.1.0 (Zig 0.17.1)\n");
        try writer.flush();
        return 0;
    }
    for (args[1..]) |arg| {
        if (!literal and std.mem.eql(u8, arg, "--")) {
            literal = true;
        } else if (!literal and std.mem.eql(u8, arg, "--elf")) {
            if (elf_mode) return error.DuplicateOption;
            elf_mode = true;
        } else if (!literal and std.mem.eql(u8, arg, "--json")) {
            if (json) return error.DuplicateOption;
            json = true;
        } else {
            if (!literal and std.mem.startsWith(u8, arg, "-")) return error.UnknownOption;
            if (count == paths.len) return error.ExpectedTwoPaths;
            paths[count] = arg;
            count += 1;
        }
    }
    if (count != 2) return error.ExpectedTwoPaths;
    const left = try std.Io.Dir.cwd().readFileAlloc(init.io, paths[0], allocator, .limited(256 * 1024 * 1024));
    defer allocator.free(left);
    const right = try std.Io.Dir.cwd().readFileAlloc(init.io, paths[1], allocator, .limited(256 * 1024 * 1024));
    defer allocator.free(right);
    // Validate both inputs completely before producing any success report.
    var a: ?diff.Elf = null;
    defer if (a) |elf| elf.deinit(allocator);
    var b: ?diff.Elf = null;
    defer if (b) |elf| elf.deinit(allocator);
    var elf_diff: ?diff.ElfDiff = null;
    defer if (elf_diff) |*d| d.deinit(allocator);
    if (elf_mode) {
        a = try diff.parseElf(allocator, left);
        b = try diff.parseElf(allocator, right);
        elf_diff = try diff.compareElf(allocator, a.?, b.?);
    }
    var bytes = try diff.compare(allocator, left, right);
    defer bytes.deinit(allocator);
    if (json) {
        const ElfReport = struct {
            left_header: diff.Header,
            right_header: diff.Header,
            header_changes: []const diff.FieldChange,
            segment_changes: []const diff.EntryChange(diff.Segment),
            section_changes: []const diff.EntryChange(diff.Section),
        };
        try std.json.Stringify.value(.{
            .schema_version = @as(u32, 1),
            .mode = @as([]const u8, if (elf_mode) "elf" else "bytes"),
            .left = paths[0],
            .right = paths[1],
            .equal = bytes.equal(),
            .bytes = .{
                .left_size = bytes.left_size,
                .right_size = bytes.right_size,
                .changed_bytes = bytes.changed_bytes,
                .added_bytes = bytes.added_bytes,
                .removed_bytes = bytes.removed_bytes,
                .range_count = bytes.range_count,
                .ranges = bytes.ranges.items,
                .ranges_truncated = bytes.ranges_truncated,
            },
            .elf = @as(?ElfReport, if (elf_diff) |d| .{
                .left_header = a.?.header,
                .right_header = b.?.header,
                .header_changes = d.header.items,
                .segment_changes = d.segments.items,
                .section_changes = d.sections.items,
            } else null),
        }, .{}, writer);
        try writer.writeByte('\n');
    } else {
        try writer.print("{s}\nleft: {d} bytes; right: {d} bytes\nchanged: {d}; added: {d}; removed: {d}\n", .{
            @as([]const u8, if (bytes.equal()) "identical" else "different"),
            bytes.left_size,
            bytes.right_size,
            bytes.changed_bytes,
            bytes.added_bytes,
            bytes.removed_bytes,
        });
        for (bytes.ranges.items) |range| try writer.print("range: [0x{x}, 0x{x})\n", .{ range.start, range.end });
        if (bytes.ranges_truncated) try writer.print("showing 256 of {d} ranges\n", .{bytes.range_count});
        if (elf_diff) |d| {
            for (d.header.items) |field| try writer.print("ELF {s}: 0x{x} -> 0x{x}\n", .{ field.field, field.left, field.right });
            try textEntries(diff.Segment, writer, "segment", d.segments.items);
            try textEntries(diff.Section, writer, "section", d.sections.items);
        }
    }
    try writer.flush();
    return if (bytes.equal()) 0 else 1;
}

fn textEntries(comptime T: type, writer: *std.Io.Writer, kind: []const u8, changes: []const diff.EntryChange(T)) !void {
    for (changes) |change| {
        try writer.print("ELF {s}[{d}] {s}\n  left: ", .{ kind, change.index, @tagName(change.status) });
        try std.json.Stringify.value(change.left, .{}, writer);
        try writer.writeAll("\n  right: ");
        try std.json.Stringify.value(change.right, .{}, writer);
        try writer.writeAll("\n  content_equal: ");
        try std.json.Stringify.value(change.content_equal, .{}, writer);
        try writer.writeByte('\n');
    }
}
