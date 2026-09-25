namespace AtlusFileSystemLibrary.FileSystems.DDS3
{
    public class EntryInfo : FileSystemEntryInfo< string >
    {
        private readonly Entry mEntry;

        public FileSystemEntryKind Kind => mEntry.Kind;

        public string FullName => mEntry.FullName;

        public string Name => mEntry.Name;

        public uint Offset => mEntry.Offset;

        internal EntryInfo( Entry entry ) : base( entry.Name )
        {
            mEntry = entry;
        }
    }
}