using System.IO;

namespace AtlusFileSystemLibrary.FileSystems.APAK
{
    public interface IEntry
    {
        string FileName { get; }

        Stream GetStream();
        void Dispose();
    }
}
