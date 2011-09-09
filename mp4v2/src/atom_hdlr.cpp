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

MP4HdlrAtom::MP4HdlrAtom(MP4File &file)
        : MP4Atom(file, "hdlr")
{
    AddVersionAndFlags(); /* 0, 1 */
    AddReserved(*this, "reserved1", 4); /* 2 */
    MP4StringProperty* pProp = new MP4StringProperty(*this, "handlerType");
    pProp->SetFixedLength(4);
    AddProperty(pProp); /* 3 */
    AddReserved(*this, "reserved2", 12); /* 4 */
    AddProperty( /* 5 */
        new MP4StringProperty(*this, "name"));
}

// There is a spec incompatiblity between QT and MP4
// QT says name field is a counted string
// MP4 says name field is a null terminated string
// Here we attempt to make all things work
void MP4HdlrAtom::Read()
{
    // read all the properties but the "name" field
    ReadProperties(0, 5);

    uint64_t pos = m_File.GetPosition();
    uint64_t end = GetEnd();
    if (pos == end) {
        // A hdlr atom with missing "name".     
        // Apparently that's what some of the iTunes m4p files have.
        return;
    }

    // take a peek at the next byte
    uint8_t strLength;
    m_File.PeekBytes(&strLength, 1);
    // if the value matches the remaining atom length
    if (pos + strLength + 1 == end) {
        // read a counted string
        MP4StringProperty* pNameProp =
            (MP4StringProperty*)m_pProperties[5];
        pNameProp->SetCountedFormat(true);
        ReadProperties(5);
        pNameProp->SetCountedFormat(false);
    } else {
        // read a null terminated string
        try {
            // Unfortunately, there are some invalid mp4 writers that don't
            // null the hdlr name string.  Generally this will be "automatically"
            // terminated for them by the size field of the subsequent atom.  So if
            // our size is off by one...let it slide.  otherwise, rethrow.
            // The Skip() call will set our start to the correct location
            // for the next Atom. See issue #52
            ReadProperties(5);
        }
        catch(Exception* x) { 
            if( m_File.GetPosition() - GetEnd() == 1 )
                delete x;
            else
                throw x;
        }
    }

    Skip(); // to end of atom
}

///////////////////////////////////////////////////////////////////////////////

}
} // namespace mp4v2::impl
