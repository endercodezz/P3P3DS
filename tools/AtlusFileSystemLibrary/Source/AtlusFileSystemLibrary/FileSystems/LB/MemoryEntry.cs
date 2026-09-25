using System.IO;
using AtlusFileSystemLibrary.Common.IO;
using AtlusFileSystemLibrary.Compressions;

namespace AtlusFileSystemLibrary.FileSystems.LB
{
    internal class MemoryEntry : Entry
    {
        private readonly Stream mStream;
        private Stream mDecompressedStream;
        private readonly bool mOwnsStream;

        public MemoryEntry( int handle, Stream stream, bool ownsStream, byte type, bool compressed, int decompressedLength, short userId, string extension )
            : base( handle, type, compressed, userId, (int)stream.Length, extension, decompressedLength )
        {
            mStream = stream;
            mOwnsStream = ownsStream;
        }

        public override Stream GetStream( bool decompress = true )
        {
            var stream = mStream;

            if ( IsCompressed && decompress )
            {
                if ( mDecompressedStream == null )
                {
                    var compression = new LBCompression();
                    mDecompressedStream = compression.Decompress( mStream );
                }

                stream = mDecompressedStream;
            }

            return new StreamView( stream, 0, stream.Length );
        }

        public override void Dispose()
        {
            if ( mOwnsStream )
                mStream?.Dispose();
        }
    }
}