using System;
using System.Collections.Generic;
using System.IO;
using AtlusFileSystemLibrary.Common.IO;

namespace AtlusFileSystemLibrary
{
    public interface IFileSystem<THandle> : IDisposable
    {
        // Capabilities
        bool IsReadOnly { get; }
        bool HasDirectories { get; }
        bool CanSave { get; }
        bool CanAddOrRemoveEntries { get; }
        string FilePath { get; }

        // Load/Save
        void Load( string path );

        void Load( Stream stream, bool ownsStream );

        void Save( string outPath );

        void Save( Stream stream );

        FileStream<THandle> OpenFile( THandle handle, FileAccess access = FileAccess.Read );

        // Stats
        bool Exists( THandle handle );
        bool IsFile( THandle handle );
        bool IsDirectory( THandle handle );
        FileSystemEntryInfo< THandle > GetInfo( THandle handle );

        // Enumeration
        IEnumerable<THandle> EnumerateFileSystemEntries( SearchOption option = SearchOption.TopDirectoryOnly );
        IEnumerable<THandle> EnumerateDirectories( SearchOption option = SearchOption.TopDirectoryOnly );
        IEnumerable<THandle> EnumerateFiles( SearchOption option = SearchOption.TopDirectoryOnly );

        IEnumerable<THandle> EnumerateFileSystemEntries( THandle handle, SearchOption option = SearchOption.TopDirectoryOnly );
        IEnumerable<THandle> EnumerateDirectories( THandle handle, SearchOption option = SearchOption.TopDirectoryOnly );
        IEnumerable<THandle> EnumerateFiles( THandle handle, SearchOption option = SearchOption.TopDirectoryOnly );

        // Editing
        void Delete( THandle handle );

        void AddFile( THandle handle, string hostPath, ConflictPolicy policy );
        void AddFile( THandle handle, Stream stream, bool ownsStream, ConflictPolicy policy );
        void AddDirectory( THandle handle, ConflictPolicy policy );
        void AddDirectory( THandle handle, string hostPath, ConflictPolicy policy );

        Stream Save();
    }
}
