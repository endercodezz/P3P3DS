using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using AtlusFileSystemLibrary.Common.IO;

namespace AtlusFileSystemLibrary.Compressions
{
    public class LBCompression : ICompression
    {
        private Stream mStream;
        private EndianBinaryReader mReader;
        private EndianBinaryWriter mWriter;
        private List<BytePattern> mPatterns;

        public bool CanCompress => true;

        public bool CanDecompress => true;

        public Stream Compress( Stream input )
        {
            mStream = new MemoryStream();

            var inputStream = input;
            if ( input is FileStream )
            {
                // Copy to memory
                inputStream = new MemoryStream();
                input.CopyTo( inputStream );
                inputStream.Position = 0;
            }

            mReader = new EndianBinaryReader( inputStream, Encoding.Default, true, Endianness.LittleEndian );
            mWriter = new EndianBinaryWriter( mStream, Encoding.Default, true, Endianness.LittleEndian );
            mPatterns = new List< BytePattern >();

            RunEncodeLoop();

            mStream.Position = 0;
            return mStream;
        }

        private void RunEncodeLoop()
        {
            while ( mReader.Position < mReader.BaseStreamLength )
            {
                if ( TryEncodeRepeatBytePattern( false ) )
                    continue;

                if ( TryEncodeZeroInterleavePattern( false ) )
                    continue;

                EncodeCopyBytes();
            }
        }

        private void WriteOp( Opcode opcode, int count )
        {
            var opcodePart = ( ( ( ( byte ) opcode ) & 0xE ) << 4 );
            if ( count <= 0x1F )
            {
                mWriter.Write( ( byte )( opcodePart | count ) );
            }
            else
            {
                mWriter.Write( ( byte ) opcodePart );
                mWriter.Write( ( short ) count );
            }
        }

        private bool TryEncodeRepeatBytePattern( bool isPeeking )
        {
            bool hasEncoded = false;

            for ( byte i = 0; i < byte.MaxValue; ++i )
            {
                if ( TryEncodeRepeatBytePattern( i, isPeeking ) )
                {
                    hasEncoded = true;
                    break;
                }
            }

            return hasEncoded;
        }

        private bool TryEncodeRepeatBytePattern( byte pattern, bool isPeeking )
        {
            var startOffset = mReader.Position;
            var repeatCount = 0;

            while ( mReader.Position + 1 <= mReader.BaseStreamLength )
            {
                if ( mReader.ReadByte() == pattern )
                {
                    ++repeatCount;
                }
                else
                {
                    mReader.SeekCurrent( -1 );
                    break;
                }
            }              

            if ( repeatCount < ( pattern == 0 ? 2 : 3 ) )
            {
                mReader.SeekBegin( startOffset );
                return false;
            }

            if ( !isPeeking )
            {
                if ( pattern == 0 )
                {
                    WriteOp( Opcode.RepeatZero, repeatCount );
                }
                else
                {
                    WriteOp( Opcode.RepeatByte, repeatCount );
                    mWriter.Write( pattern );
                }
            }
            else
            {
                mReader.SeekBegin( startOffset );
            }

            return true;
        }

        private bool TryEncodeZeroInterleavePattern( bool isPeeking )
        {
            var startOffset = mReader.Position;
            var firstBytes = new List< byte >();
            
            while ( mReader.Position + 2 <= mReader.BaseStreamLength )
            {
                var firstByte = mReader.ReadByte();
                if ( firstByte == 0 ) // prevent zero interleaving zeroes
                {
                    mReader.SeekCurrent( -1 );
                    break;
                }

                var secondByte = mReader.ReadByte();
                if ( secondByte != 0 )
                {
                    mReader.SeekCurrent( -2 );
                    break;
                }

                firstBytes.Add( firstByte );
            }

            if ( firstBytes.Count == 0 )
            {
                mReader.SeekBegin( startOffset );
                return false;
            }

            if ( !isPeeking )
            {
                WriteOp( Opcode.CopyBytesZeroInterleaved, firstBytes.Count );
                foreach ( var firstByte in firstBytes )
                    mWriter.Write( firstByte );
            }
            else
            {
                mReader.SeekBegin( startOffset );
            }

            return true;
        }

