using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using AtlusFileSystemLibrary.Common.IO;

namespace AtlusFileSystemLibrary.FileSystems.DDS3
{
    public class DDS3FileSystem : INamedFileSystem
    {
        // Constants
        internal const int SECTOR_SIZE = 0x800;

        // Fields
        private Stream mImgStream;

        private DirectoryEntry mRoot;

        private LinkedList< Tuple<long, Func<long>> > mPostWrites;

        // Properties
        public bool IsReadOnly { get; } = false;

        public bool HasDirectories { get; } = true;

        public bool CanSave { get; } = true;

        public bool CanAddOrRemoveEntries { get; } = true;

        public string FilePath { get; private set; }

        public DDS3FileSystem()
        {
            mRoot = new DirectoryEntry( null, "", uint.MaxValue );
        }

        // INamedFileSystem implementation
        public void Load( string path )
        {
            if ( path == null )
            {
                throw new ArgumentNullException( nameof( path ) );
            }

            string ddtPath = Path.ChangeExtension( path, "ddt" );
            string imgPath = Path.ChangeExtension( path, "img" );

            if ( !File.Exists( ddtPath ) )
            {
                throw new ArgumentException( "No DDT file found." );
            }

            if ( !File.Exists( imgPath ) )
            {
                throw new ArgumentException( "No IMG file found." );
            }

            FilePath = imgPath;
            mImgStream = File.OpenRead( imgPath );
            using ( var reader = new BinaryReader( File.OpenRead( ddtPath ) ) )
            {
                mRoot = ( DirectoryEntry ) ReadEntry( null, reader );
            }
        }

        public void Load( Stream stream, bool ownsStream )
        {
            throw new NotSupportedException( "DDS3 file system does not support being read from a stream right now." );
        }

        public void AddDirectory( string handle, ConflictPolicy policy )
        {
            AddDirectory( handle, null, policy );
        }

        public void AddDirectory( string handle, string hostPath, ConflictPolicy policy )
        {
            var pathParts = handle.Split( new[] { '/', '\\' }, StringSplitOptions.RemoveEmptyEntries );
            var currentDirectory = mRoot;

            int i = 0;
            for ( ; i < pathParts.Length; ++i )
            {
                string part = pathParts[ i ];
                if ( !currentDirectory.Entries.TryGetValue( part, out var foundEntry ) )
                    break;

                if ( foundEntry.Kind != FileSystemEntryKind.Directory )
                {
                    throw new DirectoryNotFoundException( "Path of path does not exist" );
                }

                currentDirectory = ( DirectoryEntry ) foundEntry;
            }

            for ( ; i < pathParts.Length; ++i )
            {
                var newDirectory = new DirectoryEntry( currentDirectory, pathParts[i], uint.MaxValue );
                currentDirectory.Entries[newDirectory.Name] = newDirectory;
                currentDirectory = newDirectory;
            }     
        }

        public void AddFile( string handle, string hostPath, ConflictPolicy policy )
        {
            AddFile( handle, File.OpenRead( hostPath ), true, policy );
        }

        public void AddFile( string handle, Stream stream, bool ownsStream, ConflictPolicy policy )
        {
            var directoryPath = Path.GetDirectoryName( handle );
            DirectoryEntry directory;

            if ( !string.IsNullOrWhiteSpace( directoryPath ) )
            {
                AddDirectory( directoryPath, policy );

                if ( !TryFindDirectory( directoryPath, out directory ) )
                {
                    throw new DirectoryNotFoundException( "Part of path doesn't exist" );
                }
            }
            else
            {
                directory = mRoot;
            }

            var fileName = Path.GetFileName( handle );
            if ( directory.Entries.TryGetValue( fileName, out var entry ) )
            {
                switch ( policy.Kind )
                {
                    case ConflictPolicy.PolicyKind.ThrowError:
                        throw new FileExistsException<string>( handle );

                    case ConflictPolicy.PolicyKind.Replace:
                        // Handled after this code
                        break;

                    case ConflictPolicy.PolicyKind.Ignore:
                        return;

                    default:
                        throw new ArgumentOutOfRangeException( nameof( policy ) );
                }
            }

            directory.Entries[fileName] = new MemoryFileEntry( directory, stream, ownsStream, fileName );
        }

