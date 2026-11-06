//! Atlus PAK containers of P3P (.bin, .pac, .fpc, ...), in the two layouts the
//! game uses. Long names: per file a 252-byte name, a 32-bit size and the data
//! padded to 64 bytes, then zeros. Counted: a 32-bit file count, then per file a
//! 32-byte name, a 32-bit size and the data, unpadded. All little-endian.
const std = @import("std");

const Layout = struct {
    name_size: usize,
    alignment: usize,
    counted: bool,
};
const long_names: Layout = .{ .name_size = 252, .alignment = 64, .counted = false };
const counted: Layout = .{ .name_size = 32, .alignment = 1, .counted = true };

const Entry = struct {
    raw_name: []const u8,
    name: []const u8,
    data: []const u8,
    /// What followed the data up to the next file: kept for an exact rebuild.
    padding: []const u8,
};

const Pak = struct {
    layout: Layout,
    entries: []Entry,
    tail: []const u8,
};

fn isName(name: []const u8) bool {
    if (name.len == 0) return false;
    for (name) |byte| if (byte < 0x20 or byte > 0x7E) return false;
    return true;
}

fn parseLayout(allocator: std.mem.Allocator, bytes: []const u8, layout: Layout) !?Pak {
    var entries: std.ArrayList(Entry) = .empty;
    errdefer entries.deinit(allocator);
    var position: usize = 0;
    var remaining: usize = std.math.maxInt(usize);
    if (layout.counted) {
        if (bytes.len < 4) return null;
        remaining = std.mem.readInt(u32, bytes[0..4], .little);
        if (remaining == 0 or remaining > bytes.len / (layout.name_size + 4)) {
            entries.deinit(allocator);
            return null;
        }
        position = 4;
    }
    const header = layout.name_size + 4;
    while (remaining != 0 and bytes.len - position >= header) : (remaining -= 1) {
        const raw_name = bytes[position..][0..layout.name_size];
        const name = raw_name[0 .. std.mem.indexOfScalar(u8, raw_name, 0) orelse raw_name.len];
        if (name.len == 0 and !layout.counted) break;
        const size = std.mem.readInt(u32, bytes[position + layout.name_size ..][0..4], .little);
        const data_start = position + header;
        if (!isName(name) or size > bytes.len - data_start) {
            entries.deinit(allocator);
            return null;
        }
        const data_end = data_start + size;
        const next = @min(std.mem.alignForward(usize, data_end, layout.alignment), bytes.len);
        try entries.append(allocator, .{ .raw_name = raw_name, .name = name, .data = bytes[data_start..data_end], .padding = bytes[data_end..next] });
        position = next;
    }
    if (entries.items.len == 0 or (layout.counted and remaining != 0)) {
        entries.deinit(allocator);
        return null;
    }
    return .{ .layout = layout, .entries = try entries.toOwnedSlice(allocator), .tail = bytes[position..] };
}

fn parse(allocator: std.mem.Allocator, bytes: []const u8) !?Pak {
    if (try parseLayout(allocator, bytes, long_names)) |pak| return pak;
    return parseLayout(allocator, bytes, counted);
}

fn build(allocator: std.mem.Allocator, pak: Pak, index: usize, data: []const u8) ![]u8 {
    var out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(allocator);
    var word: [4]u8 = undefined;
    if (pak.layout.counted) {
        std.mem.writeInt(u32, &word, @intCast(pak.entries.len), .little);
        try out.appendSlice(allocator, &word);
    }
    for (pak.entries, 0..) |entry, position| {
        const content = if (position == index) data else entry.data;
        try out.appendSlice(allocator, entry.raw_name);
        std.mem.writeInt(u32, &word, std.math.cast(u32, content.len) orelse return error.TooLarge, .little);
        try out.appendSlice(allocator, &word);
        try out.appendSlice(allocator, content);
        // The game's padding is not always zeros: it stays unless the size changes.
        if (content.len == entry.data.len) {
            try out.appendSlice(allocator, entry.padding);
        } else {
            try out.appendNTimes(allocator, 0, std.mem.alignForward(usize, out.items.len, pak.layout.alignment) - out.items.len);
        }
    }
    try out.appendSlice(allocator, pak.tail);
    return out.toOwnedSlice(allocator);
}

fn stem(name: []const u8) []const u8 {
    const slash = if (std.mem.lastIndexOfScalar(u8, name, '/')) |at| at + 1 else 0;
    const dot = std.mem.lastIndexOfScalar(u8, name[slash..], '.') orelse return name;
    return name[0 .. slash + dot];
}

