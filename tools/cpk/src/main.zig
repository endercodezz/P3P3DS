const std = @import("std");
const builtin = @import("builtin");
const cpk = @import("cpk.zig");
const iso = @import("iso.zig");
const vcdiff = @import("vcdiff.zig");

const usage =
    \\Usage: p3p-cpk list    <image.iso> <cpk path on image> [substring]
    \\       p3p-cpk list    <file.cpk> [substring]
    \\       p3p-cpk extract <image.iso> <cpk path on image> [substring] <out dir>
    \\       p3p-cpk extract <file.cpk> [substring] <out dir>
    \\       p3p-cpk diff    <mod.cpk|mod dir> <base.cpk>
    \\       p3p-cpk diff    <mod.cpk|mod dir> <image.iso> <cpk path on image>...
    \\       p3p-cpk patch   <source> <patch.xdelta> <output>
    \\       p3p-cpk check-trace <image.iso> <io-trace.csv> [prefix=hostdir ...]
    \\       p3p-cpk --help | --version
    \\list prints every file: dir/name, offset, stored and extracted size, compressed.
    \\extract writes files under their CPK directories, decompressing CRILAYLA.
    \\Every output path is checked before anything is written; `..`, absolute
    \\paths and names unsafe on Windows are refused. Bytes above ASCII become %XX.
    \\diff prints what a mod does to the base archives, for each of its files: added,
    \\changed or same, comparing extracted content, so a stored and a compressed copy
    \\of the same bytes are the same. A mod dir holds paths like data/init.bin, as a
    \\bind/ folder does. A file missing from the base but under a folder named after
    \\a base file without its extension (data/init/a.tmx, data/init.bin) is
    \\in-container: a mod manager has to build it into that file. Names match without
    \\regard to case; a match that differs in case is marked case-differs. With
    \\several base archives the first that has a file wins.
    \\patch applies a VCDIFF patch as xdelta3 writes it, checking its checksums; patches
    \\with secondary compression are refused. Compare with p3p-bindiff --elf.
    \\check-trace verifies the reads of p3p_pc_bootstrap --io-trace: each read of a
    \\.cpk must be metadata or lie in one file with the same FNV-1a 64. prefix=hostdir
    \\maps traced paths to host files, e.g. ms0:/PSP/P3P=.tmp/mods.
    \\Exit: 0 success, 1 check-trace or diff found differences (for diff: anything but
    \\same), 2 usage/input/output error.
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
    if (std.mem.eql(u8, command, "diff")) return diffCommand(init, writer, rest);
    if (std.mem.eql(u8, command, "patch")) return patchCommand(init, writer, rest);
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

const BaseFile = struct {
    file: cpk.File,
    source: cpk.Source,
};

fn lowerKey(allocator: std.mem.Allocator, file: cpk.File) ![]u8 {
    const key = try std.fmt.allocPrint(allocator, "{s}/{s}", .{ file.dir, file.name });
    for (key) |*byte| byte.* = std.ascii.toLower(byte.*);
    return key;
}

fn sameContent(allocator: std.mem.Allocator, mod_source: cpk.Source, mod: cpk.File, base: BaseFile) !bool {
    // A size of 0 is a TOC without ExtractSize: only the data can tell.
    if (mod.extract_size != 0 and base.file.extract_size != 0 and mod.extract_size != base.file.extract_size) return false;
    const left = try cpk.extract(allocator, mod_source, mod);
    defer allocator.free(left);
    const right = try cpk.extract(allocator, base.source, base.file);
    defer allocator.free(right);
    return std.mem.eql(u8, left, right);
}

