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

MP4SdpAtom::MP4SdpAtom(MP4File &file) : MP4Atom(file, "sdp ")
{
    AddProperty(
        new MP4StringProperty(*this, "sdpText"));
}

void MP4SdpAtom::Read()
{
    // read sdp string, length is implicit in size of atom
    uint64_t size = GetEnd() - m_File.GetPosition();
    char* data = (char*)MP4Malloc(size + 1);
    ASSERT(data != NULL);
    m_File.ReadBytes((uint8_t*)data, size);
    data[size] = '\0';
    ((MP4StringProperty*)m_pProperties[0])->SetValue(data);
    MP4Free(data);
}

void MP4SdpAtom::Write()
{
    // since length of string is implicit in size of atom
    // we need to handle this specially, and not write the terminating \0
    MP4StringProperty* pSdp = (MP4StringProperty*)m_pProperties[0];
    const char* sdpText = pSdp->GetValue();
    if (sdpText) {
        pSdp->SetFixedLength((uint32_t)strlen(sdpText));
    }
    MP4Atom::Write();
    pSdp->SetFixedLength(0);
}

///////////////////////////////////////////////////////////////////////////////

}
} // namespace mp4v2::impl
