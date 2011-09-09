///////////////////////////////////////////////////////////////////////////////
//
//  The contents of this file are subject to the Mozilla Public License
//  Version 1.1 (the "License"); you may not use this file except in
//  compliance with the License. You may obtain a copy of the License at
//  http://www.mozilla.org/MPL/
//
//  Software distributed under the License is distributed on an "AS IS"
//  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
//  License for the specific language governing rights and limitations
//  under the License.
// 
//  The Original Code is MP4v2.
// 
//  The Initial Developer of the Original Code is David Byron.
//  Portions created by David Byron are Copyright (C) 2010.
//  All Rights Reserved.
//
//  Contributors:
//      David Byron, dbyron@dbyron.com
//
///////////////////////////////////////////////////////////////////////////////

#include "src/impl.h"
#include "libplatform/impl.h" /* for platform_win32_impl.h which declares Utf8ToFilename */
#include <algorithm> /* for replace */
#include <windows.h>

namespace mp4v2 {
    using namespace impl;
}

/**
 * Set this to 1 to compile in extra debugging
 */
#define EXTRA_DEBUG 0

/**
 * @def LOG_PRINTF
 *
 * call log.printf if EXTRA_DEBUG is defined to 1.  Do
 * nothing otherwise
 */
#if EXTRA_DEBUG
#define LOG_PRINTF(X) log.printf X
#else
#define LOG_PRINTF(X)
#endif

/**
 * Section 2.13 "Special Characters and Noncharacters" of
 * _The Unicode Standard, Version 5.0_
 * (http://www.unicode.org/versions/Unicode5.0.0/bookmarks.html)
 * defines "The Replacement Character" U+FFFD as the
 * "general substitute character" that "can be substituted
 * for any 'unknown' character in another encoding that can
 * not be mapped in terms of known Unicode characters"
 *
 * See also section D.7 of 10646.
 */
#define REPLACEMENT_CHAR    0xFFFD

namespace mp4v2 { namespace platform { namespace win32 {

/**
 * A structure to store the number of characters required to
 * encode a particular UCS-4 character in UTF-8
 */
struct utf8_len_info
{
    /**
     * This structure applies to a number >= @p range_min.
     */
    UINT32      range_min;

    /**
     * This structure applies to a number <= @p range_max.
     */
    UINT32      range_max;

