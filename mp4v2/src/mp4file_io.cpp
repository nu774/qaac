/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is MPEG4IP.
 *
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2001.  All Rights Reserved.
 *
 * Contributor(s):
 *      Dave Mackie     dmackie@cisco.com
 */

#include "src/impl.h"

namespace mp4v2 {
namespace impl {

///////////////////////////////////////////////////////////////////////////////

// MP4File low level IO support

uint64_t MP4File::GetPosition( File* file )
{
    if( m_memoryBuffer )
        return m_memoryBufferPosition;

    if( !file )
        file = m_file;

    ASSERT( file );
    return file->position;
}

void MP4File::SetPosition( uint64_t pos, File* file )
{
    if( m_memoryBuffer ) {
        if( pos >= m_memoryBufferSize )
            throw new Exception( "position out of range", __FILE__, __LINE__, __FUNCTION__ );
        m_memoryBufferPosition = pos;
        return;
    }

    if( !file )
        file = m_file;

    ASSERT( file );
    if( file->seek( pos ))
        throw new PlatformException( "seek failed", sys::getLastError(), __FILE__, __LINE__, __FUNCTION__ );
}

uint64_t MP4File::GetSize( File* file )
{
    if( m_memoryBuffer )
        return m_memoryBufferSize;

    if( !file )
        file = m_file;

    ASSERT( file );
    return file->size;
}

void MP4File::ReadBytes( uint8_t* buf, uint32_t bufsiz, File* file )
{
    if( bufsiz == 0 )
        return;

    ASSERT( buf );
    WARNING( m_numReadBits > 0 );

    if( m_memoryBuffer ) {
        if( m_memoryBufferPosition + bufsiz > m_memoryBufferSize )
            throw new Exception( "not enough bytes, reached end-of-memory", __FILE__, __LINE__, __FUNCTION__ );
        memcpy( buf, &m_memoryBuffer[m_memoryBufferPosition], bufsiz );
        m_memoryBufferPosition += bufsiz;
        return;
    }

    if( !file )
        file = m_file;

    ASSERT( file );
    File::Size nin;
    if( file->read( buf, bufsiz, nin ))
        throw new PlatformException( "read failed", sys::getLastError(), __FILE__, __LINE__, __FUNCTION__ );
    if( nin != bufsiz )
        throw new Exception( "not enough bytes, reached end-of-file", __FILE__, __LINE__, __FUNCTION__ );
}

void MP4File::PeekBytes( uint8_t* buf, uint32_t bufsiz, File* file )
{
    const uint64_t pos = GetPosition( file );
    ReadBytes( buf, bufsiz, file );
    SetPosition( pos, file );
}

void MP4File::EnableMemoryBuffer( uint8_t* pBytes, uint64_t numBytes )
{
    ASSERT( !m_memoryBuffer );

    if (pBytes) {
        m_memoryBuffer = pBytes;
        m_memoryBufferSize = numBytes;
    } else {
        if (numBytes) {
            m_memoryBufferSize = numBytes;
        } else {
            m_memoryBufferSize = 4096;
        }
        m_memoryBuffer = (uint8_t*)MP4Malloc(m_memoryBufferSize);
    }
    m_memoryBufferPosition = 0;
}

void MP4File::DisableMemoryBuffer( uint8_t** ppBytes, uint64_t* pNumBytes )
{
    ASSERT(m_memoryBuffer != NULL);

    if (ppBytes) {
        *ppBytes = m_memoryBuffer;
    }
    if (pNumBytes) {
        *pNumBytes = m_memoryBufferPosition;
    }

    m_memoryBuffer = NULL;
    m_memoryBufferSize = 0;
    m_memoryBufferPosition = 0;
}

void MP4File::WriteBytes( uint8_t* buf, uint32_t bufsiz, File* file )
{
    ASSERT( m_numWriteBits == 0 || m_numWriteBits >= 8 );

    if( !buf || bufsiz == 0 )
        return;

    if( m_memoryBuffer ) {
        if( m_memoryBufferPosition + bufsiz > m_memoryBufferSize ) {
            m_memoryBufferSize = 2 * (m_memoryBufferSize + bufsiz);
            m_memoryBuffer = (uint8_t*)MP4Realloc( m_memoryBuffer, m_memoryBufferSize );
        }
        memcpy( &m_memoryBuffer[m_memoryBufferPosition], buf, bufsiz );
        m_memoryBufferPosition += bufsiz;
        return;
    }

    if( !file )
        file = m_file;

    ASSERT( file );
    File::Size nout;
    if( file->write( buf, bufsiz, nout ))
        throw new PlatformException( "write failed", sys::getLastError(), __FILE__, __LINE__, __FUNCTION__ );
    if( nout != bufsiz )
        throw new Exception( "not all bytes written", __FILE__, __LINE__, __FUNCTION__ );
}

uint64_t MP4File::ReadUInt(uint8_t size)
{
    switch (size) {
    case 1:
        return ReadUInt8();
    case 2:
        return ReadUInt16();
    case 3:
        return ReadUInt24();
    case 4:
        return ReadUInt32();
    case 8:
        return ReadUInt64();
    default:
        ASSERT(false);
        return 0;
    }
}

uint8_t MP4File::ReadUInt8()
{
    uint8_t data;
    ReadBytes(&data, 1);
    return data;
}

void MP4File::WriteUInt8(uint8_t value)
{
    WriteBytes(&value, 1);
}

uint16_t MP4File::ReadUInt16()
{
    uint8_t data[2];
    ReadBytes(&data[0], 2);
    return ((data[0] << 8) | data[1]);
}

void MP4File::WriteUInt16(uint16_t value)
{
    uint8_t data[2];
    data[0] = (value >> 8) & 0xFF;
    data[1] = value & 0xFF;
    WriteBytes(data, 2);
}

uint32_t MP4File::ReadUInt24()
{
    uint8_t data[3];
    ReadBytes(&data[0], 3);
    return ((data[0] << 16) | (data[1] << 8) | data[2]);
}

void MP4File::WriteUInt24(uint32_t value)
{
    uint8_t data[3];
    data[0] = (value >> 16) & 0xFF;
    data[1] = (value >> 8) & 0xFF;
    data[2] = value & 0xFF;
    WriteBytes(data, 3);
}

uint32_t MP4File::ReadUInt32()
{
    uint8_t data[4];
    ReadBytes(&data[0], 4);
    return ((data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]);
}

void MP4File::WriteUInt32(uint32_t value)
{
    uint8_t data[4];
    data[0] = (value >> 24) & 0xFF;
    data[1] = (value >> 16) & 0xFF;
    data[2] = (value >> 8) & 0xFF;
    data[3] = value & 0xFF;
    WriteBytes(data, 4);
}

uint64_t MP4File::ReadUInt64()
{
    uint8_t data[8];
    uint64_t result = 0;
    uint64_t temp;

    ReadBytes(&data[0], 8);

    for (int i = 0; i < 8; i++) {
        temp = data[i];
        result |= temp << ((7 - i) * 8);
    }
    return result;
}

void MP4File::WriteUInt64(uint64_t value)
{
    uint8_t data[8];

    for (int i = 7; i >= 0; i--) {
        data[i] = value & 0xFF;
        value >>= 8;
    }
    WriteBytes(data, 8);
}

float MP4File::ReadFixed16()
{
    uint8_t iPart = ReadUInt8();
    uint8_t fPart = ReadUInt8();

    return iPart + (((float)fPart) / 0x100);
}

void MP4File::WriteFixed16(float value)
{
    if (value >= 0x100) {
        ostringstream msg;
        msg << value << " out of range";
        throw new PlatformException(msg.str().c_str(), ERANGE, __FILE__, __LINE__, __FUNCTION__);
    }

    uint8_t iPart = (uint8_t)value;
    uint8_t fPart = (uint8_t)((value - iPart) * 0x100);

    WriteUInt8(iPart);
    WriteUInt8(fPart);
}

float MP4File::ReadFixed32()
{
    uint16_t iPart = ReadUInt16();
    uint16_t fPart = ReadUInt16();

    return iPart + (((float)fPart) / 0x10000);
}

void MP4File::WriteFixed32(float value)
{
    if (value >= 0x10000) {
        ostringstream msg;
        msg << value << " out of range";
        throw new PlatformException(msg.str().c_str(), ERANGE, __FILE__, __LINE__, __FUNCTION__);
    }

    uint16_t iPart = (uint16_t)value;
    uint16_t fPart = (uint16_t)((value - iPart) * 0x10000);

    WriteUInt16(iPart);
    WriteUInt16(fPart);
}

float MP4File::ReadFloat()
{
    union {
        float f;
        uint32_t i;
    } u;

    u.i = ReadUInt32();
    return u.f;
}

void MP4File::WriteFloat(float value)
{
    union {
        float f;
        uint32_t i;
    } u;

    u.f = value;
    WriteUInt32(u.i);
}

char* MP4File::ReadString()
{
    uint32_t length = 0;
    uint32_t alloced = 64;
    char* data = (char*)MP4Malloc(alloced);

    do {
        if (length == alloced) {
            data = (char*)MP4Realloc(data, alloced * 2);
            if (data == NULL) return NULL;
            alloced *= 2;
        }
        ReadBytes((uint8_t*)&data[length], 1);
        length++;
    } while (data[length - 1] != 0);

    data = (char*)MP4Realloc(data, length);
    return data;
}

void MP4File::WriteString(char* string)
{
    if (string == NULL) {
        uint8_t zero = 0;
        WriteBytes(&zero, 1);
    } else {
        WriteBytes((uint8_t*)string, (uint32_t)strlen(string) + 1);
    }
}

char* MP4File::ReadCountedString(uint8_t charSize, bool allowExpandedCount, uint8_t fixedLength)
{
    uint32_t charLength;
    if (allowExpandedCount) {
        uint8_t b;
        uint32_t ix = 0;
        charLength = 0;
        do {
            b = ReadUInt8();
            charLength += b;
            ix++;
            if (ix > 25)
                throw new PlatformException("Counted string too long 25 * 255",ERANGE,
                                            __FILE__, __LINE__, __FUNCTION__);
        } while (b == 255);
    } else {
        charLength = ReadUInt8();
    }
    
    if (fixedLength && (charLength > fixedLength - 1)) {
        /*
         * The counted length of this string is greater than the
         * maxiumum fixed length, so truncate the string to the
         * maximum fixed length amount (take 1 byte away from the
         * fixedlength since we've already sacrificed one byte for
         * reading the counted length, and there has been a bug where
         * a non counted string has been used in the place of a
         * counted string).
         */  
        WARNING(charLength > fixedLength - 1);
        charLength = fixedLength - 1U;
    }

    uint32_t byteLength = charLength * charSize;
    char* data = (char*)MP4Malloc(byteLength + 1);
    if (byteLength > 0) {
        ReadBytes((uint8_t*)data, byteLength);
    }
    data[byteLength] = '\0';

    // read padding
    if (fixedLength) {
        const uint8_t padsize = fixedLength - byteLength -1U;
        if( padsize ) {
            uint8_t* padbuf = (uint8_t*)malloc( padsize );
            ReadBytes( padbuf, padsize );
            free( padbuf );
        }
    }

    return data;
}

void MP4File::WriteCountedString(char* string,
                                 uint8_t charSize, bool allowExpandedCount,
                                 uint32_t fixedLength)
{
    uint32_t byteLength;
    uint8_t zero[1];

    if (string) {
        byteLength = (uint32_t)strlen(string);
        if (fixedLength && (byteLength >= fixedLength)) {
            byteLength = fixedLength-1;
        }
    }
    else {
        byteLength = 0;
    }
    uint32_t charLength = byteLength / charSize;

    if (allowExpandedCount) {
        while (charLength >= 0xFF) {
            WriteUInt8(0xFF);
            charLength -= 0xFF;
        }
        // Write the count
        WriteUInt8(charLength);
    } else {
        if (charLength > 255) {
            ostringstream msg;
            msg << "Length is " << charLength;
            throw new PlatformException(msg.str().c_str(), ERANGE, __FILE__, __LINE__, __FUNCTION__);
        }
        // Write the count
        WriteUInt8(charLength);
    }

    if (byteLength > 0) {
        // Write the string (or the portion that we want to write)
        WriteBytes((uint8_t*)string, byteLength);
    }

    // Write any padding if this is a fixed length counted string
    if (fixedLength) {
        zero[0] = 0;
        while (byteLength < fixedLength-1U) {
            WriteBytes(zero, 1);
            byteLength++;
        }
    }
}

uint64_t MP4File::ReadBits(uint8_t numBits)
{
    ASSERT(numBits > 0);
    ASSERT(numBits <= 64);

    uint64_t bits = 0;

    for (uint8_t i = numBits; i > 0; i--) {
        if (m_numReadBits == 0) {
            ReadBytes(&m_bufReadBits, 1);
            m_numReadBits = 8;
        }
        bits = (bits << 1) | ((m_bufReadBits >> (--m_numReadBits)) & 1);
    }

    return bits;
}

void MP4File::FlushReadBits()
{
    // eat any remaining bits in the read buffer
    m_numReadBits = 0;
}

void MP4File::WriteBits(uint64_t bits, uint8_t numBits)
{
    ASSERT(numBits <= 64);

    for (uint8_t i = numBits; i > 0; i--) {
        m_bufWriteBits |=
            (((bits >> (i - 1)) & 1) << (8 - ++m_numWriteBits));

        if (m_numWriteBits == 8) {
            FlushWriteBits();
        }
    }
}

void MP4File::PadWriteBits(uint8_t pad)
{
    if (m_numWriteBits) {
        WriteBits(pad ? 0xFF : 0x00, 8 - m_numWriteBits);
    }
}

void MP4File::FlushWriteBits()
{
    if (m_numWriteBits > 0) {
        WriteBytes(&m_bufWriteBits, 1);
        m_numWriteBits = 0;
        m_bufWriteBits = 0;
    }
}

uint32_t MP4File::ReadMpegLength()
{
    uint32_t length = 0;
    uint8_t numBytes = 0;
    uint8_t b;

    do {
        b = ReadUInt8();
        length = (length << 7) | (b & 0x7F);
        numBytes++;
    } while ((b & 0x80) && numBytes < 4);

    return length;
}

void MP4File::WriteMpegLength(uint32_t value, bool compact)
{
    if (value > 0x0FFFFFFF) {
        ostringstream msg;
        msg << "out of range: " << value;
        throw new PlatformException(msg.str().c_str(), ERANGE, __FILE__, __LINE__, __FUNCTION__ ); 
    }

    int8_t numBytes;

    if (compact) {
        if (value <= 0x7F) {
            numBytes = 1;
        } else if (value <= 0x3FFF) {
            numBytes = 2;
        } else if (value <= 0x1FFFFF) {
            numBytes = 3;
        } else {
            numBytes = 4;
        }
    } else {
        numBytes = 4;
    }

    int8_t i = numBytes;
    do {
        i--;
        uint8_t b = (value >> (i * 7)) & 0x7F;
        if (i > 0) {
            b |= 0x80;
        }
        WriteUInt8(b);
    } while (i > 0);
}

///////////////////////////////////////////////////////////////////////////////

}
} // namespace mp4v2::impl
