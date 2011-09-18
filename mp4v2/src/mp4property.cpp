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
 *      Kona Blend      kona8lend@@gmail.com
 */

#include "src/impl.h"

namespace mp4v2 { namespace impl {

///////////////////////////////////////////////////////////////////////////////

MP4Property::MP4Property(MP4Atom& parentAtom, const char* name)
    : m_parentAtom(parentAtom)
{
    m_name = name;
    m_readOnly = false;
    m_implicit = false;
}

bool MP4Property::FindProperty(const char* name,
                               MP4Property** ppProperty, uint32_t* pIndex)
{
    if (name == NULL) {
        return false;
    }

    if (!strcasecmp(m_name, name)) {
        log.verbose1f("\"%s\": FindProperty: matched %s", 
                      m_parentAtom.GetFile().GetFilename().c_str(), name);
        *ppProperty = this;
        return true;
    }
    return false;
}

// Integer Property

uint64_t MP4IntegerProperty::GetValue(uint32_t index)
{
    switch (this->GetType()) {
    case Integer8Property:
        return ((MP4Integer8Property*)this)->GetValue(index);
    case Integer16Property:
        return ((MP4Integer16Property*)this)->GetValue(index);
    case Integer24Property:
        return ((MP4Integer24Property*)this)->GetValue(index);
    case Integer32Property:
        return ((MP4Integer32Property*)this)->GetValue(index);
    case Integer64Property:
        return ((MP4Integer64Property*)this)->GetValue(index);
    default:
        ASSERT(false);
    }
    return (0);
}

void MP4IntegerProperty::SetValue(uint64_t value, uint32_t index)
{
    switch (this->GetType()) {
    case Integer8Property:
        ((MP4Integer8Property*)this)->SetValue(value, index);
        break;
    case Integer16Property:
        ((MP4Integer16Property*)this)->SetValue(value, index);
        break;
    case Integer24Property:
        ((MP4Integer24Property*)this)->SetValue(value, index);
        break;
    case Integer32Property:
        ((MP4Integer32Property*)this)->SetValue(value, index);
        break;
    case Integer64Property:
        ((MP4Integer64Property*)this)->SetValue(value, index);
        break;
    default:
        ASSERT(false);
    }
}

void MP4IntegerProperty::InsertValue(uint64_t value, uint32_t index)
{
    switch (this->GetType()) {
    case Integer8Property:
        ((MP4Integer8Property*)this)->InsertValue(value, index);
        break;
    case Integer16Property:
        ((MP4Integer16Property*)this)->InsertValue(value, index);
        break;
    case Integer24Property:
        ((MP4Integer24Property*)this)->InsertValue(value, index);
        break;
    case Integer32Property:
        ((MP4Integer32Property*)this)->InsertValue(value, index);
        break;
    case Integer64Property:
        ((MP4Integer64Property*)this)->InsertValue(value, index);
        break;
    default:
        ASSERT(false);
    }
}

void MP4IntegerProperty::DeleteValue(uint32_t index)
{
    switch (this->GetType()) {
    case Integer8Property:
        ((MP4Integer8Property*)this)->DeleteValue(index);
        break;
    case Integer16Property:
        ((MP4Integer16Property*)this)->DeleteValue(index);
        break;
    case Integer24Property:
        ((MP4Integer24Property*)this)->DeleteValue(index);
        break;
    case Integer32Property:
        ((MP4Integer32Property*)this)->DeleteValue(index);
        break;
    case Integer64Property:
        ((MP4Integer64Property*)this)->DeleteValue(index);
        break;
    default:
        ASSERT(false);
    }
}

void MP4IntegerProperty::IncrementValue(int32_t increment, uint32_t index)
{
    SetValue(GetValue() + increment);
}

void MP4Integer8Property::Dump(uint8_t indent,
                               bool dumpImplicits, uint32_t index)
{
    if (m_implicit && !dumpImplicits) {
        return;
    }
    if (index != 0)
        log.dump(indent, MP4_LOG_VERBOSE1, "\"%s\": %s[%u] = %u (0x%02x)",
                 m_parentAtom.GetFile().GetFilename().c_str(),
                 m_name, index, m_values[index], m_values[index]);
    else
        log.dump(indent, MP4_LOG_VERBOSE1, "\"%s\": %s = %u (0x%02x)",
                 m_parentAtom.GetFile().GetFilename().c_str(),
                 m_name, m_values[index], m_values[index]);
}

void MP4Integer16Property::Dump(uint8_t indent,
                                bool dumpImplicits, uint32_t index)
{
    if (m_implicit && !dumpImplicits) {
        return;
    }
    if (index != 0)
        log.dump(indent, MP4_LOG_VERBOSE1, "\"%s\": %s[%u] = %u (0x%04x)",
                 m_parentAtom.GetFile().GetFilename().c_str(),
                 m_name, index, m_values[index], m_values[index]);
    else
        log.dump(indent, MP4_LOG_VERBOSE1, "\"%s\": %s = %u (0x%04x)",
                 m_parentAtom.GetFile().GetFilename().c_str(),
                 m_name, m_values[index], m_values[index]);
}

void MP4Integer24Property::Dump(uint8_t indent,
                                bool dumpImplicits, uint32_t index)
{
    if (m_implicit && !dumpImplicits) {
        return;
    }
    if (index != 0)
        log.dump(indent, MP4_LOG_VERBOSE1, "\"%s\": %s[%u] = %u (0x%06x)",
                 m_parentAtom.GetFile().GetFilename().c_str(),
                 m_name, index, m_values[index], m_values[index]);
    else
        log.dump(indent, MP4_LOG_VERBOSE1, "\"%s\": %s = %u (0x%06x)",
                 m_parentAtom.GetFile().GetFilename().c_str(),
                 m_name, m_values[index], m_values[index]);
}

void MP4Integer32Property::Dump(uint8_t indent,
                                bool dumpImplicits, uint32_t index)
{
    if (m_implicit && !dumpImplicits) {
        return;
    }
    if (index != 0)
        log.dump(indent, MP4_LOG_VERBOSE1, "\"%s\": %s[%u] = %u (0x%08x)",
                 m_parentAtom.GetFile().GetFilename().c_str(),
                 m_name, index, m_values[index], m_values[index]);
    else
        log.dump(indent, MP4_LOG_VERBOSE1, "\"%s\": %s = %u (0x%08x)",
                 m_parentAtom.GetFile().GetFilename().c_str(),
                 m_name, m_values[index], m_values[index]);
}

void MP4Integer64Property::Dump(uint8_t indent,
                                bool dumpImplicits, uint32_t index)
{
    if (m_implicit && !dumpImplicits) {
        return;
    }
    if (index != 0)
        log.dump(indent, MP4_LOG_VERBOSE1, "\"%s\": %s[%u] = %" PRIu64 " (0x%016" PRIx64 ")",
                 m_parentAtom.GetFile().GetFilename().c_str(),
                 m_name, index, m_values[index], m_values[index]);
    else
        log.dump(indent, MP4_LOG_VERBOSE1, "\"%s\": %s = %" PRIu64 " (0x%016" PRIx64 ")",
                 m_parentAtom.GetFile().GetFilename().c_str(),
                 m_name, m_values[index], m_values[index]);
}

// MP4BitfieldProperty

void MP4BitfieldProperty::Read(MP4File& file, uint32_t index)
{
    if (m_implicit) {
        return;
    }
    m_values[index] = file.ReadBits(m_numBits);
}

void MP4BitfieldProperty::Write(MP4File& file, uint32_t index)
{
    if (m_implicit) {
        return;
    }
    file.WriteBits(m_values[index], m_numBits);
}

void MP4BitfieldProperty::Dump(uint8_t indent,
                               bool dumpImplicits, uint32_t index)
{
    if (m_implicit && !dumpImplicits) {
        return;
    }
    uint8_t hexWidth = m_numBits / 4;
    if (hexWidth == 0 || (m_numBits % 4)) {
        hexWidth++;
    }
    if (index != 0)
        log.dump(indent, MP4_LOG_VERBOSE1,
                 "\"%s\": %s[%u] = %" PRIu64 " (0x%0*" PRIx64 ") <%u bits>",
                 m_parentAtom.GetFile().GetFilename().c_str(),
                 m_name, index, m_values[index], (int)hexWidth, m_values[index], m_numBits);
    else
        log.dump(indent, MP4_LOG_VERBOSE1,
                 "\"%s\": %s = %" PRIu64 " (0x%0*" PRIx64 ") <%u bits>",
                 m_parentAtom.GetFile().GetFilename().c_str(),
                 m_name, m_values[index], (int)hexWidth, m_values[index], m_numBits);
}

// MP4Float32Property

void MP4Float32Property::Read(MP4File& file, uint32_t index)
{
    if (m_implicit) {
        return;
    }
    if (m_useFixed16Format) {
        m_values[index] = file.ReadFixed16();
    } else if (m_useFixed32Format) {
        m_values[index] = file.ReadFixed32();
    } else {
        m_values[index] = file.ReadFloat();
    }
}

void MP4Float32Property::Write(MP4File& file, uint32_t index)
{
    if (m_implicit) {
        return;
    }
    if (m_useFixed16Format) {
        file.WriteFixed16(m_values[index]);
    } else if (m_useFixed32Format) {
        file.WriteFixed32(m_values[index]);
    } else {
        file.WriteFloat(m_values[index]);
    }
}

void MP4Float32Property::Dump(uint8_t indent,
                              bool dumpImplicits, uint32_t index)
{
    if (m_implicit && !dumpImplicits) {
        return;
    }
    if (index != 0)
        log.dump(indent, MP4_LOG_VERBOSE1, "\"%s\": %s[%u] = %f",
                 m_parentAtom.GetFile().GetFilename().c_str(),
                 m_name, index, m_values[index]);
    else
        log.dump(indent, MP4_LOG_VERBOSE1, "\"%s\": %s = %f",
                 m_parentAtom.GetFile().GetFilename().c_str(),
                 m_name, m_values[index]);
}

// MP4StringProperty

MP4StringProperty::MP4StringProperty(
    MP4Atom& parentAtom,
    const char* name,
    bool        useCountedFormat,
    bool        useUnicode,
    bool        arrayMode )

