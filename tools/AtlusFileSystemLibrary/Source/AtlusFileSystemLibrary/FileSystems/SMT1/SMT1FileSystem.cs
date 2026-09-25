using System;
using System.Collections.Generic;
using System.IO;
using AtlusFileSystemLibrary.Common.IO;

namespace AtlusFileSystemLibrary.FileSystems.SMT1
{
    public class SMT1FileSystem : IIndexedFileSystem
    {
        // Constants
        private const int FILE_OFFSET_LIST_OFFSET = 0x8F140; // NTSC
        private const int FILE_OFFSET_COUNT = 1228;
        private const int FILE_COUNT = FILE_OFFSET_COUNT - 1;

        // Fields
        private Stream mBaseStream;
        private bool mOwnsStream;
        private readonly Dictionary<int, IEntry> mEntryMap;

        public bool IsReadOnly { get; } = true;

        public bool HasDirectories { get; } = false;

        public bool CanSave { get; } = true;

        public bool CanAddOrRemoveEntries { get; } = false;

        public int MaxFileCount => FILE_COUNT;

        public string FilePath { get; private set; }

        public SMT1FileSystem()
        {
            mEntryMap = new Dictionary< int, IEntry >();
            for ( int i = 0; i < FILE_COUNT; i++ )
                mEntryMap[i] = null;
        }

        public void AddDirectory( int handle, ConflictPolicy policy )
        {
            throw new NotSupportedException( "This filesystem does not support directories" );
        }

        public void AddDirectory( int handle, string hostPath, ConflictPolicy policy )
        {
            throw new NotSupportedException( "This filesystem does not support directories" );
        }

        public void AddFile( string hostPath )
        {
            throw new NotSupportedException( "This filesystem does not support adding new files" );
        }

        public void AddFile( Stream stream, bool ownsStream = true )
        {
            throw new NotSupportedException( "This filesystem does not support adding new files" );
        }

        public void AddFile( int handle, string hostPath, ConflictPolicy policy )
        {
            AddFile( handle, File.OpenRead( hostPath ), true, policy );
        }

        public void AddFile( int handle, Stream stream, bool ownsStream, ConflictPolicy policy )
        {
            bool added = false;

            try
            {
                if ( !Exists( handle ) )
                    throw new NotSupportedException( "This filesystem does not support adding new files" );

                if ( policy.Kind == ConflictPolicy.PolicyKind.ThrowError )
                    throw new FileExistsException<int>( handle );

                mEntryMap[handle] = new MemoryEntry( handle, stream, ownsStream );
                added = true;
            }
            finally
            {
                if ( !added && ownsStream )
                    stream.Dispose();
            }
        }

        public int AllocateHandle()
        {
            throw new NotSupportedException( "This filesystem does not support adding new files" );
        }

        public void Delete( int handle )
        {
            throw new NotSupportedException( "This filesystem does not support removing entries" );
        }

        public IEnumerable<int> EnumerateDirectories( SearchOption option = SearchOption.TopDirectoryOnly )
        {
            throw new NotSupportedException( "This filesystem does not support directories" );
        }

        public IEnumerable<int> EnumerateDirectories( int handle, SearchOption option = SearchOption.TopDirectoryOnly )
        {
            throw new NotSupportedException( "This filesystem does not support directories" );
        }

        public IEnumerable<int> EnumerateFiles( SearchOption option = SearchOption.TopDirectoryOnly )
        {
            return EnumerateFileSystemEntries();
        }

        public IEnumerable<int> EnumerateFiles( int handle, SearchOption option = SearchOption.TopDirectoryOnly )
        {
            throw new NotSupportedException( "This filesystem does not support directories" );
        }

        public IEnumerable<int> EnumerateFileSystemEntries( SearchOption option = SearchOption.TopDirectoryOnly )
        {
            return mEntryMap.Keys;
        }

        public IEnumerable<int> EnumerateFileSystemEntries( int handle, SearchOption option = SearchOption.TopDirectoryOnly )
        {
            throw new NotSupportedException( "This filesystem does not support directories" );
        }

        public FileStream<int> OpenFile( int handle, FileAccess access = FileAccess.Read )
        {
            if ( !Exists( handle ) )
                throw new FileNotFoundException<int>( handle );

            var entry = mEntryMap[handle];
            if ( entry == null )
                throw new FileNotFoundException<int>( handle );

            return new FileStream<int>( handle, entry.GetStream() );
        }

