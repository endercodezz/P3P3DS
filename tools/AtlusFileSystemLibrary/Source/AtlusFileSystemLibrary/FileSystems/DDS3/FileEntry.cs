using System.IO;

namespace AtlusFileSystemLibrary.FileSystems.DDS3
{
    internal abstract class FileEntry : Entry
    {
        public uint Length { get; }

        protected FileEntry( DirectoryEntry parent, string name, uint offset, uint length ) : base(parent, name, offset, FileSystemEntryKind.File)
        {
            Length = length;   
        }

        public abstract Stream GetStream();
    }
}