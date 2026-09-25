using System.IO;
using AtlusFileSystemLibrary.Common.IO;

namespace AtlusFileSystemLibrary.FileSystems.DDS3
{
    internal class StoredFileEntry : FileEntry
    {
        private readonly Stream mBaseStream;

        public StoredFileEntry( DirectoryEntry parent, Stream baseStream, string name, uint offset, uint length ) : base(parent, name, offset, length)
        {
            mBaseStream = baseStream;
        }

        public override void Dispose()
        {
        }

        public override Stream GetStream()
        {
            return new StreamView( mBaseStream, ((long)Offset * DDS3FileSystem.SECTOR_SIZE ), Length );
        }
    }
}