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

// MP4ChplAtom is for Nero chapter list atom which is a child of udta
MP4ChplAtom::MP4ChplAtom (MP4File &file)
        : MP4Atom(file, "chpl")
{
    // it is not completely clear if version, flags, reserved and chaptercount
    // have the right sizes but
    // one thing is clear: chaptercount is not only 8-bit it is at least 16-bit

    // add the version
    AddVersionAndFlags();

    // add reserved bytes
    AddReserved(*this,"reserved", 1);

    // define the chaptercount
    MP4Integer32Property * counter = new MP4Integer32Property(*this,"chaptercount");
    AddProperty(counter);

    // define the chapterlist
    MP4TableProperty * list = new MP4TableProperty(*this,"chapters", counter);

    // the start time as 100 nanoseconds units
    list->AddProperty(new MP4Integer64Property(*this,"starttime"));

    // the chapter name as UTF-8
    list->AddProperty(new MP4StringProperty(*this,"title", true));

    // add the chapterslist
    AddProperty(list);
}

void MP4ChplAtom::Generate ()
{
    SetVersion(1);
}

///////////////////////////////////////////////////////////////////////////////

}
} // namespace mp4v2::impl