/// `container` with the file at `path` replaced, or null when no file answers to
/// it. A path enters a nested container through a folder named after it, as in
/// mod managers: `panel/a.tmx` for `a.tmx` in `panel.bin`. Caller frees.
pub fn replace(allocator: std.mem.Allocator, container: []const u8, path: []const u8, data: []const u8) !?[]u8 {
    const pak = try parse(allocator, container) orelse return null;
    defer allocator.free(pak.entries);
    for (pak.entries, 0..) |entry, index| {
        if (std.ascii.eqlIgnoreCase(entry.name, path)) return try build(allocator, pak, index, data);
    }
    for (pak.entries, 0..) |entry, index| {
        const prefix = stem(entry.name);
        if (path.len <= prefix.len or path[prefix.len] != '/') continue;
        if (!std.ascii.eqlIgnoreCase(path[0..prefix.len], prefix)) continue;
        const inner = try replace(allocator, entry.data, path[prefix.len + 1 ..], data) orelse continue;
        defer allocator.free(inner);
        return try build(allocator, pak, index, inner);
    }
    return null;
}

const testing = std.testing;

fn longFile(out: *std.ArrayList(u8), name: []const u8, data: []const u8) !void {
    var header: [256]u8 = @splat(0);
    @memcpy(header[0..name.len], name);
    std.mem.writeInt(u32, header[252..256], @intCast(data.len), .little);
    try out.appendSlice(testing.allocator, &header);
    try out.appendSlice(testing.allocator, data);
    try out.appendNTimes(testing.allocator, 0, std.mem.alignForward(usize, out.items.len, 64) - out.items.len);
}

fn countedFile(out: *std.ArrayList(u8), name: []const u8, data: []const u8) !void {
    var header: [36]u8 = @splat(0);
    @memcpy(header[0..name.len], name);
    std.mem.writeInt(u32, header[32..36], @intCast(data.len), .little);
    try out.appendSlice(testing.allocator, &header);
    try out.appendSlice(testing.allocator, data);
}

test "replacing a file with itself rebuilds the same bytes" {
    var pak: std.ArrayList(u8) = .empty;
    defer pak.deinit(testing.allocator);
    try longFile(&pak, "a.bf", "first");
    try longFile(&pak, "dir/b.bmd", "second file");
    try pak.appendNTimes(testing.allocator, 0, 256);
    const same = (try replace(testing.allocator, pak.items, "DIR/B.BMD", "second file")).?;
    defer testing.allocator.free(same);
    try testing.expectEqualSlices(u8, pak.items, same);
}

test "a new size moves what follows, and nested containers are rebuilt" {
    var inner: std.ArrayList(u8) = .empty;
    defer inner.deinit(testing.allocator);
    try inner.appendSlice(testing.allocator, &[_]u8{ 2, 0, 0, 0 });
    try countedFile(&inner, "x.bin", "xx");
    try countedFile(&inner, "y.bin", "yy");
    var outer: std.ArrayList(u8) = .empty;
    defer outer.deinit(testing.allocator);
    try longFile(&outer, "panel.bin", inner.items);
    try longFile(&outer, "last.bin", "last");

    const out = (try replace(testing.allocator, outer.items, "panel/x.bin", "longer data")).?;
    defer testing.allocator.free(out);
    const parsed = (try parse(testing.allocator, out)).?;
    defer testing.allocator.free(parsed.entries);
    try testing.expectEqualStrings("last", parsed.entries[1].data);
    const nested = (try parse(testing.allocator, parsed.entries[0].data)).?;
    defer testing.allocator.free(nested.entries);
    try testing.expectEqualStrings("longer data", nested.entries[0].data);
    try testing.expectEqualStrings("yy", nested.entries[1].data);
}

test "paths without a file and data that is no container give null" {
    var pak: std.ArrayList(u8) = .empty;
    defer pak.deinit(testing.allocator);
    try longFile(&pak, "a.bf", "first");
    try testing.expectEqual(@as(?[]u8, null), try replace(testing.allocator, pak.items, "b.bf", "x"));
    try testing.expectEqual(@as(?[]u8, null), try replace(testing.allocator, "\x01\x00\x00\x00not a container", "a", "x"));
    try testing.expectEqual(@as(?[]u8, null), try replace(testing.allocator, "", "a", "x"));
}
