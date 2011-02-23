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
 */

#include "src/impl.h"

namespace mp4v2 {
namespace impl {

///////////////////////////////////////////////////////////////////////////////

MP4Tx3gAtom::MP4Tx3gAtom(MP4File &file)
        : MP4Atom(file, "tx3g")
{
    AddReserved(*this, "reserved1", 4); /* 0 */
    AddReserved(*this, "reserved2", 2); /* 1 */

    AddProperty(new MP4Integer16Property(*this, "dataReferenceIndex"));/* 2 */

    AddProperty(new MP4Integer32Property(*this, "displayFlags")); /* 3 */
    AddProperty(new MP4Integer8Property(*this, "horizontalJustification")); /* 4 */
    AddProperty(new MP4Integer8Property(*this, "verticalJustification")); /* 5 */

    AddProperty(new MP4Integer8Property(*this, "bgColorRed")); /* 6 */
    AddProperty(new MP4Integer8Property(*this, "bgColorGreen")); /* 7 */
    AddProperty(new MP4Integer8Property(*this, "bgColorBlue")); /* 8 */
    AddProperty(new MP4Integer8Property(*this, "bgColorAlpha")); /* 9 */

    AddProperty(new MP4Integer16Property(*this, "defTextBoxTop")); /* 10 */
    AddProperty(new MP4Integer16Property(*this, "defTextBoxLeft")); /* 11 */
    AddProperty(new MP4Integer16Property(*this, "defTextBoxBottom")); /* 12 */
    AddProperty(new MP4Integer16Property(*this, "defTextBoxRight")); /* 13 */

    AddProperty(new MP4Integer16Property(*this, "startChar")); /* 14 */
    AddProperty(new MP4Integer16Property(*this, "endChar")); /* 15 */
    AddProperty(new MP4Integer16Property(*this, "fontID")); /* 16 */
    AddProperty(new MP4Integer8Property(*this, "fontFace")); /* 17 */
    AddProperty(new MP4Integer8Property(*this, "fontSize")); /* 18 */

    AddProperty(new MP4Integer8Property(*this, "fontColorRed")); /* 19 */
    AddProperty(new MP4Integer8Property(*this, "fontColorGreen")); /* 20 */
    AddProperty(new MP4Integer8Property(*this, "fontColorBlue")); /* 21 */
    AddProperty(new MP4Integer8Property(*this, "fontColorAlpha")); /* 22 */

    ExpectChildAtom("ftab", Optional, Many);
}

void MP4Tx3gAtom::Generate()
{
    // generate children
    MP4Atom::Generate();

    ((MP4Integer16Property*)m_pProperties[2])->SetValue(1);
}

///////////////////////////////////////////////////////////////////////////////

}
} // namespace mp4v2::impl
