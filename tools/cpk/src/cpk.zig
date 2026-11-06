//! CRI CPK archives: @UTF tables, the TOC file list and CRILAYLA decompression.
//! Inputs are community files: every offset is checked, and bad data is an
//! error, never a crash. experiments/cpk-check (CriFsV2Lib) is the independent
//! oracle this tool is compared with.
const std = @import("std");

pub const max_table_size: u64 = 256 * 1024 * 1024;
pub const max_file_size: u64 = 1024 * 1024 * 1024;

/// Random-access bytes: a host file or a range of an ISO.
pub const Source = struct {
    context: *const anyopaque,
    readFn: *const fn (context: *const anyopaque, offset: u64, buffer: []u8) anyerror!void,
    size: u64,

    pub fn read(self: Source, offset: u64, buffer: []u8) !void {
        const end = std.math.add(u64, offset, buffer.len) catch return error.OutOfBounds;
        if (end > self.size) return error.OutOfBounds;
        return self.readFn(self.context, offset, buffer);
    }

    pub fn alloc(self: Source, allocator: std.mem.Allocator, offset: u64, size: u64, limit: u64) ![]u8 {
        if (size > limit or size > self.size) return error.TooLarge;
        const buffer = try allocator.alloc(u8, @intCast(size));
        errdefer allocator.free(buffer);
        try self.read(offset, buffer);
        return buffer;
    }
};

const ColumnKind = enum(u4) {
    u8,
    i8,
    u16,
    i16,
    u32,
    i32,
    u64,
    i64,
    f32,
    f64,
    string,
    raw,
    guid,
    _,

    fn size(kind: ColumnKind) !usize {
        return switch (kind) {
            .u8, .i8 => 1,
            .u16, .i16 => 2,
            .u32, .i32, .f32, .string => 4,
            .u64, .i64, .f64, .raw => 8,
            .guid => 16,
            _ => error.BadColumnType,
        };
    }
};

const Column = struct {
    name: []const u8,
    kind: ColumnKind,
    default_offset: ?usize,
    row_offset: ?usize,
};

const has_name = 0x10;
const has_default = 0x20;
const has_row_storage = 0x40;

fn be(comptime T: type, data: []const u8, offset: usize) !T {
    const end = std.math.add(usize, offset, @sizeOf(T)) catch return error.OutOfBounds;
    if (end > data.len) return error.OutOfBounds;
    return std.mem.readInt(T, data[offset..][0..@sizeOf(T)], .big);
}

