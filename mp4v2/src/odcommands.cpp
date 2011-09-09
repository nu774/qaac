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

MP4ODUpdateDescriptor::MP4ODUpdateDescriptor(MP4Atom& parentAtom)
        : MP4Descriptor(parentAtom, MP4ODUpdateODCommandTag)
{
    // just a container for ObjectDescriptors
    AddProperty( /* 0 */
        new MP4DescriptorProperty(parentAtom, NULL,
                                  MP4FileODescrTag, 0, Required, Many));
}

MP4ODRemoveDescriptor::MP4ODRemoveDescriptor(MP4Atom& parentAtom)
        : MP4Descriptor(parentAtom, MP4ODRemoveODCommandTag)
{
    MP4Integer32Property* pCount =
        new MP4Integer32Property(parentAtom, "entryCount");
    pCount->SetImplicit();
    AddProperty(pCount); /* 0 */

    MP4TableProperty* pTable =
        new MP4TableProperty(parentAtom, "entries", pCount);
    AddProperty(pTable); /* 1 */

    pTable->AddProperty( /* 1, 0 */
        new MP4BitfieldProperty(pTable->GetParentAtom(), "objectDescriptorId", 10));
}

void MP4ODRemoveDescriptor::Read(MP4File& file)
{
    // table entry count computed from descriptor size
    ((MP4Integer32Property*)m_pProperties[0])->SetReadOnly(false);
    ((MP4Integer32Property*)m_pProperties[0])->SetValue((m_size * 8) / 10);
    ((MP4Integer32Property*)m_pProperties[0])->SetReadOnly(true);

    MP4Descriptor::Read(file);
}

MP4ESUpdateDescriptor::MP4ESUpdateDescriptor(MP4Atom& parentAtom)
        : MP4Descriptor(parentAtom, MP4ESUpdateODCommandTag)
{
    AddProperty( /* 0 */
        new MP4BitfieldProperty(parentAtom, "objectDescriptorId", 10));
    AddProperty( /* 1 */
        new MP4BitfieldProperty(parentAtom, "pad", 6));
    AddProperty( /* 2 */
        new MP4DescriptorProperty(parentAtom, "esIdRefs",
                                  MP4ESIDRefDescrTag, 0, Required, Many));
}

// LATER might be able to combine with ESUpdateDescriptor
MP4ESRemoveDescriptor::MP4ESRemoveDescriptor(MP4Atom& parentAtom)
        : MP4Descriptor(parentAtom, MP4ESRemoveODCommandTag)
{
    AddProperty( /* 0 */
        new MP4BitfieldProperty(parentAtom, "objectDescriptorId", 10));
    AddProperty( /* 1 */
        new MP4BitfieldProperty(parentAtom, "pad", 6));
    AddProperty( /* 2 */
        new MP4DescriptorProperty(parentAtom, "esIdRefs",
                                  MP4ESIDRefDescrTag, 0, Required, Many));
}

MP4Descriptor* CreateODCommand(MP4Atom& parentAtom, uint8_t tag)
{
    MP4Descriptor* pDescriptor = NULL;

    switch (tag) {
    case MP4ODUpdateODCommandTag:
        pDescriptor = new MP4ODUpdateDescriptor(parentAtom);
        break;
    case MP4ODRemoveODCommandTag:
        pDescriptor = new MP4ODRemoveDescriptor(parentAtom);
        break;
    case MP4ESUpdateODCommandTag:
        pDescriptor = new MP4ESUpdateDescriptor(parentAtom);
        break;
    case MP4ESRemoveODCommandTag:
        pDescriptor = new MP4ESRemoveDescriptor(parentAtom);
        break;
    }
    return pDescriptor;
}

///////////////////////////////////////////////////////////////////////////////

}
} // namespace mp4v2::impl
