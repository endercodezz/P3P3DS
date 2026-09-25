using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using AtlusFileSystemLibrary.Common.IO;

namespace AtlusFileSystemLibrary.FileSystems.APAK
{
    public class APAKFileSystem : INamedFileSystem
    {
        // Private fields
        private Stream mBaseStream;
        private bool mOwnsStream;
        private Dictionary<string, IEntry> mEntryMap;
        public static Encoding Encoding { get; } = Encoding.ASCII;

        public short Field04 { get; set; }
        public short Field06 { get; set; }
        public int FileCount => mEntryMap.Count;
        public int Field0C { get; set; }
        public int HeaderSize { get; set; }
        public int Length { get; set; }

        public bool IsReadOnly { get; } = true;
        public bool HasDirectories { get; } = false;
        public bool CanSave { get; } = true;
        public bool CanAddOrRemoveEntries { get; } = false;
        public string FilePath { get; private set; }
        public bool IsBigEndian { get; set; } = false;

        public APAKFileSystem()
        {
            mBaseStream = null;
            mOwnsStream = true;
            mEntryMap = new Dictionary<string, IEntry>();
        }

        private APAKFileSystem( Stream baseStream, bool ownsStream )
        {
            Load( baseStream, ownsStream );
        }

        public void Load( string path )
        {
            FilePath = path;
            Load( File.OpenRead( path ), true );
        }

        public void Load( Stream stream, bool ownsStream )
        {
            mBaseStream = stream;
            mOwnsStream = ownsStream;
            ReadEntries();
        }

        private void ReadEntries()
        {
            mEntryMap = new Dictionary<string, IEntry>( StringComparer.InvariantCultureIgnoreCase );

            using ( var reader = new EndianBinaryReader( mBaseStream, Encoding.Default, true, IsBigEndian ? Endianness.BigEndian : Endianness.LittleEndian ) )
            {
                var stringBuilder = new StringBuilder();

                var signature = reader.ReadInt32();
                if (signature == 0x4150414B && !IsBigEndian)
                {
                    IsBigEndian = true;
                    reader.Endianness = Endianness.BigEndian;
                }

                Field04 = reader.ReadInt16();
                Field06 = reader.ReadInt16();
                var fileCount = reader.ReadInt32();
                Field0C = reader.ReadInt32();
                HeaderSize = reader.ReadInt32();
                Length = reader.ReadInt32();

                for ( int i = 0; i < fileCount; i++ )
                {
                    var hash = reader.ReadInt32();
                    var offset = reader.ReadInt32();
                    var size = reader.ReadInt32();
                    var alignedSize = reader.ReadInt32();
                    var alignment = reader.ReadInt32();
                    var field10 = reader.ReadInt32();
                    var field14 = reader.ReadInt32();
                    var field18 = reader.ReadInt32();
                    var fileName = Encoding.GetString(reader.ReadBytes(32)).TrimEnd('\0');
                    mEntryMap.Add( fileName, new StoredEntry( mBaseStream, hash, offset, size, alignedSize,
                        alignment, field10, field14, field18, fileName ) );
                }
            }
        }

        private bool TryFindEntry( string name, out IEntry entry )
        {
            if ( !mEntryMap.TryGetValue( name, out entry ) )
            {
                return false;
            }

            return true;
        }

        private IEntry FindEntry( string name )
        {
            if ( !TryFindEntry( name, out var entry ) )
            {
                throw new FileNotFoundException( "The specified file could not be found", name );
            }

            return entry;
        }

        public FileStream<string> OpenFile( string handle, FileAccess access = FileAccess.Read )
        {
            var entry = FindEntry( handle );
            return new FileStream<string>( handle, entry.GetStream() );
        }

        public bool Exists( string handle )
        {
            return TryFindEntry( handle, out _ );
        }

        public bool IsFile( string handle )
        {
            return Exists( handle );
        }

        public bool IsDirectory( string handle )
        {
            throw new NotSupportedException( "This filesystem does not support directories" );
        }

        public EntryInfo GetInfo( string handle )
        {
            var entry = FindEntry( handle );
            return new EntryInfo( entry );
        }

        FileSystemEntryInfo<string> IFileSystem<string>.GetInfo( string handle )
        {
            return GetInfo( handle );
        }

        public IEnumerable<string> EnumerateFileSystemEntries( SearchOption option = SearchOption.TopDirectoryOnly )
        {
            return mEntryMap.Select( x => x.Key );
        }

        public IEnumerable<string> EnumerateDirectories( SearchOption option = SearchOption.TopDirectoryOnly )
        {
            throw new NotSupportedException( "This filesystem does not support directories" );
        }

        public IEnumerable<string> EnumerateFiles( SearchOption option = SearchOption.TopDirectoryOnly )
        {
            return EnumerateFileSystemEntries();
        }

        public IEnumerable<string> EnumerateFileSystemEntries( string handle, SearchOption option = SearchOption.TopDirectoryOnly )
        {
            throw new NotSupportedException( "This filesystem does not support directories" );
        }

        public IEnumerable<string> EnumerateDirectories( string handle, SearchOption option = SearchOption.TopDirectoryOnly )
        {
            throw new NotSupportedException( "This filesystem does not support directories" );
        }

        public IEnumerable<string> EnumerateFiles( string handle, SearchOption option = SearchOption.TopDirectoryOnly )
        {
            throw new NotSupportedException( "This filesystem does not support directories" );
        }

        public void Delete( string handle )
        {
            var entry = FindEntry( handle );
            mEntryMap.Remove( entry.FileName );
        }

        public void AddFile( string handle, string hostPath, ConflictPolicy policy )
        {
            var stream = File.OpenRead( hostPath );
            AddFile( handle, stream, true, policy );
        }

        public void AddFile( string handle, Stream stream, bool ownsStream, ConflictPolicy policy )
        {
            if ( Exists( handle ) )
            {
                switch ( policy.Kind )
                {
                    case ConflictPolicy.PolicyKind.ThrowError:
                        throw new FileExistsException<string>( handle );

                    case ConflictPolicy.PolicyKind.Ignore:
                        return;
                }
            }

            mEntryMap[handle] = new MemoryEntry( stream, ownsStream, handle );
        }

        public void AddDirectory( string handle, string hostPath, ConflictPolicy policy )
        {
            throw new NotSupportedException( "This filesystem does not support directories" );
        }

        public void Save( string outPath )
        {
            using ( var fileStream = File.Create( outPath ) )
                Save( fileStream );
        }

        public void Save( Stream stream )
        {
            using ( var writer = new EndianBinaryWriter( stream, Encoding.Default, true, IsBigEndian ? Endianness.BigEndian : Endianness.LittleEndian ) )
            {
                throw new NotImplementedException();
            }
        }

        public Stream Save()
        {
            var stream = new MemoryStream();
            Save( stream );
            return stream;
        }

        public void Dispose()
        {
            foreach ( var entry in mEntryMap.Values )
                entry.Dispose();

            if ( mOwnsStream )
                mBaseStream?.Dispose();
        }

        public void AddDirectory( string handle, ConflictPolicy policy )
        {
            throw new NotSupportedException( "This filesystem does not support directories" );
        }
    }
}
