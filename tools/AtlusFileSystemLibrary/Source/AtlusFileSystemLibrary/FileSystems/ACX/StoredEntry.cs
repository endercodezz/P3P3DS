using System.IO;
using AtlusFileSystemLibrary.Common.IO;

namespace AtlusFileSystemLibrary.FileSystems.ACX
{
    internal class StoredEntry : Entry
    {
        private readonly Stream mBaseStream;

        public uint Offset { get; }

        public StoredEntry( int handle, Stream baseStream, uint offset, uint length ) : base( handle, length )
        {
            mBaseStream = baseStream;
            Offset = offset;
        }

        public override Stream GetStream()
        {
            return new StreamView( mBaseStream, Offset, Length );
        }

        public override void Dispose()
        {
        }
    }
}