        private void EncodeCopyBytes()
        {
            var startOffset = mReader.Position;
            var bytesToCopy = 0;

            while ( mReader.Position < mReader.BaseStreamLength )
            {
                // Try to figure out how many bytes we need to copy
                // By 'peeking' to see when we can encode a pattern

                if ( TryEncodeRepeatBytePattern( true ) )
                    break;

                if ( TryEncodeZeroInterleavePattern( true ) )
                    break;

                ++bytesToCopy;
                mReader.Seek( 1, SeekOrigin.Current );
            }

            mReader.SeekBegin( startOffset );

            while ( bytesToCopy > 0 )
            {
                var actualBytesToCopy = Math.Min( bytesToCopy, ushort.MaxValue );

                var bytes = new List< byte >();
                for ( int i = 0; i < actualBytesToCopy; ++i )
                    bytes.Add( mReader.ReadByte() );

                if ( actualBytesToCopy >= 4 )
                {
                    // Is this a pattern we've already written before?
                    var pattern = mPatterns.SingleOrDefault( x => x.Pattern.SequenceEqual( bytes ) );
                    if ( pattern != null )
                    {
                        // We've written this sequence of bytes before, so set up a copy from offset op
                        var relativeOffset = ( startOffset - pattern.Offset );

                        if ( relativeOffset <= byte.MaxValue )
                        {
                            WriteOp( Opcode.CopyBytesFromOffset, actualBytesToCopy );
                            mWriter.Write( ( byte ) relativeOffset );
                            bytesToCopy -= actualBytesToCopy;
                            continue;
                        }
                        else if ( relativeOffset <= ushort.MaxValue )
                        {
                            WriteOp( Opcode.CopyBytesFromLargeOffset, actualBytesToCopy );
                            mWriter.Write( ( short )relativeOffset );
                            bytesToCopy -= actualBytesToCopy;
                            continue;
                        }

                        // Fall through
                    }
                    else
                    {
                        // Add new pattern
                        mPatterns.Add( new BytePattern() { Pattern = bytes, Offset = startOffset } );
                    }
                }

                // Write bytes literally
                WriteOp( Opcode.CopyBytes, actualBytesToCopy );
                foreach ( var b in bytes )
                    mWriter.Write( b );

                bytesToCopy -= actualBytesToCopy;
            }
        }

        public Stream Decompress( Stream input, int decompressedSize = -1 )
        {
            // Based off of firefly's 'unlb'
            var decompressed = new List<byte>( decompressedSize );

            using ( var reader = new EndianBinaryReader( input, Encoding.Default, true, Endianness.LittleEndian ) )
            {
                while ( decompressed.Count < decompressedSize )
                {
                    var op = reader.ReadByte();

                    var count = op & 0x1F;
                    if ( count == 0 )
                        count = reader.ReadUInt16();

                    var opcode = ( Opcode )( ( op >> 4 ) & 0xE );
                    switch ( opcode )
                    {
                        case Opcode.CopyBytes:
                            for ( int i = 0; i < count; i++ )
                                decompressed.Add( reader.ReadByte() );
                            break;
                        case Opcode.RepeatZero:
                            for ( int i = 0; i < count; i++ )
                                decompressed.Add( 0 );
                            break;
                        case Opcode.RepeatByte:
                            {
                                var b = reader.ReadByte();
                                for ( int i = 0; i < count; i++ )
                                    decompressed.Add( b );
                            }
                            break;
                        case Opcode.CopyBytesFromOffset:
                            {
                                var offset = reader.ReadByte();
                                for ( int i = 0; i < count; i++ )
                                    decompressed.Add( decompressed[( decompressed.Count - 1 ) + -offset + 1] );
                            }
                            break;
                        case Opcode.CopyBytesFromLargeOffset:
                            {
                                var offset = reader.ReadUInt16();
                                for ( int i = 0; i < count; i++ )
                                    decompressed.Add( decompressed[( decompressed.Count - 1 ) + -offset + 1] );
                            }
                            break;
                        case Opcode.CopyBytesZeroInterleaved:
                            for ( int i = 0; i < count; i++ )
                            {
                                decompressed.Add( reader.ReadByte() );
                                decompressed.Add( 0 );
                            }
                            break;

                        default:
                            throw new InvalidDataException();
                    }
                }
            }

            return new MemoryStream( decompressed.ToArray() );
        }

        private enum Opcode
        {
            CopyBytes = 0x00,
            RepeatZero = 0x02,
            RepeatByte = 0x04,
            CopyBytesFromOffset = 0x06,
            CopyBytesFromLargeOffset = 0x08,
            CopyBytesZeroInterleaved = 0x0A,
        }

        private class BytePattern
        {
            public List<byte> Pattern { get; set; }

            public long Offset { get; set; }
        }
    }
}
