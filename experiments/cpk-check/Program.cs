// Verifies P3P's CPK reads (p3p_pc_bootstrap --io-trace) with CriFsV2Lib.
//
// For every traced read of a *.cpk file on the UMD image it reports whether
// the byte range lies in the CPK header/TOC area or inside one file's stored
// data, and for reads inside a file compares the FNV-1a 64 of the bytes the
// runtime delivered with the same range extracted by CriFsV2Lib
// (ExtractFileNoDecompression). Exit code 0 only when every checked read
// matches and nothing is unclassified.
//
// Usage: CpkCheck <umd.iso> <io-trace.csv> [prefix=hostdir ...]
//   e.g. ms0:/PSP/P3P=.tmp/mods maps traced mod archives to host files.
using System.Buffers.Binary;
using System.Globalization;
using System.Text;
using CriFsV2Lib;
using CriFsV2Lib.Definitions.Structs;

static class Program
{
    const int Sector = 2048;

    sealed class SubStream : Stream
    {
        readonly Stream _inner; readonly long _start, _length; long _pos;
        public SubStream(Stream inner, long start, long length) { _inner = inner; _start = start; _length = length; }
        public override bool CanRead => true; public override bool CanSeek => true; public override bool CanWrite => false;
        public override long Length => _length;
        public override long Position { get => _pos; set => _pos = value; }
        public override void Flush() { }
        public override int Read(byte[] buffer, int offset, int count)
        {
            if (_pos >= _length) return 0;
            count = (int)Math.Min(count, _length - _pos);
            _inner.Position = _start + _pos;
            int n = _inner.Read(buffer, offset, count);
            _pos += n; return n;
        }
        public override long Seek(long offset, SeekOrigin origin) =>
            _pos = origin switch { SeekOrigin.Begin => offset, SeekOrigin.Current => _pos + offset, _ => _length + offset };
        public override void SetLength(long value) => throw new NotSupportedException();
        public override void Write(byte[] buffer, int offset, int count) => throw new NotSupportedException();
    }

    // Minimal ISO9660 lookup (case-insensitive) for a "/PSP_GAME/..." path.
    static (long lba, long size)? FindIso(Stream iso, string path)
    {
        var pvd = new byte[Sector];
        iso.Position = 16L * Sector; iso.ReadExactly(pvd);
        long lba = BinaryPrimitives.ReadUInt32LittleEndian(pvd.AsSpan(158));
        long size = BinaryPrimitives.ReadUInt32LittleEndian(pvd.AsSpan(166));
        foreach (var part in path.Trim('/').Split('/'))
        {
            var dir = new byte[size]; iso.Position = lba * Sector; iso.ReadExactly(dir);
            bool found = false; int off = 0;
            while (off < dir.Length)
            {
                int len = dir[off];
                if (len == 0) { off = (off / Sector + 1) * Sector; continue; }
                var name = Encoding.ASCII.GetString(dir, off + 33, dir[off + 32]).Split(';')[0];
                if (string.Equals(name, part, StringComparison.OrdinalIgnoreCase))
                {
                    lba = BinaryPrimitives.ReadUInt32LittleEndian(dir.AsSpan(off + 2));
                    size = BinaryPrimitives.ReadUInt32LittleEndian(dir.AsSpan(off + 10));
                    found = true; break;
                }
                off += len;
            }
            if (!found) return null;
        }
        return (lba, size);
    }

    static ulong Fnv1a(ReadOnlySpan<byte> data)
    {
        ulong h = 0xCBF29CE484222325UL;
        foreach (var b in data) { h ^= b; h *= 0x100000001B3UL; }
        return h;
    }

