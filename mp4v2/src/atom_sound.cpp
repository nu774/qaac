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
 * Copyright (C) Cisco Systems Inc. 2004.  All Rights Reserved.
 *
 * Contributor(s):
 *      Bill May        wmay@cisco.com
 */

#include "src/impl.h"

namespace mp4v2 {
namespace impl {

///////////////////////////////////////////////////////////////////////////////

MP4SoundAtom::MP4SoundAtom(MP4File &file, const char *atomid)
        : MP4Atom(file, atomid)
{
    AddReserved(*this, "reserved1", 6); /* 0 */

    AddProperty( /* 1 */
        new MP4Integer16Property(*this, "dataReferenceIndex"));
    AddProperty( /* 2 */
        new MP4Integer16Property(*this, "soundVersion"));
    AddReserved( *this, "reserved2", 6); /* 3 */

    if (ATOMID(atomid) == ATOMID("mp4a")) {
        ExpectChildAtom("esds", Required, OnlyOne);
        ExpectChildAtom("wave", Optional, OnlyOne);
    } else if (ATOMID(atomid) == ATOMID("alac")) {
        ExpectChildAtom("alac", Optional, Optional);
        //AddProperty( new MP4BytesProperty(*this, "alacInfo", 36));
    } else if (ATOMID(atomid) == ATOMID("fLaC")) {
        ExpectChildAtom("dfLa", Optional, Optional);
    } else if (ATOMID(atomid) == ATOMID("Opus")) {
        ExpectChildAtom("dOps", Optional, Optional);
    }
}

void MP4SoundAtom::AddProperties (uint8_t version)
{
    if (version < 2) {
        AddProperty( /* 4 */
            new MP4Integer16Property(*this, "channels"));
        AddProperty( /* 5 */
            new MP4Integer16Property(*this, "sampleSize"));
        AddProperty( /* 6 */
            new MP4Integer16Property(*this, "compressionId"));
        AddProperty( /* 7 */
            new MP4Integer16Property(*this, "packetSize"));
        AddProperty( /* 8 */
            new MP4Integer32Property(*this, "timeScale"));
    }
    if (version == 1) {
        AddProperty( /* 9 */
            new MP4Integer32Property(*this, "samplesPerPacket"));
        AddProperty( /* 10 */
            new MP4Integer32Property(*this, "bytesPerPacket"));
        AddProperty( /* 11 */
            new MP4Integer32Property(*this, "bytesPerFrame"));
        AddProperty( /* 12 */
            new MP4Integer32Property(*this, "bytesPerSample"));
    }
    if (version == 2) {
        AddProperty( /* 4 */
            new MP4Integer16Property(*this, "always3"));
        AddProperty( /* 5 */
            new MP4Integer16Property(*this, "always16"));
        AddProperty( /* 6 */
            new MP4Integer16Property(*this, "alwaysMinus2"));
        AddProperty( /* 7 */
            new MP4Integer16Property(*this, "always0"));
        AddProperty( /* 8 */
            new MP4Integer32Property(*this, "always65536"));
        AddProperty( /* 9 */
            new MP4Integer32Property(*this, "sizeOfStructOnly"));
        AddProperty( /* 10 */
            new MP4Float64Property(*this, "timeScale"));
        AddProperty( /* 11 */
            new MP4Integer32Property(*this, "channels")); //numAudioChannels
        AddProperty( /* 12 */
            new MP4Integer32Property(*this, "always7F000000"));
        AddProperty( /* 13 */
            new MP4Integer32Property(*this, "constBitsPerChannel"));
        AddProperty( /* 14 */
            new MP4Integer32Property(*this, "formatSpecificFlags"));
        AddProperty( /* 15 */
            new MP4Integer32Property(*this, "constBytesPerAudioPacket"));
        AddProperty( /* 16 */
            new MP4Integer32Property(*this, "constLPCMFramesPerAudioPacket"));
    }
}
void MP4SoundAtom::Generate()
{
    const uint8_t Version = ATOMID(GetType()) == ATOMID("lpcm") ? 2 : 0;

    MP4Atom::Generate();

    ((MP4Integer16Property*)m_pProperties[1])->SetValue(1);
    ((MP4Integer16Property*)m_pProperties[2])->SetValue(Version);

    AddProperties(Version);

    // property reserved2 has non-zero fixed values
    static const uint8_t reserved2[6] = {
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00,
    };
    m_pProperties[3]->SetReadOnly(false);
    ((MP4BytesProperty*)m_pProperties[3])->SetValue(reserved2,
                                                    sizeof(reserved2));
    m_pProperties[3]->SetReadOnly(true);

    if (Version < 2) {
        ((MP4Integer16Property*)m_pProperties[4])->SetValue(2);
        ((MP4Integer16Property*)m_pProperties[5])->SetValue(0x0010);
        ((MP4Integer16Property*)m_pProperties[6])->SetValue(0);
    } else {
        ((MP4Integer16Property*)m_pProperties[4])->SetValue(3);
        ((MP4Integer16Property*)m_pProperties[5])->SetValue(16);
        ((MP4Integer16Property*)m_pProperties[6])->SetValue(0xFFFE);
        ((MP4Integer16Property*)m_pProperties[7])->SetValue(0);
        ((MP4Integer32Property*)m_pProperties[8])->SetValue(65536);
        ((MP4Integer32Property*)m_pProperties[9])->SetValue(72);
        ((MP4Integer32Property*)m_pProperties[12])->SetValue(0x7F000000);
        ((MP4Integer32Property*)m_pProperties[16])->SetValue(1);
    }
}

void MP4SoundAtom::Read()
{
    MP4Atom *parent = GetParentAtom();
    if (ATOMID(parent->GetType()) != ATOMID("stsd")) {
        // Quicktime has an interesting thing - they'll put an mp4a atom
        // which is blank inside a wave atom, which is inside an mp4a atom
        // we have a mp4a inside an wave inside an mp4a - delete all properties
        MP4ArrayIndex propCnt = m_pProperties.Size();
        for(MP4ArrayIndex i = 0; i < propCnt; ++i)
            delete m_pProperties[i];	// make sure we delete the properties themselves, then remove from  m_pProperties

        while (propCnt--)
            m_pProperties.Delete(propCnt);

        if (ATOMID(GetType()) == ATOMID("alac")) {
            AddProperty(new MP4BytesProperty(*this, "decoderConfig", m_size));
            ReadProperties();
        }
        if (m_pChildAtomInfos.Size() > 0) {
            ReadChildAtoms();
        }
    } else {
        ReadProperties(0, 3); // read first 3 properties
        AddProperties(((MP4IntegerProperty *)m_pProperties[2])->GetValue());
        ReadProperties(3); // continue
        if (m_pChildAtomInfos.Size() > 0) {
            ReadChildAtoms();
        }
    }
    Skip();
}

///////////////////////////////////////////////////////////////////////////////

}
} // namespace mp4v2::impl
