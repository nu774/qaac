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

MP4MvhdAtom::MP4MvhdAtom(MP4File &file)
        : MP4Atom(file, "mvhd")
{
    AddVersionAndFlags();
}

void MP4MvhdAtom::AddProperties(uint8_t version)
{
    MP4Integer6432Property *p;

    p = new MP4Integer6432Property(*this, "creationTime");
    p->Use64Bit(version == 1);
    AddProperty(p);

    p = new MP4Integer6432Property(*this, "modificationTime");
    p->Use64Bit(version == 1);
    AddProperty(p);

    AddProperty(new MP4Integer32Property(*this, "timeScale"));

    p = new MP4Integer6432Property(*this, "duration");
    p->Use64Bit(version == 1);
    AddProperty(p);

    MP4Float32Property* pProp;

    pProp = new MP4Float32Property(*this, "rate");
    pProp->SetFixed32Format();
    AddProperty(pProp); /* 6 */

    pProp = new MP4Float32Property(*this, "volume");
    pProp->SetFixed16Format();
    AddProperty(pProp); /* 7 */

    AddReserved(*this, "reserved1", 70); /* 8 */

    AddProperty( /* 9 */
        new MP4Integer32Property(*this, "nextTrackId"));
}

void MP4MvhdAtom::Generate()
{
    SetVersion(0);
    AddProperties(0);

    MP4Atom::Generate();

    // set creation and modification times
    MP4Timestamp now = MP4GetAbsTimestamp();
    ((MP4Integer6432Property*)m_pProperties[2])->SetValue(now);
    ((MP4Integer6432Property*)m_pProperties[3])->SetValue(now);

    ((MP4Integer32Property*)m_pProperties[4])->SetValue(1000);

    ((MP4Float32Property*)m_pProperties[6])->SetValue(1.0);
    ((MP4Float32Property*)m_pProperties[7])->SetValue(1.0);

    // property reserved has non-zero fixed values
    static uint8_t reserved[70] = {
        0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x40, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
    };
    m_pProperties[8]->SetReadOnly(false);
    ((MP4BytesProperty*)m_pProperties[8])->
    SetValue(reserved, sizeof(reserved));
    m_pProperties[8]->SetReadOnly(true);

    // set next track id
    ((MP4Integer32Property*)m_pProperties[9])->SetValue(1);
}

void MP4MvhdAtom::Read()
{
    /* read atom version */
    ReadProperties(0, 1);

    /* need to create the properties based on the atom version */
    AddProperties(GetVersion());

    /* now we can read the remaining properties */
    ReadProperties(1);

    Skip(); // to end of atom
}

void MP4MvhdAtom::BeginWrite()
{
    if (((MP4Integer6432Property*)m_pProperties[2])->GetValue() > 0xffffffff ||
        ((MP4Integer6432Property*)m_pProperties[3])->GetValue() > 0xffffffff ||
        ((MP4Integer6432Property*)m_pProperties[5])->GetValue() > 0xffffffff)
    {
        SetVersion(1);
        ((MP4Integer6432Property*)m_pProperties[2])->Use64Bit(true);
        ((MP4Integer6432Property*)m_pProperties[3])->Use64Bit(true);
        ((MP4Integer6432Property*)m_pProperties[5])->Use64Bit(true);
    }
    MP4Atom::BeginWrite();
}

///////////////////////////////////////////////////////////////////////////////

}
} // namespace mp4v2::impl
