const std = @import("std");
const builtin = @import("builtin");
const cpk = @import("cpk.zig");
const iso = @import("iso.zig");

const usage =
    \\Usage: p3p-cpk list    <image.iso> <cpk path on image> [substring]
    \\       p3p-cpk list    <file.cpk> [substring]
    \\       p3p-cpk extract <image.iso> <cpk path on image> [substring] <out dir>
    \\       p3p-cpk extract <file.cpk> [substring] <out dir>
    \\       p3p-cpk check-trace <image.iso> <io-trace.csv> [prefix=hostdir ...]
    \\       p3p-cpk --help | --version
    \\list prints every file: dir/name, offset, stored and extracted size, compressed.
    \\extract writes files under their CPK directories, decompressing CRILAYLA.
    \\Every output path is checked before anything is written; `..`, absolute
    \\paths and names unsafe on Windows are refused. Bytes above ASCII become %XX.
    \\check-trace verifies the reads of p3p_pc_bootstrap --io-trace: each read of a
    \\.cpk must be metadata or lie in one file with the same FNV-1a 64. prefix=hostdir
    \\maps traced paths to host files, e.g. ms0:/PSP/P3P=.tmp/mods.
    \\Exit: 0 success, 1 check-trace found differences, 2 usage/input/output error.
    \\
;

/// A range of an open host file.
const View = struct {
    io: std.Io,
    file: std.Io.File,
    base: u64,

    fn readAt(context: *const anyopaque, offset: u64, buffer: []u8) anyerror!void {
        const view: *const View = @ptrCast(@alignCast(context));
        const count = try view.file.readPositionalAll(view.io, buffer, view.base + offset);
        if (count != buffer.len) return error.EndOfFile;
    }

    fn source(view: *const View, size: u64) cpk.Source {
        return .{ .context = view, .readFn = readAt, .size = size };
    }
};

const Container = struct {
    file: std.Io.File,
    view: View,
    size: u64,
    is_cpk: bool,

    fn open(self: *Container, io: std.Io, path: []const u8) !void {
        self.file = try std.Io.Dir.cwd().openFile(io, path, .{});
        errdefer self.file.close(io);
        self.size = try self.file.length(io);
        self.view = .{ .io = io, .file = self.file, .base = 0 };
        self.is_cpk = false;
        var magic: [4]u8 = undefined;
        if (self.size >= magic.len) {
            try self.view.source(self.size).read(0, &magic);
            self.is_cpk = std.mem.eql(u8, &magic, "CPK ");
        }
    }

    /// Narrows the view to a CPK on the image.
    fn select(self: *Container, allocator: std.mem.Allocator, path: []const u8) !void {
        const extent = try iso.find(allocator, self.view.source(self.size), path);
        const end = std.math.add(u64, extent.offset, extent.size) catch return error.OutOfBounds;
        if (end > self.size) return error.OutOfBounds;
        self.view.base = extent.offset;
        self.size = extent.size;
    }
};

pub fn main(init: std.process.Init) void {
    const code = run(init) catch |err| {
        var buffer: [1024]u8 = undefined;
        var stderr: std.Io.File.Writer = .initStreaming(.stderr(), init.io, &buffer);
        stderr.interface.print("p3p-cpk: {s}\n", .{@errorName(err)}) catch {};
        stderr.interface.flush() catch {};
        std.process.exit(2);
    };
    std.process.exit(code);
}

fn run(init: std.process.Init) !u8 {
    const args = try init.minimal.args.toSlice(init.arena.allocator());
    var output: std.Io.Writer.Allocating = .init(init.gpa);
    defer output.deinit();
    const code = dispatch(init, &output.writer, args) catch |err| {
        writeStdout(init.io, output.written()) catch {};
        return err;
    };
    try writeStdout(init.io, output.written());
    return code;
}

extern "kernel32" fn WriteFile(file: std.os.windows.HANDLE, buffer: [*]const u8, size: u32, written: *u32, overlapped: ?*anyopaque) callconv(.winapi) c_int;
extern "kernel32" fn GetLastError() callconv(.winapi) u32;

