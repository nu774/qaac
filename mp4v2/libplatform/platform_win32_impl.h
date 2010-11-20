#ifndef MP4V2_PLATFORM_WIN32_IMPL_H
#define MP4V2_PLATFORM_WIN32_IMPL_H

#include <windows.h>

namespace mp4v2 { namespace platform { namespace win32 {

class Utf8ToWideChar
{
public:
    Utf8ToWideChar( string utf8string ) :
        _wideCharString( NULL ),
        _cchWideCharString( 0 )
    {
        // NOTE: The return values of MultiByteToWideChar will include room
        //  for the NUL character since we use -1 for the input length.
        _cchWideCharString = ::MultiByteToWideChar( CP_UTF8, 0, utf8string.c_str(), -1, NULL, 0 );
        if( _cchWideCharString > 0 )
        {
            _wideCharString = new WCHAR[_cchWideCharString];
            if( _wideCharString == NULL )
            {
                _cchWideCharString = 0;
            }
            else
            {
                ::MultiByteToWideChar( CP_UTF8, 0, utf8string.c_str(), -1, _wideCharString, _cchWideCharString );
                // Guarantee its NUL terminated.
                _wideCharString[_cchWideCharString-1] = L'\0';
            }
        }
    }
    ~Utf8ToWideChar( )
    {
        if( _wideCharString != NULL )
        {
            delete [] _wideCharString;
            _wideCharString = NULL;
        }
    }
    operator LPCWSTR( ) const { return _wideCharString; }
    operator LPWSTR( ) const { return _wideCharString; }
    int size( ) const { return _cchWideCharString; }

private:
    WCHAR* _wideCharString;
    int _cchWideCharString;
};

}}}

#endif
