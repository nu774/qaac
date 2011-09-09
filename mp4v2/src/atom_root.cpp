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

namespace mp4v2 { namespace impl {

///////////////////////////////////////////////////////////////////////////////

MP4RootAtom::MP4RootAtom(MP4File &file)
    : MP4Atom( file, NULL )
    , m_rewrite_ftyp         ( NULL )
    , m_rewrite_ftypPosition ( 0 )
    , m_rewrite_free         ( NULL )
    , m_rewrite_freePosition ( 0 )
{
    ExpectChildAtom( "moov", Required, OnlyOne );
    ExpectChildAtom( "ftyp", Optional, OnlyOne );
    ExpectChildAtom( "mdat", Optional, Many );
    ExpectChildAtom( "free", Optional, Many );
    ExpectChildAtom( "skip", Optional, Many );
    ExpectChildAtom( "udta", Optional, Many );
    ExpectChildAtom( "moof", Optional, Many );
}

void MP4RootAtom::BeginWrite(bool use64)
{
    m_rewrite_ftyp = (MP4FtypAtom*)FindChildAtom( "ftyp" );
    if( m_rewrite_ftyp ) {
        m_rewrite_free = (MP4FreeAtom*)MP4Atom::CreateAtom( m_File, NULL, "free" );
        m_rewrite_free->SetSize( 32*4 ); // room for 32 additional brands
        AddChildAtom( m_rewrite_free );

        m_rewrite_ftypPosition = m_File.GetPosition();
        m_rewrite_ftyp->Write();

        m_rewrite_freePosition = m_File.GetPosition();
        m_rewrite_free->Write();
    }

    m_pChildAtoms[GetLastMdatIndex()]->BeginWrite( m_File.Use64Bits( "mdat" ));
}

void MP4RootAtom::Write()
{
    // no-op
}

void MP4RootAtom::FinishWrite(bool use64)
{
    if( m_rewrite_ftyp ) {
        const uint64_t savepos = m_File.GetPosition();
        m_File.SetPosition( m_rewrite_ftypPosition );
        m_rewrite_ftyp->Write();

        const uint64_t newpos = m_File.GetPosition();
        if( newpos > m_rewrite_freePosition )
            m_rewrite_free->SetSize( m_rewrite_free->GetSize() - (newpos - m_rewrite_freePosition) ); // shrink
        else if( newpos < m_rewrite_freePosition )
            m_rewrite_free->SetSize( m_rewrite_free->GetSize() + (m_rewrite_freePosition - newpos) ); // grow

        m_rewrite_free->Write();
        m_File.SetPosition( savepos );
    }

    // finish writing last mdat atom
    const uint32_t mdatIndex = GetLastMdatIndex();
    m_pChildAtoms[mdatIndex]->FinishWrite( m_File.Use64Bits( "mdat" ));

    // write all atoms after last mdat
    const uint32_t size = m_pChildAtoms.Size();
    for ( uint32_t i = mdatIndex + 1; i < size; i++ )
        m_pChildAtoms[i]->Write();
}

void MP4RootAtom::BeginOptimalWrite()
{
    WriteAtomType("ftyp", OnlyOne);
    WriteAtomType("moov", OnlyOne);
    WriteAtomType("udta", Many);

    m_pChildAtoms[GetLastMdatIndex()]->BeginWrite(m_File.Use64Bits("mdat"));
}

void MP4RootAtom::FinishOptimalWrite()
{
    // finish writing mdat
    m_pChildAtoms[GetLastMdatIndex()]->FinishWrite(m_File.Use64Bits("mdat"));

    // find moov atom
    uint32_t size = m_pChildAtoms.Size();
    MP4Atom* pMoovAtom = NULL;

    uint32_t i;
    for (i = 0; i < size; i++) {
        if (!strcmp("moov", m_pChildAtoms[i]->GetType())) {
            pMoovAtom = m_pChildAtoms[i];
            break;
        }
    }
    ASSERT(i < size);
    ASSERT(pMoovAtom != NULL);

    // rewrite moov so that updated chunkOffsets are written to disk
    m_File.SetPosition(pMoovAtom->GetStart());
    uint64_t oldSize = pMoovAtom->GetSize();

    pMoovAtom->Write();

    // sanity check
    uint64_t newSize = pMoovAtom->GetSize();
    ASSERT(oldSize == newSize);
}

uint32_t MP4RootAtom::GetLastMdatIndex()
{
    for (int32_t i = m_pChildAtoms.Size() - 1; i >= 0; i--) {
        if (!strcmp("mdat", m_pChildAtoms[i]->GetType())) {
            return i;
        }
    }
    ASSERT(false);
    return (uint32_t)-1;
}

void MP4RootAtom::WriteAtomType(const char* type, bool onlyOne)
{
    uint32_t size = m_pChildAtoms.Size();

    for (uint32_t i = 0; i < size; i++) {
        if (!strcmp(type, m_pChildAtoms[i]->GetType())) {
            m_pChildAtoms[i]->Write();
            if (onlyOne) {
                break;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

}} // namespace mp4v2::impl
