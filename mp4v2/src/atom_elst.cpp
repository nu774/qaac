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

MP4ElstAtom::MP4ElstAtom(MP4File &file)
        : MP4Atom(file, "elst")
{
    AddVersionAndFlags();

    MP4Integer32Property* pCount =
        new MP4Integer32Property(*this, "entryCount");
    AddProperty(pCount);

    MP4TableProperty* pTable = new MP4TableProperty(*this, "entries", pCount);
    AddProperty(pTable);
}

void MP4ElstAtom::AddProperties(uint8_t version)
{
    MP4TableProperty* pTable = (MP4TableProperty*)m_pProperties[3];

    if (version == 1) {
        pTable->AddProperty(
            new MP4Integer64Property(pTable->GetParentAtom(), "segmentDuration"));
        pTable->AddProperty(
            new MP4Integer64Property(pTable->GetParentAtom(), "mediaTime"));
    } else {
        pTable->AddProperty(
            new MP4Integer32Property(pTable->GetParentAtom(), "segmentDuration"));
        pTable->AddProperty(
            new MP4Integer32Property(pTable->GetParentAtom(), "mediaTime"));
    }

    pTable->AddProperty(
        new MP4Integer16Property(pTable->GetParentAtom(), "mediaRate"));
    pTable->AddProperty(
        new MP4Integer16Property(pTable->GetParentAtom(), "reserved"));
}

void MP4ElstAtom::Generate()
{
    SetVersion(0);
    AddProperties(GetVersion());

    MP4Atom::Generate();
}

void MP4ElstAtom::Read()
{
    /* read atom version */
    ReadProperties(0, 1);

    /* need to create the properties based on the atom version */
    AddProperties(GetVersion());

    /* now we can read the remaining properties */
    ReadProperties(1);

    Skip(); // to end of atom
}

///////////////////////////////////////////////////////////////////////////////

}
} // namespace mp4v2::impl
