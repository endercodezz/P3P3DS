using System;
using System.IO;
using AtlusFileSystemLibrary.Common.IO;

namespace AtlusFileSystemLibrary.FileSystems.SMT1
{
    internal class MemoryEntry : IEntry
    {
        private readonly Stream mStream;
        private readonly bool mOwnsStream;
        private readonly Lazy<ContentKind> mContentKind;

        public int Handle { get; }

        public int Length => ( int )mStream.Length;

        public ContentKind ContentKind => mContentKind.Value;

        public MemoryEntry( int handle, Stream stream, bool ownsStream )
        {
            Handle = handle;
            mStream = stream;
            mOwnsStream = ownsStream;
            mContentKind = new Lazy< ContentKind >( () => ContentKindDetector.Detect( GetStream() ) );
        }

        public Stream GetStream()
        {
            return new StreamView( mStream, 0, mStream.Length );
        }

        public void Dispose()
        {
            if ( mOwnsStream )
                mStream.Dispose();
        }
    }
}