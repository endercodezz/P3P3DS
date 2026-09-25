using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using AtlusFileSystemLibrary.Common.IO;

namespace AtlusFileSystemLibrary.FileSystems.PAK
{
    public class PAKFileSystem : INamedFileSystem
    {
        // Helper utilities
        public static bool IsValid( string filepath )
        {
            using ( var stream = File.OpenRead( filepath ) )
                return IsValid( stream );
        }

        public static bool IsValid( Stream stream )
        {
            return DetectVersion( stream ) != FormatVersion.Unknown;
        }

        public static bool TryOpen( string filepath, out PAKFileSystem archive )
        {
            var stream = File.OpenRead( filepath );
            bool success = TryOpen( stream, true, out archive );
            if ( !success )
                stream.Dispose();
            else
                archive.FilePath = filepath;

            return success;
        }

        public static bool TryOpen( Stream stream, bool ownsStream, out PAKFileSystem archive )
        {
            var version = DetectVersion( stream );
            if ( version == FormatVersion.Unknown )
            {
                archive = null;
                return false;
            }

            archive = new PAKFileSystem( stream, ownsStream, version );
            return true;
        }

        private static bool IsValidFormatVersion1( Stream stream )
        {
            // check if the file is too small to be a proper pak file
            if ( stream.Length <= 256 )
            {
                return false;
            }

            // read some test data
            byte[] testData = new byte[256];
            stream.Read( testData, 0, 256 );
            stream.Position = 0;

            // check if first byte is zero, if so then no name can be stored thus making the file corrupt
            if ( testData[0] == 0x00 )
                return false;

            bool nameTerminated = false;
            for ( int i = 0; i < 252; i++ )
            {
                if ( testData[i] == 0x00 )
                    nameTerminated = true;

                // If the name has already been terminated but there's still data in the reserved space,
                // fail the test
                if ( nameTerminated && testData[i] != 0x00 )
                    return false;
            }

            int testLength = BitConverter.ToInt32( testData, 252 );

            // sanity check, if the length of the first file is >= 100 mb, fail the test
            if ( testLength >= stream.Length || testLength < 0 )
            {
                return false;
            }

            return true;
        }

        private static bool IsValidFormatVersion2And3( Stream stream, int entrySize, out bool isBigEndian )
        {
            isBigEndian = false;

            // check stream length
            if ( stream.Length <= 4 + entrySize )
                return false;

            byte[] testData = new byte[4 + entrySize];
            stream.Read( testData, 0, 4 + entrySize );
            stream.Position = 0;

            int numOfFiles = BitConverter.ToInt32( testData, 0 );

            // num of files sanity check
            if ( numOfFiles > 1024 || numOfFiles < 1 || ( numOfFiles * entrySize ) > stream.Length )
            {
                numOfFiles = EndiannessUtils.Swap( numOfFiles );

                if ( numOfFiles > 1024 || numOfFiles < 1 || ( numOfFiles * entrySize ) > stream.Length )
                    return false;

                isBigEndian = true;
            }

            // check if the name field is correct
            bool nameTerminated = false;
            for ( int i = 0; i < entrySize - 4; i++ )
            {
                if ( testData[4 + i] == 0x00 )
                {
                    if ( i == 0 )
                        return false;

                    nameTerminated = true;
                }

                if ( testData[4 + i] != 0x00 && nameTerminated )
                    return false;
            }

            // first entry length sanity check
            int length = BitConverter.ToInt32( testData, entrySize );
            if ( length >= stream.Length || length < 0 )
            {
                length = EndiannessUtils.Swap( length );

                if ( length >= stream.Length || length < 0 )
                    return false;

                isBigEndian = true;
            }

            return true;
        }

        private static FormatVersion DetectVersion( Stream stream )
        {
            if ( IsValidFormatVersion1( stream ) )
                return FormatVersion.Version1;

            if ( IsValidFormatVersion2And3( stream, 36, out var isBigEndian ) )
                return isBigEndian ? FormatVersion.Version2BE : FormatVersion.Version2;

            if ( IsValidFormatVersion2And3( stream, 28, out isBigEndian ) )
                return isBigEndian ? FormatVersion.Version3BE : FormatVersion.Version3;

            return FormatVersion.Unknown;
        }