        public bool Exists( int handle )
        {
            return mEntryMap.ContainsKey( handle );
        }

        public EntryInfo GetInfo( int handle )
        {
            if ( !Exists(handle))
                throw new FileNotFoundException<int>( handle );

            return new EntryInfo( mEntryMap[handle] );
        }

        FileSystemEntryInfo<int> IFileSystem<int>.GetInfo( int handle ) => GetInfo( handle );

        public bool IsDirectory( int handle )
        {
            throw new NotSupportedException( "This filesystem does not support directories" );
        }

        public bool IsFile( int handle )
        {
            return Exists( handle );
        }

        public void Load( string path )
        {
            var directoryPath = Path.GetDirectoryName( path );
            var elfPath = Path.Combine( directoryPath, "SLPS_031.70" );

            if ( !File.Exists( path ) )
                throw new ArgumentException( "DATA.BIN could not be found at the given path", nameof( path ) );

            if ( !File.Exists( elfPath ) )
                throw new ArgumentException( "ELF executable (SLPS_031.70) could not be found at the given path", nameof( path ) );

            // Time to start mounting DATA.BIN
            mBaseStream = File.OpenRead( path );
            FilePath = path;
            mOwnsStream = true;

            // Read file offsets
            using ( var reader = new EndianBinaryReader( File.OpenRead( elfPath ), Endianness.LittleEndian ) )
            {
                reader.SeekBegin( FILE_OFFSET_LIST_OFFSET );

                var currentOffset = reader.ReadInt32();
                for ( int i = 0; i < FILE_COUNT; i++ )
                {
                    var nextOffset = reader.ReadInt32();
                    var length = nextOffset - currentOffset;
                    mEntryMap[ i ] = new StoredEntry( i, mBaseStream, currentOffset, length );
                    currentOffset = nextOffset;
                }
            }
        }

        public void Load( Stream stream, bool ownsStream )
        {
            throw new NotSupportedException( "This filesystem does not support being loaded from a stream" );
        }

        public void Save( string outPath )
        {
            var directoryPath = Path.GetDirectoryName( outPath );
            var elfPath = Path.Combine( directoryPath, "SLPS_031.70" );

            Save( outPath, elfPath );
        }

        public void Save( string outPath, string elfPath )
        {
            if ( elfPath != null && File.Exists( elfPath ) )
            {
                // Create DATA.BIN & update ELF
                using ( var binWriter = new EndianBinaryWriter( FileUtils.Create( outPath, FilePath ), Endianness.LittleEndian ) )
                using ( var elfWriter = new EndianBinaryWriter( File.OpenWrite( elfPath ), Endianness.LittleEndian ) )
                {
                    elfWriter.SeekBegin( FILE_OFFSET_LIST_OFFSET );

                    int nextOffset = 0;
                    int index = 0;
                    foreach ( var entry in mEntryMap.Values )
                    {
                        if ( entry == null )
                            throw new FileNotFoundException<int>( index );

                        elfWriter.Write( nextOffset );
                        entry.GetStream().FullyCopyTo( binWriter.BaseStream );
                        binWriter.WriteAlignmentPadding( 2048 );
                        nextOffset = ( int )binWriter.BaseStream.Position;
                        ++index;
                    }

                    elfWriter.Write( nextOffset );

                    // Close backing stream
                    Dispose();
                }
            }
            else
            {
                // Create just DATA.BIN
                using ( var binWriter = new EndianBinaryWriter( FileUtils.Create( outPath, FilePath ), Endianness.LittleEndian ) )
                {
                    int index = 0;
                    foreach ( var entry in mEntryMap.Values )
                    {
                        if ( entry == null )
                            throw new FileNotFoundException<int>( index );

                        entry.GetStream().FullyCopyTo( binWriter.BaseStream );
                        binWriter.WriteAlignmentPadding( 2048 );

                        ++index;
                    }

                    // Close backing stream
                    Dispose();
                }
            }

            // Reload file
            Load( outPath );
        }

        public void Save( Stream stream )
        {
            throw new NotSupportedException( "This filesystem does not support being saved to a stream" );
        }

        public Stream Save()
        {
            throw new NotSupportedException( "This filesystem does not support being saved to a stream" );
        }

        public void Dispose()
        {
            foreach ( var entry in mEntryMap.Values )
                entry?.Dispose();

            if ( mOwnsStream )
                mBaseStream?.Dispose();
        }
    }
}
