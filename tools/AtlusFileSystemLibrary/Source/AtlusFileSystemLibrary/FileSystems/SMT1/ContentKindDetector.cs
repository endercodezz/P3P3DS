using System.IO;
using System.Text;
using System.Linq;
using AtlusFileSystemLibrary.Common.IO;
using AtlusFileSystemLibrary.Common.Utilities;

namespace AtlusFileSystemLibrary.FileSystems.SMT1
{
    internal static class ContentKindDetector
    {
        private static readonly (ContentKind, string)[] sSignatures =
        {
            ( ContentKind.TIM, "10 00 00 00 08 00 00 00" ), // 4BPP
            ( ContentKind.TIM, "10 00 00 00 09 00 00 00" ), // 8BPP
            ( ContentKind.TIM, "10 00 00 00 02 00 00 00" ), // 16BPP
            ( ContentKind.TIM, "10 00 00 00 03 00 00 00" ), // 24BPP
            ( ContentKind.TIMH, "08 00 00 00 80 13 00 00" ),
            ( ContentKind.SC02, "53 43 30 32" ),
        };

        public static ContentKind Detect( Stream stream )
        {
            var kind = ContentKind.Unknown;

            foreach ( var signature in sSignatures )
            {
                if ( SignatureScanner.Matches( stream, signature.Item2 ) )
                {
                    kind = signature.Item1;
                    break;
                }
            }

            if ( kind == ContentKind.Unknown )
            {
                // Maybe its an archive
                using ( var reader = new EndianBinaryReader( stream, Encoding.Default, true, Endianness.LittleEndian ) )
                {
                    var count = reader.ReadUInt32();
                    var baseOffset = reader.ReadInt32();

                    if ( ( baseOffset - 0x8 ) == ( count * 4 ) )
                    {
                        var offsets = reader.ReadInt32s( ( int )count );
                        var isArchive = true;
                        for ( int i = 1; i < offsets.Length; i++ )
                        {
                            if ( offsets[ i - 1 ] > offsets[ i ] )
                            {
                                isArchive = false;
                                break;
                            }        
                        }

                        if ( isArchive )
                            kind = ContentKind.Archive;
                    }
                }
            }

            stream.Position = 0;

            return kind;
        }
    }
}