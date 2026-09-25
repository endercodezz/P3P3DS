using System;
using System.IO;

namespace AtlusFileSystemLibrary.FileSystems.ACX
{
    internal abstract class Entry : IDisposable
    {
        public int Handle { get; }

        public uint Length { get; }

        protected Entry( int handle, uint length )
        {
            Handle = handle;
            Length = length;
        }

        public abstract Stream GetStream();
        public abstract void Dispose();
    }
}