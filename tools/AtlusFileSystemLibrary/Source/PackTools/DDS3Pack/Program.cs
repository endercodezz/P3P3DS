using System;
using System.Collections.Generic;
using System.IO;
using AtlusFileSystemLibrary;
using AtlusFileSystemLibrary.Common.IO;
using AtlusFileSystemLibrary.FileSystems.DDS3;

namespace DDS3Pack
{
    internal static class Program
    {
        public static IReadOnlyDictionary<string, ICommand> Commands { get; } = new Dictionary<string, ICommand>
        {
            { "pack", new PackCommand() },
            { "unpack", new UnpackCommand() },
            { "replace", new ReplaceCommand() }
        };

        private static void Main( string[] args )
        {
            if ( args.Length == 0 )
            {
                Console.WriteLine( "DDS3Pack 1.3 - A DDS3 img/ddt pack/unpacker made by TGE (2018)\n" +
                                   "\n" +
                                   "Usage:\n" +
                                   "  DDS3Pack <command>\n" +
                                   "\n" +
                                   "Commands:\n" +
                                   "\n" +
                                   "    pack        Packs the given input into a ddt/img pair and outputs it to the specified output path.\n" +
                                   "        Usage:\n" +
                                   "            pack <input directory path> [output file path]\n" +
                                   "\n" +
                                   "    unpack      Unpacks the given input ddt/img file and outputs it to the specified output directory.\n" +
                                   "        Usage:\n" +
                                   "            unpack <input file path> [output directory path]\n" +
                                   "\n" +
                                   "    replace     Replaces the specified file(s) with the contents of the specified input\n" +
                                   "        Usage:\n" +
                                   "            replace <input file path> <file name to replace> <file path> [output file path]\n" +
                                   "            replace <input file path> <path to file directory> [output file path]\n" +
                                   "\n" );
                return;
            }

            if ( !Commands.TryGetValue( args[0], out var command ) )
            {
                Console.WriteLine( "Invalid command specified." );
                return;
            }

            if ( command.Execute( args ) )
            {
                Console.WriteLine( "Command executed successfully." );
            }
            else
            {
                Console.WriteLine( "Command failed." );
            }
        }
    }

    internal interface ICommand
    {
        bool Execute( string[] args );
    }

    internal class PackCommand : ICommand
    {
        public bool Execute( string[] args )
        {
            if ( args.Length < 2 )
            {
                Console.WriteLine( "Expected at least 1 argument" );
                return false;
            }

            var inputPath = args[1];
            if ( !Directory.Exists( inputPath ) )
            {
                Console.WriteLine( "Input directory doesn't exist" );
                return false;
            }

            var outputPath = Path.ChangeExtension( inputPath, "ddt" );
            if ( args.Length > 2 )
                outputPath = args[2];

            using ( var fs = new DDS3FileSystem() )
            {
                foreach ( string file in Directory.EnumerateFiles( inputPath, "*.*", SearchOption.AllDirectories ) )
                {
                    var filePath = file.Substring( inputPath.Length )
                                       .Trim( Path.DirectorySeparatorChar )
                                       .Replace( "\\", "/" );

                    Console.WriteLine( $"Adding file: {filePath}" );

                    fs.AddFile( filePath, file, ConflictPolicy.ThrowError );
                }

                Console.WriteLine( "Saving ddt/img..." );
                fs.Save( outputPath );
            }

            return true;
        }
    }

    internal class UnpackCommand : ICommand
    {
        public bool Execute( string[] args )
        {
            if ( args.Length < 2 )
            {
                Console.WriteLine( "Expected at least 1 argument." );
                return false;
            }

            var inputPath = args[1];
            if ( !File.Exists( inputPath ) )
            {
                Console.WriteLine( "Input file doesn't exist." );
                return false;
            }

            var outputPath = Path.ChangeExtension( inputPath, null );
            if ( args.Length > 2 )
                outputPath = args[2];

            Directory.CreateDirectory( outputPath );

            DDS3FileSystem fs = new DDS3FileSystem();

            try
            {
                fs.Load( inputPath );
            }
            catch ( Exception e )
            {
                Console.WriteLine( "Invalid ddt/img file." );
                return false;
            }

            using ( fs )
            {
                foreach ( string file in fs.EnumerateFiles( SearchOption.AllDirectories ) )
                {
                    using ( var stream = FileUtils.Create( outputPath + Path.DirectorySeparatorChar + file ) )
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
            if ( args.Length < 3 )
            {
                Console.WriteLine( "Expected at least 2 arguments." );
                return false;
            }

            var inputPath = args[1];
            if ( !File.Exists( inputPath ) )
            {
                Console.WriteLine( "Input file doesn't exist." );
                return false;
            }

            DDS3FileSystem fs = new DDS3FileSystem();

            try
            {
                fs.Load( inputPath );
            }
            catch ( Exception e )
            {
                Console.WriteLine( "Invalid ddt/img file" );
                return false;
            }

            string outputPath = inputPath;

            if ( Directory.Exists( args[2] ) )
            {
                var directoryPath = args[2];

                if ( args.Length > 3 )
                {
                    outputPath = args[3];
                }

                using ( fs )
                {
                    foreach ( string file in Directory.EnumerateFiles( directoryPath, "*.*", SearchOption.AllDirectories ) )
                    {
                        var filePath = file.Substring( directoryPath.Length )
                                           .Trim( Path.DirectorySeparatorChar )
                                           .Replace( "\\", "/" );

                        bool exists = fs.Exists( filePath );
                        Console.WriteLine( $"{(exists? "Replacing" : "Adding")} file: {filePath}" );

                        fs.AddFile( filePath, file, ConflictPolicy.Replace );
                    }

                    Console.WriteLine( "Saving ddt/img..." );
                    fs.Save( outputPath );
                }
            }
            else
            {
                if ( args.Length > 4 )
                {
                    outputPath = args[4];
                }

                using ( fs )
                {
                    var entryName = args[2];

                    if ( !fs.Exists( entryName ) )
                    {
                        Console.WriteLine( "Specified entry doesn't exist." );
                        return false;
                    }

                    var filePath = args[3];
                    if ( !File.Exists( filePath ) )
                    {
                        Console.WriteLine( "Specified replacement file doesn't exist." );
                        return false;
                    }

                    Console.WriteLine( $"Replacing file: {filePath}" );
                    fs.AddFile( entryName, filePath, ConflictPolicy.Replace );

                    Console.WriteLine( "Saving ddt/img..." );
                    fs.Save( outputPath );
                }
            }

            return true;
        }
    }
}
