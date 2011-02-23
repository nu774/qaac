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

MP4FtabAtom::MP4FtabAtom(MP4File &file)
        : MP4Atom(file, "ftab")
{
    MP4Integer16Property* pCount = new MP4Integer16Property(*this, "entryCount"); /* 0 */
    AddProperty(pCount);

    MP4TableProperty* pTable = new MP4TableProperty(*this, "fontEntries", pCount);  /* 1 */
    AddProperty(pTable);

    pTable->AddProperty(new MP4Integer16Property(pTable->GetParentAtom(), "fontID"));  /* 0 */
    pTable->AddProperty(new MP4StringProperty(pTable->GetParentAtom(), "name", true));  /* 1 */
}

///////////////////////////////////////////////////////////////////////////////

}
} // namespace mp4v2::impl
