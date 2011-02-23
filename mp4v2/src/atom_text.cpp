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
 * Contributer has declined to give copyright information, and gives
 * it freely to the world.
 */

#include "src/impl.h"

namespace mp4v2 {
namespace impl {

///////////////////////////////////////////////////////////////////////////////

MP4TextAtom::MP4TextAtom(MP4File &file)
        : MP4Atom(file, "text")
{
    // The atom type "text" is used in two complete unrelated ways
    // i.e. it's real two atoms with the same name
    // To handle that we need to postpone property creation until
    // we know who our parent atom is (stsd or gmhd) which gives us
    // the context info we need to know who we are
}

void MP4TextAtom::AddPropertiesStsdType()
{

    AddReserved(*this, "reserved1", 6); /* 0 */

    AddProperty(new MP4Integer16Property(*this, "dataReferenceIndex"));/* 1 */

    AddProperty(new MP4Integer32Property(*this, "displayFlags")); /* 2 */
    AddProperty(new MP4Integer32Property(*this, "textJustification")); /* 3 */

    AddProperty(new MP4Integer16Property(*this, "bgColorRed")); /* 4 */
    AddProperty(new MP4Integer16Property(*this, "bgColorGreen")); /* 5 */
    AddProperty(new MP4Integer16Property(*this, "bgColorBlue")); /* 6 */

    AddProperty(new MP4Integer16Property(*this, "defTextBoxTop")); /* 7 */
    AddProperty(new MP4Integer16Property(*this, "defTextBoxLeft")); /* 8 */
    AddProperty(new MP4Integer16Property(*this, "defTextBoxBottom")); /* 9 */
    AddProperty(new MP4Integer16Property(*this, "defTextBoxRight")); /* 10 */

    AddReserved(*this, "reserved2", 8); /* 11 */

    AddProperty(new MP4Integer16Property(*this, "fontNumber")); /* 12 */
    AddProperty(new MP4Integer16Property(*this, "fontFace")); /* 13 */

    AddReserved(*this, "reserved3", 1); /* 14 */
    AddReserved(*this, "reserved4", 2); /* 15 */

    AddProperty(new MP4Integer16Property(*this, "foreColorRed")); /* 16 */
    AddProperty(new MP4Integer16Property(*this, "foreColorGreen")); /* 17 */
    AddProperty(new MP4Integer16Property(*this, "foreColorBlue")); /* 18 */

}

void MP4TextAtom::AddPropertiesGmhdType()
{

    AddProperty(new MP4BytesProperty(*this, "textData", 36)); /* 0 */

}


void MP4TextAtom::Generate()
{
    ASSERT(m_pParentAtom);
    if (ATOMID(m_pParentAtom->GetType()) == ATOMID("stsd")) {
        AddPropertiesStsdType();
        GenerateStsdType();
    } else if (ATOMID(m_pParentAtom->GetType()) == ATOMID("gmhd")) {
        AddPropertiesGmhdType();
        GenerateGmhdType();
    } else {
        log.warningf("%s: \"%s\": text atom in unexpected context, can not generate", __FUNCTION__,
                     GetFile().GetFilename().c_str());
    }

}

void MP4TextAtom::GenerateStsdType()
{
    // generate children
    MP4Atom::Generate();

    ((MP4Integer16Property*)m_pProperties[1])->SetValue(1);

    ((MP4Integer32Property*)m_pProperties[2])->SetValue(1);
    ((MP4Integer32Property*)m_pProperties[3])->SetValue(1);

}

void MP4TextAtom::GenerateGmhdType()
{
    MP4Atom::Generate();

    // property 0 has non-zero fixed values
    static uint8_t textData[36] = {
        0x00, 0x01,
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x01,
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00,
        0x40, 0x00,
        0x00, 0x00,
    };
    ((MP4BytesProperty*)m_pProperties[0])->SetValue(textData, sizeof(textData));

}

void MP4TextAtom::Read ()
{
    if (ATOMID(m_pParentAtom->GetType()) == ATOMID("stsd")) {
        AddPropertiesStsdType();
    } else if (ATOMID(m_pParentAtom->GetType()) == ATOMID("gmhd")) {
        AddPropertiesGmhdType();
    }

    MP4Atom::Read();
}

///////////////////////////////////////////////////////////////////////////////

}
} // namespace mp4v2::impl
