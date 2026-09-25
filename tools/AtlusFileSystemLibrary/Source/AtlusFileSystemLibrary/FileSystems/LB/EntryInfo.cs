namespace AtlusFileSystemLibrary.FileSystems.LB
{
    public class EntryInfo : FileSystemEntryInfo< int >
    {
        private readonly Entry mEntry;

        public int Index => mEntry.Handle;

        public byte Type => mEntry.Type;

        public bool IsCompressed => mEntry.IsCompressed;

        public short UserId => mEntry.UserId;

        public int Length => mEntry.Length;

        public string Extension => mEntry.Extension;

        public int DecompressedLength => mEntry.DecompressedLength;

        internal EntryInfo( Entry entry ) : base( entry.Handle )
        {
            mEntry = entry;
        }
    }
}