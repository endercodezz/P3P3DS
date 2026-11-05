const std = @import("std");
const Allocator = std.mem.Allocator;

pub const Range = struct { start: usize, end: usize };
pub const ByteDiff = struct {
    left_size: usize,
    right_size: usize,
    changed_bytes: usize = 0,
    added_bytes: usize,
    removed_bytes: usize,
    range_count: usize = 0,
    ranges: std.ArrayList(Range) = .empty,
    ranges_truncated: bool = false,

    pub fn deinit(self: *ByteDiff, allocator: Allocator) void {
        self.ranges.deinit(allocator);
    }

    pub fn equal(self: ByteDiff) bool {
        return self.changed_bytes == 0 and self.added_bytes == 0 and self.removed_bytes == 0;
    }
};

// Positional comparison; ranges are half-open and include a missing tail.
// Keep output bounded without stopping the comparison or losing total counts.
pub fn compare(allocator: Allocator, left: []const u8, right: []const u8) !ByteDiff {
    const common = @min(left.len, right.len);
    const total = @max(left.len, right.len);
    var result: ByteDiff = .{
        .left_size = left.len,
        .right_size = right.len,
        .added_bytes = right.len - common,
        .removed_bytes = left.len - common,
    };
    errdefer result.deinit(allocator);
    var start: ?usize = null;
    for (0..total) |i| {
        const different = i >= common or left[i] != right[i];
        if (i < common and different) result.changed_bytes += 1;
        if (different and start == null) start = i;
        if (!different and start != null) {
            try appendRange(allocator, &result, .{ .start = start.?, .end = i });
            start = null;
        }
    }
    if (start) |s| try appendRange(allocator, &result, .{ .start = s, .end = total });
    return result;
}

fn appendRange(allocator: Allocator, result: *ByteDiff, range: Range) !void {
    result.range_count += 1;
    if (result.ranges.items.len < 256) {
        try result.ranges.append(allocator, range);
    } else result.ranges_truncated = true;
}

pub const Header = struct {
    os_abi: u8,
    abi_version: u8,
    elf_type: u16,
    machine: u16,
    version: u32,
    entry: u32,
    program_offset: u32,
    section_offset: u32,
    flags: u32,
    header_size: u16,
    program_entry_size: u16,
    program_count: u16,
    section_entry_size: u16,
    section_count: u16,
    string_table_index: u16,
};

pub const Segment = struct {
    kind: u32,
    file_offset: u32,
    virtual_address: u32,
    physical_address: u32,
    file_size: u32,
    memory_size: u32,
    flags: u32,
    alignment: u32,
};

pub const Section = struct {
    name: []const u8,
    name_offset: u32,
    kind: u32,
    flags: u32,
    virtual_address: u32,
    file_offset: u32,
    size: u32,
    link: u32,
    info: u32,
    alignment: u32,
    entry_size: u32,
};

pub const Elf = struct {
    header: Header,
    segments: []Segment,
    sections: []Section,
    bytes: []const u8,

    pub fn deinit(self: Elf, allocator: Allocator) void {
        allocator.free(self.segments);
        allocator.free(self.sections);
    }
};

fn span(bytes: []const u8, offset: usize, size: usize) ![]const u8 {
    if (offset > bytes.len or size > bytes.len - offset) return error.TruncatedElf;
    return bytes[offset..][0..size];
}

fn read(comptime T: type, bytes: []const u8, offset: usize) T {
    return std.mem.readInt(T, bytes[offset..][0..@sizeOf(T)], .little);
}

