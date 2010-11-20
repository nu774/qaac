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
 * Contributer has declined to give copyright information, and gives
 * it freely to the world.
 *
 * Contributor(s):
 */

#include "src/impl.h"

namespace mp4v2 {
namespace impl {

///////////////////////////////////////////////////////////////////////////////

MP4PaspAtom::MP4PaspAtom()
        : MP4Atom("pasp")
{

    AddProperty( /* 0 */
        new MP4Integer32Property("hSpacing"));

    AddProperty( /* 1 */
        new MP4Integer32Property("vSpacing"));

}

void MP4PaspAtom::Generate()
{

    MP4Atom::Generate();

    ((MP4Integer32Property*)m_pProperties[0])->SetValue(1);
    ((MP4Integer32Property*)m_pProperties[1])->SetValue(1);

}

///////////////////////////////////////////////////////////////////////////////

}
} // namespace mp4v2::impl
