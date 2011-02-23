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
//  Portions created by David Byron are Copyright (C) 2009, 2010, 2011.
//  All Rights Reserved.
//
//  Contributors:
//      David Byron, dbyron@dbyron.com
//
///////////////////////////////////////////////////////////////////////////////

#include <iomanip>
#include <iostream>
#include "src/impl.h"

namespace mp4v2 { namespace impl {

MP4LogCallback Log::_cb_func = NULL;

// There's no mechanism to set the log level at runtime at
// the moment so construct this so it only logs important
// stuff.
Log log(MP4_LOG_WARNING);

///////////////////////////////////////////////////////////////////////////////

/**
 * Log class constructor
 */
Log::Log( MP4LogLevel verbosity_ /* = MP4_LOG_NONE */ )
    : _verbosity ( verbosity_ )
    , verbosity  ( _verbosity )
{
}

///////////////////////////////////////////////////////////////////////////////

/**
 * Log class destructor
 */
Log::~Log()
{
}

///////////////////////////////////////////////////////////////////////////////

/**
 * Mutator for the callback function
 *
 * @param value the function to call
 */
void
Log::setLogCallback( MP4LogCallback value )
{
    Log::_cb_func = value;
}

///////////////////////////////////////////////////////////////////////////////

/**
 * Mutator for the verbosity
 *
 * @param value the verbosity to use
 */
void
Log::setVerbosity( MP4LogLevel value )
{
    _verbosity = value;
}

///////////////////////////////////////////////////////////////////////////////

/**
 * Log an error message
 *
 * @param format the format string to use to process the
 * remaining arguments.  @p format should not contain a
 * newline.
 */
void
Log::errorf( const char* format,
             ... )
{
    va_list     ap;

    va_start(ap,format);
    this->vprintf(MP4_LOG_ERROR,format,ap);
    va_end(ap);
}

///////////////////////////////////////////////////////////////////////////////

/**
 * Log a warning message
 *
 * @param format the format string to use to process the
 * remaining arguments.  @p format should not contain a
 * newline.
 */
void
Log::warningf( const char* format,
               ... )
{
    va_list     ap;

    va_start(ap,format);
    this->vprintf(MP4_LOG_WARNING,format,ap);
    va_end(ap);
}

///////////////////////////////////////////////////////////////////////////////

/**
 * Log an info message
 *
 * @param format the format string to use to process the
 * remaining arguments.  @p format should not contain a
 * newline.
 */
void
Log::infof( const char* format,
            ... )
{
    va_list     ap;

    va_start(ap,format);
    this->vprintf(MP4_LOG_INFO,format,ap);
    va_end(ap);
}

/**
 * Log a verbose1 message
 *
 * @param format the format string to use to process the
 * remaining arguments.  @p format should not contain a
 * newline.
 */
void
Log::verbose1f( const char* format,
                ... )
{
    va_list     ap;

    va_start(ap,format);
    this->vprintf(MP4_LOG_VERBOSE1,format,ap);
    va_end(ap);
}

///////////////////////////////////////////////////////////////////////////////

/**
 * Log a verbose2 message
 *
 * @param format the format string to use to process the
 * remaining arguments.  @p format should not contain a
 * newline.
 */
void
Log::verbose2f( const char* format,
                ... )
{
    va_list     ap;

    va_start(ap,format);
    this->vprintf(MP4_LOG_VERBOSE2,format,ap);
    va_end(ap);
}

///////////////////////////////////////////////////////////////////////////////

/**
 * Log a verbose3 message
 *
 * @param format the format string to use to process the
 * remaining arguments.  @p format should not contain a
 * newline.
 */
void
Log::verbose3f( const char* format,
                ... )
{
    va_list     ap;

    va_start(ap,format);
    this->vprintf(MP4_LOG_VERBOSE3,format,ap);
    va_end(ap);
}

///////////////////////////////////////////////////////////////////////////////

/**
 * Log a verbose4 message
 *
 * @param format the format string to use to process the
 * remaining arguments.  @p format should not contain a
 * newline.
 */
void
Log::verbose4f( const char* format,
                ... )
{
    va_list     ap;

    va_start(ap,format);
    this->vprintf(MP4_LOG_VERBOSE4,format,ap);
    va_end(ap);
}

///////////////////////////////////////////////////////////////////////////////

/**
 * Dump info to the console or a callback
 *
 * @param indent the number of spaces to indent the info
 *
 * @param verbosity the level of detail the message contains
 *
 * @param format the format string to use to process the
 * remaining arguments.  @p format should not contain a
 * newline.
 */
void
Log::dump ( uint8_t       indent,
            MP4LogLevel   verbosity_,
            const char*   format, ... )
{
    va_list     ap;

    va_start(ap,format);
    this->vdump(indent,verbosity,format,ap);
    va_end(ap);
}

///////////////////////////////////////////////////////////////////////////////

/**
 * Dump info if it has appropriate verbosity, either to
 * standard out (with a newline appended to @p format) or to
 * the callback function (with no newline appended).
 *
 * @param indent the number of spaces to indent the info
 *
 * @param verbosity the level of detail the message contains
 *
 * @param format the format string to use to process @p ap.
 * @p format should not contain a newline.
 *
 * @param ap varargs to build the message
 */
void
Log::vdump( uint8_t     indent,
            MP4LogLevel verbosity_,
            const char* format,
            va_list     ap )
{
    // Make sure nothing gets logged with MP4_LOG_NONE.
    // That way people who ask for nothing to get logged
    // won't get anything logged.
    ASSERT(verbosity_ != MP4_LOG_NONE);
    ASSERT(format);
    ASSERT(format[0] != '\0');

    if (verbosity_ > this->_verbosity)
    {
        // We're not set verbose enough to log this
        return;
    }

    if (Log::_cb_func)
    {
        ostringstream   new_format;

        if (indent > 0)
        {
            string      indent_str(indent,' ');
            // new_format << setw(indent) << setfill(' ') << "" << setw(0);
            // new_format << format;
            new_format << indent_str << format;
            Log::_cb_func(verbosity_,new_format.str().c_str(),ap);
            return;
        }

        Log::_cb_func(verbosity_,format,ap);
        return;
    }

    // No callback set so log to standard out.  
    if (indent > 0)
    {
        ::fprintf(stdout,"%*c",indent,' ');
    }
    ::vfprintf(stdout,format,ap);
    ::fprintf(stdout,"\n");
}

///////////////////////////////////////////////////////////////////////////////

/**
 * Log a message
 *
 * @param verbosity the level of detail the message contains
 *
 * @param format the format string to use to process the
 * remaining arguments.  @p format should not contain a
 * newline.
 */
void
Log::printf( MP4LogLevel        verbosity,
             const char*        format,
             ... )
{
    va_list     ap;

    va_start(ap,format);
    this->vprintf(verbosity,format,ap);
    va_end(ap);
}

///////////////////////////////////////////////////////////////////////////////

/**
 * Log a message if it has appropriate verbosity, either to
 * standard out (with a newline appended to @p format) or to
 * the callback function (with no newline appended).
 *
 * @param verbosity the level of detail the message contains
 *
 * @param format the format string to use to process @p ap.
 * @p format should not contain a newline.
 *
 * @param ap varargs to build the message
 */
void
Log::vprintf( MP4LogLevel       verbosity_,
              const char*       format,
              va_list           ap )
{
    // Make sure nothing gets logged with MP4_LOG_NONE.
    // That way people who ask for nothing to get logged
    // won't get anything logged.
    ASSERT(verbosity_ != MP4_LOG_NONE);
    ASSERT(format);

    if (verbosity_ > this->_verbosity)
    {
        // We're not set verbose enough to log this
        return;
    }

    if (Log::_cb_func)
    {
        Log::_cb_func(verbosity_,format,ap);
        return;
    }

    // No callback set so log to standard out.  
    ::vfprintf(stdout,format,ap);
    ::fprintf(stdout,"\n");
}

///////////////////////////////////////////////////////////////////////////////

/**
 * Log a buffer as ascii-hex
 *
 * @param indent the number of spaces to indent the buffer
 *
 * @param verbosity the level of detail the message contains
 *
 * @param pBytes the buffer to log
 *
 * @param numBytes the number of bytes to log
 *
 * @param format the format string to use to process the
 * remaining arguments, where the format + remaining args
 * describe @p pBytes.  The resulting string should not
 * contain a newline.  Only the first 255 characters of the
 * resulting string (not including the NUL terminator) make
 * it to the log callback or stdout.
 */
void
Log::hexDump( uint8_t           indent,
              MP4LogLevel       verbosity_,
              const uint8_t*    pBytes,
              uint32_t          numBytes,
              const char*       format,
              ... )
{
    va_list     ap;

    ASSERT(pBytes || (numBytes == 0));
    ASSERT(format);

    if (verbosity_ > this->_verbosity)
    {
        // We're not set verbose enough to log this
        return;
    }

    // Build the description by processing format and the
    // remaining args.  Since we don't have asprintf, pick
    // an arbitrary length for the string and use snprintf.
    // To save a memory allocation, only do this if there's
    // a non-empty format string or non-zero indent
    char *desc = NULL;
    if (format[0] || indent)
    {
        desc = (char *)MP4Calloc(256 + indent);
        sprintf(desc,"%*c",indent,' ');
        va_start(ap,format);
        vsnprintf(desc + indent,255,format,ap);
        va_end(ap);
    }

    // From here we can use the C++ standard lib classes and
    // build a string for each line
    for (uint32_t i = 0;(i < numBytes);i += 16)
    {
        // ios_base::ate means at end.  With out this desc
        // gets overwritten with each << operation
        ostringstream oneLine(desc ? desc : "",ios_base::ate);

        // Append the byte offset this line starts with as
        // an 8 character, leading 0, hex number.  Leave the
        // fill character set to 0 for the remaining
        // operations
        oneLine << ':' << hex << setw(8) << setfill('0') <<
            std::right << i << setw(0) << setfill(' ') << ": ";

        uint32_t curlen = min((uint32_t)16,numBytes - i);
        const uint8_t *b = pBytes + i;
        uint32_t j;

        for (j = 0;(j < curlen);j++)
        {
            oneLine << hex << setw(2) << setfill('0') << right << static_cast<uint32_t>(b[j]);
            oneLine << setw(0) << setfill(' ') << ' ';
        }

        for (; j < 16; j++)
        {
            oneLine << "   ";
        }

        b = pBytes + i;
        for (j = 0;(j < curlen);j++)
        {
            if (isprint(static_cast<int>(b[j])))
            {
                oneLine << static_cast<char>(b[j]);
            }
            else
            {
                oneLine << '.';
            }
        }

        // We can either call the callback directly or use
        // the Log::printf function.  To call the callback
        // directly, we need a va_list.  (I think) we need
        // and extra function call to build that, so we may
        // as well call Log::printf.  It's going to
        // double-check the verbosity and the callback
        // function pointer, but that seems OK (13-feb-09,
        // dbyron)
        this->printf(verbosity_,"%s",oneLine.str().c_str());
    }

    if (desc)
    {
        MP4Free(desc);
        desc = NULL;
    }
}

///////////////////////////////////////////////////////////////////////////////

/**
 * Log an Exception as an error
 *
 * @param x the exception to log
 */
void
Log::errorf ( const Exception&      x )
{
    this->printf(MP4_LOG_ERROR,"%s",x.msg().c_str());
}

///////////////////////////////////////////////////////////////////////////////

}} // namespace mp4v2::impl

using namespace mp4v2::impl;

extern "C"
void MP4SetLogCallback( MP4LogCallback cb_func )
{
    Log::setLogCallback(cb_func);
}

extern "C"
MP4LogLevel MP4LogGetLevel(void)
{
    return mp4v2::impl::log.verbosity;
}

extern "C"
void MP4LogSetLevel( MP4LogLevel verbosity )
{
    try
    {
        mp4v2::impl::log.setVerbosity(verbosity);
    }
    catch( Exception* x ) {
        mp4v2::impl::log.errorf(*x);
        delete x;
    }
    catch( ... ) {
        mp4v2::impl::log.errorf( "%s: failed", __FUNCTION__ );
    }
}

