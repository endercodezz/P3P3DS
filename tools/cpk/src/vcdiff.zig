//! VCDIFF (RFC 3284) as xdelta3 writes it, for patches shipped with mods:
//! default code table, no secondary compression. Patches are community files:
//! every length and address is checked.
const std = @import("std");

pub const max_output_size: u64 = 1024 * 1024 * 1024;

const Kind = enum { noop, add, run, copy };
const Op = struct { kind: Kind = .noop, size: u8 = 0, mode: u8 = 0 };
const Entry = struct { first: Op = .{}, second: Op = .{} };

/// RFC 3284 section 5.6.
const code_table: [256]Entry = blk: {
    @setEvalBranchQuota(10000);
    var table: [256]Entry = @splat(.{});
    table[0].first = .{ .kind = .run };
    for (0..18) |size| table[1 + size].first = .{ .kind = .add, .size = size };
    var index: usize = 19;
    for (0..9) |mode| {
        table[index].first = .{ .kind = .copy, .mode = mode };
        index += 1;
        for (4..19) |size| {
            table[index].first = .{ .kind = .copy, .size = size, .mode = mode };
            index += 1;
        }
    }
    for (0..6) |mode| for (1..5) |add| for (4..7) |copy| {
        table[index] = .{ .first = .{ .kind = .add, .size = add }, .second = .{ .kind = .copy, .size = copy, .mode = mode } };
        index += 1;
    };
    for (6..9) |mode| for (1..5) |add| {
        table[index] = .{ .first = .{ .kind = .add, .size = add }, .second = .{ .kind = .copy, .size = 4, .mode = mode } };
        index += 1;
    };
    for (0..9) |mode| {
        table[index] = .{ .first = .{ .kind = .copy, .size = 4, .mode = mode }, .second = .{ .kind = .add, .size = 1 } };
        index += 1;
    }
    break :blk table;
};

const Reader = struct {
    data: []const u8,
    position: usize = 0,

    fn byte(self: *Reader) !u8 {
        if (self.position >= self.data.len) return error.TruncatedPatch;
        self.position += 1;
        return self.data[self.position - 1];
    }

    fn take(self: *Reader, size: u64) ![]const u8 {
        if (size > self.data.len - self.position) return error.TruncatedPatch;
        const start = self.position;
        self.position += @intCast(size);
        return self.data[start..self.position];
    }

    /// Big-endian base 128, the high bit of each byte means more follow.
    fn varint(self: *Reader) !u64 {
        var value: u64 = 0;
        while (true) {
            const next = try self.byte();
            if (value >> 57 != 0) return error.BadPatch;
            value = (value << 7) | (next & 0x7F);
            if (next & 0x80 == 0) return value;
        }
    }
};

const AddressCache = struct {
    near: [4]u64 = @splat(0),
    next_near: usize = 0,
    same: [3 * 256]u64 = @splat(0),

    fn decode(self: *AddressCache, addresses: *Reader, here: u64, mode: u8) !u64 {
        const address = switch (mode) {
            0 => try addresses.varint(),
            1 => std.math.sub(u64, here, try addresses.varint()) catch return error.BadPatch,
            2...5 => std.math.add(u64, self.near[mode - 2], try addresses.varint()) catch return error.BadPatch,
            6...8 => self.same[(@as(usize, mode) - 6) * 256 + try addresses.byte()],
            else => return error.BadPatch,
        };
        self.near[self.next_near] = address;
        self.next_near = (self.next_near + 1) % self.near.len;
        self.same[address % self.same.len] = address;
        return address;
    }
};

fn adler32(data: []const u8) u32 {
    var a: u32 = 1;
    var b: u32 = 0;
    for (data) |value| {
        a = (a + value) % 65521;
        b = (b + a) % 65521;
    }
    return (b << 16) | a;
}

const Window = struct {
    segment: []const u8,
    target: []u8,
    written: usize = 0,
    data: Reader,
    instructions: Reader,
    addresses: Reader,
    cache: AddressCache = .{},

    fn apply(self: *Window, op: Op) !void {
        if (op.kind == .noop) return;
        const size = if (op.size != 0) op.size else try self.instructions.varint();
        if (size > self.target.len - self.written) return error.BadPatch;
        const count: usize = @intCast(size);
        switch (op.kind) {
            .noop => unreachable,
            .add => @memcpy(self.target[self.written..][0..count], try self.data.take(size)),
            .run => @memset(self.target[self.written..][0..count], try self.data.byte()),
            .copy => {
                const here = self.segment.len + self.written;
                const address = try self.cache.decode(&self.addresses, here, op.mode);
                if (address >= here) return error.BadPatch;
                // Byte by byte: a copy may read what it has just written.
                var from: usize = @intCast(address);
                for (self.target[self.written..][0..count], self.written..) |*out, position| {
                    if (from >= self.segment.len + position) return error.BadPatch;
                    out.* = if (from < self.segment.len) self.segment[from] else self.target[from - self.segment.len];
                    from += 1;
                }
            },
        }
        self.written += count;
    }
};