/// Pipes of Git Bash/MSYS are asynchronous, and the Zig 0.17.1 file writers panic
/// when such a pipe fills up (`.PENDING => unreachable`); WriteFile waits instead.
/// A reader that went away (`| head`) is not an error.
fn writeStdout(io: std.Io, bytes: []const u8) !void {
    const stdout = std.Io.File.stdout();
    if (builtin.os.tag != .windows) {
        stdout.writeStreamingAll(io, bytes) catch |err| switch (err) {
            error.BrokenPipe => {},
            else => return err,
        };
        return;
    }
    var rest = bytes;
    while (rest.len != 0) {
        var written: u32 = 0;
        const count: u32 = @intCast(@min(rest.len, 1 << 20));
        if (WriteFile(stdout.handle, rest.ptr, count, &written, null) == 0) {
            const broken_pipe = 109;
            const no_data = 232;
            const code = GetLastError();
            if (code == broken_pipe or code == no_data) return;
            return error.WriteFailed;
        }
        rest = rest[written..];
    }
}

fn dispatch(init: std.process.Init, writer: *std.Io.Writer, args: []const []const u8) !u8 {
    if (args.len < 2) return error.ExpectedCommand;
    const command = args[1];
    if (std.mem.eql(u8, command, "--help")) {
        try writer.writeAll(usage);
        return 0;
    }
    if (std.mem.eql(u8, command, "--version")) {
        try writer.writeAll("p3p-cpk 0.1.0 (Zig 0.17.1)\n");
        return 0;
    }
    const rest = args[2..];
    if (std.mem.eql(u8, command, "list")) return archiveCommand(init, writer, rest, false);
    if (std.mem.eql(u8, command, "extract")) return archiveCommand(init, writer, rest, true);
    if (std.mem.eql(u8, command, "check-trace")) return checkTrace(init, writer, rest);
    return error.UnknownCommand;
}

fn archiveCommand(init: std.process.Init, writer: *std.Io.Writer, args: []const []const u8, is_extract: bool) !u8 {
    const allocator = init.gpa;
    if (args.len == 0) return error.ExpectedPath;
    var container: Container = undefined;
    try container.open(init.io, args[0]);
    defer container.file.close(init.io);
    var operands = args[1..];
    if (!container.is_cpk) {
        if (operands.len == 0) return error.ExpectedCpkPathOnImage;
        try container.select(allocator, operands[0]);
        operands = operands[1..];
    }
    var out_dir: []const u8 = "";
    if (is_extract) {
        if (operands.len == 0) return error.ExpectedOutputDirectory;
        out_dir = operands[operands.len - 1];
        operands = operands[0 .. operands.len - 1];
    }
    if (operands.len > 1) return error.TooManyArguments;
    const filter = if (operands.len == 1) operands[0] else "";

    const source = container.view.source(container.size);
    const archive = try cpk.Archive.open(allocator, source);
    defer archive.deinit(allocator);
    if (is_extract) return extractFiles(init, writer, source, archive, filter, out_dir);
    for (archive.files) |file| {
        if (!try file.matches(allocator, filter)) continue;
        try writer.print("{s}/{s} offset=0x{X} size=0x{X} extract=0x{X} compressed={d}\n", .{
            file.dir,
            file.name,
            file.offset,
            file.size,
            file.extract_size,
            @intFromBool(file.listedCompressed()),
        });
    }
    return 0;
}

fn extractFiles(init: std.process.Init, writer: *std.Io.Writer, source: cpk.Source, archive: cpk.Archive, filter: []const u8, out_dir: []const u8) !u8 {
    const allocator = init.gpa;
    const io = init.io;
    var paths: std.ArrayList([]u8) = .empty;
    defer {
        for (paths.items) |path| allocator.free(path);
        paths.deinit(allocator);
    }
    var selected: std.ArrayList(cpk.File) = .empty;
    defer selected.deinit(allocator);
    // Refuse a bad archive before the first byte is written.
    for (archive.files) |file| {
        if (!try file.matches(allocator, filter)) continue;
        const path = try cpk.safePath(allocator, file.dir, file.name);
        errdefer allocator.free(path);
        try paths.append(allocator, path);
        try selected.append(allocator, file);
    }
    const cwd = std.Io.Dir.cwd();
    try cwd.createDirPath(io, out_dir);
    var out = try cwd.openDir(io, out_dir, .{});
    defer out.close(io);
    for (selected.items, paths.items) |file, path| {
        const data = try cpk.extract(allocator, source, file);
        defer allocator.free(data);
        if (std.fs.path.dirnamePosix(path)) |parent| try out.createDirPath(io, parent);
        try out.writeFile(io, .{ .sub_path = path, .data = data });
    }
    try writer.print("extracted {d} files\n", .{selected.items.len});
    return 0;
}