    /**
     * The number of characters required to encode a number
     * in [@p range_min,@p range_max] as UTF-8.
     */
    size_t      num_chars;
};

/**
 * A structure to store the number of characters required to
 * encode a particular UCS-4 character in UTF-8.  For now
 * we're using wide characters (which according to
 * http://msdn.microsoft.com/en-us/library/ms776414.aspx
 * means UTF-16 since Windows 2000) so we're only using up
 * to 4-byte UTF-8 sequences.  Parts of the range aren't
 * valid (e.g. [U+D800,U+DFFF] but that's handled elsewhere.
 */
static struct utf8_len_info s_len_info[] =
{
    { 0x00000000, 0x0000007F, 1 },
    { 0x00000080, 0x000007FF, 2 },
    { 0x00000800, 0x0000FFFF, 3 },
    { 0x00010000, 0x001FFFFF, 4 },
    { 0x00200000, 0x03FFFFFF, 5 },
    { 0x04000000, 0x7FFFFFFF, 6 }
};

/**
 * Utf8ToFilename constructor
 *
 * @param utf8string a UTF-8 encoded string that does not
 * begin with \\\?\\ nor \\\?\\UNC\\
 *
 * @see IsValidUTF16 to see whether the constructor
 * succeeded
 */
Utf8ToFilename::Utf8ToFilename( const string &utf8string )
    : _wideCharString( NULL )
      , utf8( _utf8 )
{
    // See
    // http://msdn.microsoft.com/en-us/library/aa365247%28v=vs.85%29.aspx
    // for notes about path lengths, prefixes, etc.  The
    // goal is to support the longest path possible.
    // Relative paths are limited to 260 characters but
    // absolute paths can be up to about 32767
    // characters if properly prefixed.

    // If utf8string is a relative path, convert it to
    // UTF-16 and be done.
    if (!IsAbsolute(utf8string))
    {
        _wideCharString = ConvertToUTF16(utf8string);
        return;
    }

    // Since the prefix has backslashes, convert any forward
    // slashes in utf8string to backslashes to keep Windows
    // happy
    const string *utf8ToUse = &utf8string;
    string forwardSlash;

    if (utf8string.find('/') != std::string::npos)
    {
        forwardSlash = utf8string;
        std::replace(forwardSlash.begin(),forwardSlash.end(),'/','\\');
        utf8ToUse = &forwardSlash;
    }
    ASSERT(utf8ToUse);
    ASSERT((*utf8ToUse).length() > 0);

    // utf8string is an absolute path.  It could be a
    // UNC path (\\host\path).  The prefix is different
    // for UNC paths than it is for non-UNC paths.
    string prefixedPath;

    if (IsUncPath(*utf8ToUse))
    {
        // utf8string begins with two backslashes, but
        // with a prefix we only need one so we can't
        // just prepend a prefix.
        prefixedPath = "\\\\?\\UNC" + (*utf8ToUse).substr(1);
    }
    else
    {
        prefixedPath = "\\\\?\\" + *utf8ToUse;
    }

    // Transform prefixedPath to UTF-16 so it's
    // appropriate for CreateFileW
    _wideCharString = ConvertToUTF16(prefixedPath);
}

Utf8ToFilename::~Utf8ToFilename( )
{
    if( _wideCharString != NULL )
    {
        free(_wideCharString);
        _wideCharString = NULL;
    }
}

/**
 * Convert a UTF-8 encoded string to a UTF-16 string
 *
 * @param utf8 the NUL-terminated UTF-8 string to decode
 *
 * @retval NULL error allocating memory for UTF-16 string
 *
 * @retval non-NULL NUL-terminated UTF-16 version of @p
 * utf8.  Invalid portions of UTF-8 are represented by a
 * replacement character U+FFFD.  The caller is
 * responsible for freeing this memory.
 */
wchar_t *
Utf8ToFilename::ConvertToUTF16 ( const string &utf8string )
{
    int         num_bytes;
    size_t      num_chars;
    wchar_t     *retval;

    ASSERT(sizeof(wchar_t) == 2);

    // Store the utf8 string in our member variable so it's
    // available
    _utf8 = utf8string;

    // We need to find out how many characters we're dealing
    // with so we know how much memory to allocate.  At the
    // same time, it's possible that the string we've been
    // given isn't valid UTF-8.  So, just use the length of
    // the string we've been given as the number of
    // characters to allocate.  The decoded string can't be
    // longer than this, even taking into account surrogate
    // pairs since they require 4 UTF-8 characters but only
    // two UTF-16 character elements.
    num_chars = utf8string.length();

    LOG_PRINTF((MP4_LOG_VERBOSE4,"%s: entry point (%d character string)",
                __FUNCTION__,num_chars));

    /*
    ** Allocate space for the decoded string.  Add one
    ** for the NUL terminator.
    */
    num_bytes = (num_chars + 1) * sizeof(wchar_t);
    retval = (wchar_t *)malloc(num_bytes);
    if (!retval)
    {
        log.errorf("%s: error allocating memory for %d byte(s)",__FUNCTION__,num_bytes);
        return NULL;
    }

    /*
    ** ConvertToUTF16Buf zeroes out the memory so don't
    ** do it here
    */

    // ConvertToUTF16Buf shouldn't fail if we allocated
    // enough memory for the entire string.  Check
    // anyway just to be safe.
    if (!ConvertToUTF16Buf(utf8string.c_str(),retval,num_bytes))
    {
        // But ASSERT so we can find the problem and fix
        // it.
        ASSERT(0);
        free(retval);
        retval = NULL;
        return NULL;
    }

    return retval;
}

/**
 * Convert a UTF-8 encoded string to a UTF-16 string in
 * a previously allocated buffer.
 *
 * @param utf8 the NUL-terminated UTF-8 string to decode
 *
 * @param utf16_buf the buffer in which to place the
 * UTF-16 version of @p utf8.  If there's enough space
 * to hold a NUL terminator, @p utf16_buf contains one.
 * If not, @p utf16_buf is not NUL terminated.
 *
 * @param num_bytes the number of bytes that @p
 * utf16_str points to
 *
 * @retval 0 error converting @p name to UTF-16,
 * including when @p utf8 requires more space to encode
 * in UTF-16 than indicated by @p num_bytes.  In that
 * case, @p utf16_buf contains the UTF-16 encoding of as
 * much of @p utf8 as possible.
 *
 * @retval 1 successfully converted @p name to @p UTF-16
 * in @p utf16_buf. wide character (UTF-16) version of
 * @p Invalid portions of UTF-8 are represented by a
 * replacement character U+FFFD.
 */
int
Utf8ToFilename::ConvertToUTF16Buf ( const char      *utf8,
                                    wchar_t         *utf16_buf,
                                    size_t          num_bytes )
{
    size_t      i;
    const UINT8 *next_char;
    size_t      num_chars;
    size_t      num_utf16_chars;
    size_t      num_input_bytes;
    const UINT8 *p;
    wchar_t     this_utf16[2];

    ASSERT(utf8);
    ASSERT(utf16_buf || (num_bytes == 0));
    ASSERT(sizeof(wchar_t) == 2);

    ASSERT(num_bytes % sizeof(wchar_t) == 0);

    LOG_PRINTF((MP4_LOG_VERBOSE4,"%s: converting \"%s\"",__FUNCTION__,utf8));

    num_chars = strlen(utf8);

    // If the input is NUL-terminated (which it better
    // be), the NUL-terminator is a valid input byte as
    // well
    num_input_bytes = num_chars + 1;

    // Make sure the buffer we've been given is long
    // enough.  We might need one UTF-16 character for
    // every UTF-8 character.  And one more for the NUL
    // terminator.
    //
    // Here, check that there's room for a NUL
    // terminator in the output string.  This makes it
    // safe to dereference p in the while loop below.
    // It's probably enough to check num_bytes == 0 here
    // but if we did that we'd have to change the error
    // message after the while loop to be less specific.
    // This way we give the caller more info about the
    // input string.
    if (num_bytes < sizeof(wchar_t))
    {
        log.errorf("%s: %u byte(s) is not enough to transform a %u byte UTF-8 string "
                   "to NUL-terminated UTF-16",__FUNCTION__,num_bytes,num_input_bytes);
        return 0;
    }

    ASSERT(num_bytes > 0);
    ASSERT(utf16_buf);
    memset(utf16_buf,0,num_bytes);

    // The number of UTF-16 characters we've got space for
    // in utf16_buf
    num_utf16_chars = num_bytes / sizeof(wchar_t);

    p = (const UINT8 *)utf8;
    i = 0;
    while (*p && (i < num_utf16_chars))
    {
        LOG_PRINTF((MP4_LOG_VERBOSE4,"%s: decoding first UTF-8 byte 0x%02X (UTF-16 "
                    "character %d of at most %d)",__FUNCTION__,*p,(i + 1),
                    num_utf16_chars));

        memset(this_utf16,0,sizeof(this_utf16));

        // This function decodes illegal bytes/sequences
        // with a replacement character and returns the
        // pointer to the next character to decode.  Pass
        // NULL since we don't care about detecting invalid
        // characters here.
        next_char = Utf8DecodeChar(p,num_input_bytes,this_utf16,NULL);

        // We've always got one character to assign
        utf16_buf[i++] = this_utf16[0];

        // If we're dealing with a surrogate pair,
        // assign the low half too
        if (this_utf16[1])
        {
            // We may not have any more room in the
            // UTF-16 buffer.  Check to make sure we
            // don't step on someone else's memory.  We
            // need to return failure here instead of
            // depending on our other logic to do it for
            // us.  We'll get out of the while loop with
            // no extra code, but if we're dealing with
            // the UTF-16 encoding of the last character
            // in the input string, there won't appear
            // to be anything wrong.
            if (i >= num_utf16_chars)
            {
                log.errorf("%s: out of space in %u  byte output string to store surrogate "
                           "pair low half (0x%04X)",__FUNCTION__,num_bytes,this_utf16[1]);
                return 0;
            }
             
            utf16_buf[i++] = this_utf16[1];
        }

        // Put this here to make it brutally clear that
        // the cast is safe
        ASSERT(next_char >= p);
        num_input_bytes -= (size_t)(next_char - p);
        p = next_char;
    }

    if (*p)
    {
        // Since num_input_bytes includes 1 for the
        // NUL-terminator, it's got to be bigger than
        // one here.
        ASSERT(num_input_bytes > 1);
        log.errorf("%s: %u byte(s) of input string remain(s) undecoded (%s): out of space in "
                   "%u byte output string",__FUNCTION__,(num_input_bytes - 1),p,num_bytes);
        return 0;
    }

    return 1;
}

/**
 * Accessor for the length of a prefix (i.e. \\\?\\ or
 * \\\?\\UNC\\) that begins a filename
 *
 * @param utf8string the UTF-8 encoded filename to
 * examine
 *
 * @return the length of the prefix of @p utf8string in
 * characters
 */
int
Utf8ToFilename::GetPrefixLen ( const string &utf8string )
{
    if (utf8string.find("\\\\?\\") == 0)
    {
        return strlen("\\\\?\\");
    }

    if (utf8string.find("\\\\?\\UNC\\") == 0)
    {
        return strlen("\\\\?\\UNC\\");
    }

    return 0;
}

/**
 * Determine if a path is absolute or not
 *
 * @param utf8string the UTF-8 encoded path to examine
 * that does not begin with \\\?\\ nor \\\?\\UNC\\
 *
 * @retval 0 @p utf8string is not an absolute path
 * @retval 1 @p utf8string is an absolute path
 */       
int
Utf8ToFilename::IsAbsolute ( const string &utf8string )
{
    // Assume utf8string doesn't already start with a
    // long filename prefix (i.e. \\?\ or \\?\UNC\)
    // since the logic here depends on that.
    ASSERT(GetPrefixLen(utf8string) == 0);

    // Is an empty string absolute or relative?  It's
    // not absolute since we can't tell what
    // drive/volume it's for so say it's relative.
    if (utf8string.length() == 0)
    {
        return 0;
    }
        
    // Here we're looking for:
    //  x:   drive relative
    //  x:\  absolute path
    if (utf8string[1] == ':')
    {
        // It starts with x:, but is it x:/ ?
        if ((utf8string.length() >= 2) && IsPathSeparator(utf8string[2]))
        {
            // Yup -- it's absolute
            return 1;
        }

        // Nope, not x:/, just x:something
        return 0;
    }

    // UNC paths are absolute paths too
    return IsUncPath(utf8string);
}

/**
 * Determine if a character is a valid path separator
 *
 * @param c the character to check
 *
 * @retval 0 @p c is not a valid path separator
 * @retval 1 @p c is a valid path separator
 */
int
Utf8ToFilename::IsPathSeparator ( char c )
{
    return ((c == '\\') || (c == '/'));
}

/**
 * Determine if a path is a UNC path
 *
 * @param utf8string the UTF-8 encoded path to examine
 * that does not begin with \\\?\\ nor \\\?\\UNC\\
 *
 * @retval 0 @p utf8string is not a UNC path
 * @retval 1 @p utf8string is a UNC path
 */       
int
Utf8ToFilename::IsUncPath ( const string &utf8string )
{
    const char  *host;
    int         num_slashes;
    const char  *p;

    // Assume utf8string doesn't already start with a
    // long filename prefix (i.e. \\?\ or \\?\UNC\)
    // since the logic here depends on that.
    ASSERT(GetPrefixLen(utf8string) == 0);

    // Is an empty string a UNC path?  No.
    if (utf8string.length() == 0)
    {
        return 0;
    }

    //  Recognize:
    //    //volume/path
    //    \\volume\path
    if (!IsPathSeparator(utf8string[0]))
    {
        // If it doesn't start with a path separator, it's
        // not a UNC path.
        return 0;
    }

    // The path starts with a slash, so it could be a UNC
    // path.  See if it starts with two slashes...Be careful
    // though, it might have more than 2 slashes.
    p = utf8string.c_str();
    num_slashes = 0;
    while (*p && IsPathSeparator(*p))
    {
        num_slashes++;
        p++;
    }

    // We found a slash at the beginning so we better have
    // at least one here
    ASSERT(num_slashes >= 1);
    if ((num_slashes > 2) || !(*p))
    {
        // If we've got more than two slashes or we've
        // run off the end of the string (///foo or
        // //)...who knows how the OS will handle it,
        // but it's not a UNC path.
        log.errorf("%s: don't understand path(%s)",__FUNCTION__,utf8string.c_str());
        return 0;
    }

    // If we've only got one slash, it looks like a
    // drive relative path.  If it's something like
    // /foo//bar it's not clear how the OS handles it,
    // but that's someone else's problem.  It's not a
    // UNC path.
    if (num_slashes == 1)
    {
        return 0;
    }
    
    // If we're here, we've got two slashes followed by
    // a non-slash.  Something like //foo.  To be a
    // proper UNC path, we need to see a hostname
    // (e.g. foo), and then another slash.  If not, it's
    // not a UNC path.
    ASSERT(num_slashes == 2);

    // Tempting to use STRTOK_R here, but that modifies
    // the original string.  Instead of making a copy,
    // search manually.
    host = p;
    while (*p && !IsPathSeparator(*p))
    {
        p++;
    }

    // We checked for separators above, so we better
    // have moved on at least a bit
    ASSERT(host != p);
    if (!(*p))
    {
        // We ran off the end of the string without finding
        // another separator.  So, we've got something like
        // 
        //  //foobar
        // 
        // which isn't a UNC path.
        log.warningf("%s: incomplete UNC path: host only(%s)",__FUNCTION__,
                     utf8string.c_str());
        return 0;
    }

    // p points to a separator, so...we've got one of:
    //  //host//
    //  //host//blah
    //  //host/bar
    //
    // Of these, only the last is a proper UNC path.  See
    // what we've got after p.
    num_slashes = 0;
    while (*p && IsPathSeparator(*p))
    {
        num_slashes++;
        p++;
    }

    // We better have at least one slash or our logic is
    // broken
    ASSERT(num_slashes >= 1);
    if (!(*p))
    {
        // //host// (or maybe //host///), but no path
        // part after the host
        log.warningf("%s: incomplete UNC path: no path after host(%s)",
                     __FUNCTION__,utf8string.c_str());
        return 0;
    }

    if (num_slashes > 1)
    {
        // Another busted case //host//blah or
        // //host///blah, etc.
        log.warningf("%s: invalid UNC path: too many slashes after host(%s)",
                     __FUNCTION__,utf8string.c_str());
        return 0;
    }
    
    // If we're here it means num_slashes is exactly 1
    // so we've got //host/something so we're calling
    // that a UNC path.
    return 1;
}

/**
 * Accessor for whether the UTF-16 encoded string is valid
 *
 * @retval false the UTF-16 encoded string is not valid
 * @retval true the UTF-16 encoded string is valid
 */
bool
Utf8ToFilename::IsUTF16Valid( ) const
{
    return (_wideCharString ? true : false);
}

/**
 * Decode one UTF-8 encoded character into a UTF-16
 * character.  The trouble here is that UTF-16 is really a
 * variable length encoding to handle surrogate pairs
 * (0xD800 --> 0xDFFF).  This way UTF-16 can handle more
 * than 2^16 characters.  So we need to be careful.  UCS-2
 * is a fixed width (16-bit) encoding that we could use, but
 * then we can only handle 2^16 characters (the BMP).  To
 * handle all 2^21 characters, we need UTF-16.
 *
 * What does Windows really use?  UTF-16.  See
 * http://unicode.org/iuc/iuc17/b2/slides.ppt for a
 * discussion.
 * http://discuss.fogcreek.com/joelonsoftware5/default.asp?cmd=show&ixPost=168543
 * also has some info.
 *
 * @param utf8_char the UTF-8 character to decode, possibly
 * occupying multiple bytes, not necessarily NUL terminated
 *
 * @param num_bytes the number of bytes that @p utf8_char
 * points to (must be > 0)
 *
 * @param utf16 populated with the UTF-16 equivalent of @p
 * utf8_char.  Note that this must point to at least 2
 * wchar_t's of memory so there's room to hold a surrogate
 * pair.
 *
 * @param invalid populated with 1 if @p utf8_char doesn't
 * point to a valid UTF-8 encoded character, 0 if @p
 * utf8_char is valid.
 *
 * @return the next byte to examine for subsequent decoding
 * (some number of bytes after @p utf8_char).  This may not
 * be valid to dereference depending on the value of @p
 * num_bytes.
 */
const UINT8 *
Utf8ToFilename::Utf8DecodeChar ( const UINT8    *utf8_char,
                                 size_t         num_bytes,
                                 wchar_t        *utf16,
                                 int            *invalid )

{
    wchar_t     high_half;
    int         i;
    UINT8       len;
    wchar_t     low_half;
    UINT8       mask;
    const UINT8 *p;
    UINT32      ucs4;
    int         valid_len;

    ASSERT(utf8_char);
    ASSERT(num_bytes > 0);
    ASSERT(utf16);

    LOG_PRINTF((MP4_LOG_VERBOSE4,"%s: decoding UTF-8 string at address 0x%p",
                __FUNCTION__,utf8_char));

    /*
    ** Assume utf8_char is invalid until we learn otherwise
    */
    if (invalid)
    {
        *invalid = 1;
    }

    /*
    ** Traverse the UTF-8 encoding and figure out what we've
    ** got.
    */
    p = (const UINT8 *)(utf8_char);

    /*
    ** This is the number of bytes we expect based on the
    ** first octet.  If subsequent bytes are NUL or invalid,
    ** then it may not the same as the actual len.
    */
    len = Utf8NumOctets(*p);
    if (len == 0)
    {
        log.errorf("%s: 0x%02X is not a valid first byte of a UTF-8 encoded character",__FUNCTION__,*p);

        /*
        ** Use the replacement character and advance past
        ** the invalid byte
        */
        *utf16 = REPLACEMENT_CHAR;
        return p + 1;
    }

    /*
    ** Handle one byte encodings in a special case.  See
    ** below for an explanation of how we mask successive
    ** bytes of an encoding to see why.  We're depending on
    ** the validation in Utf8NumOctets here to make this OK.
    */
    if (len == 1)
    {
        /*
        ** There's no intermediate UCS-4 step here.  We go
        ** straight to UTF-16 since they're the same.
        */
        LOG_PRINTF((MP4_LOG_VERBOSE4,"%s: one byte UTF-16 encoding: 0x%02X",
                    __FUNCTION__,*p));
        *utf16 = *p;
        if (invalid)
        {
            *invalid = 0;
        }
        return p + 1;
    }

    /*
    ** Make sure we've got enough bytes in our input string
    ** to form a valid UTF-8 character
    */
    if (len > num_bytes)
    {
        log.errorf("%s: first byte 0x%02X indicates a %d byte "
                   "UTF-8 character, but we only have %u valid byte(s)",
                   __FUNCTION__,*p,len,num_bytes);
        *utf16 = REPLACEMENT_CHAR;
        return p + 1;
    }

    /*
    ** Traverse the bytes that should be part of this UTF-8
    ** encoded character and make sure we don't have an
    ** overlength encoding, and make sure that each
    ** character is valid.
    */

    /*
    ** As we traverse each character, we mask off the
    ** appropriate number of bits and include them in the
    ** overall result.
    **
    ** 1 byte encoding [U+00000000,U+0000007F]: 7 bits (7 bits total) (handled above)
    ** 2 byte encoding [U+00000080,U+000007FF]: 5 bits, 6 bits (11 bits total)
    ** 3 byte encoding [U+00000800,U+0000FFFF]: 4 bits, 6 bits, 6 bits (16 bits total)
    ** 4 byte encoding [U+00010000,U+001FFFFF]: 3 bits, 6 bits, 6 bits, 6 bits (21 bits total)
    ** 5 byte encoding [U+00200000,U+03FFFFFF]: 2 bits, 6 bits, 6 bits, 6 bits, 6 bits (26 bits total)
    ** 6 byte encoding [U+04000000,U+7FFFFFFF]: 1 bit, 6 bits, 6 bits, 6 bits, 6 bits, 6 bits (31 bits total)
    **
    ** So, mask the initial byte appropriately, then take
    ** the bottom 6 bits from the remaining bytes.  To be
    ** brutally explicit, the first byte mask is:
    **
    ** 1 byte encoding: 0x7F (or 0x80 - 1) (or (1 << 7) - 1)
    ** 2 byte encoding: 0x1F (or 0x20 - 1) (or (1 << 5) - 1)
    ** 3 byte encoding: 0x0F (or 0x10 - 1) (or (1 << 4) - 1)
    ** 4 byte encoding: 0x07 (or 0x08 - 1) (or (1 << 3) - 1)
    ** 5 byte encoding: 0x03 (or 0x04 - 1) (or (1 << 2) - 1)
    ** 6 byte encoding: 0x01 (or 0x02 - 1) (or (1 << 1) - 1)
    **    
    ** So, the one byte encoding is a special case (again,
    ** handled above), but for the other lengths, the mask
    ** is (1 << (7 - len)) - 1.
    */

    /*
    ** Handle the first byte of multi-byte encodings since
    ** it's special
    */
    ASSERT(len > 1);
    ASSERT(len <= 6);
    mask = (1 << (7 - len)) - 1;
    ucs4 = *p & mask;
    p++;

    /*
    ** Now handle the remaining bytes
    */
    for (i = 1;(i < len);i++)
    {
        if ((*p < 0x80) || (*p > 0xBF))
        {
            log.errorf("%s: 0x%02X is not a valid continuation character in a UTF-8 encoding",
                       __FUNCTION__,*p);

            /*
            ** Use the replacement character and return the
            ** next byte after the invalid sequence as the
            ** place for subsequent decoding operations.  In
            ** this case the invalid continuation character
            ** could be the beginning of the next valid
            ** sequence, so return that.
            */
            *utf16 = REPLACEMENT_CHAR;
            return p;
        }
            
        /*
        ** For the remainder of the bytes, shift over what
        ** we've already got by 6 bits, and then OR in the
        ** bottom 6 bits of the current byte.
        */
        ucs4 = (ucs4 << 6) | (*p & 0x3F);
        p++;
    }

    /*
    ** p is now pointing to the beginning of the next UTF-8
    ** sequence to decode...
    */

    /*
    ** Finally, detect overlong encodings.  For example, a
    ** line feed (U+000A) should be encoded as 0x0A
    ** (0b00001010) but could in theory be encoded in UTF-8
    ** as 0xC0 0x8A (0b10001010).
    **
    ** Another example is the forward slash (/) (U+002F).
    ** It should be encoded as 0x2F, but could in theory be
    ** encoded in UTF-8 as 0xC0 0xAF (which we'll catch
    ** because 0xC0 is an invalid first byte of a UTF-8
    ** encoding), but could also be 0xE0 0x80 0xAF.
    **
    ** I can't see any reasonable way to do this other than
    ** to check the decoded character against its expected
    ** length
    */
    valid_len = Utf8LenFromUcs4(ucs4);
    if (valid_len == 0)
    {
        /*
        ** This should never happen
        */
        log.errorf("%s: decoded a character that we can't encode again (0x%08X)",__FUNCTION__,ucs4);
        ASSERT(0);

        /*
        ** If it does, use the replacement character
        */
        *utf16 = REPLACEMENT_CHAR;
        return p;
    }

    if (len != valid_len)
    {
        ASSERT(len > valid_len);
        log.errorf("%s: overlong encoding(%s)...should be %d byte(s), not %d",__FUNCTION__,
                   utf8_char,valid_len,len);
        *utf16 = REPLACEMENT_CHAR;
        return p;
    }

    /*
    ** UTF-16 can only hold 21 bits.  As of now (21-dec-10),
    ** there's no Unicode code point bigger than 2^21.  To
    ** be safe, check...
    */
    if (ucs4 > 0x0010FFFF)
    {
        log.errorf("%s: code point 0x%08X is too big",__FUNCTION__,ucs4);
        *utf16 = REPLACEMENT_CHAR;
        return p;
    }

    /*
    ** Check to make sure we're not working with a "code
    ** point" that is in the range used to indicate
    ** surrogate pairs.
    */
    if ((ucs4 >= 0x0000D800) && (ucs4 <= 0x0000DFFF))
    {
        log.errorf("%s: code point 0x%08X is in the range used to indicate surrogate pairs",
                   __FUNCTION__,ucs4);
        *utf16 = REPLACEMENT_CHAR;
        return p;
    }

    /*
    ** To (try to) be complete, check for a couple more
    ** invalid code points
    */
    if ((ucs4 == 0x0000FFFF) || (ucs4 == 0x0000FFFE))
    {
        log.errorf("%s: invalid code point (0x%08X)",__FUNCTION__,ucs4);
        *utf16 = REPLACEMENT_CHAR;
        return p;
    }

    /*
    ** Finally, convert from UCS-4 to UTF-16.  This may be a
    ** straightforward assignment, but we have to deal with
    ** surrogate pairs
    */
    if (ucs4 <= 0x0000FFFF)
    {
        *utf16 = ucs4 & 0xFFFF;
        LOG_PRINTF((MP4_LOG_VERBOSE4,"%s: UTF-16 encoding of 0x%08X is 0x%04X",
                    __FUNCTION__,ucs4,*utf16));
        if (invalid)
        {
            *invalid = 0;
        }
        return p;
    }

    /*
    ** Transform UCS-4 into a UTF-16 surrogate pair
    */

    /*
    ** Grab bits [10,20] (where bit 0 is the LSB) and shift
    ** them down
    */
    high_half = 0xD800 + ((ucs4 - 0x00010000) >> 10);

    /*
    ** And the bottom 10 bits [0,9]
    */
    low_half = 0xDC00 + (ucs4 & 0x03FF);

    utf16[0] = high_half;
    utf16[1] = low_half;

    LOG_PRINTF((MP4_LOG_VERBOSE4,"%s: UTF-16 encoding of 0x%08X is 0x%04X:0x%04X",
                __FUNCTION__,ucs4,utf16[0],utf16[1]));

    if (invalid)
    {
        *invalid = 0;
    }

    return p;
}

/**
 * Determine the number of bytes required to hold the UTF-8
 * encoding of a UCS-4 code point
 *
 * @param ucs4 the code point
 *
 * @param use_syslog 1 to use syslog, 0 otherwise
 *
 * @retval 0 @p ucs4 is not a valid code point
 *
 * @retval [1,6] the number of bytes required to hold the
 * UTF-8 encoding of @p ucs4
 */
size_t
Utf8ToFilename::Utf8LenFromUcs4 ( UINT32 ucs4 )
{
    size_t      table_idx;

    LOG_PRINTF((MP4_LOG_VERBOSE4,"%s: processing UCS-4 code point 0x%08X",
                __FUNCTION__,ucs4));

    for (table_idx = 0;(table_idx < (sizeof(s_len_info) /
                                     sizeof(struct utf8_len_info)));
         table_idx++)
    {
        if ((s_len_info[table_idx].range_min <= ucs4) &&
            (ucs4 <= s_len_info[table_idx].range_max))
        {
            return s_len_info[table_idx].num_chars;
        }
    }

    log.errorf("%s: 0x%08X is an invalid code point",__FUNCTION__,ucs4);

    return 0;
}

/**
 * Determine the number of octets that a UTF-8 encoded
 * character should occupy based on its first byte
 *
 * @param utf8_first_byte the byte to examine
 *
 * @retval 0 @p utf8_first_byte is not a valid first byte of
 * a UTF-8 encoded character
 *
 * @retval [1,6] the number of octets that @p
 * utf8_first_byte should occupy
 */
UINT8
Utf8ToFilename::Utf8NumOctets ( UINT8 utf8_first_byte )
{
    /**
     * Here's a mapping from the first byte of a UTF-8
     * character to the number of bytes it should contain
     * based on information from
     * http://www.unicode.org/versions/corrigendum1.html as
     * well as
     * http://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt
     *
     * [0x00,0x7F]: 1       (0-127)     (128 possible values)
     * [0x80,0xBF]: invalid (128-191)   (64 possible values)
     * [0xC0,0xDF]: 2       (192-223)   (32 possible values) (see below)
     * [0xE0,0xEF]: 3       (224-239)   (16 possible values)
     * [0xF0,0xF7]: 4       (240 - 247) (8 possible values)
     * [0xF8,0xFB]: 5       (248 - 251) (4 possible values)
     * [0xFC,0xFD]: 6       (252 - 253) (2 possible values)
     * [0xFE,0xFF]: invalid (254 - 255) (2 possible values)
     *
     * There's some gray area about 0xC0 and 0xC1.  It's
     * clear they are invalid first bytes but the question
     * is how to handle it.  If I reject them here, they'll
     * get replaced with the REPLACEMENT character.  But, if
     * I allow them here, it's likely that both this byte
     * and the subsequent one will get replaced with only
     * one replacement character.  This is what
     * http://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt
     * assumes in sections 4.1.1, 4.2.1 and 4.3.1.
     */
    if (utf8_first_byte <= 0x7F)
    {
        return 1;
    }

    if ((utf8_first_byte >= 0x80) && (utf8_first_byte <= 0xBF))
    {
        return 0;
    }

    if ((utf8_first_byte >= 0xC0) && (utf8_first_byte <= 0xDF))
    {
        return 2;
    }

    if ((utf8_first_byte >= 0xE0) && (utf8_first_byte <= 0xEF))
    {
        return 3;
    }

    if ((utf8_first_byte >= 0xF0) && (utf8_first_byte <= 0xF7))
    {
        return 4;
    }

    if ((utf8_first_byte >= 0xF8) && (utf8_first_byte <= 0xFB))
    {
        return 5;
    }

    if ((utf8_first_byte >= 0xFC) && (utf8_first_byte <= 0xFD))
    {
        return 6;
    }

    ASSERT((utf8_first_byte == 0xFE) || (utf8_first_byte == 0xFF));
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

}}} // namespace mp4v2::platform::win32
