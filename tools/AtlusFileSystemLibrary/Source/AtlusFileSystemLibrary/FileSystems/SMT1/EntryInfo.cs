namespace AtlusFileSystemLibrary.FileSystems.SMT1
{
    public class EntryInfo : FileSystemEntryInfo<int>
    {
        private readonly IEntry mEntry;

        public int Index => mEntry.Handle;

        public int Length => mEntry.Length;

        public ContentKind ContentKind => mEntry.ContentKind;

        internal EntryInfo( IEntry entry ) : base( entry.Handle )
        {
            mEntry = entry;
        }
    }
}