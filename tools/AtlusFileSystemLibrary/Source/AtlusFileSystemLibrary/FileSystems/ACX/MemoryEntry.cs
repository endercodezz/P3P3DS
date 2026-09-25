using System.IO;
using AtlusFileSystemLibrary.Common.IO;

namespace AtlusFileSystemLibrary.FileSystems.ACX
{
    internal class MemoryEntry : Entry
    {
        private readonly Stream mStream;
        private readonly bool mOwnsStream;

        public MemoryEntry( int handle, Stream stream, bool ownsStream ) : base( handle, (uint)stream.Length )
        {
            mStream = stream;
            mOwnsStream = ownsStream;
        }

        public override void Dispose()
        {
            if ( mOwnsStream )
                mStream.Dispose();
        }

        public override Stream GetStream()
        {
            return new StreamView( mStream, 0, mStream.Length );
        }
    }
}