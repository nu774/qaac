// Note that we have a separate platform_win32_impl.h to deal with the fact that windows.h defines a macro
// called FindAtom, which mp4v2 also defines.  In older versions of visual studio, this actually causes
// some pretty seriously issues with naming collisions and the defined macros (think infamous min/max macro
// of windows.h vs stdc++'s min/max template functions)
#include <windows.h>

///////////////////////////////////////////////////////////////////////////////

namespace mp4v2 { namespace platform { namespace win32 {

class Utf8ToFilename
{
    public:
    Utf8ToFilename( const string &utf8string );
    ~Utf8ToFilename( );

    bool                IsUTF16Valid( ) const;

    operator LPCWSTR( ) const { return _wideCharString; }
    operator LPWSTR( ) const { return _wideCharString; }

    private:
    Utf8ToFilename ( const Utf8ToFilename &src );
    Utf8ToFilename &operator= ( const Utf8ToFilename &src );
    
    wchar_t             *ConvertToUTF16 ( const string &utf8 );

    static int          ConvertToUTF16Buf ( const char      *utf8,
                                            wchar_t         *utf16_buf,
                                            size_t          num_bytes );
    static int          GetPrefixLen ( const string &utf8string );

    static int          IsAbsolute ( const string &utf8string );

    static int          IsPathSeparator ( char c );

    static int          IsUncPath ( const string &utf8string );

    static const UINT8  *Utf8DecodeChar (
        const UINT8     *utf8_char,
        size_t          num_bytes,
        wchar_t         *utf16,
        int             *invalid
        );

    static size_t       Utf8LenFromUcs4 ( UINT32 ucs4 );

    static UINT8        Utf8NumOctets ( UINT8 utf8_first_byte );

    /**
     * The UTF-8 encoding of the filename actually used
     */
    string      _utf8;

    /**
     * The UTF-16 encoding of the filename actually used
     */
    wchar_t*    _wideCharString;

    public:

    /**
     * Accessor for @p _utf8
     */
    const string&       utf8;
};

}}} // namespace mp4v2::platform::win32