        // Private fields
        private Stream mBaseStream;
        private bool mOwnsStream;
        private Dictionary<string, IEntry> mEntryMap;

        // Properties
        private bool IsBigEndian => Version == FormatVersion.Version2BE || Version == FormatVersion.Version3BE;

        public bool IsReadOnly { get; } = false;

        public bool HasDirectories { get; } = false;

        public bool CanSave { get; } = true;

        public bool CanAddOrRemoveEntries { get; } = true;

        public string FilePath { get; private set; }

        public FormatVersion Version { get; private set; }

        public PAKFileSystem()
        {
            mBaseStream = null;
            mOwnsStream = true;
            mEntryMap = new Dictionary< string, IEntry >();
            Version = FormatVersion.Version1;
        }

        public PAKFileSystem( FormatVersion version ) : this()
        {
            Version = version;
        }

        private PAKFileSystem( Stream baseStream, bool ownsStream, FormatVersion version )
        {
            Version = version;
            Load( baseStream, ownsStream );
        }

        // INamedFileSystem implementation
        public void Load( string path )
        {
            FilePath = path;
            Load( File.OpenRead( path ), true );
        }

        public void Load( Stream stream, bool ownsStream )
        {
            mBaseStream = stream;
            mOwnsStream = ownsStream;
            Version = DetectVersion( mBaseStream );

            ReadEntries();
        }

