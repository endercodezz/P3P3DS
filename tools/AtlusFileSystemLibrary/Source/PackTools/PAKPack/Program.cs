using System;
using System.Collections.Generic;
using System.IO;
using AtlusFileSystemLibrary;
using AtlusFileSystemLibrary.Common.IO;
using AtlusFileSystemLibrary.FileSystems.PAK;

namespace PAKPack
{
    internal static class Program
    {
        public static IReadOnlyDictionary< string, ICommand > Commands { get; } = new Dictionary<string, ICommand>
        {
            { "pack", new PackCommand() },
            { "unpack", new UnpackCommand() },
            { "replace", new ReplaceCommand() },
            { "add", new AddCommand() },
            { "list", new ListCommand() }
        };

        public static IReadOnlyDictionary< string, FormatVersion > FormatsByName { get; } = new Dictionary< string, FormatVersion >
        {
            { "v1", FormatVersion.Version1 },
            { "v2", FormatVersion.Version2 },
            { "v2be", FormatVersion.Version2BE },
            { "v3", FormatVersion.Version3 },
            { "v3be", FormatVersion.Version3BE }
        };

        public static IReadOnlyDictionary<FormatVersion, string> FormatVersionEnumToString { get; } = new Dictionary<FormatVersion, string>
        {
            { FormatVersion.Version1, "v1"},
            { FormatVersion.Version2, "v2" },
            { FormatVersion.Version2BE, "v2be" },
            { FormatVersion.Version3, "v3" },
            { FormatVersion.Version3BE, "v3be" }
        };

        private static void Main( string[] args )
        {
            if ( args.Length == 0 )
            {
                Console.WriteLine( "PAKPack 1.4 - A PAK pack/unpacker made by TGE (2018)\n" +
                                   "\n" +
                                   "Usage:\n" +
                                   "  PAKPack <command>\n" +
                                   "\n" +
                                   "Commands:\n" +
                                   "\n" +
                                   "    pack        Packs the given input into a PAK file and outputs it to the specified output path.\n" +
                                   "        Usage:\n" +
                                   "            pack <input directory path> <format> [output file path]\n" +
                                   "\n" +
                                   "    unpack      Unpacks the given input PAK file and outputs it to the specified output directory.\n" +
                                   "        Usage:\n" +
                                   "            unpack <input file path> [output directory path]\n" +
                                   "\n" +
                                   "    replace     Replaces the specified file(s) with the contents of the specified input\n" +
                                   "        Usage:\n" +
                                   "            replace <input pak file path> <file name to replace> <file path> [output file path]\n" +
                                   "            replace <input pak file path> <path to file directory> [output file path]\n" +
                                   "\n" +
                                   "    add         Adds or replaces the specified file(s) with the contents of the specified input\n" +
                                   "        Usage:\n" +
                                   "            add <input pak file path> <file name to add/replace> <file path> [output file path]\n" +
                                   "            add <input pak file path> <path to file directory> [output file path]\n" +
                                   "\n" +
                                   "    list        Lists the files contained in the pak file\n" +
                                   "        Usage:\n" +
                                   "            list <input pak file path>\n" +
                                   "\n" );
                return;
            }

            if ( !Commands.TryGetValue( args[ 0 ], out var command ) )
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
            if ( args.Length < 3 )
            {
                Console.WriteLine( "Expected at least 2 arguments" );
                return false;
            }

            var inputPath = args[ 1 ];
            if ( !Directory.Exists( inputPath ) )
            {
                Console.WriteLine( "Input directory doesn't exist" );
                return false;
            }

            var formatName = args[ 2 ];
            if ( !Program.FormatsByName.TryGetValue( formatName, out var format ) )
            {
                Console.WriteLine( "Invalid format specified." );
                return false;
            }

            var outputPath = Path.ChangeExtension( inputPath, "pak" );
            if ( args.Length > 3 )
                outputPath = args[ 3 ];

            using ( var pak = new PAKFileSystem( format ) )
            {
                foreach ( string file in Directory.EnumerateFiles( inputPath, "*.*", SearchOption.AllDirectories ) )
                {
                    Console.WriteLine( $"Adding {file}" );

                    pak.AddFile( file.Substring( inputPath.Length )
                                     .Trim( Path.DirectorySeparatorChar )
                                     .Replace( "\\", "/" ),
                                 file, ConflictPolicy.Ignore );
                }

                Console.WriteLine( $"Saving..." );
                pak.Save( outputPath );
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

            if ( !PAKFileSystem.TryOpen( inputPath, out var pak ) )
            {
                Console.WriteLine( "Invalid PAK file." );
                return false;
            }

            using ( pak )
            {
                Console.WriteLine( $"PAK format version: {Program.FormatVersionEnumToString[pak.Version]}" );

                foreach ( string file in pak.EnumerateFiles() )
                {
                    var normalizedFilePath = file.Replace( "../", "" ); // Remove backwards relative path
                    using ( var stream = FileUtils.Create( outputPath + Path.DirectorySeparatorChar + normalizedFilePath ) )
                    using ( var inputStream = pak.OpenFile( file ) )
                    {
                        Console.WriteLine( $"Extracting {file}" );
                        inputStream.CopyTo( stream );
                    }
                }
            }

            return true;
        }
    }

