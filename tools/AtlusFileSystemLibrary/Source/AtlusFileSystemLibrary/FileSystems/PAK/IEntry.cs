using System;
using System.IO;

namespace AtlusFileSystemLibrary.FileSystems.PAK
{
    internal interface IEntry : IDisposable
    {
        string FileName { get; }

        int Length { get; }

        Stream GetStream();
    }
}