const TracedCpk = struct {
    owned_file: ?std.Io.File,
    view: *View,
    size: u64,
    archive: cpk.Archive,
};

fn hashRange(source: cpk.Source, offset: u64, size: u64) !u64 {
    var hash = cpk.fnv_offset;
    var buffer: [65536]u8 = undefined;
    var done: u64 = 0;
    while (done < size) {
        const count: usize = @intCast(@min(size - done, buffer.len));
        try source.read(offset + done, buffer[0..count]);
        hash = cpk.fnv1a(hash, buffer[0..count]);
        done += count;
    }
    return hash;
}

fn hostPath(allocator: std.mem.Allocator, maps: []const []const u8, path: []const u8) !?[]u8 {
    for (maps) |map| {
        const split = std.mem.indexOfScalar(u8, map, '=') orelse return error.BadPrefixMap;
        const prefix = map[0..split];
        if (path.len < prefix.len or !std.ascii.eqlIgnoreCase(path[0..prefix.len], prefix)) continue;
        const relative = std.mem.trimStart(u8, path[prefix.len..], "/");
        return try std.fmt.allocPrint(allocator, "{s}/{s}", .{ map[split + 1 ..], relative });
    }
    return null;
}

fn openTraced(init: std.process.Init, writer: *std.Io.Writer, image: *const Container, maps: []const []const u8, path: []const u8) !TracedCpk {
    const allocator = init.gpa;
    const io = init.io;
    const view = try allocator.create(View);
    errdefer allocator.destroy(view);
    var owned_file: ?std.Io.File = null;
    errdefer if (owned_file) |file| file.close(io);
    var size: u64 = 0;
    if (try hostPath(allocator, maps, path)) |host| {
        defer allocator.free(host);
        const file = try std.Io.Dir.cwd().openFile(io, host, .{});
        owned_file = file;
        size = try file.length(io);
        view.* = .{ .io = io, .file = file, .base = 0 };
    } else {
        const start = if (std.mem.indexOfScalar(u8, path, ':')) |colon| colon + 1 else 0;
        const extent = try iso.find(allocator, image.view.source(image.size), path[start..]);
        size = extent.size;
        view.* = .{ .io = io, .file = image.file, .base = extent.offset };
    }
    const archive = try cpk.Archive.open(allocator, view.source(size));
    errdefer archive.deinit(allocator);
    var first_data = size;
    if (archive.files.len != 0) {
        first_data = std.math.maxInt(u64);
        for (archive.files) |file| first_data = @min(first_data, file.offset);
    }
    try writer.print("{s}: {d} files, first data at 0x{X}, size 0x{X}\n", .{ path, archive.files.len, first_data, size });
    return .{ .owned_file = owned_file, .view = view, .size = size, .archive = archive };
}

fn lessThan(_: void, a: []const u8, b: []const u8) bool {
    return std.mem.lessThan(u8, a, b);
}