    internal abstract class AddOrReplaceCommand : ICommand
    {
        protected static bool Execute( string[] args, bool allowAdd )
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

            if ( !PAKFileSystem.TryOpen( inputPath, out var pak ) )
            {
                Console.WriteLine( "Invalid PAK file." );
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

                using ( pak )
                {
                    foreach ( string file in Directory.EnumerateFiles( directoryPath, "*.*", SearchOption.AllDirectories ) )
                    {
                        Console.WriteLine( $"{( pak.Exists(file) ? "Replacing" : "Adding" )} {file}" );
                        pak.AddFile( file.Substring( directoryPath.Length )
                                         .Trim( Path.DirectorySeparatorChar )
                                         .Replace( "\\", "/" ),
                                     file, ConflictPolicy.Replace );
                    }

                    Console.WriteLine( "Saving..." );
                    pak.Save( outputPath );
                }
            }
            else
            {
                if ( args.Length > 4 )
                {
                    outputPath = args[4];
                }

                using ( pak )
                {
                    var entryName = args[2];
                    var entryExists = pak.Exists( entryName );

                    if ( !allowAdd && !entryExists )
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

                    Console.WriteLine( $"{( entryExists ? "Replacing" : "Adding")} {entryName}" );
                    pak.AddFile( entryName, filePath, ConflictPolicy.Replace );

                    Console.WriteLine( "Saving..." );
                    pak.Save( outputPath );
                }
            }

            return true;
        }

        public abstract bool Execute( string[] args );
    }

    internal class ReplaceCommand : AddOrReplaceCommand
    {
        public override bool Execute( string[] args ) => Execute( args, false );
    }

    internal class AddCommand : AddOrReplaceCommand
    {
        public override bool Execute( string[] args ) => Execute( args, true );
    }

    internal class ListCommand : ICommand
    {
        public bool Execute( string[] args )
        {
            if ( args.Length < 2 )
            {
                Console.WriteLine( "Expected 1 argument." );
                return false;
            }

            var inputPath = args[1];
            if ( !File.Exists( inputPath ) )
            {
                Console.WriteLine( "Input file doesn't exist." );
                return false;
            }

            if ( !PAKFileSystem.TryOpen( inputPath, out var pak ) )
            {
                Console.WriteLine( "Invalid PAK file." );
                return false;
            }

            using ( pak )
            {
                Console.WriteLine( $"PAK format version: {Program.FormatVersionEnumToString[pak.Version]}" );

                foreach ( string file in pak.EnumerateFiles() )
                    Console.WriteLine( file );
            }

            return true;
        }
    }
}
