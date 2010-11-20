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
 *
 */

#include "src/impl.h"

namespace mp4v2 {
namespace impl {

///////////////////////////////////////////////////////////////////////////////

IPodUUIDAtom::IPodUUIDAtom()
        : MP4Atom("uuid")
{
    //
    // This is a hack, the contents of this atom need to be well defined.
    //
    static uint8_t ipod_magic[] = {
        0x6b, 0x68, 0x40, 0xf2, 0x5f, 0x24, 0x4f, 0xc5,
        0xba, 0x39, 0xa5, 0x1b, 0xcf, 0x03, 0x23, 0xf3
    };

    SetExtendedType(ipod_magic);

    MP4Integer32Property* value = new MP4Integer32Property("value");
    value->SetValue(1);
    AddProperty(value);
}

///////////////////////////////////////////////////////////////////////////////

}
} // namespace mp4v2::impl