// Only metadata and file bytes are read. Do not reproduce PSPRecomp's
// load/relocate/import/Allegrex pipeline here (elf32.hpp::Elf32Image).
pub fn parseElf(allocator: Allocator, bytes: []const u8) !Elf {
    if (std.mem.startsWith(u8, bytes, "~PSP")) return error.EncryptedPsp;
    if (std.mem.startsWith(u8, bytes, "\x00PBP")) return error.PbpContainer;
    _ = try span(bytes, 0, 52);
    if (!std.mem.eql(u8, bytes[0..4], "\x7fELF")) return error.NotElf;
    if (bytes[4] != 1 or bytes[5] != 1) return error.UnsupportedElfEncoding;
    if (bytes[6] != 1 or read(u32, bytes, 20) != 1) return error.InvalidElfVersion;
    if (read(u16, bytes, 18) != 8) return error.UnsupportedMachine;
    const h: Header = .{
        .os_abi = bytes[7],
        .abi_version = bytes[8],
        .elf_type = read(u16, bytes, 16),
        .machine = read(u16, bytes, 18),
        .version = read(u32, bytes, 20),
        .entry = read(u32, bytes, 24),
        .program_offset = read(u32, bytes, 28),
        .section_offset = read(u32, bytes, 32),
        .flags = read(u32, bytes, 36),
        .header_size = read(u16, bytes, 40),
        .program_entry_size = read(u16, bytes, 42),
        .program_count = read(u16, bytes, 44),
        .section_entry_size = read(u16, bytes, 46),
        .section_count = read(u16, bytes, 48),
        .string_table_index = read(u16, bytes, 50),
    };
    if (h.header_size != 52) return error.InvalidHeaderSize;
    if (h.program_count == 0xffff or h.string_table_index == 0xffff or
        (h.section_count == 0 and h.section_offset != 0)) return error.ExtendedNumberingUnsupported;
    if ((h.program_count > 0 and h.program_entry_size < 32) or
        (h.section_count > 0 and h.section_entry_size < 40)) return error.InvalidTableEntrySize;
    // Promote before multiplication, even when compiling for a 32-bit host.
    _ = try tableSpan(bytes, h.program_offset, h.program_count, h.program_entry_size);
    _ = try tableSpan(bytes, h.section_offset, h.section_count, h.section_entry_size);
    const segments = try allocator.alloc(Segment, h.program_count);
    errdefer allocator.free(segments);
    for (segments, 0..) |*s, i| {
        const p = try span(bytes, @as(usize, h.program_offset) + i * h.program_entry_size, 32);
        s.* = .{
            .kind = read(u32, p, 0),
            .file_offset = read(u32, p, 4),
            .virtual_address = read(u32, p, 8),
            .physical_address = read(u32, p, 12),
            .file_size = read(u32, p, 16),
            .memory_size = read(u32, p, 20),
            .flags = read(u32, p, 24),
            .alignment = read(u32, p, 28),
        };
        if (s.file_size > 0) _ = try span(bytes, s.file_offset, s.file_size);
        if (s.kind == 1 and s.memory_size < s.file_size) return error.InvalidLoadSize;
        if (@as(u64, s.virtual_address) + s.memory_size > 0x100000000) return error.AddressOverflow;
    }
    const sections = try allocator.alloc(Section, h.section_count);
    errdefer allocator.free(sections);
    for (sections, 0..) |*s, i| {
        const p = try span(bytes, @as(usize, h.section_offset) + i * h.section_entry_size, 40);
        s.* = .{
            .name = "",
            .name_offset = read(u32, p, 0),
            .kind = read(u32, p, 4),
            .flags = read(u32, p, 8),
            .virtual_address = read(u32, p, 12),
            .file_offset = read(u32, p, 16),
            .size = read(u32, p, 20),
            .link = read(u32, p, 24),
            .info = read(u32, p, 28),
            .alignment = read(u32, p, 32),
            .entry_size = read(u32, p, 36),
        };
        if (s.kind != 8 and s.size > 0) _ = try span(bytes, s.file_offset, s.size);
        if (@as(u64, s.virtual_address) + s.size > 0x100000000) return error.AddressOverflow;
    }
    if (h.string_table_index != 0) {
        if (h.string_table_index >= sections.len) return error.InvalidStringTable;
        const strings = sections[h.string_table_index];
        if (strings.kind != 3) return error.InvalidStringTable;
        const names = try span(bytes, strings.file_offset, strings.size);
        for (sections) |*s| {
            if (s.name_offset >= names.len) return error.InvalidSectionName;
            const tail = names[s.name_offset..];
            const end = std.mem.indexOfScalar(u8, tail, 0) orelse return error.InvalidSectionName;
            s.name = tail[0..end];
        }
    } else {
        for (sections) |s| if (s.name_offset != 0) return error.InvalidSectionName;
    }
    return .{ .header = h, .segments = segments, .sections = sections, .bytes = bytes };
}

