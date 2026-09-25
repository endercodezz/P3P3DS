using System.Collections.Generic;

namespace AtlusFileSystemLibrary.FileSystems.DDS3
{
    public class StringComparer : IComparer<string>
    {
        public static StringComparer Instance { get; } = new StringComparer();

        // compare__Ct12basic_string3ZcZt18string_char_traits1ZcZt24__default_alloc_template2b1i0RCt12basic_string3ZcZt18string_char_traits1ZcZt24__default_alloc_template2b1i0UiUi( x, y, 0, 0xFFFFFFFF )
        public int Compare( string x, string y )
        {
            if ( x == null || y == null )
                return 0;

            var result = 0;
            var isLessThan = 0;
            var areEqual = true;

            var xIndex = 0;
            var yIndex = 0;

            int charCount = x.Length < y.Length ? x.Length : y.Length;

            do
            {
                if ( charCount == 0 )
                    break;

                char xc = x[ xIndex++ ];
                var yc = y[ yIndex++ ];

                isLessThan = xc < yc ? 1 : 0;
                areEqual = xc == yc;
                --charCount;
            }
            while ( areEqual );

            if ( !areEqual )
            {
                result = -isLessThan;
                result |= 1;
            }

            if ( result == 0 )
            {
                result = x.Length - y.Length;
            }

            return result;
        }
    }
}