using System;
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using AtlusFileSystemLibrary.Common.IO;

namespace AtlusFileSystemLibrary.FileSystems.ACX
{
    public class ACXFileSystem : IIndexedFileSystem
    {
        private int mNextHandle;
        private readonly Dictionary<int, Entry> mEntryMap;
        private Stream mBaseStream;

        public bool IsReadOnly { get; } = false;
        public bool HasDirectories { get; } = false;
        public bool CanSave { get; } = true;
        public bool CanAddOrRemoveEntries { get; } = true;

        public string FilePath { get; private set; }

        public int AllocateHandle() => mNextHandle++;

        public ACXFileSystem()
        {
            mEntryMap = new Dictionary< int, Entry >();
        }

        public void Load( string path )
        {
            FilePath = path;
            Load( File.OpenRead( path ), true );
        }

        public void Load( Stream stream, bool ownsStream )
        {
            mBaseStream = stream;

            using ( var reader = new EndianBinaryReader( stream, Encoding.Default, true, Endianness.BigEndian ) )
            {
                var entryCount = reader.ReadInt64();
                for ( var i = 0; i < entryCount; i++ )
                {
                    var entry = new StoredEntry( AllocateHandle(), stream, reader.ReadUInt32(), reader.ReadUInt32() );
                    mEntryMap[entry.Handle] = entry;
                }
            }
        }

        public FileStream<int> OpenFile( int handle, FileAccess access = FileAccess.Read )
        {
            if ( !mEntryMap.TryGetValue( handle, out var entry ) )
            {
                throw new FileNotFoundException<int>( handle );
            }

            return new FileStream<int>( handle, entry.GetStream() );
        }

        public bool Exists( int handle )
        {
            return mEntryMap.ContainsKey( handle );
        }

        public bool IsFile( int handle )
        {
            return Exists( handle );
        }

        public bool IsDirectory( int handle )
        {
            throw new NotSupportedException( "This filesystem does not support directories" );
        }

        public IEnumerable< int > EnumerateFileSystemEntries( SearchOption option = SearchOption.TopDirectoryOnly )
        {
            return mEntryMap.Keys;
        }

        public IEnumerable< int > EnumerateDirectories( SearchOption option = SearchOption.TopDirectoryOnly )
        {
            throw new NotSupportedException( "This filesystem does not support directories" );
        }

        public IEnumerable< int > EnumerateFiles( SearchOption option = SearchOption.TopDirectoryOnly )
        {
            return EnumerateFileSystemEntries( option );
        }

        public IEnumerable< int > EnumerateFileSystemEntries( int handle, SearchOption option = SearchOption.TopDirectoryOnly )
        {
            throw new NotSupportedException( "This filesystem does not support directories" );
        }

        public IEnumerable< int > EnumerateDirectories( int handle, SearchOption option = SearchOption.TopDirectoryOnly )
        {
            throw new NotSupportedException( "This filesystem does not support directories" );
        }

        public IEnumerable< int > EnumerateFiles( int handle, SearchOption option = SearchOption.TopDirectoryOnly )
        {
            throw new NotSupportedException( "This filesystem does not support directories" );
        }

        public void Delete( int handle )
        {
            if ( !mEntryMap.TryGetValue( handle, out var entry ) )
            {
                throw new FileNotFoundException<int>( handle );
            }

            mEntryMap.Remove( entry.Handle );
        }

        public void AddFile( int handle, string hostPath, ConflictPolicy policy )
        {
            var stream = File.OpenRead( hostPath );
            bool result = false;

            try
            {
                result = TryAddFile( handle, stream, true, policy );
            }
            finally
            {
                if ( !result )
                    stream.Dispose();
            }
        }

        public void AddFile( int handle, Stream stream, bool ownsStream, ConflictPolicy policy )
        {
            TryAddFile( handle, stream, ownsStream, policy );
        }

        private bool TryAddFile( int handle, Stream stream, bool ownsStream, ConflictPolicy policy )
        {
            if ( mEntryMap.TryGetValue( handle, out _ ) )
            {
                switch ( policy.Kind )
                {
                    case ConflictPolicy.PolicyKind.ThrowError:
                        throw new FileExistsException<int>( handle );
                    case ConflictPolicy.PolicyKind.Replace:
                        // Handled later
                        break;
                    case ConflictPolicy.PolicyKind.Ignore:
                        return false;
                }
            }

            mEntryMap[handle] = new MemoryEntry( handle, stream, ownsStream );

            return true;
        }

        public void AddDirectory( int handle, ConflictPolicy policy )
        {
            throw new NotSupportedException( "This filesystem does not support directories" );
        }

        public void AddDirectory( int handle, string hostPath, ConflictPolicy policy )
        {
            throw new NotSupportedException( "This filesystem does not support directories" );
        }

        public void Save( string outPath )
        {
            using ( var fileStream = File.Create( outPath ) )
                Save( fileStream );
        }

        [SuppressMessage( "ReSharper", "AccessToDisposedClosure" )]
        public void Save( Stream stream )
        {
            using ( var writer = new EndianBinaryWriter( stream, Encoding.Default, true, Endianness.BigEndian ) )
            {
                var fixups = new List< Tuple< long, Action > >();
                writer.Write( ( long ) mEntryMap.Count );

                foreach ( var entry in mEntryMap.Values.OrderBy(x => x.Handle) )
                {
                    fixups.Add( new Tuple< long, Action >( writer.BaseStream.Position, () =>
                    {
                        entry.GetStream().FullyCopyTo( writer.BaseStream );

                        var padBytes = AlignmentUtils.GetAlignedDifference( writer.BaseStream.Position, 4 );

                        for ( int i = 0; i < padBytes; i++ )
                            writer.Write( ( byte )0 );

                    } ));
                    writer.Write( ( int ) 0 );
                    writer.Write( entry.Length );
                }

                foreach ( var fixup in fixups )
                {
                    var offset = writer.BaseStream.Position;
                    fixup.Item2();
                    var nextOffset = writer.BaseStream.Position;
                    writer.BaseStream.Seek( fixup.Item1, SeekOrigin.Begin );
                    writer.Write( (int)offset );
                    writer.BaseStream.Seek( nextOffset, SeekOrigin.Begin );
                }
            }
        }

        public Stream Save()
        {
            var stream = new MemoryStream();
            Save( stream );
            return stream;
        }

        public void AddFile( string hostPath )
        {
            AddFile( File.OpenRead( hostPath ));
        }

        public void AddFile( Stream stream, bool ownsStream = true )
        {
            var entry = new MemoryEntry( AllocateHandle(), stream, ownsStream );
            mEntryMap[ AllocateHandle() ] = entry;
        }

        public void Dispose()
        {
            mBaseStream?.Dispose();
        }

        public FileSystemEntryInfo<int> GetInfo( int handle )
        {
            return new FileSystemEntryInfo< int >( handle );
        }
    }
}
