namespace AtlusFileSystemLibrary.FileSystems.PAK
{
    public enum FormatVersion
    {
        Unknown,

        /// <summary>
        /// 252 bytes filename, 4 bytes filesize
        /// </summary>
        Version1,

        /// <summary>
        /// Entry count header, 32 bytes filename, 4 bytes filesize
        /// </summary>
        Version2,

        Version2BE,

        /// <summary>
        /// Entry count header, 24 bytes filename, 4 bytes filesize
        /// </summary>
        Version3,

        Version3BE,

        Autodetect
    }
}