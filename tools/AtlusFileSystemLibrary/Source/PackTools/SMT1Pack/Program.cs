using System;
using System.Collections.Generic;
using System.IO;
using AtlusFileSystemLibrary;
using AtlusFileSystemLibrary.Common.IO;
using AtlusFileSystemLibrary.Common.PackTool;
using AtlusFileSystemLibrary.FileSystems.SMT1;

namespace SMT1Pack
{
    internal static class Program
    {
        private static void Main( string[] args )
        {
            var tool = new SMT1PackTool();
            tool.ToolMain( args );
        }
    }

    internal class SMT1PackTool : PackToolBase
    {
        public override string Usage => "SMT1Pack 1.1 - An SMT1 DATA.BIN pack/unpacker made by TGE (2018)\n" +
                                        "\n" +
                                        "Usage:\n" +
                                        "  SMT1Pack <command>\n" +
                                        "\n" +
                                        "Commands:\n" +
                                        "\n" +
                                        "    pack       Packs the given input into a DATA.BIN file and outputs it to the specified output path.\n" +
                                        "               Note: If present, the executable will be updated accordingly.\n" +
                                        "       Usage:\n" +
                                        "           pack <input directory path> [output file path]\n" +
                                        "\n" +
                                        "    unpack     Unpacks the given input DATA.BIN file and outputs it to the specified output directory.\n" +
                                        "               Note: This requires the executable file to be in the same directory as the DATA.BIN file.\n" +
                                        "       Usage:\n" +
                                        "           unpack <input file path> [output directory path]\n" +
                                        "\n" +
                                        "    replace    Replaces the specified file(s) with the contents of the specified input\n" +
                                        "               Note: This requires the executable file to be in the same directory as the DATA.BIN file.\n" +
                                        "       Usage:\n" +
                                        "           replace <input pak file path> <file name to replace> <file path> [output file path]\n" +
                                        "           replace <input pak file path> <path to file directory> [output file path]\n" +
                                        "\n";

        public override IReadOnlyDictionary<string, ICommand> Commands => new Dictionary<string, ICommand>()
        {
            { "pack", new PackCommand() },
            { "unpack", new UnpackCommand() },
            { "replace", new ReplaceCommand() },
        };
    }


    internal class PackCommand : ICommand
    {
        public bool Execute( string[] args )
        {
            if ( args.Length < 1 )
            {
                Console.WriteLine( "Expected at least 1 argument" );
                return false;
            }

            var inputPath = args[0];
            if ( !Directory.Exists( inputPath ) )
            {
                Console.WriteLine( "Input directory doesn't exist" );
                return false;
            }

            var outputPath = Path.ChangeExtension( inputPath, "BIN" );
            if ( args.Length > 1 )
                outputPath = args[1];

            using ( var fs = new SMT1FileSystem() )
            {
                foreach ( string file in Directory.EnumerateFiles( inputPath, "*.*", SearchOption.AllDirectories ) )
                {
                    var name = Path.GetFileNameWithoutExtension( file );
                    if ( !int.TryParse( name, out var handle ) )
                    {
                        Console.WriteLine( $"Skipping file: {file}; file name not a valid index" );
                        continue;
                    }

                    Console.WriteLine( $"Adding file: {file}" );
                    fs.AddFile( handle, file, ConflictPolicy.Replace );
                }

                Console.WriteLine( "Saving..." );
                fs.Save( outputPath );
            }

            return true;
        }
    }

    internal class UnpackCommand : ICommand
    {
        public bool Execute( string[] args )
        {
            if ( args.Length < 1 )
            {
                Console.WriteLine( "Expected at least 1 argument." );
                return false;
            }

            var inputPath = args[0];
            if ( !File.Exists( inputPath ) )
            {
                Console.WriteLine( "Input file doesn't exist." );
                return false;
            }

            var outputPath = Path.ChangeExtension( inputPath, null );
            if ( args.Length > 1 )
                outputPath = args[1];

            Directory.CreateDirectory( outputPath );

            var fs = new SMT1FileSystem();

            try
            {
                fs.Load( inputPath );
            }
            catch ( Exception )
            {
                Console.WriteLine( "Couldn't load DATA.BIN. Did you make sure to put the game executable in the same directory?" );
                return false;
            }

            using ( fs )
            {
                foreach ( int file in fs.EnumerateFiles( SearchOption.AllDirectories ) )
                {
                    var info = fs.GetInfo( file );
                    var extension = ".bin";

                    switch ( info.ContentKind )
                    {
                        case ContentKind.TIM:
                            extension = ".tim";
                            break;
                        case ContentKind.TIMH:
                            extension = ".timh";
                            break;
                        case ContentKind.SC02:
                            extension = ".sc02";
                            break;
                        case ContentKind.Archive:
                            extension = ".arc";
                            break;
                    }

                    var name = file.ToString( "D4" ) + extension;

                    using ( var stream = FileUtils.Create( outputPath + Path.DirectorySeparatorChar + name ) )
                    using ( var inputStream = fs.OpenFile( file ) )
                    {
                        Console.WriteLine( $"Extracting: {name}" );
                        inputStream.CopyTo( stream );
                    }
                }
            }

            return true;
        }
    }

    internal class ReplaceCommand : ICommand
    {
        public bool Execute( string[] args )
        {
            if ( args.Length < 2 )
            {
                Console.WriteLine( "Expected at least 2 arguments." );
                return false;
            }

            var inputPath = args[0];
            if ( !File.Exists( inputPath ) )
            {
                Console.WriteLine( "Input file doesn't exist." );
                return false;
            }

            var fs = new SMT1FileSystem();

            try
            {
                fs.Load( inputPath );
            }
            catch ( Exception )
            {
                Console.WriteLine( "Couldn't load DATA.BIN. Did you make sure to put the game executable in the same directory?" );
                return false;
            }

            string outputPath = inputPath;

            if ( Directory.Exists( args[1] ) )
            {
                var directoryPath = args[1];

                if ( args.Length > 2 )
                {
                    outputPath = args[2];
                }

                using ( fs )
                {
                    foreach ( string file in Directory.EnumerateFiles( directoryPath, "*.*", SearchOption.AllDirectories ) )
                    {
                        var name = Path.GetFileName( file );

                        if ( int.TryParse( name, out var handle ) )
                        {
                            Console.WriteLine( $"Replacing file: {file}" );
                            fs.AddFile( handle, file, ConflictPolicy.Replace );
                        }
                        else
                        {
                            Console.WriteLine( $"Skipping file: {file}; name does not contain a valid index" );
                        }
                    }

                    Console.WriteLine( "Saving..." );
                    fs.Save( outputPath );
                }
            }
            else
            {
                if ( args.Length > 3 )
                {
                    outputPath = args[3];
                }

                using ( fs )
                {
                    var entryName = args[1];

                    if ( int.TryParse( entryName, out var handle ) )
                    {
                        if ( !fs.Exists( handle ) )
                        {
                            Console.WriteLine( "Specified entry doesn't exist." );
                            return false;
                        }

                        var filePath = args[ 2 ];
                        if ( !File.Exists( filePath ) )
                        {
                            Console.WriteLine( "Specified replacement file doesn't exist." );
                            return false;
                        }

                        Console.WriteLine( $"Replacing file: {filePath}" );
                        fs.AddFile( handle, filePath, ConflictPolicy.Replace );

                        Console.WriteLine( "Saving..." );
                        fs.Save( outputPath );
                    }
                    else
                    {
                        Console.WriteLine( "Specified entry doesn't exist." );
                        return false;
                    }
                }
            }

            return true;
        }
    }
}
