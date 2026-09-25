using System.IO;
using AtlusFileSystemLibrary.Common.IO;
using AtlusFileSystemLibrary.Compressions;

namespace AtlusFileSystemLibrary.FileSystems.LB
{
    internal class StoredEntry : Entry
    {
        private readonly Stream mBaseStream;
        private Stream mDecompressedStream;

        public int Offset { get; }

        public StoredEntry( int handle, Stream baseStream, int offset, byte type, bool isCompressed, short userId, int length, string extension, int decompressedLength ) 
            : base( handle, type, isCompressed, userId, length, extension, decompressedLength )
        {
            mBaseStream = baseStream;
            Offset = offset;
        }

        public override Stream GetStream( bool decompress = true )
        {
            if ( decompress && IsCompressed )
            {
                if ( mDecompressedStream == null )
                {
                    var compression = new LBCompression();
                    mDecompressedStream = compression.Decompress( new StreamView( mBaseStream, Offset, Length ), DecompressedLength );
                }

                return mDecompressedStream;
            }
            else
            {
                return new StreamView( mBaseStream, Offset, Length );
            }
        }

        public override void Dispose()
        {
            mDecompressedStream?.Dispose();
        }
    }
}