using System;
using System.Collections.Generic;
using System.IO;
using AtlusFileSystemLibrary;
using AtlusFileSystemLibrary.Common.IO;
using AtlusFileSystemLibrary.Common.PackTool;
using AtlusFileSystemLibrary.FileSystems.ACX;


namespace ACXPack
{
    internal static class Program
    {
        private static void Main( string[] args )
        {
            var tool = new ACXPackTool();
            tool.ToolMain( args );
        }
    }

    internal class ACXPackTool : PackToolBase
    {
        public override string Usage => "ACXPack 1.2 - An ACX pack/unpacker made by TGE (2018)\n" +
                                        "\n" +
                                        "Usage:\n" +
                                        "  ACXPack <command>\n" +
                                        "\n" +
                                        "Commands:\n" +
                                        "\n" +
                                        "    pack        Packs the given input into an ACX file and outputs it to the specified output path.\n" +
                                        "        Usage:\n" +
                                        "            pack <input directory path> [output file path]\n" +
                                        "\n" +
                                        "    unpack      Unpacks the given input ACX file and outputs it to the specified output directory.\n" +
                                        "        Usage:\n" +
                                        "            unpack <input file path> [output directory path]\n" +
                                        "\n" +
                                        "    replace     Replaces the specified file(s) with the contents of the specified input\n" +
                                        "        Usage:\n" +
                                        "            replace <input acx file path> <file name to replace> <file path> [output file path]\n" +
                                        "            replace <input acx file path> <path to file directory> [output file path]\n" +
                                        "\n";

        public override IReadOnlyDictionary< string, ICommand > Commands => new Dictionary< string, ICommand >()
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

            var outputPath = Path.ChangeExtension( inputPath, "acx" );
            if ( args.Length > 1 )
                outputPath = args[1];

            using ( var fs = new ACXFileSystem() )
            {
                foreach ( string file in Directory.EnumerateFiles( inputPath, "*.*", SearchOption.AllDirectories ) )
                {
                    Console.WriteLine( $"Adding file: {file}" );
                    fs.AddFile( file );
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

            var fs = new ACXFileSystem();

            try
            {
                fs.Load( inputPath );
            }
            catch ( Exception )
            {
                Console.WriteLine( "Invalid acx file." );
                return false;
            }

            using ( fs )
            {
                foreach ( int file in fs.EnumerateFiles( SearchOption.AllDirectories ) )
                {
                    using ( var stream = FileUtils.Create( outputPath + Path.DirectorySeparatorChar + file.ToString("D3") + ".adx" ) )
                    using ( var inputStream = fs.OpenFile( file ) )
                    {
                        Console.WriteLine( $"Extracting: {file}" );
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

            var fs = new ACXFileSystem();

            try
            {
                fs.Load( inputPath );
            }
            catch ( Exception )
            {
                Console.WriteLine( "Invalid acx file" );
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
                        Console.WriteLine( $"Adding/Replacing file: {file}" );
                        var name = Path.GetFileName( file );
                        if ( int.TryParse( name, out var handle ) )
                        {
                            fs.AddFile( handle, file, ConflictPolicy.Replace );
                        }
                        else
                        {
                            fs.AddFile( file );
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

                        var filePath = args[2];
                        if ( !File.Exists( filePath ) )
                        {
                            Console.WriteLine( "Specified replacement file doesn't exist." );
                            return false;
                        }

                        Console.WriteLine( $"Adding/Replacing file: {filePath}" );
                        fs.AddFile( handle, filePath, ConflictPolicy.Replace );

                        Console.WriteLine( "Saving..." );
                        fs.Save( outputPath );
                    }
                }
            }

            return true;
        }
    }
}
