using System;

namespace AtlusFileSystemLibrary
{
    public class FileSystemEntryException : Exception
    {
        public object Handle { get; }

        public FileSystemEntryException( string message, object handle ) : base( message )
        {
            Handle = handle;
        }
    }

    public class FileSystemEntryException<T> : FileSystemEntryException
    {
        public new T Handle
        {
            get => ( T )base.Handle;
        }

        public FileSystemEntryException( string message, T handle ) : base( message, handle )
        {
        }
    }

    public class FileExistsException<T> : FileSystemEntryException<T>
    {
        public FileExistsException( T handle ) : base( "The given file already exists.", handle )
        {
        }
    }

    public class FileNotFoundException<T> : FileSystemEntryException<T>
    {
        public FileNotFoundException( T handle ) : base( "The given file does not exist.", handle )
        {
        }
    }
}