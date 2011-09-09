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

MP4StscAtom::MP4StscAtom(MP4File &file)
        : MP4Atom(file, "stsc")
{
    AddVersionAndFlags();

    MP4Integer32Property* pCount =
        new MP4Integer32Property(*this, "entryCount");
    AddProperty(pCount);

    MP4TableProperty* pTable = new MP4TableProperty(*this, "entries", pCount);
    AddProperty(pTable);

    pTable->AddProperty(
        new MP4Integer32Property(pTable->GetParentAtom(), "firstChunk"));
    pTable->AddProperty(
        new MP4Integer32Property(pTable->GetParentAtom(), "samplesPerChunk"));
    pTable->AddProperty(
        new MP4Integer32Property(pTable->GetParentAtom(), "sampleDescriptionIndex"));

    // As an optimization we add an implicit property to this table,
    // "firstSample" that corresponds to the first sample of the firstChunk
    MP4Integer32Property* pSample =
        new MP4Integer32Property(*this, "firstSample");
    pSample->SetImplicit();
    pTable->AddProperty(pSample);
}

void MP4StscAtom::Read()
{
    // Read as usual
    MP4Atom::Read();

    // Compute the firstSample values for later use
    uint32_t count =
        ((MP4Integer32Property*)m_pProperties[2])->GetValue();

    MP4Integer32Property* pFirstChunk = (MP4Integer32Property*)
                                        ((MP4TableProperty*)m_pProperties[3])->GetProperty(0);
    MP4Integer32Property* pSamplesPerChunk = (MP4Integer32Property*)
            ((MP4TableProperty*)m_pProperties[3])->GetProperty(1);
    MP4Integer32Property* pFirstSample = (MP4Integer32Property*)
                                         ((MP4TableProperty*)m_pProperties[3])->GetProperty(3);

    MP4SampleId sampleId = 1;

    for (uint32_t i = 0; i < count; i++) {
        pFirstSample->SetValue(sampleId, i);

        if (i < count - 1) {
            sampleId +=
                (pFirstChunk->GetValue(i+1) - pFirstChunk->GetValue(i))
                * pSamplesPerChunk->GetValue(i);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////

}
} // namespace mp4v2::impl
