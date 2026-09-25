using System;
using System.Globalization;
using System.IO;

namespace AtlusFileSystemLibrary.Common.Utilities
{
    public static class SignatureScanner
    {
        public static bool Matches( Stream stream, string signature )
        {
            if ( stream == null )
                throw new ArgumentNullException( nameof( stream ) );

            // Remove spaces
            signature = signature.Replace( " ", "" );

            if ( signature.Length % 2 != 0 )
                throw new ArgumentException( "Signature should only consist out of whole bytes", nameof( signature ) );

            // Read bytes
            int byteCount = signature.Length / 2;
            var bytes = new byte[byteCount];
            stream.Read( bytes, 0, bytes.Length );
            stream.Position = 0;

            // Match bytes
            var byteIndex = 0;
            for ( int i = 0; i < signature.Length; i += 2 )
            {
                var byteChar = signature[ i ].ToString() + signature[ i + 1 ].ToString();
                if ( byteChar == "**" )
                    continue;

                byte byteVal = byte.Parse( byteChar, NumberStyles.HexNumber );
                if ( bytes[byteIndex++] != byteVal )
                    return false;
            }

            return true;
        }
    }
}