const Table = struct {
    data: []const u8,
    columns: []Column,
    rows_offset: usize,
    strings_offset: usize,
    row_size: usize,
    row_count: usize,

    fn parse(allocator: std.mem.Allocator, data: []const u8) !Table {
        if (data.len < 0x20 or !std.mem.eql(u8, data[0..4], "@UTF")) return error.BadTable;
        const rows_offset = @as(usize, try be(u16, data, 0x0A)) + 8;
        const strings_offset = @as(usize, try be(u32, data, 0x0C)) + 8;
        const column_count = try be(u16, data, 0x18);
        const row_size = try be(u16, data, 0x1A);
        const row_count = try be(u32, data, 0x1C);
        const rows_bytes = std.math.mul(usize, row_size, row_count) catch return error.BadTable;
        const rows_end = std.math.add(usize, rows_offset, rows_bytes) catch return error.BadTable;
        if (rows_end > data.len or strings_offset > data.len) return error.BadTable;

        var table: Table = .{
            .data = data,
            .columns = try allocator.alloc(Column, column_count),
            .rows_offset = rows_offset,
            .strings_offset = strings_offset,
            .row_size = row_size,
            .row_count = row_count,
        };
        errdefer allocator.free(table.columns);
        var cursor: usize = 0x20;
        var row_offset: usize = 0;
        for (table.columns) |*column_entry| {
            if (cursor >= data.len) return error.BadTable;
            const flags = data[cursor];
            cursor += 1;
            const kind: ColumnKind = @fromBackingInt(@intCast(@as(u4, @truncate(flags))));
            const size = try kind.size();
            column_entry.* = .{ .name = "", .kind = kind, .default_offset = null, .row_offset = null };
            if (flags & has_name != 0) {
                column_entry.name = try table.stringAt(try be(u32, data, cursor));
                cursor += 4;
            }
            if (flags & has_default != 0) {
                if (cursor + size > data.len) return error.BadTable;
                column_entry.default_offset = cursor;
                cursor += size;
            }
            if (flags & has_row_storage != 0) {
                column_entry.row_offset = row_offset;
                row_offset += size;
            }
        }
        if (row_offset > row_size) return error.BadTable;
        return table;
    }

    fn deinit(table: Table, allocator: std.mem.Allocator) void {
        allocator.free(table.columns);
    }

    fn stringAt(table: Table, offset: u32) ![]const u8 {
        const start = std.math.add(usize, table.strings_offset, offset) catch return error.BadString;
        if (start >= table.data.len) return error.BadString;
        const length = std.mem.indexOfScalar(u8, table.data[start..], 0) orelse return error.BadString;
        return table.data[start..][0..length];
    }

    fn column(table: Table, name: []const u8) ?*const Column {
        for (table.columns) |*candidate|
            if (std.mem.eql(u8, candidate.name, name)) return candidate;
        return null;
    }

    fn valueOffset(table: Table, col: *const Column, row: usize) ?usize {
        if (col.row_offset) |offset| return table.rows_offset + row * table.row_size + offset;
        return col.default_offset;
    }

    /// Null when the column has neither a row value nor a default.
    fn number(table: Table, col: *const Column, row: usize) !?u64 {
        const offset = table.valueOffset(col, row) orelse return null;
        return switch (col.kind) {
            .u8, .i8 => try be(u8, table.data, offset),
            .u16, .i16 => try be(u16, table.data, offset),
            .u32, .i32 => try be(u32, table.data, offset),
            .u64, .i64 => try be(u64, table.data, offset),
            else => error.BadColumnType,
        };
    }

    fn string(table: Table, col: *const Column, row: usize) ![]const u8 {
        const offset = table.valueOffset(col, row) orelse return "";
        if (col.kind != .string) return error.BadColumnType;
        return table.stringAt(try be(u32, table.data, offset));
    }
};

/// Tables may be masked with a byte stream that starts at 0x5F and multiplies by 0x15.
fn unmask(data: []u8) void {
    var mask: u8 = 0x5F;
    for (data) |*byte| {
        byte.* ^= mask;
        mask *%= 0x15;
    }
}

/// A chunk is `magic`, 4 bytes of flags, a 64-bit little-endian size, then the table.
fn readChunk(allocator: std.mem.Allocator, source: Source, offset: u64, magic: *const [4]u8) ![]u8 {
    var header: [16]u8 = undefined;
    try source.read(offset, &header);
    if (!std.mem.eql(u8, header[0..4], magic)) return error.BadChunk;
    const size = std.mem.readInt(u64, header[8..16], .little);
    const data = try source.alloc(allocator, offset + header.len, size, max_table_size);
    errdefer allocator.free(data);
    if (data.len >= 4 and std.mem.eql(u8, data[0..4], "\x1F\x9E\xF3\xF5")) unmask(data);
    return data;
}

pub const File = struct {
    dir: []const u8,
    name: []const u8,
    /// From the start of the CPK.
    offset: u64,
    /// Stored size; `extract_size` is the size the TOC gives after decompression.
    size: u64,
    extract_size: u64,

    /// From the TOC alone: `extract` decides by the data, but reading it for every
    /// file of a listing means thousands of seeks across the image.
    pub fn listedCompressed(file: File) bool {
        return file.extract_size != 0 and file.extract_size != file.size;
    }

    /// Case-insensitive substring of `dir/name`.
    pub fn matches(file: File, allocator: std.mem.Allocator, filter: []const u8) !bool {
        if (filter.len == 0) return true;
        const key = try std.fmt.allocPrint(allocator, "{s}/{s}", .{ file.dir, file.name });
        defer allocator.free(key);
        return std.ascii.findIgnoreCase(key, filter) != null;
    }
};