pub const Result = struct { output: []u8, windows: u64 };

/// Caller frees `output`.
pub fn apply(allocator: std.mem.Allocator, source: []const u8, patch: []const u8) !Result {
    var input: Reader = .{ .data = patch };
    if (!std.mem.eql(u8, try input.take(4), "\xD6\xC3\xC4\x00")) return error.NotVcdiff;
    const header = try input.byte();
    if (header & 0x03 != 0) return error.UnsupportedCompression; // secondary compressor, custom code table
    if (header & 0x04 != 0) _ = try input.take(try input.varint()); // xdelta: file names
    if (header & ~@as(u8, 0x07) != 0) return error.BadPatch;

    var output: std.ArrayList(u8) = .empty;
    errdefer output.deinit(allocator);
    var windows: u64 = 0;
    while (input.position < patch.len) : (windows += 1) {
        const indicator = try input.byte();
        const from_source = indicator & 0x01 != 0;
        const from_target = indicator & 0x02 != 0;
        if (from_source and from_target or indicator & ~@as(u8, 0x07) != 0) return error.BadPatch;
        var segment: []const u8 = "";
        var segment_start: usize = 0;
        if (from_source or from_target) {
            const size = try input.varint();
            const position = try input.varint();
            const end = std.math.add(u64, position, size) catch return error.BadPatch;
            const base = if (from_source) source else output.items;
            if (end > base.len) return error.BadPatch;
            segment_start = @intCast(position);
            segment = base[segment_start..@intCast(end)];
        }
        _ = try input.varint(); // length of the delta encoding
        const target_size = try input.varint();
        if (target_size > max_output_size - output.items.len) return error.TooLarge;
        if (try input.byte() != 0) return error.UnsupportedCompression;
        const data_size = try input.varint();
        const instructions_size = try input.varint();
        const addresses_size = try input.varint();
        var checksum: ?u32 = null;
        if (indicator & 0x04 != 0) checksum = std.mem.readInt(u32, (try input.take(4))[0..4], .big);
        const data = try input.take(data_size);
        const instructions = try input.take(instructions_size);
        const addresses = try input.take(addresses_size);

        // A window may copy from output, which growing the list would move.
        const target = try allocator.alloc(u8, @intCast(target_size));
        defer allocator.free(target);
        var window: Window = .{
            .segment = segment,
            .target = target,
            .data = .{ .data = data },
            .instructions = .{ .data = instructions },
            .addresses = .{ .data = addresses },
        };
        while (window.instructions.position < instructions.len) {
            const entry = code_table[try window.instructions.byte()];
            try window.apply(entry.first);
            try window.apply(entry.second);
        }
        if (window.written != target.len) return error.BadPatch;
        if (checksum) |expected| if (adler32(target) != expected) return error.ChecksumMismatch;
        try output.appendSlice(allocator, target);
    }
    return .{ .output = try output.toOwnedSlice(allocator), .windows = windows };
}

const testing = std.testing;

test "default code table landmarks from RFC 3284" {
    try testing.expectEqual(Kind.run, code_table[0].first.kind);
    try testing.expectEqual(@as(u8, 17), code_table[18].first.size);
    try testing.expectEqual(Op{ .kind = .copy, .size = 0, .mode = 0 }, code_table[19].first);
    try testing.expectEqual(Op{ .kind = .copy, .size = 18, .mode = 8 }, code_table[162].first);
    try testing.expectEqual(Op{ .kind = .add, .size = 1 }, code_table[163].first);
    try testing.expectEqual(Op{ .kind = .copy, .size = 4, .mode = 0 }, code_table[163].second);
    try testing.expectEqual(Op{ .kind = .copy, .size = 6, .mode = 5 }, code_table[234].second);
    try testing.expectEqual(Op{ .kind = .copy, .size = 4, .mode = 6 }, code_table[235].second);
    try testing.expectEqual(Op{ .kind = .copy, .size = 4, .mode = 8 }, code_table[255].first);
    try testing.expectEqual(Op{ .kind = .add, .size = 1 }, code_table[255].second);
}

test "adler32" {
    try testing.expectEqual(@as(u32, 0x11E60398), adler32("Wikipedia"));
}
