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
 * Copyright (C) Cisco Systems Inc. 2001 - 2004.  All Rights Reserved.
 *
 * 3GPP features implementation is based on 3GPP's TS26.234-v5.60,
 * and was contributed by Ximpo Group Ltd.
 *
 * Portions created by Ximpo Group Ltd. are
 * Copyright (C) Ximpo Group Ltd. 2003, 2004.  All Rights Reserved.
 *
 *  Portions created by Adnecto d.o.o. are
 *  Copyright (C) Adnecto d.o.o. 2005.  All Rights Reserved
 *
 * Contributor(s):
 *      Dave Mackie                dmackie@cisco.com
 *      Alix Marchandise-Franquet  alix@cisco.com
 *      Ximpo Group Ltd.           mp4v2@ximpo.com
 *      Danijel Kopcinovic         danijel.kopcinovic@adnecto.net
 */

#include "src/impl.h"

namespace mp4v2 { namespace impl {

///////////////////////////////////////////////////////////////////////////////

MP4AtomInfo::MP4AtomInfo(const char* name, bool mandatory, bool onlyOne)
{
    m_name = name;
    m_mandatory = mandatory;
    m_onlyOne = onlyOne;
    m_count = 0;
}

MP4Atom::MP4Atom(const char* type)
{
    SetType(type);
    m_unknownType = false;
    m_pFile = NULL;
    m_start = 0;
    m_end = 0;
    m_largesizeMode = false;
    m_size = 0;
    m_pParentAtom = NULL;
    m_depth = 0xFF;
}

MP4Atom::~MP4Atom()
{
    uint32_t i;

    for (i = 0; i < m_pProperties.Size(); i++) {
        delete m_pProperties[i];
    }
    for (i = 0; i < m_pChildAtomInfos.Size(); i++) {
        delete m_pChildAtomInfos[i];
    }
    for (i = 0; i < m_pChildAtoms.Size(); i++) {
        delete m_pChildAtoms[i];
    }
}

MP4Atom* MP4Atom::CreateAtom( MP4Atom* parent, const char* type )
{
    MP4Atom* atom = factory( parent, type );
    ASSERT( atom );
    return atom;
}

// generate a skeletal self

void MP4Atom::Generate()
{
    uint32_t i;

    // for all properties
    for (i = 0; i < m_pProperties.Size(); i++) {
        // ask it to self generate
        m_pProperties[i]->Generate();
    }

    // for all mandatory, single child atom types
    for (i = 0; i < m_pChildAtomInfos.Size(); i++) {
        if (m_pChildAtomInfos[i]->m_mandatory
                && m_pChildAtomInfos[i]->m_onlyOne) {

            // create the mandatory, single child atom
            MP4Atom* pChildAtom =
                CreateAtom(this, m_pChildAtomInfos[i]->m_name);

            AddChildAtom(pChildAtom);

            // and ask it to self generate
            pChildAtom->Generate();
        }
    }
}

MP4Atom* MP4Atom::ReadAtom(MP4File* pFile, MP4Atom* pParentAtom)
{
    uint8_t hdrSize = 8;
    uint8_t extendedType[16];

    uint64_t pos = pFile->GetPosition();

    VERBOSE_READ(pFile->GetVerbosity(),
                 printf("ReadAtom: pos = 0x%" PRIx64 "\n", pos));

    uint64_t dataSize = pFile->ReadUInt32();

    char type[5];
    pFile->ReadBytes((uint8_t*)&type[0], 4);
    type[4] = '\0';

    // extended size
    const bool largesizeMode = (dataSize == 1);
    if (dataSize == 1) {
        dataSize = pFile->ReadUInt64();
        hdrSize += 8;
        pFile->Check64BitStatus(type);
    }

    // extended type
    if (ATOMID(type) == ATOMID("uuid")) {
        pFile->ReadBytes(extendedType, sizeof(extendedType));
        hdrSize += sizeof(extendedType);
    }

    if (dataSize == 0) {
        // extends to EOF
        dataSize = pFile->GetSize() - pos;
    }

    dataSize -= hdrSize;

    VERBOSE_READ(pFile->GetVerbosity(),
                 printf("ReadAtom: type = \"%s\" data-size = %" PRIu64 " (0x%" PRIx64 ") hdr %u\n",
                        type, dataSize, dataSize, hdrSize));

    if (pos + hdrSize + dataSize > pParentAtom->GetEnd()) {
        VERBOSE_ERROR(pFile->GetVerbosity(),
                      printf("ReadAtom: invalid atom size, extends outside parent atom - skipping to end of \"%s\" \"%s\" %" PRIu64 " vs %" PRIu64 "\n",
                             pParentAtom->GetType(), type,
                             pos + hdrSize + dataSize,
                             pParentAtom->GetEnd()));
        VERBOSE_READ(pFile->GetVerbosity(),
                     printf("parent %s (%" PRIu64 ") pos %" PRIu64 " hdr %d data %" PRIu64 " sum %" PRIu64 "\n",
                            pParentAtom->GetType(),
                            pParentAtom->GetEnd(),
                            pos,
                            hdrSize,
                            dataSize,
                            pos + hdrSize + dataSize));
#if 0
        throw new MP4Error("invalid atom size", "ReadAtom");
#else
        // skip to end of atom
        dataSize = pParentAtom->GetEnd() - pos - hdrSize;
#endif
    }

    MP4Atom* pAtom = CreateAtom(pParentAtom, type);
    pAtom->SetFile(pFile);
    pAtom->SetStart(pos);
    pAtom->SetEnd(pos + hdrSize + dataSize);
    pAtom->SetLargesizeMode(largesizeMode);
    pAtom->SetSize(dataSize);
    if (ATOMID(type) == ATOMID("uuid")) {
        pAtom->SetExtendedType(extendedType);
    }
    if (pAtom->IsUnknownType()) {
        if (!IsReasonableType(pAtom->GetType())) {
            VERBOSE_READ(pFile->GetVerbosity(),
                         printf("Warning: atom type %s is suspect\n", pAtom->GetType()));
        } else {
            VERBOSE_READ(pFile->GetVerbosity(),
                         printf("Info: atom type %s is unknown\n", pAtom->GetType()));
        }

        if (dataSize > 0) {
            pAtom->AddProperty(
                new MP4BytesProperty("data", dataSize));
        }
    }

    pAtom->SetParentAtom(pParentAtom);

	try {
		pAtom->Read();
	}
	catch (MP4Error* e) {
		// delete atom and rethrow so we don't leak memory.
		delete pAtom;	
		throw e;
	}


    return pAtom;
}

bool MP4Atom::IsReasonableType(const char* type)
{
    for (uint8_t i = 0; i < 4; i++) {
        if ((unsigned char) isalnum(type[i])) {
            continue;
        }
        if (i == 3 && type[i] == ' ') {
            continue;
        }
        return false;
    }
    return true;
}

// generic read
void MP4Atom::Read()
{
    ASSERT(m_pFile);

    if (ATOMID(m_type) != 0 && m_size > 1000000) {
        VERBOSE_READ(GetVerbosity(),
                     printf("Warning: %s atom size %" PRIu64 " is suspect\n",
                            m_type, m_size));
    }

    ReadProperties();

    // read child atoms, if we expect there to be some
    if (m_pChildAtomInfos.Size() > 0) {
        ReadChildAtoms();
    }

    Skip(); // to end of atom
}

void MP4Atom::Skip()
{
    if (m_pFile->GetPosition() != m_end) {
        VERBOSE_READ(m_pFile->GetVerbosity(),
                     printf("Skip: %" PRIu64 " bytes\n", m_end - m_pFile->GetPosition()));
    }
    m_pFile->SetPosition(m_end);
}

MP4Atom* MP4Atom::FindAtom(const char* name)
{
    if (!IsMe(name)) {
        return NULL;
    }

    if (!IsRootAtom()) {
        VERBOSE_FIND(m_pFile->GetVerbosity(),
                     printf("FindAtom: matched %s\n", name));

        name = MP4NameAfterFirst(name);

        // I'm the sought after atom
        if (name == NULL) {
            return this;
        }
    }

    // else it's one of my children
    return FindChildAtom(name);
}

bool MP4Atom::FindProperty(const char *name,
                           MP4Property** ppProperty, uint32_t* pIndex)
{
    if (!IsMe(name)) {
        return false;
    }

    if (!IsRootAtom()) {
        VERBOSE_FIND(m_pFile->GetVerbosity(),
                     printf("FindProperty: matched %s\n", name));

        name = MP4NameAfterFirst(name);

        // no property name given
        if (name == NULL) {
            return false;
        }
    }

    return FindContainedProperty(name, ppProperty, pIndex);
}

bool MP4Atom::IsMe(const char* name)
{
    if (name == NULL) {
        return false;
    }

    // root atom always matches
    if (!strcmp(m_type, "")) {
        return true;
    }

    // check if our atom name is specified as the first component
    if (!MP4NameFirstMatches(m_type, name)) {
        return false;
    }

    return true;
}

MP4Atom* MP4Atom::FindChildAtom(const char* name)
{
    uint32_t atomIndex = 0;

    // get the index if we have one, e.g. moov.trak[2].mdia...
    (void)MP4NameFirstIndex(name, &atomIndex);

    // need to get to the index'th child atom of the right type
    for (uint32_t i = 0; i < m_pChildAtoms.Size(); i++) {
        if (MP4NameFirstMatches(m_pChildAtoms[i]->GetType(), name)) {
            if (atomIndex == 0) {
                // this is the one, ask it to match
                return m_pChildAtoms[i]->FindAtom(name);
            }
            atomIndex--;
        }
    }

    return NULL;
}

bool MP4Atom::FindContainedProperty(const char *name,
                                    MP4Property** ppProperty, uint32_t* pIndex)
{
    uint32_t numProperties = m_pProperties.Size();
    uint32_t i;
    // check all of our properties
    for (i = 0; i < numProperties; i++) {
        if (m_pProperties[i]->FindProperty(name, ppProperty, pIndex)) {
            return true;
        }
    }

    // not one of our properties,
    // presumably one of our children's properties
    // check child atoms...

    // check if we have an index, e.g. trak[2].mdia...
    uint32_t atomIndex = 0;
    (void)MP4NameFirstIndex(name, &atomIndex);

    // need to get to the index'th child atom of the right type
    for (i = 0; i < m_pChildAtoms.Size(); i++) {
        if (MP4NameFirstMatches(m_pChildAtoms[i]->GetType(), name)) {
            if (atomIndex == 0) {
                // this is the one, ask it to match
                return m_pChildAtoms[i]->FindProperty(name, ppProperty, pIndex);
            }
            atomIndex--;
        }
    }

    VERBOSE_FIND(m_pFile->GetVerbosity(),
                 printf("FindProperty: no match for %s\n", name));
    return false;
}

void MP4Atom::ReadProperties(uint32_t startIndex, uint32_t count)
{
    uint32_t numProperties = min(count, m_pProperties.Size() - startIndex);

    // read any properties of the atom
    for (uint32_t i = startIndex; i < startIndex + numProperties; i++) {

        m_pProperties[i]->Read(m_pFile);

        if (m_pFile->GetPosition() > m_end) {
            VERBOSE_READ(GetVerbosity(),
                         printf("ReadProperties: insufficient data for property: %s pos 0x%" PRIx64 " atom end 0x%" PRIx64 "\n",
                                m_pProperties[i]->GetName(),
                                m_pFile->GetPosition(), m_end));

            ostringstream oss;
            oss << "atom '" << GetType() << "' is too small; overrun at property: " << m_pProperties[i]->GetName();
            throw new MP4Error( oss.str().c_str(), "Atom ReadProperties" );
        }

        if (m_pProperties[i]->GetType() == TableProperty) {
            VERBOSE_READ_TABLE(GetVerbosity(),
                               printf("Read: "); m_pProperties[i]->Dump(stdout, 0, true));
        } else if (m_pProperties[i]->GetType() != DescriptorProperty) {
            VERBOSE_READ(GetVerbosity(),
                         printf("Read: "); m_pProperties[i]->Dump(stdout, 0, true));
        }
    }
}

void MP4Atom::ReadChildAtoms()
{
    bool this_is_udta = ATOMID(m_type) == ATOMID("udta");

    VERBOSE_READ(GetVerbosity(),
                 printf("ReadChildAtoms: of %s\n", m_type[0] ? m_type : "root"));
    for (uint64_t position = m_pFile->GetPosition();
            position < m_end;
            position = m_pFile->GetPosition()) {
        // make sure that we have enough to read at least 8 bytes
        // size and type.
        if (m_end - position < 2 * sizeof(uint32_t)) {
            // if we're reading udta, it's okay to have 4 bytes of 0
            if (this_is_udta &&
                    m_end - position == sizeof(uint32_t)) {
                uint32_t mbz = m_pFile->ReadUInt32();
                if (mbz != 0) {
                    VERBOSE_WARNING(GetVerbosity(),
                                    printf("Error: In udta atom, end value is not zero %x\n",
                                           mbz));
                }
                continue;
            }
            // otherwise, output a warning, but don't care
            VERBOSE_WARNING(GetVerbosity(),
                            printf("Error: In %s atom, extra %" PRId64 " bytes at end of atom\n",
                                   m_type, (m_end - position)));
            for (uint64_t ix = 0; ix < m_end - position; ix++) {
                (void)m_pFile->ReadUInt8();
            }
            continue;
        }
        MP4Atom* pChildAtom = MP4Atom::ReadAtom(m_pFile, this);

        AddChildAtom(pChildAtom);

        MP4AtomInfo* pChildAtomInfo = FindAtomInfo(pChildAtom->GetType());

        // if child atom is of known type
        // but not expected here print warning
        if (pChildAtomInfo == NULL && !pChildAtom->IsUnknownType()) {
            VERBOSE_READ(GetVerbosity(),
                         printf("Warning: In atom %s unexpected child atom %s\n",
                                GetType(), pChildAtom->GetType()));
        }

        // if child atoms should have just one instance
        // and this is more than one, print warning
        if (pChildAtomInfo) {
            pChildAtomInfo->m_count++;

            if (pChildAtomInfo->m_onlyOne && pChildAtomInfo->m_count > 1) {
                VERBOSE_READ(GetVerbosity(),
                             printf("Warning: In atom %s multiple child atoms %s\n",
                                    GetType(), pChildAtom->GetType()));
            }
        }

    }

    // if mandatory child atom doesn't exist, print warning
    uint32_t numAtomInfo = m_pChildAtomInfos.Size();
    for (uint32_t i = 0; i < numAtomInfo; i++) {
        if (m_pChildAtomInfos[i]->m_mandatory
                && m_pChildAtomInfos[i]->m_count == 0) {
            VERBOSE_READ(GetVerbosity(),
                         printf("Warning: In atom %s missing child atom %s\n",
                                GetType(), m_pChildAtomInfos[i]->m_name));
        }
    }

    VERBOSE_READ(GetVerbosity(),
                 printf("ReadChildAtoms: finished %s\n", m_type));
}

MP4AtomInfo* MP4Atom::FindAtomInfo(const char* name)
{
    uint32_t numAtomInfo = m_pChildAtomInfos.Size();
    for (uint32_t i = 0; i < numAtomInfo; i++) {
        if (ATOMID(m_pChildAtomInfos[i]->m_name) == ATOMID(name)) {
            return m_pChildAtomInfos[i];
        }
    }
    return NULL;
}

// generic write
void MP4Atom::Write()
{
    ASSERT(m_pFile);

    BeginWrite();

    WriteProperties();

    WriteChildAtoms();

    FinishWrite();
}

void MP4Atom::Rewrite()
{
    ASSERT(m_pFile);

    if (!m_end) {
        // This atom hasn't been written yet...
        return;
    }

    uint64_t fPos = m_pFile->GetPosition();
    m_pFile->SetPosition(GetStart());
    Write();
    m_pFile->SetPosition(fPos);
}

void MP4Atom::BeginWrite(bool use64)
{
    m_start = m_pFile->GetPosition();
    //use64 = m_pFile->Use64Bits();
    if (use64) {
        m_pFile->WriteUInt32(1);
    } else {
        m_pFile->WriteUInt32(0);
    }
    m_pFile->WriteBytes((uint8_t*)&m_type[0], 4);
    if (use64) {
        m_pFile->WriteUInt64(0);
    }
    if (ATOMID(m_type) == ATOMID("uuid")) {
        m_pFile->WriteBytes(m_extendedType, sizeof(m_extendedType));
    }
}

void MP4Atom::FinishWrite(bool use64)
{
    m_end = m_pFile->GetPosition();
    m_size = (m_end - m_start);
    VERBOSE_WRITE(GetVerbosity(),
                  printf("end: type %s %" PRIu64 " %" PRIu64 " size %" PRIu64 "\n", m_type,
                         m_start, m_end,
                         m_size));
    //use64 = m_pFile->Use64Bits();
    if (use64) {
        m_pFile->SetPosition(m_start + 8);
        m_pFile->WriteUInt64(m_size);
    } else {
        ASSERT(m_size <= (uint64_t)0xFFFFFFFF);
        m_pFile->SetPosition(m_start);
        m_pFile->WriteUInt32(m_size);
    }
    m_pFile->SetPosition(m_end);

    // adjust size to just reflect data portion of atom
    m_size -= (use64 ? 16 : 8);
    if (ATOMID(m_type) == ATOMID("uuid")) {
        m_size -= sizeof(m_extendedType);
    }
}

void MP4Atom::WriteProperties(uint32_t startIndex, uint32_t count)
{
    uint32_t numProperties = min(count, m_pProperties.Size() - startIndex);

    VERBOSE_WRITE(GetVerbosity(),
                  printf("Write: type %s\n", m_type));

    for (uint32_t i = startIndex; i < startIndex + numProperties; i++) {
        m_pProperties[i]->Write(m_pFile);

        if (m_pProperties[i]->GetType() == TableProperty) {
            VERBOSE_WRITE_TABLE(GetVerbosity(),
                                printf("Write: "); m_pProperties[i]->Dump(stdout, 0, false));
        } else {
            VERBOSE_WRITE(GetVerbosity(),
                          printf("Write: "); m_pProperties[i]->Dump(stdout, 0, false));
        }
    }
}

void MP4Atom::WriteChildAtoms()
{
    uint32_t size = m_pChildAtoms.Size();
    for (uint32_t i = 0; i < size; i++) {
        m_pChildAtoms[i]->Write();
    }

    VERBOSE_WRITE(GetVerbosity(),
                  printf("Write: finished %s\n", m_type));
}

void MP4Atom::AddProperty(MP4Property* pProperty)
{
    ASSERT(pProperty);
    m_pProperties.Add(pProperty);
    pProperty->SetParentAtom(this);
}

void MP4Atom::AddVersionAndFlags()
{
    AddProperty(new MP4Integer8Property("version"));
    AddProperty(new MP4Integer24Property("flags"));
}

void MP4Atom::AddReserved(const char* name, uint32_t size)
{
    MP4BytesProperty* pReserved = new MP4BytesProperty(name, size);
    pReserved->SetReadOnly();
    AddProperty(pReserved);
}

void MP4Atom::ExpectChildAtom(const char* name, bool mandatory, bool onlyOne)
{
    m_pChildAtomInfos.Add(new MP4AtomInfo(name, mandatory, onlyOne));
}

uint8_t MP4Atom::GetVersion()
{
    if (strcmp("version", m_pProperties[0]->GetName())) {
        return 0;
    }
    return ((MP4Integer8Property*)m_pProperties[0])->GetValue();
}

void MP4Atom::SetVersion(uint8_t version)
{
    if (strcmp("version", m_pProperties[0]->GetName())) {
        return;
    }
    ((MP4Integer8Property*)m_pProperties[0])->SetValue(version);
}

uint32_t MP4Atom::GetFlags()
{
    if (strcmp("flags", m_pProperties[1]->GetName())) {
        return 0;
    }
    return ((MP4Integer24Property*)m_pProperties[1])->GetValue();
}

void MP4Atom::SetFlags(uint32_t flags)
{
    if (strcmp("flags", m_pProperties[1]->GetName())) {
        return;
    }
    ((MP4Integer24Property*)m_pProperties[1])->SetValue(flags);
}

void MP4Atom::Dump(FILE* pFile, uint8_t indent, bool dumpImplicits)
{
    if ( m_type[0] != '\0' ) {
        // create list of ancestors
        list<string> tlist;
        for( MP4Atom* atom = this; atom; atom = atom->GetParentAtom() ) {
            const char* const type = atom->GetType();
            if( type && type[0] != '\0' )
                tlist.push_front( type );
        }

        // create contextual atom-name
        string can;
        const list<string>::iterator ie = tlist.end();
        for( list<string>::iterator it = tlist.begin(); it != ie; it++ )
            can += *it + '.';
        if( can.length() )
            can.resize( can.length() - 1 );

        Indent( pFile, indent );
        fprintf( pFile, "type %s (%s)\n", m_type, can.c_str() );
        fflush( pFile );
    }

    uint32_t i;
    uint32_t size;

    // dump our properties
    size = m_pProperties.Size();
    for (i = 0; i < size; i++) {

        /* skip details of tables unless we're told to be verbose */
        if (m_pProperties[i]->GetType() == TableProperty
                && !(GetVerbosity() & MP4_DETAILS_TABLE)) {
            Indent(pFile, indent + 1);
            fprintf(pFile, "<table entries suppressed>\n");
            continue;
        }

        m_pProperties[i]->Dump(pFile, indent + 1, dumpImplicits);
    }

    // dump our children
    size = m_pChildAtoms.Size();
    for (i = 0; i < size; i++) {
        m_pChildAtoms[i]->Dump(pFile, indent + 1, dumpImplicits);
    }
}

uint32_t MP4Atom::GetVerbosity()
{
    ASSERT(m_pFile);
    return m_pFile->GetVerbosity();
}

uint8_t MP4Atom::GetDepth()
{
    if (m_depth < 0xFF) {
        return m_depth;
    }

    MP4Atom *pAtom = this;
    m_depth = 0;

    while ((pAtom = pAtom->GetParentAtom()) != NULL) {
        m_depth++;
        ASSERT(m_depth < 255);
    }
    return m_depth;
}

bool MP4Atom::GetLargesizeMode()
{
    return m_largesizeMode;
}

void MP4Atom::SetLargesizeMode( bool mode )
{
    m_largesizeMode = mode;
}

bool
MP4Atom::descendsFrom( MP4Atom* parent, const char* type )
{
    const uint32_t id = ATOMID( type );
    for( MP4Atom* atom = parent; atom; atom = atom->GetParentAtom() ) {
        if( id == ATOMID(atom->GetType()) )
            return true;
    }
    return false;
}

// UDTA child atom types to be constructed as MP4UdtaElementAtom.
// List gleaned from QTFF 2007-09-04.
static const char* const UDTA_ELEMENTS[] = {
    "\xA9" "arg",
    "\xA9" "ark",
    "\xA9" "cok",
    "\xA9" "com",
    "\xA9" "cpy",
    "\xA9" "day",
    "\xA9" "dir",
    "\xA9" "ed1",
    "\xA9" "ed2",
    "\xA9" "ed3",
    "\xA9" "ed4",
    "\xA9" "ed5",
    "\xA9" "ed6",
    "\xA9" "ed7",
    "\xA9" "ed8",
    "\xA9" "ed9",
    "\xA9" "fmt",
    "\xA9" "inf",
    "\xA9" "isr",
    "\xA9" "lab",
    "\xA9" "lal",
    "\xA9" "mak",
    "\xA9" "nak",
    "\xA9" "nam",
    "\xA9" "pdk",
    "\xA9" "phg",
    "\xA9" "prd",
    "\xA9" "prf",
    "\xA9" "prk",
    "\xA9" "prl",
    "\xA9" "req",
    "\xA9" "snk",
    "\xA9" "snm",
    "\xA9" "src",
    "\xA9" "swf",
    "\xA9" "swk",
    "\xA9" "swr",
    "\xA9" "wrt",
    "Allf",
    "hinf",
    "hnti",
    "name",
    "LOOP",
    "ptv ",
    "SelO",
    "WLOC",
    NULL // must be last
};

MP4Atom*
MP4Atom::factory( MP4Atom* parent, const char* type )
{
    // type may be NULL only in case of root-atom
    if( !type )
        return new MP4RootAtom();

    // construct atoms which are context-savvy
    if( parent ) {
        const char* const ptype = parent->GetType();

        if( descendsFrom( parent, "ilst" )) {
            if( ATOMID( ptype ) == ATOMID( "ilst" ))
                return new MP4ItemAtom( type );

            if( ATOMID( type ) == ATOMID( "data" ))
                return new MP4DataAtom();

            if( ATOMID( ptype ) == ATOMID( "----" )) {
                if( ATOMID( type ) == ATOMID( "mean" ))
                    return new MP4MeanAtom();
                if( ATOMID( type ) == ATOMID( "name" ))
                    return new MP4NameAtom();
            }
        }
        else if( ATOMID( ptype ) == ATOMID( "meta" )) {
            if( ATOMID( type ) == ATOMID( "hdlr" ))
                return new MP4ItmfHdlrAtom();
        }
        else if( ATOMID( ptype ) == ATOMID( "udta" )) {
            for( const char* const* p = UDTA_ELEMENTS; *p; p++ )
                if( !strcmp( type, *p ))
                    return new MP4UdtaElementAtom( type );
        }
    }

    // no-context construction (old-style)
    switch( (uint8_t)type[0] ) {
        case 'S':
            if( ATOMID(type) == ATOMID("SVQ3") )
                return new MP4VideoAtom( type );
            if( ATOMID(type) == ATOMID("SMI ") )
                return new MP4SmiAtom();
            break;

        case 'a':
            if( ATOMID(type) == ATOMID("avc1") )
                return new MP4Avc1Atom();
            if( ATOMID(type) == ATOMID("ac-3") )
                return new MP4Ac3Atom();
            if( ATOMID(type) == ATOMID("avcC") )
                return new MP4AvcCAtom();
            if( ATOMID(type) == ATOMID("alis") )
                return new MP4UrlAtom( type );
            if( ATOMID(type) == ATOMID("alaw") )
                return new MP4SoundAtom( type );
            if( ATOMID(type) == ATOMID("alac") )
                return new MP4SoundAtom( type );
            break;

        case 'c':
            if( ATOMID(type) == ATOMID("chap") )
                return new MP4TrefTypeAtom( type );
            if( ATOMID(type) == ATOMID("chpl") )
                return new MP4ChplAtom();
            if( ATOMID(type) == ATOMID("colr") )
                return new MP4ColrAtom();
            break;

        case 'd':
            if( ATOMID(type) == ATOMID("d263") )
                return new MP4D263Atom();
            if( ATOMID(type) == ATOMID("damr") )
                return new MP4DamrAtom();
            if( ATOMID(type) == ATOMID("dref") )
                return new MP4DrefAtom();
            if( ATOMID(type) == ATOMID("dpnd") )
                return new MP4TrefTypeAtom( type );
            if( ATOMID(type) == ATOMID("dac3") )
                return new MP4DAc3Atom();
            break;

        case 'e':
            if( ATOMID(type) == ATOMID("elst") )
                return new MP4ElstAtom();
            if( ATOMID(type) == ATOMID("enca") )
                return new MP4EncaAtom();
            if( ATOMID(type) == ATOMID("encv") )
                return new MP4EncvAtom();
            break;

        case 'f':
            if( ATOMID(type) == ATOMID("free") )
                return new MP4FreeAtom();
            if( ATOMID(type) == ATOMID("ftyp") )
                return new MP4FtypAtom();
            if( ATOMID(type) == ATOMID("ftab") )
                return new MP4FtabAtom();
            break;

        case 'g':
            if( ATOMID(type) == ATOMID("gmin") )
                return new MP4GminAtom();
            break;

        case 'h':
            if( ATOMID(type) == ATOMID("hdlr") )
                return new MP4HdlrAtom();
            if( ATOMID(type) == ATOMID("hint") )
                return new MP4TrefTypeAtom( type );
            if( ATOMID(type) == ATOMID("hnti") )
                return new MP4HntiAtom();
            if( ATOMID(type) == ATOMID("hinf") )
                return new MP4HinfAtom();
            if( ATOMID(type) == ATOMID("h263") )
                return new MP4VideoAtom( type );
            if( ATOMID(type) == ATOMID("href") )
                return new MP4HrefAtom();
            break;

        case 'i':
            if( ATOMID(type) == ATOMID("ipir") )
                return new MP4TrefTypeAtom( type );
            if( ATOMID(type) == ATOMID("ima4") )
                return new MP4SoundAtom( type );
            break;

        case 'j':
            if( ATOMID(type) == ATOMID("jpeg") )
                return new MP4VideoAtom("jpeg");
            break;

        case 'm':
            if( ATOMID(type) == ATOMID("mdhd") )
                return new MP4MdhdAtom();
            if( ATOMID(type) == ATOMID("mvhd") )
                return new MP4MvhdAtom();
            if( ATOMID(type) == ATOMID("mdat") )
                return new MP4MdatAtom();
            if( ATOMID(type) == ATOMID("mpod") )
                return new MP4TrefTypeAtom( type );
            if( ATOMID(type) == ATOMID("mp4a") )
                return new MP4SoundAtom( type );
            if( ATOMID(type) == ATOMID("mp4s") )
                return new MP4Mp4sAtom();
            if( ATOMID(type) == ATOMID("mp4v") )
                return new MP4Mp4vAtom();
            break;

        case 'n':
            if( ATOMID(type) == ATOMID("nmhd") )
                return new MP4NmhdAtom();
            break;

        case 'o':
            if( ATOMID(type) == ATOMID("ohdr") )
                return new MP4OhdrAtom();
            break;

        case 'p':
            if( ATOMID(type) == ATOMID("pasp") )
                return new MP4PaspAtom();
            break;

        case 'r':
            if( ATOMID(type) == ATOMID("rtp ") )
                return new MP4RtpAtom();
            if( ATOMID(type) == ATOMID("raw ") )
                return new MP4VideoAtom( type );
            break;

        case 's':
            if( ATOMID(type) == ATOMID("s263") )
                return new MP4S263Atom();
            if( ATOMID(type) == ATOMID("samr") )
                return new MP4AmrAtom( type );
            if( ATOMID(type) == ATOMID("sawb") )
                return new MP4AmrAtom( type );
            if( ATOMID(type) == ATOMID("sdtp") )
                return new MP4SdtpAtom();
            if( ATOMID(type) == ATOMID("stbl") )
                return new MP4StblAtom();
            if( ATOMID(type) == ATOMID("stsd") )
                return new MP4StsdAtom();
            if( ATOMID(type) == ATOMID("stsz") )
                return new MP4StszAtom();
            if( ATOMID(type) == ATOMID("stsc") )
                return new MP4StscAtom();
            if( ATOMID(type) == ATOMID("stz2") )
                return new MP4Stz2Atom();
            if( ATOMID(type) == ATOMID("stdp") )
                return new MP4StdpAtom();
            if( ATOMID(type) == ATOMID("sdp ") )
                return new MP4SdpAtom();
            if( ATOMID(type) == ATOMID("sync") )
                return new MP4TrefTypeAtom( type );
            if( ATOMID(type) == ATOMID("skip") )
                return new MP4FreeAtom( type );
            if (ATOMID(type) == ATOMID("sowt") )
                return new MP4SoundAtom( type );
            break;

        case 't':
            if( ATOMID(type) == ATOMID("text") )
                return new MP4TextAtom();
            if( ATOMID(type) == ATOMID("tx3g") )
                return new MP4Tx3gAtom();
            if( ATOMID(type) == ATOMID("tkhd") )
                return new MP4TkhdAtom();
            if( ATOMID(type) == ATOMID("tfhd") )
                return new MP4TfhdAtom();
            if( ATOMID(type) == ATOMID("trun") )
                return new MP4TrunAtom();
            if( ATOMID(type) == ATOMID("twos") )
                return new MP4SoundAtom( type );
            break;

        case 'u':
            if( ATOMID(type) == ATOMID("udta") )
                return new MP4UdtaAtom();
            if( ATOMID(type) == ATOMID("url ") )
                return new MP4UrlAtom();
            if( ATOMID(type) == ATOMID("urn ") )
                return new MP4UrnAtom();
            if( ATOMID(type) == ATOMID("ulaw") )
                return new MP4SoundAtom( type );
            break;

        case 'v':
            if( ATOMID(type) == ATOMID("vmhd") )
                return new MP4VmhdAtom();
            break;

        case 'y':
            if( ATOMID(type) == ATOMID("yuv2") )
                return new MP4VideoAtom( type );
            break;

        default:
            break;
    }

    // default to MP4StandardAtom implementation
    return new MP4StandardAtom( type ); 
}

///////////////////////////////////////////////////////////////////////////////

}} // namespace mp4v2::impl
