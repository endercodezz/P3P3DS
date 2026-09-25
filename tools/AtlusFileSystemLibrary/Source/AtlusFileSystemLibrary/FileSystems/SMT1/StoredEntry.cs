using System;
using System.IO;
using AtlusFileSystemLibrary.Common.IO;

namespace AtlusFileSystemLibrary.FileSystems.SMT1
{
    internal class StoredEntry : IEntry
    {
        private readonly Stream mBaseStream;
        private readonly Lazy<ContentKind> mContentKind;

        public int Handle { get; }

        public int Length { get; }

        public ContentKind ContentKind => mContentKind.Value;

        public int Offset { get; }

        public StoredEntry( int handle, Stream baseStream, int offset, int length )
        {
            Handle = handle;
            mBaseStream = baseStream;
            Length = length;
            Offset = offset;
            mContentKind = new Lazy<ContentKind>( () => ContentKindDetector.Detect( GetStream() ) );
        }

        public Stream GetStream()
        {
            return new StreamView( mBaseStream, Offset, Length );
        }

        public void Dispose()
        {
            // Nothing to do
        }
    }
}