fn checkTrace(init: std.process.Init, writer: *std.Io.Writer, args: []const []const u8) !u8 {
    const allocator = init.gpa;
    const io = init.io;
    if (args.len < 2) return error.ExpectedImageAndTrace;
    var image: Container = undefined;
    try image.open(io, args[0]);
    defer image.file.close(io);
    const maps = args[2..];
    const trace = try std.Io.Dir.cwd().readFileAlloc(io, args[1], allocator, .limited(cpk.max_file_size));
    defer allocator.free(trace);

    var opened: std.StringHashMapUnmanaged(TracedCpk) = .empty;
    defer {
        var values = opened.valueIterator();
        while (values.next()) |traced| {
            traced.archive.deinit(allocator);
            if (traced.owned_file) |file| file.close(io);
            allocator.destroy(traced.view);
        }
        opened.deinit(allocator);
    }
    var per_file: std.StringHashMapUnmanaged(u32) = .empty;
    defer {
        var keys = per_file.keyIterator();
        while (keys.next()) |key| allocator.free(key.*);
        per_file.deinit(allocator);
    }

    var total: u64 = 0;
    var header: u64 = 0;
    var matched: u64 = 0;
    var mismatched: u64 = 0;
    var unclassified: u64 = 0;
    var skipped: u64 = 0;
    var lines = std.mem.splitScalar(u8, trace, '\n');
    _ = lines.next(); // column names
    while (lines.next()) |raw_line| {
        const line = std.mem.trimEnd(u8, raw_line, "\r");
        var fields = std.mem.splitScalar(u8, line, ',');
        const path = fields.next() orelse continue;
        const offset_text = fields.next() orelse continue;
        const size_text = fields.next() orelse continue;
        const hash_text = fields.next() orelse continue;
        if (!std.ascii.endsWithIgnoreCase(path, ".cpk")) {
            skipped += 1;
            continue;
        }
        const offset = try std.fmt.parseInt(u64, offset_text, 10);
        const size = try std.fmt.parseInt(u64, size_text, 10);
        const hex = if (std.mem.startsWith(u8, hash_text, "0x")) hash_text[2..] else hash_text;
        const hash = try std.fmt.parseInt(u64, hex, 16);
        const end = std.math.add(u64, offset, size) catch return error.OutOfBounds;
        total += 1;

        if (!opened.contains(path)) {
            var traced = try openTraced(init, writer, &image, maps, path);
            errdefer {
                traced.archive.deinit(allocator);
                if (traced.owned_file) |file| file.close(io);
                allocator.destroy(traced.view);
            }
            try opened.put(allocator, path, traced);
        }
        const traced = opened.getPtr(path).?;

        var overlaps = false;
        var hits: usize = 0;
        var hit: cpk.File = undefined;
        for (traced.archive.files) |file| {
            const file_end = file.offset + file.size;
            if (offset < file_end and file.offset < end) overlaps = true;
            if (offset >= file.offset and end <= file_end) {
                hits += 1;
                hit = file;
            }
        }
        // Metadata (header, TOC, ITOC/ETOC) is every byte outside file data.
        if (!overlaps) {
            header += 1;
            continue;
        }
        if (hits != 1) {
            unclassified += 1;
            if (unclassified <= 10) try writer.print("UNCLASSIFIED {s} 0x{X}+0x{X}\n", .{ path, offset, size });
            continue;
        }
        const key = try std.fmt.allocPrint(allocator, "{s}/{s}", .{ hit.dir, hit.name });
        const count = try per_file.getOrPut(allocator, key);
        if (count.found_existing) {
            allocator.free(key);
            count.value_ptr.* += 1;
        } else {
            count.value_ptr.* = 1;
        }
        if (try hashRange(traced.view.source(traced.size), offset, size) == hash) {
            matched += 1;
        } else {
            mismatched += 1;
            if (mismatched <= 10) try writer.print("MISMATCH {s} 0x{X}+0x{X}\n", .{ count.key_ptr.*, offset, size });
        }
    }

    var keys: std.ArrayList([]const u8) = .empty;
    defer keys.deinit(allocator);
    var key_iterator = per_file.keyIterator();
    while (key_iterator.next()) |key| try keys.append(allocator, key.*);
    std.mem.sort([]const u8, keys.items, {}, lessThan);
    for (keys.items[0..@min(keys.items.len, 20)]) |key|
        try writer.print("  read {d}x {s}\n", .{ per_file.get(key).?, key });
    try writer.print("cpk reads={d} header/toc={d} file_matched={d} mismatched={d} unclassified={d} non_cpk={d}\n", .{ total, header, matched, mismatched, unclassified, skipped });
    return if (mismatched == 0 and unclassified == 0) 0 else 1;
}