pub const Archive = struct {
    toc: []u8,
    files: []File,

    pub fn open(allocator: std.mem.Allocator, source: Source) !Archive {
        const header_data = try readChunk(allocator, source, 0, "CPK ");
        defer allocator.free(header_data);
        const header = try Table.parse(allocator, header_data);
        defer header.deinit(allocator);
        if (header.row_count == 0) return error.BadTable;
        const toc_column = header.column("TocOffset") orelse return error.NoToc;
        const content_column = header.column("ContentOffset") orelse return error.MissingColumn;
        const toc_offset = try header.number(toc_column, 0) orelse return error.NoToc;
        var content_offset = try header.number(content_column, 0) orelse return error.MissingColumn;
        if (toc_offset == 0) return error.NoToc;
        // File offsets count from the TOC when it comes before the content.
        content_offset = @min(content_offset, toc_offset);

        const toc_data = try readChunk(allocator, source, toc_offset, "TOC ");
        errdefer allocator.free(toc_data);
        const toc = try Table.parse(allocator, toc_data);
        defer toc.deinit(allocator);
        const name = toc.column("FileName") orelse return error.MissingColumn;
        const size = toc.column("FileSize") orelse return error.MissingColumn;
        const offset = toc.column("FileOffset") orelse return error.MissingColumn;
        const dir = toc.column("DirName");
        const extract_column = toc.column("ExtractSize");

        const files = try allocator.alloc(File, toc.row_count);
        errdefer allocator.free(files);
        for (files, 0..) |*file, row| {
            file.* = .{
                .dir = if (dir) |col| try toc.string(col, row) else "",
                .name = try toc.string(name, row),
                .size = try toc.number(size, row) orelse return error.MissingColumn,
                .offset = 0,
                .extract_size = if (extract_column) |col| try toc.number(col, row) orelse 0 else 0,
            };
            const relative = try toc.number(offset, row) orelse return error.MissingColumn;
            file.offset = std.math.add(u64, relative, content_offset) catch return error.OutOfBounds;
            const end = std.math.add(u64, file.offset, file.size) catch return error.OutOfBounds;
            if (end > source.size and file.size != 0) return error.OutOfBounds;
        }
        return .{ .toc = toc_data, .files = files };
    }

    pub fn deinit(archive: Archive, allocator: std.mem.Allocator) void {
        allocator.free(archive.files);
        allocator.free(archive.toc);
    }
};

const layla_prefix = 0x100;
const layla_header = 0x10;

fn isCompressed(source: Source, file: File) !bool {
    if (file.size < layla_header) return false;
    var magic: [8]u8 = undefined;
    try source.read(file.offset, &magic);
    return std.mem.eql(u8, &magic, "CRILAYLA");
}

/// A file outside any archive, such as one in a `bind/` folder: all of `source`.
pub fn looseFile(source: Source, dir: []const u8, name: []const u8) !File {
    var file: File = .{ .dir = dir, .name = name, .offset = 0, .size = source.size, .extract_size = source.size };
    if (try isCompressed(source, file)) {
        var header: [layla_header]u8 = undefined;
        try source.read(0, &header);
        file.extract_size = @as(u64, std.mem.readInt(u32, header[8..12], .little)) + layla_prefix;
    }
    return file;
}

/// The file's content, decompressed when it is CRILAYLA. Caller frees.
pub fn extract(allocator: std.mem.Allocator, source: Source, file: File) ![]u8 {
    if (file.size == 0) return allocator.alloc(u8, 0);
    if (!try isCompressed(source, file)) return source.alloc(allocator, file.offset, file.size, max_file_size);
    var header: [layla_header]u8 = undefined;
    try source.read(file.offset, &header);
    // Some archives store a size smaller than the compressed data: trust the data's header.
    const stream_size = @as(u64, std.mem.readInt(u32, header[12..16], .little)) + layla_header + layla_prefix;
    const raw = try source.alloc(allocator, file.offset, @max(file.size, stream_size), max_file_size);
    defer allocator.free(raw);
    return decompress(allocator, raw);
}

