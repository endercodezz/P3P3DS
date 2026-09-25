namespace AtlusFileSystemLibrary.FileSystems.APAK
{
    public class EntryInfo : FileSystemEntryInfo<string>
    {
        private IEntry mEntry;

        public EntryInfo( IEntry entry ) : base( entry.FileName )
        {
            mEntry = entry;
        }
    }
}