        public void Delete( string handle )
        {
            if ( !TryFindEntry( handle, out var entry ) )
            {
                throw new FileNotFoundException( "The specified entry does not exist", handle );
            }

            if ( entry.Parent != null )
            {
                entry.Parent.Entries.Remove( entry.Name );
            }
        }

        public IEnumerable<string> EnumerateFileSystemEntries( SearchOption option = SearchOption.TopDirectoryOnly )
        {
            foreach ( var entry in EnumerateFileSystemEntriesInternal(mRoot, option) )
                yield return entry.FullName;
        }

        public IEnumerable<string> EnumerateFileSystemEntries( string handle, SearchOption option = SearchOption.TopDirectoryOnly )
        {
            if ( !TryFindDirectory( handle, out var directory ) )
            {
                throw new DirectoryNotFoundException( $"The specified directory does not exist: {handle}" );
            }

            foreach ( var entry in EnumerateFileSystemEntriesInternal( directory, option ) )
                yield return entry.FullName;
        }

        public IEnumerable<string> EnumerateDirectories( SearchOption option = SearchOption.TopDirectoryOnly )
        {
            foreach ( var entry in EnumerateFileSystemEntriesInternal( mRoot, option ) )
            {
                if (entry.Kind == FileSystemEntryKind.Directory)
                    yield return entry.FullName;
            }
        }

        public IEnumerable<string> EnumerateDirectories( string handle, SearchOption option = SearchOption.TopDirectoryOnly )
        {
            if ( !TryFindDirectory( handle, out var directory ) )
            {
                throw new DirectoryNotFoundException( $"The specified directory does not exist: {handle}" );
            }

            foreach ( var entry in EnumerateFileSystemEntriesInternal( directory, option ) )
            {
                if ( entry.Kind == FileSystemEntryKind.Directory )
                    yield return entry.FullName;
            }
        }

        public IEnumerable<string> EnumerateFiles( SearchOption option = SearchOption.TopDirectoryOnly )
        {
            foreach ( var entry in EnumerateFileSystemEntriesInternal( mRoot, option ) )
            {
                if ( entry.Kind == FileSystemEntryKind.File )
                    yield return entry.FullName;
            }
        }

        public IEnumerable<string> EnumerateFiles( string handle, SearchOption option = SearchOption.TopDirectoryOnly )
        {
            if ( !TryFindDirectory( handle, out var directory ) )
            {
                throw new DirectoryNotFoundException( $"The specified directory does not exist: {handle}" );
            }

            foreach ( var entry in EnumerateFileSystemEntriesInternal( directory, option ) )
            {
                if ( entry.Kind == FileSystemEntryKind.File )
                    yield return entry.FullName;
            }
        }

        public FileStream<string> OpenFile( string handle, FileAccess access = FileAccess.Read )
        {
            if ( !TryFindFile( handle, out var file ) )
            {
                throw new FileNotFoundException( "The specified file does not exist", handle );
            }

            return new FileStream<string>( handle, file.GetStream() );
        }

        public bool Exists( string handle )
        {
            return TryFindEntry( handle, out _ );
        }

        public bool IsFile( string handle )
        {
            return TryFindFile( handle, out _ );
        }

        public bool IsDirectory( string handle )
        {
            return TryFindDirectory( handle, out _ );
        }

