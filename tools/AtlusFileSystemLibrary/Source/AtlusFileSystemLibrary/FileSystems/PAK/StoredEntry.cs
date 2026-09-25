using System.IO;
using AtlusFileSystemLibrary.Common.IO;

namespace AtlusFileSystemLibrary.FileSystems.PAK
{
    internal class StoredEntry : IEntry
    {
        private readonly Stream mBaseStream;

        public string FileName { get; }

        public int Length { get; }

        public int Offset { get; }

        public StoredEntry( Stream baseStream, string fileName, int length, int offset )
        {
            mBaseStream = baseStream;
            FileName = fileName;
            Length = length;
            Offset = offset;
        }

        public Stream GetStream()
        {
            return new StreamView( mBaseStream, Offset, Length );
        }

        public void Dispose()
        {
        }
    }
}