        private void ReadEntries()
        {
            mEntryMap = new Dictionary<string, IEntry>( StringComparer.InvariantCultureIgnoreCase );

            using ( var reader = new EndianBinaryReader( mBaseStream, Encoding.Default, true, IsBigEndian ? Endianness.BigEndian : Endianness.LittleEndian ) )
            {
                var stringBuilder = new StringBuilder();

                if ( Version == FormatVersion.Version1 )
                {
                    while ( true )
                    {
                        long entryStartPosition = reader.BaseStream.Position;
                        if ( entryStartPosition == reader.BaseStream.Length )
                        {
                            break;
                        }

                        // read entry name
                        while ( true )
                        {
                            byte b = reader.ReadByte();
                            if ( b == 0 )
                                break;

                            stringBuilder.Append( ( char )b );

                            // just to be safe
                            if ( stringBuilder.Length == 252 )
                                break;
                        }

                        string fileName = stringBuilder.ToString();

                        // set position to length field
                        reader.BaseStream.Position = entryStartPosition + 252;

                        // read entry length
                        int length = reader.ReadInt32();

                        if ( fileName.Length == 0 || length <= 0 || length > 1024 * 1024 * 100 )
                        {
                            break;
                        }

                        // make an entry
                        var entry = new StoredEntry( mBaseStream, fileName, length, ( int )reader.BaseStream.Position );

                        // clear string builder for next iteration
                        stringBuilder.Clear();

                        reader.BaseStream.Position = AlignmentUtils.Align( reader.BaseStream.Position + entry.Length, 64 );

                        mEntryMap[entry.FileName] = entry;
                    }
                }
                else if ( Version == FormatVersion.Version2 || Version == FormatVersion.Version2BE || Version == FormatVersion.Version3 || Version == FormatVersion.Version3BE )
                {
                    int entryCount = reader.ReadInt32();
                    int nameLength = 32;
                    if ( Version == FormatVersion.Version3 )
                        nameLength = 24;

                    for ( int i = 0; i < entryCount; i++ )
                    {
                        long entryStartPosition = reader.BaseStream.Position;
                        if ( entryStartPosition == reader.BaseStream.Length )
                        {
                            break;
                        }

                        // read entry name
                        for ( int j = 0; j < nameLength; j++ )
                        {
                            byte b = reader.ReadByte();

                            if ( b != 0 )
                                stringBuilder.Append( ( char )b );
                        }

                        string fileName = stringBuilder.ToString();

                        // read entry length
                        int length = reader.ReadInt32();

                        if ( fileName.Length == 0 || length <= 0 || length > 1024 * 1024 * 100 )
                        {
                            break;
                        }

                        // make an entry
                        var entry = new StoredEntry( mBaseStream, fileName, length, ( int )reader.BaseStream.Position );

                        // clear string builder for next iteration
                        stringBuilder.Clear();

                        reader.BaseStream.Position += entry.Length;

                        mEntryMap[entry.FileName] = entry;
                    }
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

        public IEnumerable< string > EnumerateFileSystemEntries( SearchOption option = SearchOption.TopDirectoryOnly )
        {
            return mEntryMap.Select( x => x.Key );
        }

        public IEnumerable< string > EnumerateDirectories( SearchOption option = SearchOption.TopDirectoryOnly )
        {
            throw new NotSupportedException( "This filesystem does not support directories" );
        }

        public IEnumerable< string > EnumerateFiles( SearchOption option = SearchOption.TopDirectoryOnly )
        {
            return EnumerateFileSystemEntries();
        }

        public IEnumerable< string > EnumerateFileSystemEntries( string handle, SearchOption option = SearchOption.TopDirectoryOnly )
        {
            throw new NotSupportedException( "This filesystem does not support directories" );
        }

        public IEnumerable< string > EnumerateDirectories( string handle, SearchOption option = SearchOption.TopDirectoryOnly )
        {
            throw new NotSupportedException( "This filesystem does not support directories" );
        }

        public IEnumerable< string > EnumerateFiles( string handle, SearchOption option = SearchOption.TopDirectoryOnly )
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

            mEntryMap[ handle ] = new MemoryEntry( stream, ownsStream, handle );
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
                if ( Version == FormatVersion.Version2 || Version == FormatVersion.Version2BE || Version == FormatVersion.Version3 || Version == FormatVersion.Version3BE )
                    writer.Write( mEntryMap.Count );

                int nameLength;
                switch ( Version )
                {
                    case FormatVersion.Version1:
                        nameLength = 252;
                        break;
                    case FormatVersion.Version2:
                    case FormatVersion.Version2BE:
                        nameLength = 32;
                        break;
                    case FormatVersion.Version3:
                    case FormatVersion.Version3BE:
                        nameLength = 24;
                        break;
                    default:
                        throw new NotImplementedException( "Invalid format version" );
                }

                foreach ( var entry in mEntryMap.Values )
                {
                    // write entry name
                    for ( int j = 0; j < nameLength; j++ )
                    {
                        byte b = 0;
                        if ( j < entry.FileName.Length )
                            b = ( byte )entry.FileName[j];

                        writer.Write( b );
                    }

                    var entryLength = entry.Length;
                    var paddingByteCount = 0;

                    if ( Version != FormatVersion.Version1 )
                    {
                        paddingByteCount = AlignmentUtils.GetAlignedDifference( entry.Length, 32 );
                        entryLength += paddingByteCount;
                    }

                    // write entry length
                    writer.Write( entryLength );

                    // write data
                    var dataStream = entry.GetStream();
                    dataStream.FullyCopyTo( writer.BaseStream );

                    switch ( Version )
                    {
                        case FormatVersion.Version1:
                            {
                                paddingByteCount = AlignmentUtils.GetAlignedDifference( writer.BaseStream.Position, 64 );
                                for ( int i = 0; i < paddingByteCount; i++ )
                                    writer.Write( ( byte )0 );
                            }
                            break;
                        case FormatVersion.Version2:
                        case FormatVersion.Version2BE:
                        case FormatVersion.Version3:
                        case FormatVersion.Version3BE:
                            {
                                for ( int i = 0; i < paddingByteCount; i++ )
                                    writer.Write( ( byte )0 );
                            }
                            break;
                        default:
                            throw new NotImplementedException( "Invalid format version" );
                    }
                }

                if ( Version == FormatVersion.Version1 )
                {
                    for ( int i = 0; i < 256; i++ )
                        writer.Write( ( byte ) 0 );
                }
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

            if (mOwnsStream)
                mBaseStream?.Dispose();
        }

        public void AddDirectory( string handle, ConflictPolicy policy )
        {
            throw new NotSupportedException( "This filesystem does not support directories" );
        }
    }
}