        public void Save( string outPath )
        {
            string ddtPath = Path.ChangeExtension( outPath, "ddt" );
            string imgPath = Path.ChangeExtension( outPath, "img" );

            using ( var ddtWriter = new EndianBinaryWriter( FileUtils.Create( ddtPath ), Endianness.LittleEndian ) )
            using ( var imgWriter = new EndianBinaryWriter( FileUtils.Create( imgPath, FilePath ), Endianness.LittleEndian ) )
            {
                mPostWrites = new LinkedList< Tuple< long, Func< long > > >();
                WriteEntry( mRoot, ddtWriter, imgWriter );
                DoPostWrites( ddtWriter );

                // Close backing stream
                Dispose();
            }

            // Reload
            Load( outPath );
        }

        public Stream Save()
        {
            throw new NotSupportedException( "Saving to a stream is not supported right now." );
        }

        public void Save( Stream stream )
        {
            throw new NotSupportedException( "Saving to a stream is not supported right now." );
        }

        public void Dispose()
        {
            foreach ( var entry in EnumerateFileSystemEntriesInternal(mRoot, SearchOption.TopDirectoryOnly) )
                entry.Dispose();

            mImgStream?.Dispose();
        }

        // Read/Write methods
        private Entry ReadEntry( DirectoryEntry parent, BinaryReader reader )
        {
            uint nameOffset = reader.ReadUInt32();
            uint offset = reader.ReadUInt32();
            int count = reader.ReadInt32();

            string name = string.Empty;
            long savedPos = reader.BaseStream.Position; 
            if ( nameOffset != 0 )
            {
                reader.BaseStream.Seek( nameOffset, SeekOrigin.Begin );

                char c;
                var stringBuilder = new StringBuilder();

                while ( ( c = reader.ReadChar() ) != 0 )
                    stringBuilder.Append( c );

                name = stringBuilder.ToString();
                reader.BaseStream.Seek( savedPos, SeekOrigin.Begin );
            }

            Entry entry;
            if ( count < 0 )
            {
                // Directory
                var directory = new DirectoryEntry( parent, name, offset );
                int entryCount = -count;

                reader.BaseStream.Seek( offset, SeekOrigin.Begin );

                for ( int i = 0; i < entryCount; i++ )
                {
                    var childEntry = ReadEntry( directory, reader );
                    directory.Entries[ childEntry.Name ] = childEntry;
                }

                entry =  directory;
            }
            else
            {
                entry = new StoredFileEntry( parent, mImgStream, name, offset, (uint)count );
            }

            reader.BaseStream.Seek( savedPos, SeekOrigin.Begin );
            return entry;
        }

        private void WriteEntry( Entry entry, EndianBinaryWriter ddtWriter, EndianBinaryWriter imgWriter )
        {
            ddtWriter.WriteAlignmentPadding( 4 );

            // name offset
            if ( !string.IsNullOrWhiteSpace(entry.Name) )
            {
                WriteOffset( ddtWriter, () =>
                {
                    foreach ( char c in entry.Name )
                        ddtWriter.Write( ( byte ) c );

                    ddtWriter.Write( ( byte ) 0 );
                } );
            }
            else
            {
                ddtWriter.Write( ( int ) 0 );
            }

            // offset
            if ( entry.Kind == FileSystemEntryKind.Directory )
            {
                WriteOffsetAligned( ddtWriter, 4, () =>
                {
                    foreach ( var childEntry in ( ( DirectoryEntry )entry ).Entries.Values.OrderBy( x => x.Name, StringComparer.Instance ) )
                        WriteEntry( childEntry, ddtWriter, imgWriter );
                } );
            }
            else
            {
                ddtWriter.Write( ( int ) ( imgWriter.BaseStream.Position / SECTOR_SIZE ) );
            }

            // count
            ddtWriter.Write( entry.Kind == FileSystemEntryKind.File
                              ? ( ( FileEntry ) entry ).Length
                              : ( uint ) -(( ( DirectoryEntry ) entry ).Entries.Count) );

            if ( entry.Kind == FileSystemEntryKind.File )
            {
                ( ( FileEntry ) entry ).GetStream().FullyCopyTo( imgWriter.BaseStream );
                imgWriter.WriteAlignmentPadding( SECTOR_SIZE );
            }
        }