const Diff = struct {
    base_files: *const std.StringHashMapUnmanaged(BaseFile),
    /// Lower-case base paths without their extension -> the base file's path.
    stems: *const std.StringHashMapUnmanaged([]const u8),
    files: u64 = 0,
    added: u64 = 0,
    in_container: u64 = 0,
    changed: u64 = 0,
    same: u64 = 0,
    case_differs: u64 = 0,

    /// The deepest folder of `key` that is named after a base file.
    fn container(self: *const Diff, key: []const u8) ?[]const u8 {
        var end = key.len;
        while (std.mem.lastIndexOfScalar(u8, key[0..end], '/')) |slash| {
            if (slash == 0) return null;
            if (self.stems.get(key[0..slash])) |found| return found;
            end = slash;
        }
        return null;
    }

    fn compare(self: *Diff, allocator: std.mem.Allocator, writer: *std.Io.Writer, source: cpk.Source, file: cpk.File) !void {
        self.files += 1;
        const key = try lowerKey(allocator, file);
        defer allocator.free(key);
        const found = self.base_files.get(key) orelse {
            if (self.container(key)) |container_path| {
                self.in_container += 1;
                try writer.print("in-container {s}/{s} container={s}\n", .{ file.dir, file.name, container_path });
            } else {
                self.added += 1;
                try writer.print("added {s}/{s} extract=0x{X}\n", .{ file.dir, file.name, file.extract_size });
            }
            return;
        };
        const exact = std.mem.eql(u8, file.dir, found.file.dir) and std.mem.eql(u8, file.name, found.file.name);
        const mark: []const u8 = if (exact) "" else " case-differs";
        if (!exact) self.case_differs += 1;
        if (try sameContent(allocator, source, file, found)) {
            self.same += 1;
            try writer.print("same {s}/{s}{s}\n", .{ file.dir, file.name, mark });
        } else {
            self.changed += 1;
            try writer.print("changed {s}/{s} extract=0x{X} -> 0x{X}{s}\n", .{ file.dir, file.name, found.file.extract_size, file.extract_size, mark });
        }
    }
};

fn pathLessThan(_: void, a: []u8, b: []u8) bool {
    return std.mem.lessThan(u8, a, b);
}

fn diffDirectory(init: std.process.Init, writer: *std.Io.Writer, diff: *Diff, dir: std.Io.Dir) !void {
    const allocator = init.gpa;
    const io = init.io;
    var arena_state: std.heap.ArenaAllocator = .init(allocator);
    defer arena_state.deinit();
    const arena = arena_state.allocator();
    var paths: std.ArrayList([]u8) = .empty;
    var walker = try dir.walk(allocator);
    defer walker.deinit();
    while (try walker.next(io)) |entry| {
        if (entry.kind != .file) continue;
        const path = try arena.dupe(u8, entry.path);
        std.mem.replaceScalar(u8, path, '\\', '/');
        try paths.append(arena, path);
    }
    // The order of a directory listing differs between systems.
    std.mem.sort([]u8, paths.items, {}, pathLessThan);
    for (paths.items) |path| {
        const file = try dir.openFile(io, path, .{});
        defer file.close(io);
        const view: View = .{ .io = io, .file = file, .base = 0 };
        const source = view.source(try file.length(io));
        const split = std.mem.lastIndexOfScalar(u8, path, '/');
        const parent = if (split) |slash| path[0..slash] else "";
        const name = if (split) |slash| path[slash + 1 ..] else path;
        try diff.compare(allocator, writer, source, try cpk.looseFile(source, parent, name));
    }
}