    : MP4Property( parentAtom, name )
    , m_arrayMode        ( arrayMode )
    , m_useCountedFormat ( useCountedFormat )
    , m_useExpandedCount ( false )
    , m_useUnicode       ( useUnicode )
    , m_fixedLength      ( 0 )
{
    SetCount( 1 );
    m_values[0] = NULL;
}

MP4StringProperty::~MP4StringProperty()
{
    uint32_t count = GetCount();
    for (uint32_t i = 0; i < count; i++) {
        MP4Free(m_values[i]);
    }
}

void MP4StringProperty::SetCount(uint32_t count)
{
    uint32_t oldCount = m_values.Size();

    m_values.Resize(count);

    for (uint32_t i = oldCount; i < count; i++) {
        m_values[i] = NULL;
    }
}

void MP4StringProperty::SetValue(const char* value, uint32_t index)
{
    if (m_readOnly) {
        ostringstream msg;
        msg << "property " << m_name << "is read-only";
        throw new PlatformException(msg.str().c_str(), EACCES, __FILE__, __LINE__, __FUNCTION__ );
    }

    MP4Free(m_values[index]);

    if (m_fixedLength) {
        m_values[index] = (char*)MP4Calloc(m_fixedLength + 1);
        if (value) {
            strncpy(m_values[index], value, m_fixedLength);
        }
    } else {
        if (value) {
            m_values[index] = MP4Stralloc(value);
        } else {
            m_values[index] = NULL;
        }
    }
}

void MP4StringProperty::Read( MP4File& file, uint32_t index )
{
    if( m_implicit )
        return;

    uint32_t begin = index;
    uint32_t max   = index + 1;

    if( m_arrayMode ) {
        begin = 0;
        max   = GetCount();
    }

    for( uint32_t i = begin; i < max; i++ ) {
        char*& value = m_values[i];

        // Generally a default atom setting, e.g. see atom_avc1.cpp, "JVT/AVC Coding"; we'll leak this string if
        // we don't free.  Note that MP4Free checks for null.
        MP4Free(value); 

        if( m_useCountedFormat ) {
            value = file.ReadCountedString( (m_useUnicode ? 2 : 1), m_useExpandedCount, m_fixedLength );
        }
        else if( m_fixedLength ) {
            value = (char*)MP4Calloc( m_fixedLength + 1 );
            file.ReadBytes( (uint8_t*)value, m_fixedLength );
        }
        else {
            value = file.ReadString();
        }
    }
}

void MP4StringProperty::Write( MP4File& file, uint32_t index )
{
    if( m_implicit )
        return;

    uint32_t begin = index;
    uint32_t max   = index + 1;

    if( m_arrayMode ) {
        begin = 0;
        max   = GetCount();
    }

    for( uint32_t i = begin; i < max; i++ ) {
        char*& value = m_values[i];

        if( m_useCountedFormat ) {
            file.WriteCountedString( value, (m_useUnicode ? 2 : 1), m_useExpandedCount, m_fixedLength );
        }
        else if( m_fixedLength ) {
            file.WriteBytes( (uint8_t*)value, m_fixedLength );
        }
        else {
            file.WriteString( value );
        }
    }
}

void MP4StringProperty::Dump( uint8_t indent, bool dumpImplicits, uint32_t index )
{
    if( m_implicit && !dumpImplicits )
        return;

    if( !m_arrayMode ) {
        char indexd[32];
        if( index != 0 )
            snprintf( indexd, 32, "[%u]", index );
        else
            indexd[0] = '\0';

        if( m_useUnicode )
            log.dump(indent, MP4_LOG_VERBOSE1, "\"%s\": %s%s = %ls",
                     m_parentAtom.GetFile().GetFilename().c_str(),
                     m_name, indexd, (wchar_t*)m_values[index] );
        else
            log.dump(indent, MP4_LOG_VERBOSE1, "\"%s\": %s%s = %s",
                     m_parentAtom.GetFile().GetFilename().c_str(),
                     m_name, indexd, m_values[index] );
    }
    else if( log.verbosity >= MP4_LOG_VERBOSE2 )
    {
        const uint32_t max = GetCount();

        log.dump(indent, MP4_LOG_VERBOSE2, "\"%s\": %s (size=%u)",
                 m_parentAtom.GetFile().GetFilename().c_str(),
                 m_name, max );

        for( uint32_t i = 0; i < max; i++ ) {
            char*& value = m_values[i];

            if( m_useUnicode )
                log.dump(indent, MP4_LOG_VERBOSE2, "\"%s\": %s[%u] = %ls",
                         m_parentAtom.GetFile().GetFilename().c_str(),
                         m_name, i, (wchar_t*)value );
            else
                log.dump(indent, MP4_LOG_VERBOSE2, "\"%s\": %s[%u] = %s",
                         m_parentAtom.GetFile().GetFilename().c_str(),
                         m_name, i, value );
        }
    }
    else {
        log.dump(indent, MP4_LOG_VERBOSE1, "\"%s\": <table entries suppressed>",
                 m_parentAtom.GetFile().GetFilename().c_str() );
    }
}

// MP4BytesProperty

MP4BytesProperty::MP4BytesProperty(MP4Atom& parentAtom, const char* name, uint32_t valueSize,
                                   uint32_t defaultValueSize)
        : MP4Property(parentAtom, name)
        , m_fixedValueSize(0)
        , m_defaultValueSize(defaultValueSize)
{
    SetCount(1);
    m_values[0] = (uint8_t*)MP4Calloc(valueSize);
    m_valueSizes[0] = valueSize;
}

MP4BytesProperty::~MP4BytesProperty()
{
    uint32_t count = GetCount();
    for (uint32_t i = 0; i < count; i++) {
        MP4Free(m_values[i]);
    }
}

void MP4BytesProperty::SetCount(uint32_t count)
{
    uint32_t oldCount = m_values.Size();

    m_values.Resize(count);
    m_valueSizes.Resize(count);

    for (uint32_t i = oldCount; i < count; i++) {
        m_values[i] = NULL;
        m_valueSizes[i] = m_defaultValueSize;
    }
}

void MP4BytesProperty::SetValue(const uint8_t* pValue, uint32_t valueSize,
                                uint32_t index)
{
    if (m_readOnly) {
        ostringstream msg;
        msg << "property " << m_name << "is read-only";
        throw new PlatformException(msg.str().c_str(), EACCES, __FILE__, __LINE__, __FUNCTION__ );
    }
    if (m_fixedValueSize) {
        if (valueSize > m_fixedValueSize) {
            ostringstream msg;
            msg << GetParentAtom().GetType() << "." << GetName() << " value size " << valueSize << " exceeds fixed value size " << m_fixedValueSize;
            throw new Exception(msg.str().c_str(), __FILE__, __LINE__, __FUNCTION__ );
        }
        if (m_values[index] == NULL) {
            m_values[index] = (uint8_t*)MP4Calloc(m_fixedValueSize);
            m_valueSizes[index] = m_fixedValueSize;
        }
        if (pValue) {
            memcpy(m_values[index], pValue, valueSize);
        }
    } else {
        MP4Free(m_values[index]);
        if (pValue) {
            m_values[index] = (uint8_t*)MP4Malloc(valueSize);
            memcpy(m_values[index], pValue, valueSize);
            m_valueSizes[index] = valueSize;
        } else {
            m_values[index] = NULL;
            m_valueSizes[index] = 0;
        }
    }
}

void MP4BytesProperty::SetValueSize(uint32_t valueSize, uint32_t index)
{
    if (m_fixedValueSize) {
        throw new Exception("can't change size of fixed sized property",
                            __FILE__, __LINE__, __FUNCTION__ );
    }
    if (m_values[index] != NULL) {
        m_values[index] = (uint8_t*)MP4Realloc(m_values[index], valueSize);
    }
    m_valueSizes[index] = valueSize;
}

void MP4BytesProperty::SetFixedSize(uint32_t fixedSize)
{
    m_fixedValueSize = 0;
    for (uint32_t i = 0; i < GetCount(); i++) {
        SetValueSize(fixedSize, i);
    }
    m_fixedValueSize = fixedSize;
}

void MP4BytesProperty::Read(MP4File& file, uint32_t index)
{
    if (m_implicit) {
        return;
    }
    MP4Free(m_values[index]);
    m_values[index] = (uint8_t*)MP4Malloc(m_valueSizes[index]);
    file.ReadBytes(m_values[index], m_valueSizes[index]);
}

void MP4BytesProperty::Write(MP4File& file, uint32_t index)
{
    if (m_implicit) {
        return;
    }
    file.WriteBytes(m_values[index], m_valueSizes[index]);
}

void MP4BytesProperty::Dump(uint8_t indent,
                            bool dumpImplicits, uint32_t index)
{
    if( m_implicit && !dumpImplicits )
        return;

    const uint32_t size  = m_valueSizes[index];
    const uint8_t* const value = m_values[index];

    if( size == 0 ) {
        log.dump(indent, MP4_LOG_VERBOSE2, "\"%s\": %s = <%u bytes>",
                 m_parentAtom.GetFile().GetFilename().c_str(),
                 m_name, size );
        return;
    }

    if( size <= 16 ) {
        ostringstream oss;
        ostringstream text;

        oss << "  ";
        for( uint32_t i = 0; i < size; i++ ) {
            if( i )
                oss << ' ';
            oss << hex << setw(2) << setfill('0') << right << static_cast<uint32_t>(value[i]);
            text << (isprint( static_cast<int>(value[i]) ) ? static_cast<char>(value[i]) : '.');
        }

        oss << "  |" << text.str() << "|";

        log.dump(indent, MP4_LOG_VERBOSE2, "\"%s\": %s = <%u bytes>%s",
                 m_parentAtom.GetFile().GetFilename().c_str(),
                 m_name, size, oss.str().c_str() );
        return;
    }

    // specialization for ilst item data always show all bytes except for covr
    bool showall = false;
    MP4Atom* const datac = m_parentAtom.GetParentAtom(); // data container
    MP4Atom* const datacc = datac->GetParentAtom();
    if( datacc &&
        ATOMID( datacc->GetType() ) == ATOMID( "ilst" ) &&
        ATOMID( datac->GetType() ) != ATOMID( "covr" ) )
    {
        showall = true;
    }

    uint32_t adjsize;
    bool supressed;

    if( showall ||
        size < 128 || log.verbosity >= MP4_LOG_VERBOSE2 )
    {
        adjsize = size;
        supressed = false;
    }
    else {
        adjsize = 128;
        supressed = true;
    }

    ostringstream oss;
    ostringstream text;

    log.dump(indent, MP4_LOG_VERBOSE2, "\"%s\": %s = <%u bytes>",
             m_parentAtom.GetFile().GetFilename().c_str(),
             m_name, size );
    log.hexDump(indent, MP4_LOG_VERBOSE2, value, adjsize, "\"%s\": %s",
                m_parentAtom.GetFile().GetFilename().c_str(),
                m_name);

    if( supressed ) {
        log.dump(indent, MP4_LOG_VERBOSE1, "\"%s\": <remaining bytes supressed>",
                 m_parentAtom.GetFile().GetFilename().c_str() );
    }
}

// MP4TableProperty

MP4TableProperty::MP4TableProperty(MP4Atom& parentAtom, const char* name, MP4IntegerProperty* pCountProperty)
        : MP4Property(parentAtom, name)
{
    m_pCountProperty = pCountProperty;
    m_pCountProperty->SetReadOnly();
}

MP4TableProperty::~MP4TableProperty()
{
    for (uint32_t i = 0; i < m_pProperties.Size(); i++) {
        delete m_pProperties[i];
    }
}

void MP4TableProperty::AddProperty(MP4Property* pProperty)
{
    ASSERT(pProperty);
    ASSERT(pProperty->GetType() != TableProperty);
    ASSERT(pProperty->GetType() != DescriptorProperty);
    m_pProperties.Add(pProperty);
    pProperty->SetCount(0);
}

bool MP4TableProperty::FindProperty(const char *name,
                                    MP4Property** ppProperty, uint32_t* pIndex)
{
    ASSERT(m_name);

    // check if first component of name matches ourselves
    if (!MP4NameFirstMatches(m_name, name)) {
        return false;
    }

    // check if the specified table entry exists
    uint32_t index;
    bool haveIndex = MP4NameFirstIndex(name, &index);
    if (haveIndex) {
        if (index >= GetCount()) {
            return false;
        }
        if (pIndex) {
            *pIndex = index;
        }
    }
    
    log.verbose1f("\"%s\": FindProperty: matched %s", 
                  m_parentAtom.GetFile().GetFilename().c_str(), name);

    // get name of table property
    const char *tablePropName = MP4NameAfterFirst(name);
    if (tablePropName == NULL) {
        if (!haveIndex) {
            *ppProperty = this;
            return true;
        }
        return false;
    }

    // check if this table property exists
    return FindContainedProperty(tablePropName, ppProperty, pIndex);
}

bool MP4TableProperty::FindContainedProperty(const char *name,
        MP4Property** ppProperty, uint32_t* pIndex)
{
    uint32_t numProperties = m_pProperties.Size();

    for (uint32_t i = 0; i < numProperties; i++) {
        if (m_pProperties[i]->FindProperty(name, ppProperty, pIndex)) {
            return true;
        }
    }
    return false;
}

void MP4TableProperty::Read(MP4File& file, uint32_t index)
{
    ASSERT(index == 0);

    if (m_implicit) {
        return;
    }

    uint32_t numProperties = m_pProperties.Size();

    if (numProperties == 0) {
        WARNING(numProperties == 0);
        return;
    }

    uint32_t numEntries = GetCount();

    /* for each property set size */
    for (uint32_t j = 0; j < numProperties; j++) {
        m_pProperties[j]->SetCount(numEntries);
    }

    for (uint32_t i = 0; i < numEntries; i++) {
        ReadEntry(file, i);
    }
}

void MP4TableProperty::ReadEntry(MP4File& file, uint32_t index)
{
    for (uint32_t j = 0; j < m_pProperties.Size(); j++) {
        m_pProperties[j]->Read(file, index);
    }
}

void MP4TableProperty::Write(MP4File& file, uint32_t index)
{
    ASSERT(index == 0);

    if (m_implicit) {
        return;
    }

    uint32_t numProperties = m_pProperties.Size();

    if (numProperties == 0) {
        WARNING(numProperties == 0);
        return;
    }

    uint32_t numEntries = GetCount();

    if (m_pProperties[0]->GetCount() != numEntries) {
        log.errorf("%s: \"%s\": %s %s \"%s\"table entries %u doesn't match count %u",
                   __FUNCTION__, m_parentAtom.GetFile().GetFilename().c_str(),
                   GetParentAtom().GetType(),
                   GetName(), m_pProperties[0]->GetName(),
                   m_pProperties[0]->GetCount(), numEntries);

        ASSERT(m_pProperties[0]->GetCount() == numEntries);
    }

    for (uint32_t i = 0; i < numEntries; i++) {
        WriteEntry(file, i);
    }
}

void MP4TableProperty::WriteEntry(MP4File& file, uint32_t index)
{
    for (uint32_t j = 0; j < m_pProperties.Size(); j++) {
        m_pProperties[j]->Write(file, index);
    }
}

void MP4TableProperty::Dump(uint8_t indent,
                            bool dumpImplicits, uint32_t index)
{
    ASSERT(index == 0);

    // implicit tables just can't be dumped
    if (m_implicit) {
        return;
    }

    uint32_t numProperties = m_pProperties.Size();

    if (numProperties == 0) {
        WARNING(numProperties == 0);
        return;
    }

    uint32_t numEntries = GetCount();

    for (uint32_t i = 0; i < numEntries; i++) {
        for (uint32_t j = 0; j < numProperties; j++) {
            m_pProperties[j]->Dump(indent + 1, dumpImplicits, i);
        }
    }
}

// MP4DescriptorProperty

MP4DescriptorProperty::MP4DescriptorProperty(MP4Atom& parentAtom, const char* name,
        uint8_t tagsStart, uint8_t tagsEnd, bool mandatory, bool onlyOne)
        : MP4Property(parentAtom, name)
{
    SetTags(tagsStart, tagsEnd);
    m_sizeLimit = 0;
    m_mandatory = mandatory;
    m_onlyOne = onlyOne;
}

MP4DescriptorProperty::~MP4DescriptorProperty()
{
    for (uint32_t i = 0; i < m_pDescriptors.Size(); i++) {
        delete m_pDescriptors[i];
    }
}

MP4Descriptor* MP4DescriptorProperty::AddDescriptor(uint8_t tag)
{
    // check that tag is in expected range
    ASSERT(tag >= m_tagsStart && tag <= m_tagsEnd);

    MP4Descriptor* pDescriptor = CreateDescriptor(m_parentAtom, tag);
    ASSERT(pDescriptor);

    m_pDescriptors.Add(pDescriptor);

    return pDescriptor;
}

void MP4DescriptorProperty::DeleteDescriptor(uint32_t index)
{
    delete m_pDescriptors[index];
    m_pDescriptors.Delete(index);
}

void MP4DescriptorProperty::Generate()
{
    // generate a default descriptor
    // if it is mandatory, and single
    if (m_mandatory && m_onlyOne) {
        MP4Descriptor* pDescriptor =
            AddDescriptor(m_tagsStart);
        pDescriptor->Generate();
    }
}

bool MP4DescriptorProperty::FindProperty(const char *name,
        MP4Property** ppProperty, uint32_t* pIndex)
{
    // we're unnamed, so just check contained properties
    if (m_name == NULL || !strcmp(m_name, "")) {
        return FindContainedProperty(name, ppProperty, pIndex);
    }

    // check if first component of name matches ourselves
    if (!MP4NameFirstMatches(m_name, name)) {
        return false;
    }

    // check if the specific descriptor entry exists
    uint32_t descrIndex;
    bool haveDescrIndex = MP4NameFirstIndex(name, &descrIndex);

    if (haveDescrIndex && descrIndex >= GetCount()) {
        return false;
    }

    log.verbose1f("\"%s\": matched %s",
                  m_parentAtom.GetFile().GetFilename().c_str(),
                  name);

    // get name of descriptor property
    name = MP4NameAfterFirst(name);
    if (name == NULL) {
        if (!haveDescrIndex) {
            *ppProperty = this;
            return true;
        }
        return false;
    }

    /* check rest of name */
    if (haveDescrIndex) {
        return m_pDescriptors[descrIndex]->FindProperty(name,
                ppProperty, pIndex);
    } else {
        return FindContainedProperty(name, ppProperty, pIndex);
    }
}

bool MP4DescriptorProperty::FindContainedProperty(const char *name,
        MP4Property** ppProperty, uint32_t* pIndex)
{
    for (uint32_t i = 0; i < m_pDescriptors.Size(); i++) {
        if (m_pDescriptors[i]->FindProperty(name, ppProperty, pIndex)) {
            return true;
        }
    }
    return false;
}

void MP4DescriptorProperty::Read(MP4File& file, uint32_t index)
{
    ASSERT(index == 0);

    if (m_implicit) {
        return;
    }

    uint64_t start = file.GetPosition();

    while (true) {
        // enforce size limitation
        if (m_sizeLimit && file.GetPosition() >= start + m_sizeLimit) {
            break;
        }

        uint8_t tag;
        try {
            file.PeekBytes(&tag, 1);
        }
        catch (Exception* x) {
            if (file.GetPosition() >= file.GetSize()) {
                // EOF
                delete x;
                break;
            }
            throw x;
        }

        // check if tag is in desired range
        if (tag < m_tagsStart || tag > m_tagsEnd) {
            break;
        }

        MP4Descriptor* pDescriptor =
            AddDescriptor(tag);

        pDescriptor->Read(file);
    }

    // warnings
    if (m_mandatory && m_pDescriptors.Size() == 0) {
        log.warningf("%s: \"%s\": Mandatory descriptor 0x%02x missing",
                     __FUNCTION__, GetParentAtom().GetFile().GetFilename().c_str(), m_tagsStart);
    } else if (m_onlyOne && m_pDescriptors.Size() > 1) {
        log.warningf("%s: \"%s\": Descriptor 0x%02x has more than one instance",
                     __FUNCTION__, GetParentAtom().GetFile().GetFilename().c_str(), m_tagsStart);
    }
}

void MP4DescriptorProperty::Write(MP4File& file, uint32_t index)
{
    ASSERT(index == 0);

    if (m_implicit) {
        return;
    }

    for (uint32_t i = 0; i < m_pDescriptors.Size(); i++) {
        m_pDescriptors[i]->Write(file);
    }
}

void MP4DescriptorProperty::Dump(uint8_t indent,
                                 bool dumpImplicits, uint32_t index)
{
    ASSERT(index == 0);

    if (m_implicit && !dumpImplicits) {
        return;
    }

    if (m_name) {
        if (index != 0)
            log.dump(indent, MP4_LOG_VERBOSE1, "\"%s\": %s[%u]",
                     m_parentAtom.GetFile().GetFilename().c_str(),
                     m_name, index);
        else
            log.dump(indent, MP4_LOG_VERBOSE1, "\"%s\": %s",
                     m_parentAtom.GetFile().GetFilename().c_str(),
                     m_name);
        indent++;
    }

    for (uint32_t i = 0; i < m_pDescriptors.Size(); i++) {
        m_pDescriptors[i]->Dump(indent, dumpImplicits);
    }
}

///////////////////////////////////////////////////////////////////////////////

MP4LanguageCodeProperty::MP4LanguageCodeProperty( MP4Atom& parentAtom, const char* name, bmff::LanguageCode value )
    : MP4Property( parentAtom, name )
{
    SetValue( value );
}

MP4LanguageCodeProperty::MP4LanguageCodeProperty( MP4Atom& parentAtom, const char* name, const string& code )
    : MP4Property( parentAtom, name )
{
    SetValue( bmff::enumLanguageCode.toType( code ));
}

void
MP4LanguageCodeProperty::Dump( uint8_t indent, bool dumpImplicits, uint32_t index )
{
    uint16_t data = 0;

    string svalue;
    bmff::enumLanguageCode.toString( _value, svalue );
    if( svalue.length() == 3 ) {
        data = (((svalue[0] - 0x60) & 0x001f) << 10)
             | (((svalue[1] - 0x60) & 0x001f) <<  5)
             | (((svalue[2] - 0x60) & 0x001f)      );
    }

    log.dump(indent, MP4_LOG_VERBOSE2, "\"%s\": %s = %s (0x%04x)",
             m_parentAtom.GetFile().GetFilename().c_str(),
             m_name, bmff::enumLanguageCode.toString( _value, true ).c_str(), data );
}

uint32_t
MP4LanguageCodeProperty::GetCount()
{
    return 1;
}

MP4PropertyType
MP4LanguageCodeProperty::GetType()
{
    return LanguageCodeProperty;
}

bmff::LanguageCode
MP4LanguageCodeProperty::GetValue()
{
    return _value;
}

void
MP4LanguageCodeProperty::Read( MP4File& file, uint32_t index )
{
    uint16_t data = file.ReadBits( 16 );

    char code[3];
    code[0] = ((data & 0x7c00) >> 10) + 0x60;
    code[1] = ((data & 0x03e0) >>  5) + 0x60;
    code[2] = ((data & 0x001f)      ) + 0x60;

    SetValue( bmff::enumLanguageCode.toType( string( code, sizeof(code) )));
}

void
MP4LanguageCodeProperty::SetCount( uint32_t count )
{
    // do nothing; count is always 1
}

void
MP4LanguageCodeProperty::SetValue( bmff::LanguageCode value )
{
    _value = value;
}

void
MP4LanguageCodeProperty::Write( MP4File& file, uint32_t index )
{
    uint16_t data = 0;

    string svalue;
    bmff::enumLanguageCode.toString( _value, svalue );
    if( svalue.length() == 3 ) {
        data = (((svalue[0] - 0x60) & 0x001f) << 10)
             | (((svalue[1] - 0x60) & 0x001f) <<  5)
             | (((svalue[2] - 0x60) & 0x001f)      );
    }

    file.WriteBits( data, 16 );
}

///////////////////////////////////////////////////////////////////////////////

MP4BasicTypeProperty::MP4BasicTypeProperty( MP4Atom& parentAtom, const char* name, itmf::BasicType type )
    : MP4Property( parentAtom, name )
{
    SetValue( type );
}

void
MP4BasicTypeProperty::Dump( uint8_t indent, bool dumpImplicits, uint32_t index )
{
    log.dump(indent, MP4_LOG_VERBOSE1, "\"%s\": %s = %s (0x%02x)",
             m_parentAtom.GetFile().GetFilename().c_str(), m_name,
             itmf::enumBasicType.toString( _value, true ).c_str(), _value );
}

uint32_t
MP4BasicTypeProperty::GetCount()
{
    return 1;
}

MP4PropertyType
MP4BasicTypeProperty::GetType()
{
    return BasicTypeProperty;
}

itmf::BasicType
MP4BasicTypeProperty::GetValue()
{
    return _value;
}

void
MP4BasicTypeProperty::Read( MP4File& file, uint32_t index )
{
    SetValue( static_cast<itmf::BasicType>( file.ReadBits( 8 )));
}

void
MP4BasicTypeProperty::SetCount( uint32_t count )
{
    // do nothing; count is always 1
}

void
MP4BasicTypeProperty::SetValue( itmf::BasicType value )
{
    _value = value;
}

void
MP4BasicTypeProperty::Write( MP4File& file, uint32_t index )
{
    file.WriteBits( _value, 8 );
}

///////////////////////////////////////////////////////////////////////////////

}} // namespace mp4v2::impl
