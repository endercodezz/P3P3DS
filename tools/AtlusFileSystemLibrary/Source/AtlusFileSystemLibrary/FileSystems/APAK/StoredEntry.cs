using System.IO;
using AtlusFileSystemLibrary.Common.IO;

namespace AtlusFileSystemLibrary.FileSystems.APAK
{
    public class StoredEntry : IEntry
    {
        private Stream mBaseStream;

        public int Hash { get; set; }
        public int Offset { get; set; }
        public int Size { get; set; }
        public int AlignedSize { get; set; }
        public int Alignment { get; set; }
        public int Field10 { get; set; }
        public int Field14 { get; set; }
        public int Field18 { get; set; }
        public string FileName { get; set; }

        public StoredEntry( Stream baseStream, int hash, int offset, int size, int alignedSize, int alignment, 
            int field10, int field14, int field18, string fileName )
        {
            mBaseStream = baseStream;
            Hash = hash;
            Offset = offset;
            Size = size;
            AlignedSize = alignedSize;
            Alignment = alignment;
            Field10 = field10;
            Field14 = field14;
            Field18 = field18;
            FileName = fileName;
        }

        public Stream GetStream()
        {
            return new StreamView( mBaseStream, Offset, Size );
        }

        public void Dispose()
        {
        }
    }
}
