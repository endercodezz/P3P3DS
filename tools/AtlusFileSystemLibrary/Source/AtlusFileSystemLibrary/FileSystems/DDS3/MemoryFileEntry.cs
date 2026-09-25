using System.IO;
using AtlusFileSystemLibrary.Common.IO;

namespace AtlusFileSystemLibrary.FileSystems.DDS3
{
    internal class MemoryFileEntry : FileEntry
    {
        private readonly Stream mStream;
        private readonly bool mOwnsStream;

        public MemoryFileEntry( DirectoryEntry parent, Stream stream, bool ownsStream, string name ) : base( parent, name, uint.MaxValue, (uint)stream.Length )
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