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

void MP4RootAtom::BeginWrite()
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

    m_pChildAtoms[GetLastMdatIndex()]->BeginWrite();
}

void MP4RootAtom::Write()
{
    // no-op
}

void MP4RootAtom::FinishWrite()
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
    m_pChildAtoms[mdatIndex]->FinishWrite();

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

    /*
     * Since stco<->co64 can switch due to chunk relocation,
     * moov size can change.
     * We reserve enough free space for moov size growth.
     */
    uint32_t room_size = 1024;
    uint32_t ntracks = m_File.GetNumberOfTracks();
    for (uint32_t i = 0; i < ntracks; ++i) {
        MP4TrackId id = m_File.FindTrackId(i);
        MP4Atom *stco = m_File.FindTrackAtom(id, "mdia.minf.stbl.stco");
        if (stco) {
            MP4Property *prop;
            stco->FindProperty("stco.entryCount", &prop);
            uint32_t cnt = dynamic_cast<MP4IntegerProperty*>(prop)->GetValue();
            room_size += cnt * 4;
        }
    }
    m_File.WriteUInt32(room_size + 8);
    m_File.WriteBytes((uint8_t*)"free", 4);
    uint64_t pos = m_File.GetPosition();
    m_File.SetPosition(pos + room_size);

    m_pChildAtoms[GetLastMdatIndex()]->BeginWrite();
}

void MP4RootAtom::FinishOptimalWrite()
{
    // finish writing mdat
    m_pChildAtoms[GetLastMdatIndex()]->FinishWrite();

    // find moov atom
    MP4Atom* pMoovAtom = FindChildAtom("moov");

    // rewrite moov so that updated chunkOffsets are written to disk
    m_File.SetPosition(pMoovAtom->GetStart());
    uint64_t oldSize = pMoovAtom->GetSize();

    pMoovAtom->Write();
    if (pMoovAtom->GetSize() != oldSize)
        WriteAtomType("udta", Many);

    uint64_t pos = m_File.GetPosition();
    uint64_t mdat_start = m_pChildAtoms[GetLastMdatIndex()]->GetStart();
    ASSERT(pos == mdat_start || (mdat_start > pos && mdat_start - pos >= 8));
    if (mdat_start > pos) {
        m_File.WriteUInt32(mdat_start - pos);
        m_File.WriteBytes((uint8_t*)"free", 4);
    }
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