fn tableSpan(bytes: []const u8, offset: u32, count: u16, stride: u16) ![]const u8 {
    if (count == 0) return bytes[0..0];
    if (offset < 52) return error.InvalidTableOffset;
    const size = @as(u64, count) * stride;
    if (size > std.math.maxInt(usize)) return error.TruncatedElf;
    return span(bytes, offset, @intCast(size));
}

pub const FieldChange = struct { field: []const u8, left: u64, right: u64 };
pub fn EntryChange(comptime T: type) type {
    return struct {
        index: usize,
        status: enum { added, removed, modified },
        left: ?T,
        right: ?T,
        content_equal: ?bool,
    };
}
pub const ElfDiff = struct {
    header: std.ArrayList(FieldChange) = .empty,
    segments: std.ArrayList(EntryChange(Segment)) = .empty,
    sections: std.ArrayList(EntryChange(Section)) = .empty,

    pub fn deinit(self: *ElfDiff, allocator: Allocator) void {
        self.header.deinit(allocator);
        self.segments.deinit(allocator);
        self.sections.deinit(allocator);
    }
};

pub fn compareElf(allocator: Allocator, left: Elf, right: Elf) !ElfDiff {
    var result: ElfDiff = .{};
    errdefer result.deinit(allocator);
    inline for (@typeInfo(Header).@"struct".field_names) |name| {
        const a = @field(left.header, name);
        const b = @field(right.header, name);
        if (a != b) try result.header.append(allocator, .{ .field = name, .left = a, .right = b });
    }
    try compareEntries(Segment, allocator, left.segments, right.segments, left.bytes, right.bytes, &result.segments);
    try compareEntries(Section, allocator, left.sections, right.sections, left.bytes, right.bytes, &result.sections);
    return result;
}

fn payload(comptime T: type, item: T, bytes: []const u8) ?[]const u8 {
    if (T == Section) {
        if (item.kind == 8) return null; // SHT_NOBITS has no file payload.
        if (item.size == 0) return bytes[0..0];
        return bytes[item.file_offset..][0..item.size];
    } else {
        if (item.file_size == 0) return bytes[0..0];
        return bytes[item.file_offset..][0..item.file_size];
    }
}

fn compareEntries(comptime T: type, allocator: Allocator, left: []const T, right: []const T, a_bytes: []const u8, b_bytes: []const u8, out: *std.ArrayList(EntryChange(T))) !void {
    for (0..@max(left.len, right.len)) |i| {
        const a: ?T = if (i < left.len) left[i] else null;
        const b: ?T = if (i < right.len) right[i] else null;
        var content_equal: ?bool = null;
        if (a != null and b != null) {
            const ap = payload(T, a.?, a_bytes);
            const bp = payload(T, b.?, b_bytes);
            if (ap != null and bp != null) content_equal = std.mem.eql(u8, ap.?, bp.?);
            var metadata_equal = true;
            inline for (@typeInfo(T).@"struct".field_names) |name| {
                if (comptime std.mem.eql(u8, name, "name")) {
                    if (!std.mem.eql(u8, @field(a.?, name), @field(b.?, name))) metadata_equal = false;
                } else if (@field(a.?, name) != @field(b.?, name)) metadata_equal = false;
            }
            if (metadata_equal and content_equal != false) continue;
        }
        try out.append(allocator, .{
            .index = i,
            .status = if (a == null) .added else if (b == null) .removed else .modified,
            .left = a,
            .right = b,
            .content_equal = content_equal,
        });
    }
}