fn diffCommand(init: std.process.Init, writer: *std.Io.Writer, args: []const []const u8) !u8 {
    const allocator = init.gpa;
    const io = init.io;
    if (args.len < 2) return error.ExpectedModAndBase;
    const mod_dir: ?std.Io.Dir = std.Io.Dir.cwd().openDir(io, args[0], .{ .iterate = true }) catch |err| switch (err) {
        error.NotDir => null,
        else => return err,
    };
    defer if (mod_dir) |dir| dir.close(io);
    var mod: Container = undefined;
    if (mod_dir == null) {
        try mod.open(io, args[0]);
        if (!mod.is_cpk) {
            mod.file.close(io);
            return error.ModIsNotCpk;
        }
    }
    defer if (mod_dir == null) mod.file.close(io);
    var base: Container = undefined;
    try base.open(io, args[1]);
    defer base.file.close(io);
    const paths = args[2..];
    if (base.is_cpk and paths.len != 0) return error.TooManyArguments;
    if (!base.is_cpk and paths.len == 0) return error.ExpectedCpkPathOnImage;

    var arena_state: std.heap.ArenaAllocator = .init(allocator);
    defer arena_state.deinit();
    const arena = arena_state.allocator();
    const views = try arena.alloc(View, @max(paths.len, 1));
    const archives = try arena.alloc(cpk.Archive, views.len);
    var opened: usize = 0;
    defer for (archives[0..opened]) |archive| archive.deinit(allocator);
    var base_files: std.StringHashMapUnmanaged(BaseFile) = .empty;
    defer base_files.deinit(allocator);
    var stems: std.StringHashMapUnmanaged([]const u8) = .empty;
    defer stems.deinit(allocator);
    for (views, archives, 0..) |*view, *archive, index| {
        view.* = base.view;
        var size = base.size;
        if (!base.is_cpk) {
            const extent = try iso.find(allocator, base.view.source(base.size), paths[index]);
            const end = std.math.add(u64, extent.offset, extent.size) catch return error.OutOfBounds;
            if (end > base.size) return error.OutOfBounds;
            view.base = extent.offset;
            size = extent.size;
        }
        const source = view.source(size);
        archive.* = try cpk.Archive.open(allocator, source);
        opened += 1;
        for (archive.files) |file| {
            const key = try lowerKey(arena, file);
            const slot = try base_files.getOrPut(allocator, key);
            if (!slot.found_existing) slot.value_ptr.* = .{ .file = file, .source = source };
            const dot = std.mem.lastIndexOfScalar(u8, key, '.') orelse continue;
            if (dot <= file.dir.len + 1) continue; // no extension, or a name like `.hidden`
            const stem = try stems.getOrPut(allocator, key[0..dot]);
            if (!stem.found_existing) stem.value_ptr.* = try std.fmt.allocPrint(arena, "{s}/{s}", .{ file.dir, file.name });
        }
    }

    var diff: Diff = .{ .base_files = &base_files, .stems = &stems };
    if (mod_dir) |dir| {
        try diffDirectory(init, writer, &diff, dir);
    } else {
        const mod_source = mod.view.source(mod.size);
        const archive = try cpk.Archive.open(allocator, mod_source);
        defer archive.deinit(allocator);
        for (archive.files) |file| try diff.compare(allocator, writer, mod_source, file);
    }
    try writer.print("mod files={d} added={d} in_container={d} changed={d} same={d} case_differs={d} base files={d}\n", .{ diff.files, diff.added, diff.in_container, diff.changed, diff.same, diff.case_differs, base_files.count() });
    return if (diff.added == 0 and diff.in_container == 0 and diff.changed == 0) 0 else 1;
}

fn patchCommand(init: std.process.Init, writer: *std.Io.Writer, args: []const []const u8) !u8 {
    const allocator = init.gpa;
    const io = init.io;
    if (args.len != 3) return error.ExpectedSourcePatchOutput;
    const cwd = std.Io.Dir.cwd();
    const source = try cwd.readFileAlloc(io, args[0], allocator, .limited(vcdiff.max_output_size));
    defer allocator.free(source);
    const patch_data = try cwd.readFileAlloc(io, args[1], allocator, .limited(vcdiff.max_output_size));
    defer allocator.free(patch_data);
    const result = try vcdiff.apply(allocator, source, patch_data);
    defer allocator.free(result.output);
    try cwd.writeFile(io, .{ .sub_path = args[2], .data = result.output });
    try writer.print("patched {d} -> {d} bytes, {d} windows\n", .{ source.len, result.output.len, result.windows });
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
