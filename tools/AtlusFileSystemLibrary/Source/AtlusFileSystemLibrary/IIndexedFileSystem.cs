using System.IO;

namespace AtlusFileSystemLibrary
{
    public interface IIndexedFileSystem : IFileSystem<int>
    {
        int AllocateHandle();

        void AddFile( string hostPath );
        void AddFile( Stream stream, bool ownsStream = true );
    }
}