const BitReader = struct {
    data: []const u8,
    position: usize,
    bits_left: u4 = 0,

    /// The stream runs backwards from the prefix to the end of the header, MSB first.
    fn take(self: *BitReader, count: u4) !u16 {
        var result: u16 = 0;
        var remaining = count;
        while (remaining != 0) {
            if (self.bits_left == 0) {
                if (self.position == layla_header) return error.TruncatedStream;
                self.position -= 1;
                self.bits_left = 8;
            }
            const now = @min(remaining, self.bits_left);
            const shift: u3 = @intCast(self.bits_left - now);
            const mask = (@as(u16, 1) << now) - 1;
            result = (result << now) | ((self.data[self.position] >> shift) & mask);
            self.bits_left -= now;
            remaining -= now;
        }
        return result;
    }
};

/// LZSS written from the end of the output; the first 0x100 bytes are stored
/// uncompressed after the stream.
pub fn decompress(allocator: std.mem.Allocator, data: []const u8) ![]u8 {
    if (data.len < layla_header or !std.mem.eql(u8, data[0..8], "CRILAYLA")) return error.BadStream;
    const body_size: u64 = std.mem.readInt(u32, data[8..12], .little);
    const prefix_offset = @as(u64, std.mem.readInt(u32, data[12..16], .little)) + layla_header;
    if (prefix_offset + layla_prefix > data.len) return error.TruncatedStream;
    if (body_size + layla_prefix > max_file_size) return error.TooLarge;

    const start: usize = @intCast(prefix_offset);
    const out = try allocator.alloc(u8, @intCast(body_size + layla_prefix));
    errdefer allocator.free(out);
    @memcpy(out[0..layla_prefix], data[start..][0..layla_prefix]);

    var bits: BitReader = .{ .data = data, .position = start };
    var write = out.len; // one past the next byte to write
    while (write > layla_prefix) {
        if (try bits.take(1) == 0) {
            write -= 1;
            out[write] = @intCast(try bits.take(8));
            continue;
        }
        const distance = @as(usize, try bits.take(13)) + 3;
        var length: usize = 3;
        const widths = [_]u4{ 2, 3, 5, 8 };
        var level: usize = 0;
        while (true) {
            const width = widths[level];
            const value = try bits.take(width);
            length += value;
            if (value != (@as(u16, 1) << width) - 1) break;
            level = @min(level + 1, widths.len - 1);
        }
        if (length > write - layla_prefix) return error.CorruptStream;
        for (0..length) |_| {
            write -= 1;
            const from = write + distance;
            if (from >= out.len) return error.CorruptStream;
            out[write] = out[from];
        }
    }
    return out;
}

pub const fnv_offset: u64 = 0xCBF29CE484222325;

pub fn fnv1a(hash: u64, data: []const u8) u64 {
    var result = hash;
    for (data) |byte| {
        result ^= byte;
        result *%= 0x100000001B3;
    }
    return result;
}

fn isReservedName(component: []const u8) bool {
    const stem = component[0 .. std.mem.indexOfScalar(u8, component, '.') orelse component.len];
    const names = [_][]const u8{ "con", "prn", "aux", "nul" };
    for (names) |name| if (std.ascii.eqlIgnoreCase(stem, name)) return true;
    if (stem.len == 4 and std.ascii.isDigit(stem[3]))
        return std.ascii.eqlIgnoreCase(stem[0..3], "com") or std.ascii.eqlIgnoreCase(stem[0..3], "lpt");
    return false;
}

/// A relative path for `dir/name` that stays inside the output directory on
/// Windows and Linux, or error.UnsafePath. Bytes above ASCII (Shift-JIS names)
/// become %XX. Caller frees.
pub fn safePath(allocator: std.mem.Allocator, dir: []const u8, name: []const u8) ![]u8 {
    var out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(allocator);
    const parts = [_][]const u8{ dir, name };
    for (parts, 0..) |part, index| {
        if (index == 0 and part.len == 0) continue;
        var components = std.mem.splitAny(u8, part, "/\\");
        while (components.next()) |component| {
            if (component.len == 0 or isReservedName(component)) return error.UnsafePath;
            const last = component[component.len - 1];
            if (last == '.' or last == ' ') return error.UnsafePath; // also `.` and `..`
            if (out.items.len != 0) try out.append(allocator, '/');
            for (component) |byte| {
                if (byte < 0x20 or byte == 0x7F or std.mem.indexOfScalar(u8, "<>:\"|?*", byte) != null)
                    return error.UnsafePath;
                if (byte < 0x80) {
                    try out.append(allocator, byte);
                } else {
                    var hex: [3]u8 = undefined;
                    try out.appendSlice(allocator, try std.fmt.bufPrint(&hex, "%{X:0>2}", .{byte}));
                }
            }
        }
    }
    return out.toOwnedSlice(allocator);
}

