using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace AtlusFileSystemLibrary
{
    public interface ICompression
    {
        bool CanCompress { get; }

        bool CanDecompress { get; }

        Stream Compress( Stream input );

        Stream Decompress( Stream input, int decompressedSize = -1 );
    }
}
