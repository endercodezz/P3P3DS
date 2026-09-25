namespace AtlusFileSystemLibrary.FileSystems.PAK
{
    public class EntryInfo : FileSystemEntryInfo< string >
    {
        private readonly IEntry mEntry;

        public int Offset
        {
            get
            {
                if ( mEntry is StoredEntry entry )
                    return entry.Offset;

                return -1;
            }
        }

        public int Length => mEntry.Length;

        internal EntryInfo( IEntry entry ) : base( entry.FileName )
        {
            mEntry = entry;
        }
    }
}