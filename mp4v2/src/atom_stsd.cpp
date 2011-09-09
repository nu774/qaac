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
 * Copyright (C) Cisco Systems Inc. 2001 - 2004.  All Rights Reserved.
 *
 * 3GPP features implementation is based on 3GPP's TS26.234-v5.60,
 * and was contributed by Ximpo Group Ltd.
 *
 * Portions created by Ximpo Group Ltd. are
 * Copyright (C) Ximpo Group Ltd. 2003, 2004.  All Rights Reserved.
 *
 * Contributor(s):
 *      Dave Mackie         dmackie@cisco.com
 *      Alix Marchandise-Franquet   alix@cisco.com
 *              Ximpo Group Ltd.                mp4v2@ximpo.com
 */

#include "src/impl.h"

namespace mp4v2 {
namespace impl {

///////////////////////////////////////////////////////////////////////////////

MP4StsdAtom::MP4StsdAtom(MP4File &file)
        : MP4Atom(file, "stsd")
{
    AddVersionAndFlags();

    MP4Integer32Property* pCount =
        new MP4Integer32Property(*this, "entryCount");
    pCount->SetReadOnly();
    AddProperty(pCount);

    ExpectChildAtom("mp4a", Optional, Many);
    ExpectChildAtom("enca", Optional, Many);
    ExpectChildAtom("mp4s", Optional, Many);
    ExpectChildAtom("mp4v", Optional, Many);
    ExpectChildAtom("encv", Optional, Many);
    ExpectChildAtom("rtp ", Optional, Many);
    ExpectChildAtom("samr", Optional, Many); // For AMR-NB
    ExpectChildAtom("sawb", Optional, Many); // For AMR-WB
    ExpectChildAtom("s263", Optional, Many); // For H.263
    ExpectChildAtom("avc1", Optional, Many);
    ExpectChildAtom("alac", Optional, Many);
    ExpectChildAtom("text", Optional, Many);
    ExpectChildAtom("ac-3", Optional, Many);
}

void MP4StsdAtom::Read()
{
    /* do the usual read */
    MP4Atom::Read();

    // check that number of children == entryCount
    MP4Integer32Property* pCount =
        (MP4Integer32Property*)m_pProperties[2];

    if (m_pChildAtoms.Size() != pCount->GetValue()) {
        log.warningf("%s: \"%s\": stsd inconsistency with number of entries",
                     __FUNCTION__, GetFile().GetFilename().c_str() );

        /* fix it */
        pCount->SetReadOnly(false);
        pCount->SetValue(m_pChildAtoms.Size());
        pCount->SetReadOnly(true);
    }
}

///////////////////////////////////////////////////////////////////////////////

}
} // namespace mp4v2::impl
