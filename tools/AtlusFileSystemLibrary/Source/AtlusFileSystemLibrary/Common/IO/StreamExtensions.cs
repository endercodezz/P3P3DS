using System.IO;

namespace AtlusFileSystemLibrary.Common.IO
{
    public static class StreamExtensions
    {
        public static void FullyCopyTo( this Stream @this, Stream stream )
        {
            @this.Position = 0;
            @this.CopyTo( stream );
        }
    }
}