    static int Main(string[] args)
    {
        if (args.Length < 2) { Console.Error.WriteLine("usage: CpkCheck <umd.iso> <io-trace.csv> [prefix=hostdir ...]"); return 2; }
        using var iso = File.OpenRead(args[0]);
        if (args[1] == "--list")
        {
            // CpkCheck <iso> --list <iso path of .cpk> [<name substring> <extract dir>]
            var loc = FindIso(iso, args[2]) ?? throw new InvalidOperationException("not on UMD: " + args[2]);
            var cpk = CriFsLib.Instance.CreateCpkReader(new SubStream(iso, loc.lba * Sector, loc.size), false);
            foreach (var file in cpk.GetFiles())
            {
                var key = $"{file.Directory}/{file.FileName}";
                if (args.Length > 3 && !key.Contains(args[3], StringComparison.OrdinalIgnoreCase)) continue;
                Console.WriteLine($"{key} offset=0x{file.FileOffset:X} size=0x{file.FileSize:X} extract=0x{file.ExtractSize:X}");
                if (args.Length > 4)
                {
                    using var data = cpk.ExtractFile(file);
                    var outPath = Path.Combine(args[4], file.FileName);
                    Directory.CreateDirectory(args[4]);
                    File.WriteAllBytes(outPath, data.Span.ToArray());
                }
            }
            return 0;
        }
        var hostMaps = args.Skip(2).Select(a => a.Split('=', 2)).ToArray();
        var readers = new Dictionary<string, (ICpkReaderHolder holder, CpkFile[] files, long headerEnd)>();
        int total = 0, header = 0, matched = 0, mismatched = 0, unclassified = 0, skipped = 0;
        var perFile = new Dictionary<string, int>();
        foreach (var line in File.ReadLines(args[1]).Skip(1))
        {
            var f = line.Split(',');
            if (f.Length < 4) continue;
            var path = f[0];
            if (!path.EndsWith(".cpk", StringComparison.OrdinalIgnoreCase)) { ++skipped; continue; }
            long offset = long.Parse(f[1], CultureInfo.InvariantCulture);
            long size = long.Parse(f[2], CultureInfo.InvariantCulture);
            ulong hash = ulong.Parse(f[3].StartsWith("0x") ? f[3][2..] : f[3], NumberStyles.HexNumber);
            ++total;
            if (!readers.TryGetValue(path, out var entry))
            {
                Stream sub;
                long cpkSize;
                var map = hostMaps.FirstOrDefault(m => path.StartsWith(m[0], StringComparison.OrdinalIgnoreCase));
                if (map != null)
                {
                    var host = File.OpenRead(Path.Combine(map[1], path[map[0].Length..].TrimStart('/')));
                    sub = host; cpkSize = host.Length;
                }
                else
                {
                    var isoPath = path[(path.IndexOf(':') + 1)..];
                    var location = FindIso(iso, isoPath) ?? throw new InvalidOperationException("not on UMD: " + path);
                    sub = new SubStream(iso, location.lba * Sector, location.size); cpkSize = location.size;
                }
                var location2 = (lba: 0L, size: cpkSize);
                var reader = CriFsLib.Instance.CreateCpkReader(sub, false);
                var files = reader.GetFiles();
                long firstData = files.Length == 0 ? location2.size : files.Min(x => x.FileOffset);
                entry = (new ICpkReaderHolder(reader), files, firstData);
                readers[path] = entry;
                Console.WriteLine($"{path}: {files.Length} files, first data at 0x{firstData:X}, size 0x{location2.size:X}");
            }
            // Metadata (header, TOC, ITOC/ETOC) is every byte outside file data.
            if (!entry.files.Any(x => offset < x.FileOffset + x.FileSize && x.FileOffset < offset + size)) { ++header; continue; }
            var hit = entry.files.Where(x => offset >= x.FileOffset && offset + size <= x.FileOffset + x.FileSize).ToArray();
            if (hit.Length != 1) { ++unclassified; if (unclassified <= 10) Console.WriteLine($"UNCLASSIFIED {path} 0x{offset:X}+0x{size:X}"); continue; }
            var file = hit[0];
            using var raw = entry.holder.Reader.ExtractFileNoDecompression(file, out _);
            var span = raw.Span.Slice((int)(offset - file.FileOffset), (int)size);
            var key = $"{file.Directory}/{file.FileName}";
            perFile[key] = perFile.GetValueOrDefault(key) + 1;
            if (Fnv1a(span) == hash) ++matched;
            else { ++mismatched; if (mismatched <= 10) Console.WriteLine($"MISMATCH {key} 0x{offset:X}+0x{size:X}"); }
        }
        foreach (var (key, count) in perFile.OrderBy(x => x.Key).Take(20)) Console.WriteLine($"  read {count}x {key}");
        Console.WriteLine($"cpk reads={total} header/toc={header} file_matched={matched} mismatched={mismatched} unclassified={unclassified} non_cpk={skipped}");
        return mismatched == 0 && unclassified == 0 ? 0 : 1;
    }

    sealed class ICpkReaderHolder(CriFsV2Lib.Definitions.ICpkReader reader)
    {
        public CriFsV2Lib.Definitions.ICpkReader Reader { get; } = reader;
    }
}