test "positional differences include missing tails and half-open ranges" {
    const a = std.testing.allocator;
    var d = try compare(a, "abcdef", "aXcYe!");
    defer d.deinit(a);
    try std.testing.expectEqual(@as(usize, 3), d.changed_bytes);
    try std.testing.expectEqualSlices(Range, &.{ .{ .start = 1, .end = 2 }, .{ .start = 3, .end = 4 }, .{ .start = 5, .end = 6 } }, d.ranges.items);
    var tail = try compare(a, "abc", "aX");
    defer tail.deinit(a);
    try std.testing.expectEqual(@as(usize, 1), tail.changed_bytes);
    try std.testing.expectEqual(@as(usize, 1), tail.removed_bytes);
    try std.testing.expectEqual(@as(usize, 3), tail.ranges.items[0].end);
}

test "empty inputs and bounded report retain complete counts" {
    const a = std.testing.allocator;
    var empty = try compare(a, "", "");
    defer empty.deinit(a);
    try std.testing.expect(empty.equal());
    const left: [600]u8 = @splat(0);
    var right = left;
    for (&right, 0..) |*v, i| v.* = @intCast(i % 2);
    var d = try compare(a, &left, &right);
    defer d.deinit(a);
    try std.testing.expectEqual(@as(usize, 300), d.changed_bytes);
    try std.testing.expectEqual(@as(usize, 300), d.range_count);
    try std.testing.expectEqual(@as(usize, 256), d.ranges.items.len);
    try std.testing.expect(d.ranges_truncated);
}

fn minimalElf() [52]u8 {
    var b: [52]u8 = @splat(0);
    @memcpy(b[0..7], "\x7fELF\x01\x01\x01");
    std.mem.writeInt(u16, b[16..18], 0xffa0, .little);
    std.mem.writeInt(u16, b[18..20], 8, .little);
    std.mem.writeInt(u32, b[20..24], 1, .little);
    std.mem.writeInt(u16, b[40..42], 52, .little);
    return b;
}

test "PRX without section tables and every truncated header" {
    const a = std.testing.allocator;
    const bytes = minimalElf();
    const elf = try parseElf(a, &bytes);
    defer elf.deinit(a);
    try std.testing.expectEqual(@as(u16, 0xffa0), elf.header.elf_type);
    for (0..bytes.len) |n| try std.testing.expectError(error.TruncatedElf, parseElf(a, bytes[0..n]));
    var change = bytes;
    std.mem.writeInt(u32, change[24..28], 0x1234, .little);
    const other = try parseElf(a, &change);
    defer other.deinit(a);
    var d = try compareElf(a, elf, other);
    defer d.deinit(a);
    try std.testing.expectEqual(@as(usize, 1), d.header.items.len);
    try std.testing.expectEqualStrings("entry", d.header.items[0].field);
}

test "table overflow, invalid stride, extended numbering and encoding" {
    const a = std.testing.allocator;
    var bytes = minimalElf();
    std.mem.writeInt(u32, bytes[28..32], 0xfffffff0, .little);
    std.mem.writeInt(u16, bytes[42..44], 32, .little);
    std.mem.writeInt(u16, bytes[44..46], 2, .little);
    try std.testing.expectError(error.TruncatedElf, parseElf(a, &bytes));
    bytes[42] = 0;
    try std.testing.expectError(error.InvalidTableEntrySize, parseElf(a, &bytes));
    bytes = minimalElf();
    std.mem.writeInt(u16, bytes[50..52], 0xffff, .little);
    try std.testing.expectError(error.ExtendedNumberingUnsupported, parseElf(a, &bytes));
    bytes = minimalElf();
    bytes[5] = 2;
    try std.testing.expectError(error.UnsupportedElfEncoding, parseElf(a, &bytes));
    try std.testing.expectError(error.EncryptedPsp, parseElf(a, "~PSP"));
    try std.testing.expectError(error.PbpContainer, parseElf(a, "\x00PBP"));
}
