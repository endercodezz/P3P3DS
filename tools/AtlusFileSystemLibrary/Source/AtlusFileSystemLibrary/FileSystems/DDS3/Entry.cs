using System;
using System.Text;

namespace AtlusFileSystemLibrary.FileSystems.DDS3
{
    internal abstract class Entry : IDisposable
    {
        private string mFullName;
        private DirectoryEntry mParent;

        public DirectoryEntry Parent
        {
            get => mParent;
            set
            {
                if ( value != mParent )
                {
                    mParent = value;
                    mFullName = null;
                }
            }
        }

        public FileSystemEntryKind Kind { get; }

        public string Name { get; }

        public uint Offset { get; }

        public string FullName
        {
            get
            {
                if ( mFullName == null )
                {
                    var builder = new StringBuilder( 64 );

                    var parent = Parent;
                    while ( parent != null )
                    {
                        builder.Insert( 0, parent.Name + "/" );
                        parent = parent.Parent;
                    }

                    builder.Append( Name );
                    mFullName = builder.ToString();
                }

                return mFullName;
            }
        }

        protected Entry( DirectoryEntry parent, string name, uint offset, FileSystemEntryKind kind)
        {
            mParent = parent;
            Kind = kind;
            Name = name;
            Offset = offset;
        }

        public abstract void Dispose();
    }
}
