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

MP4TkhdAtom::MP4TkhdAtom(MP4File &file)
        : MP4Atom(file, "tkhd")
{
    AddVersionAndFlags();
}

void MP4TkhdAtom::AddProperties(uint8_t version)
{
    MP4Integer6432Property *p;

    p = new MP4Integer6432Property(*this, "creationTime");
    p->Use64Bit(version == 1);
    AddProperty(p);

    p = new MP4Integer6432Property(*this, "modificationTime");
    p->Use64Bit(version == 1);
    AddProperty(p);

    AddProperty( /* 4 */
        new MP4Integer32Property(*this, "trackId"));
    AddReserved(*this, "reserved1", 4); /* 5 */

    p = new MP4Integer6432Property(*this, "duration");
    p->Use64Bit(version == 1);
    AddProperty(p);

    AddReserved(*this, "reserved2", 8); /* 7 */

    AddProperty( /* 8 */
        new MP4Integer16Property(*this, "layer"));
    AddProperty( /* 9 */
        new MP4Integer16Property(*this, "alternate_group"));

    MP4Float32Property* pProp;

    pProp = new MP4Float32Property(*this, "volume");
    pProp->SetFixed16Format();
    AddProperty(pProp); /* 10 */

    AddReserved(*this, "reserved3", 2); /* 11 */

    AddProperty(new MP4BytesProperty(*this, "matrix", 36)); /* 12 */

    pProp = new MP4Float32Property(*this, "width");
    pProp->SetFixed32Format();
    AddProperty(pProp); /* 13 */

    pProp = new MP4Float32Property(*this, "height");
    pProp->SetFixed32Format();
    AddProperty(pProp); /* 14 */
}

void MP4TkhdAtom::Generate()
{
    SetVersion(0);
    AddProperties(0);

    MP4Atom::Generate();

    // set creation and modification times
    MP4Timestamp now = MP4GetAbsTimestamp();
    ((MP4Integer6432Property*)m_pProperties[2])->SetValue(now);
    ((MP4Integer6432Property*)m_pProperties[3])->SetValue(now);

    // property "matrix" has non-zero fixed values
    // this default identity matrix indicates no transformation, i.e.
    // 1, 0, 0
    // 0, 1, 0
    // 0, 0, 1
    // see http://developer.apple.com/documentation/QuickTime/QTFF/QTFFChap4/chapter_5_section_4.html

    static uint8_t matrix[36] = {
        0x00, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x40, 0x00, 0x00, 0x00,
    };

    ((MP4BytesProperty*)m_pProperties[12])->
    SetValue(matrix, sizeof(matrix));
}

void MP4TkhdAtom::Read()
{
    /* read atom version */
    ReadProperties(0, 1);

    /* need to create the properties based on the atom version */
    AddProperties(GetVersion());

    /* now we can read the remaining properties */
    ReadProperties(1);

    Skip(); // to end of atom
}

void MP4TkhdAtom::BeginWrite()
{
    if (((MP4Integer6432Property*)m_pProperties[2])->GetValue() > 0xffffffff ||
        ((MP4Integer6432Property*)m_pProperties[3])->GetValue() > 0xffffffff ||
        ((MP4Integer6432Property*)m_pProperties[6])->GetValue() > 0xffffffff)
    {
        SetVersion(1);
        ((MP4Integer6432Property*)m_pProperties[2])->Use64Bit(true);
        ((MP4Integer6432Property*)m_pProperties[3])->Use64Bit(true);
        ((MP4Integer6432Property*)m_pProperties[6])->Use64Bit(true);
    }
    MP4Atom::BeginWrite();
}

///////////////////////////////////////////////////////////////////////////////

}
} // namespace mp4v2::impl
