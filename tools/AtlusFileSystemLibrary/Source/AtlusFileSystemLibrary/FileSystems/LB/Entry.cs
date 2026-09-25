using System;
using System.IO;

namespace AtlusFileSystemLibrary.FileSystems.LB
{
    internal abstract class Entry : IDisposable
    {
        public int Handle { get; }

        public byte Type { get; }

        public bool IsCompressed { get; }

        public short UserId { get; }

        public int Length { get; }

        public string Extension { get; }

        public int DecompressedLength { get; }

        protected Entry( int handle, byte type, bool isCompressed, short userId, int length, string extension, int decompressedLength )
        {
            Handle = handle;
            Type = type;
            IsCompressed = isCompressed;
            UserId = userId;
            Length = length;
            Extension = extension;
            DecompressedLength = decompressedLength;
        }

        public abstract Stream GetStream( bool decompress = true );

        public abstract void Dispose();
    }
}