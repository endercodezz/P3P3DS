using System.Collections.Generic;

namespace AtlusFileSystemLibrary.FileSystems.DDS3
{
    internal class DirectoryEntry : Entry
    {
        public Dictionary<string, Entry> Entries { get; }

        public DirectoryEntry( DirectoryEntry parent, string name, uint offset ) : base( parent, name, offset, FileSystemEntryKind.Directory )
        {
            Entries = new Dictionary< string, Entry >();
        }

        public override void Dispose()
        {
            foreach ( var entry in Entries.Values )
            {
                entry.Dispose();
            }
        }
    }
}