        private void WriteOffset( EndianBinaryWriter writer, Action action )
        {
            WriteOffset( writer, () =>
            {
                long offset = writer.BaseStream.Position;
                action();
                return offset;
            } );
        }

        private void WriteOffsetAligned( EndianBinaryWriter writer, int alignment, Action action )
        {
            WriteOffset( writer, () =>
            {
                writer.WriteAlignmentPadding( alignment );
                long offset = writer.BaseStream.Position;
                action();
                return offset;
            } );
        }

        private void WriteOffset( EndianBinaryWriter writer, Func<long> action )
        {
            mPostWrites.AddLast( new Tuple<long, Func<long>>(writer.BaseStream.Position, action) );
            writer.Write( ( int ) 0 );
        }

        private void DoPostWrites( EndianBinaryWriter ddtWriter )
        {
            var current = mPostWrites.First;
            while ( current != null )
            {
                DoPostWrite( current.Value, ddtWriter );
                current = current.Next;
            }
        }

        private void DoPostWrite( Tuple< long, Func<long> > postWrite, EndianBinaryWriter writer )
        {
            long offsetOffset = postWrite.Item1;

            // Do actual write
            long offset = postWrite.Item2();

            // Write offset
            long returnPos = writer.BaseStream.Position;
            writer.BaseStream.Seek( offsetOffset, SeekOrigin.Begin );
            writer.Write( ( int )offset );

            // Seek back for next one
            writer.BaseStream.Seek( returnPos, SeekOrigin.Begin );
        }

        // Lookup
        private bool TryFindEntry( string name, out Entry entry )
        {
            var pathParts = name.Split( new[] { '/', '\\' }, StringSplitOptions.RemoveEmptyEntries );
            var currentDirectory = mRoot;

            for ( int i = 0; i < pathParts.Length; i++ )
            {
                if ( !currentDirectory.Entries.TryGetValue( pathParts[ i ], out var foundEntry ) )
                {
                    entry = null;
                    return false;
                }

                bool isLast = i == pathParts.Length - 1;
                if ( isLast )
                {
                    entry = foundEntry;
                    return true;
                }
                if ( foundEntry.Kind == FileSystemEntryKind.File )
                {
                    entry = null;
                    return false;
                }

                currentDirectory = ( DirectoryEntry ) foundEntry;
            }

            entry = null;
            return false;
        }

        private bool TryFindFile( string name, out FileEntry file )
        {
            if ( !TryFindEntry( name, out var entry ) )
            {
                file = null;
                return false;
            }

            if ( entry.Kind != FileSystemEntryKind.File )
            {
                file = null;
                return false;
            }

            file = ( FileEntry )entry;
            return true;
        }

        private bool TryFindDirectory( string name, out DirectoryEntry directory )
        {
            if ( !TryFindEntry( name, out var entry ) )
            {
                directory = null;
                return false;
            }

            if ( entry.Kind != FileSystemEntryKind.Directory )
            {
                directory = null;
                return false;
            }

            directory = ( DirectoryEntry )entry;
            return true;
        }

        private static IEnumerable< Entry > EnumerateFileSystemEntriesInternal( DirectoryEntry directory, SearchOption option )
        {
            foreach ( var entry in directory.Entries.Values )
            {
                yield return entry;

                if ( entry.Kind == FileSystemEntryKind.Directory && option == SearchOption.AllDirectories )
                {
                    foreach ( var subEntry in EnumerateFileSystemEntriesInternal( ( DirectoryEntry )entry, option ) )
                        yield return subEntry;
                }
            }
        }

        public EntryInfo GetInfo( string handle )
        {
            if ( !TryFindEntry( handle, out var entry ) )
            {
                throw new FileNotFoundException( "The specified file does not exist", handle );
            }

            return new EntryInfo( entry );
        }

        FileSystemEntryInfo< string > IFileSystem< string >.GetInfo( string handle ) => GetInfo( handle );
    }
}
