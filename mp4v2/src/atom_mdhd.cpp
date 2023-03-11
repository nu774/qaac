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

namespace mp4v2 { namespace impl {

///////////////////////////////////////////////////////////////////////////////

MP4MdhdAtom::MP4MdhdAtom(MP4File &file)
        : MP4Atom(file, "mdhd")
{
    AddVersionAndFlags();
}

void MP4MdhdAtom::AddProperties(uint8_t version)
{
    MP4Integer6432Property *p;

    p = new MP4Integer6432Property(*this, "creationTime");
    p->Use64Bit(version == 1);
    AddProperty(p);

    p = new MP4Integer6432Property(*this, "modificationTime");
    p->Use64Bit(version == 1);
    AddProperty(p);

    AddProperty(new MP4Integer32Property(*this, "timeScale"));

    p = new MP4Integer6432Property(*this, "duration");
    p->Use64Bit(version == 1);
    AddProperty(p);

    AddProperty( new MP4LanguageCodeProperty(*this,  "language" ));
    AddReserved(*this, "reserved", 2);
}

void MP4MdhdAtom::Generate()
{
    SetVersion(0);
    AddProperties(0);

    MP4Atom::Generate();

    // set creation and modification times
    MP4Timestamp now = MP4GetAbsTimestamp();
    ((MP4Integer6432Property*)m_pProperties[2])->SetValue(now);
    ((MP4Integer6432Property*)m_pProperties[3])->SetValue(now);
}

void MP4MdhdAtom::Read()
{
    /* read atom version */
    ReadProperties(0, 1);

    /* need to create the properties based on the atom version */
    AddProperties(GetVersion());

    /* now we can read the remaining properties */
    ReadProperties(1);

    Skip(); // to end of atom
}

void MP4MdhdAtom::BeginWrite()
{
    if (((MP4Integer6432Property*)m_pProperties[2])->GetValue() > 0xffffffff ||
        ((MP4Integer6432Property*)m_pProperties[3])->GetValue() > 0xffffffff ||
        ((MP4Integer6432Property*)m_pProperties[5])->GetValue() > 0xffffffff)
    {
        SetVersion(1);
        ((MP4Integer6432Property*)m_pProperties[2])->Use64Bit(true);
        ((MP4Integer6432Property*)m_pProperties[3])->Use64Bit(true);
        ((MP4Integer6432Property*)m_pProperties[5])->Use64Bit(true);
    }
    else
    {
        SetVersion(0);
        ((MP4Integer6432Property*)m_pProperties[2])->Use64Bit(false);
        ((MP4Integer6432Property*)m_pProperties[3])->Use64Bit(false);
        ((MP4Integer6432Property*)m_pProperties[5])->Use64Bit(false);
    }
    MP4Atom::BeginWrite();
}

///////////////////////////////////////////////////////////////////////////////

}} // namespace mp4v2::impl
