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
    if (version == 1) {
        AddProperty(
            new MP4Integer64Property(*this, "creationTime"));
        AddProperty(
            new MP4Integer64Property(*this, "modificationTime"));
    } else {
        AddProperty(
            new MP4Integer32Property(*this, "creationTime"));
        AddProperty(
            new MP4Integer32Property(*this, "modificationTime"));
    }

    AddProperty(
        new MP4Integer32Property(*this, "timeScale"));

    if (version == 1) {
        AddProperty(
            new MP4Integer64Property(*this, "duration"));
    } else {
        AddProperty(
            new MP4Integer32Property(*this, "duration"));
    }

    AddProperty( new MP4LanguageCodeProperty(*this,  "language" ));
    AddReserved(*this, "reserved", 2);
}

void MP4MdhdAtom::Generate()
{
    uint8_t version = m_File.Use64Bits(GetType()) ? 1 : 0;
    SetVersion(version);
    AddProperties(version);

    MP4Atom::Generate();

    // set creation and modification times
    MP4Timestamp now = MP4GetAbsTimestamp();
    if (version == 1) {
        ((MP4Integer64Property*)m_pProperties[2])->SetValue(now);
        ((MP4Integer64Property*)m_pProperties[3])->SetValue(now);
    } else {
        ((MP4Integer32Property*)m_pProperties[2])->SetValue(now);
        ((MP4Integer32Property*)m_pProperties[3])->SetValue(now);
    }
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

///////////////////////////////////////////////////////////////////////////////

}} // namespace mp4v2::impl
