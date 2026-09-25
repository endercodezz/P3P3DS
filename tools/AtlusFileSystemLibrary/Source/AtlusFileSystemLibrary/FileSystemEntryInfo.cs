using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace AtlusFileSystemLibrary
{
    public class FileSystemEntryInfo<T>
    {
        public T Handle { get; }

        internal FileSystemEntryInfo( T handle )
        {
            Handle = handle;
        }
    }
}
