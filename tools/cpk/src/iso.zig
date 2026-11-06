//! Minimal ISO9660 path lookup (case-insensitive) for a CPK on a UMD image.
const std = @import("std");
const cpk = @import("cpk.zig");

pub const sector = 2048;
const max_directory_size = 64 * 1024 * 1024;
const directory_flag = 2;

pub const Extent = struct { offset: u64, size: u64 };

fn entryExtent(record: []const u8) Extent {
    return .{
        .offset = @as(u64, std.mem.readInt(u32, record[2..6], .little)) * sector,
        .size = std.mem.readInt(u32, record[10..14], .little),
    };
}

fn findEntry(directory: []const u8, name: []const u8) ?[]const u8 {
    var offset: usize = 0;
    while (offset < directory.len) {
        const length = directory[offset];
        if (length == 0) { // records never cross a sector: padding up to the next one
            offset = (offset / sector + 1) * sector;
            continue;
        }
        if (length < 34 or offset + length > directory.len) return null;
        const record = directory[offset..][0..length];
        const name_length = record[32];
        if (33 + @as(usize, name_length) > length) return null;
        const full = record[33..][0..name_length];
        const stem = full[0 .. std.mem.indexOfScalar(u8, full, ';') orelse full.len];
        if (std.ascii.eqlIgnoreCase(stem, name)) return record;
        offset += length;
    }
    return null;
}

/// `path` like `/PSP_GAME/USRDIR/umd0.cpk`.
pub fn find(allocator: std.mem.Allocator, source: cpk.Source, path: []const u8) !Extent {
    var descriptor: [sector]u8 = undefined;
    try source.read(16 * sector, &descriptor);
    if (descriptor[0] != 1 or !std.mem.eql(u8, descriptor[1..6], "CD001")) return error.NotIso;
    var extent = entryExtent(descriptor[156..190]);
    var is_directory = true;
    var parts = std.mem.tokenizeScalar(u8, path, '/');
    while (parts.next()) |part| {
        if (!is_directory) return error.NotFoundOnImage;
        const directory = try source.alloc(allocator, extent.offset, extent.size, max_directory_size);
        defer allocator.free(directory);
        const record = findEntry(directory, part) orelse return error.NotFoundOnImage;
        extent = entryExtent(record);
        is_directory = record[25] & directory_flag != 0;
    }
    if (is_directory) return error.NotFoundOnImage;
    return extent;
}
