using System;
using System.Collections.Generic;
using System.IO;
using AtlusFileSystemLibrary;
using AtlusFileSystemLibrary.Common.IO;
using AtlusFileSystemLibrary.Common.PackTool;
using AtlusFileSystemLibrary.FileSystems.APAK;

namespace APAKPack
{
    internal static class Program
    {
        private static void Main( string[] args )
        {
            var tool = new APAKPackTool();
            tool.ToolMain( args );
        }
    }

    internal class APAKPackTool : PackToolBase
    {
        public override string Usage => "APAKPack 1.0 - An APAK pack/unpacker made by TGE (2020)\n" +
                                        "\n" +
                                        "Usage:\n" +
                                        "  LBPack <command>\n" +
                                        "\n" +
                                        "Commands:\n" +
                                        "\n" +
                                        "    unpack      Unpacks the given input LB file and outputs it to the specified output directory.\n" +
                                        "        Usage:\n" +
                                        "            unpack <input file path> [output directory path]\n";

        public override IReadOnlyDictionary<string, ICommand> Commands => new Dictionary<string, ICommand>()
        {
            { "unpack", new UnpackCommand() },
        };
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

            var fs = new APAKFileSystem();

            try
            {
                fs.Load( inputPath );
            }
            catch ( Exception )
            {
                Console.WriteLine( "Invalid APAK file." );
                return false;
            }

            using ( fs )
            {
                foreach ( var file in fs.EnumerateFiles( SearchOption.AllDirectories ) )
                {
                    using ( var stream = FileUtils.Create( $"{outputPath}{Path.DirectorySeparatorChar}{file}" ) )
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
}
