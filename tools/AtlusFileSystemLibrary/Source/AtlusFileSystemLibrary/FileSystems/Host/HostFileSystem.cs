using System;
using System.Collections.Generic;
using System.IO;
using AtlusFileSystemLibrary.Common.IO;

namespace AtlusFileSystemLibrary.FileSystems.Host
{
#pragma warning disable S3881 // "IDisposable" should be implemented correctly
    /// <summary>
    /// IFileSystem Wrapper for the host filesystem.
    /// </summary>
    public class HostFileSystem : INamedFileSystem
#pragma warning restore S3881 // "IDisposable" should be implemented correctly
    {
        public string RootDirectory { get; set; } = string.Empty;

        public bool IsReadOnly => false;

        public bool HasDirectories => true;

        public bool CanSave => false;

        public bool CanAddOrRemoveEntries => true;

        public string FilePath { get; private set; }

        public HostFileSystem()
        {
        }

        public HostFileSystem( string rootDirectory )
        {
            RootDirectory = rootDirectory;
        }

        public void AddDirectory( string handle, ConflictPolicy policy )
        {
            Directory.CreateDirectory( Path.Combine( RootDirectory, handle ) );
        }

        public void AddDirectory( string handle, string hostPath, ConflictPolicy policy )
        {
            Directory.CreateDirectory( Path.Combine( RootDirectory, handle ) );
        }

        public void AddFile( string handle, string hostPath, ConflictPolicy policy )
        {
            File.Copy( hostPath, Path.Combine(RootDirectory, handle), policy.Kind == ConflictPolicy.PolicyKind.Replace );
        }

        public void AddFile( string handle, Stream stream, bool ownsStream, ConflictPolicy policy )
        {
            using ( var fileStream = File.Create( Path.Combine( RootDirectory, handle ) ) )
                stream.CopyTo( fileStream );
        }

        public void Delete( string handle )
        {
            File.Delete( Path.Combine( RootDirectory, handle ) );
        }

        public IEnumerable<string> EnumerateDirectories( SearchOption option = SearchOption.TopDirectoryOnly )
        {
            if (RootDirectory.Length == 0)
                throw new InvalidOperationException( "Must specify a root directory to enumerate over directories in this filesystem" );

            return EnumerateDirectories( RootDirectory, option );
        }

        public IEnumerable<string> EnumerateDirectories( string handle, SearchOption option = SearchOption.TopDirectoryOnly )
        {
            return Directory.EnumerateDirectories( Path.Combine( RootDirectory, handle ), "*.*", option );
        }

        public IEnumerable<string> EnumerateFiles( SearchOption option = SearchOption.TopDirectoryOnly )
        {
            if ( RootDirectory.Length == 0 )
                throw new InvalidOperationException( "Must specify a root directory to enumerate over directories in this filesystem" );

            return EnumerateFiles( RootDirectory, option );
        }

        public IEnumerable<string> EnumerateFiles( string handle, SearchOption option = SearchOption.TopDirectoryOnly )
        {
            return Directory.EnumerateFiles( Path.Combine( RootDirectory, handle ), "*.*", option );
        }

        public IEnumerable<string> EnumerateFileSystemEntries( SearchOption option = SearchOption.TopDirectoryOnly )
        {
            if ( RootDirectory.Length == 0 )
                throw new InvalidOperationException( "Must specify a root directory to enumerate over directories in this filesystem" );

            return EnumerateFileSystemEntries( RootDirectory, option );
        }

        public IEnumerable<string> EnumerateFileSystemEntries( string handle, SearchOption option = SearchOption.TopDirectoryOnly )
        {
            return Directory.EnumerateFileSystemEntries( Path.Combine( RootDirectory, handle ), "*.*", option );
        }

        public FileStream<string> OpenFile( string handle, FileAccess access = FileAccess.Read )
        {
            return new FileStream<string>(
                handle,
                new FileStream( Path.Combine( RootDirectory, handle ), FileMode.Open, access,
                                access == FileAccess.Read ? FileShare.Read : FileShare.None ) );
        }

        public bool Exists( string handle )
        {
            var absPath = Path.Combine( RootDirectory, handle );
            return File.Exists( absPath ) || Directory.Exists( absPath );
        }

        public FileSystemEntryInfo<string> GetInfo( string handle )
        {
            return new FileSystemEntryInfo<string>( handle );
        }

        public bool IsDirectory( string handle )
        {
            return Directory.Exists( handle );
        }

        public bool IsFile( string handle )
        {
            return File.Exists( handle );
        }

        public void Load( string path )
        {
            throw new NotSupportedException();
        }

        public void Load( Stream stream, bool ownsStream )
        {
            throw new NotSupportedException();
        }

        public void Save( string outPath )
        {
            throw new NotSupportedException();
        }

        public void Save( Stream stream )
        {
            throw new NotSupportedException();
        }

        public Stream Save()
        {
            throw new NotSupportedException();
        }

        public void Dispose()
        {
            // Nothing to dispose
        }
    }
}
