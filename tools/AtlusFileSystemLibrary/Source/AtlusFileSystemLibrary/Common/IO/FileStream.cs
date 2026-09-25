using System.IO;

namespace AtlusFileSystemLibrary.Common.IO
{
    public class FileStream<T> : Stream
    {
        private readonly Stream mSource;

        public T Handle { get; }

        public FileStream( T handle, Stream source )
        {
            mSource = source;
            Handle = handle;
        }

        public override void Flush()
        {
            mSource.Flush();
        }

        public override long Seek( long offset, SeekOrigin origin )
        {
            return mSource.Seek( offset, origin );
        }

        public override void SetLength( long value )
        {
            mSource.SetLength( value );
        }

        public override int Read( byte[] buffer, int offset, int count )
        {
            return mSource.Read( buffer, offset, count );
        }

        public override void Write( byte[] buffer, int offset, int count )
        {
            mSource.Write( buffer, offset, count );
        }

        public override bool CanRead
        {
            get { return mSource.CanRead; }
        }

        public override bool CanSeek
        {
            get { return mSource.CanSeek; }
        }

        public override bool CanWrite
        {
            get { return mSource.CanWrite; }
        }

        public override long Length
        {
            get { return mSource.Length; }
        }

        public override long Position
        {
            get { return mSource.Position; }
            set { mSource.Position = value; }
        }
    }
}