const testing = std.testing;

test {
    _ = @import("vcdiff.zig");
}

test "literal and copy blocks decompress from the end" {
    // Written backwards: literals 'c', 'b', 'a', then 3 bytes copied from
    // distance 3, which gives "abcabc" after the prefix.
    var stream: [layla_header + 8 + layla_prefix]u8 = @splat(0);
    @memcpy(stream[0..8], "CRILAYLA");
    std.mem.writeInt(u32, stream[8..12], 6, .little);
    std.mem.writeInt(u32, stream[12..16], 8, .little);
    for (stream[layla_header + 8 ..], 0..) |*byte, index| byte.* = @truncate(index);
    const bit_text = "0" ++ "01100011" ++ "0" ++ "01100010" ++ "0" ++ "01100001" ++ "1" ++ "0000000000000" ++ "00";
    var position: usize = layla_header + 8;
    var used: u4 = 8;
    for (bit_text) |bit| {
        if (used == 8) {
            position -= 1;
            used = 0;
        }
        if (bit == '1') stream[position] |= @as(u8, 0x80) >> @intCast(used);
        used += 1;
    }
    const out = try decompress(testing.allocator, &stream);
    defer testing.allocator.free(out);
    try testing.expectEqual(@as(usize, layla_prefix + 6), out.len);
    try testing.expectEqualStrings("abcabc", out[layla_prefix..]);
    try testing.expectEqual(@as(u8, 0xFF), out[0xFF]);
}

test "a short stream is an error" {
    var stream: [layla_header + 1 + layla_prefix]u8 = @splat(0xFF);
    @memcpy(stream[0..8], "CRILAYLA");
    std.mem.writeInt(u32, stream[8..12], 6, .little);
    std.mem.writeInt(u32, stream[12..16], 1, .little);
    try testing.expectError(error.TruncatedStream, decompress(testing.allocator, &stream));
    std.mem.writeInt(u32, stream[12..16], 0x7FFF_FFFF, .little);
    try testing.expectError(error.TruncatedStream, decompress(testing.allocator, &stream));
}

test "unsafe paths are refused" {
    const bad = [_][2][]const u8{
        .{ "..", "a" },     .{ "a/../..", "b" }, .{ "", "../a" }, .{ "/etc", "a" },
        .{ "C:", "a" },     .{ "a\\..", "b" },   .{ "", "" },     .{ "a", "b/" },
        .{ "", "NUL.txt" }, .{ "", "com1" },     .{ "", "a." },   .{ "", "a b " },
        .{ "", "a\x01" },   .{ "", "a:b" },      .{ ".", "a" },
    };
    for (bad) |pair| try testing.expectError(error.UnsafePath, safePath(testing.allocator, pair[0], pair[1]));
}

test "safe paths keep their directories" {
    const path = try safePath(testing.allocator, "data\\battle", "a.bin");
    defer testing.allocator.free(path);
    try testing.expectEqualStrings("data/battle/a.bin", path);
    const encoded = try safePath(testing.allocator, "", "\x83\xA0.bin");
    defer testing.allocator.free(encoded);
    try testing.expectEqualStrings("%83%A0.bin", encoded);
}

test "table masking is its own inverse" {
    var data = "@UTF....".*;
    unmask(&data);
    try testing.expectEqualSlices(u8, "\x1F\x9E\xF3\xF5", data[0..4]);
    unmask(&data);
    try testing.expectEqualStrings("@UTF....", &data);
}

test "fnv1a 64" {
    try testing.expectEqual(@as(u64, 0xAF63DC4C8601EC8C), fnv1a(fnv_offset, "a"));
}
