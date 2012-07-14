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

#if 1
MP4QosDescriptorBase::MP4QosDescriptorBase (MP4Atom& parentAtom, uint8_t tag)
        : MP4Descriptor(parentAtom, tag)
{
    switch (tag) {
    case MP4QosDescrTag:
        AddProperty( /* 0 */
            new MP4Integer8Property(parentAtom, "predefined"));
        AddProperty( /* 1 */
            new MP4QosQualifierProperty(parentAtom, "qualifiers",
                                        MP4QosTagsStart,
                                        MP4QosTagsEnd, Optional, Many));
        break;
    case MP4MaxDelayQosTag:
        AddProperty( /* 0 */
            new MP4Integer32Property(parentAtom, "maxDelay"));
        break;
    case MP4PrefMaxDelayQosTag:
        AddProperty( /* 0 */
            new MP4Integer32Property(parentAtom, "prefMaxDelay"));
        break;
    case MP4LossProbQosTag:
        AddProperty( /* 0 */
            new MP4Float32Property(parentAtom, "lossProb"));
        break;
    case MP4MaxGapLossQosTag:
        AddProperty( /* 0 */
            new MP4Integer32Property(parentAtom, "maxGapLoss"));
        break;
    case MP4MaxAUSizeQosTag:
        AddProperty( /* 0 */
            new MP4Integer32Property(parentAtom, "maxAUSize"));
        break;
    case MP4AvgAUSizeQosTag:
        AddProperty( /* 0 */
            new MP4Integer32Property(parentAtom, "avgAUSize"));
        break;
    case MP4MaxAURateQosTag:
        AddProperty( /* 0 */
            new MP4Integer32Property(parentAtom, "maxAURate"));
        break;
    }
}

#else
MP4QosDescriptor::MP4QosDescriptor(MP4Atom &parentAtom)
        : MP4Descriptor(parentAtom, MP4QosDescrTag)
{
    AddProperty( /* 0 */
        new MP4Integer8Property(parentAtom, "predefined"));
    AddProperty( /* 1 */
        new MP4QosQualifierProperty(parentAtom, "qualifiers",
                                    MP4QosTagsStart, MP4QosTagsEnd, Optional, Many));
}

MP4MaxDelayQosQualifier::MP4MaxDelayQosQualifier(MP4Atom &parentAtom)
        : MP4QosQualifier(parentAtom, MP4MaxDelayQosTag)
{
    AddProperty( /* 0 */
        new MP4Integer32Property(parentAtom, "maxDelay"));
}

MP4PrefMaxDelayQosQualifier::MP4PrefMaxDelayQosQualifier(MP4Atom &parentAtom)
        : MP4QosQualifier(parentAtom, MP4PrefMaxDelayQosTag)
{
    AddProperty( /* 0 */
        new MP4Integer32Property(parentAtom, "prefMaxDelay"));
}

MP4LossProbQosQualifier::MP4LossProbQosQualifier(MP4Atom &parentAtom)
        : MP4QosQualifier(parentAtom, MP4LossProbQosTag)
{
    AddProperty( /* 0 */
        new MP4Float32Property(parentAtom, "lossProb"));
}

MP4MaxGapLossQosQualifier::MP4MaxGapLossQosQualifier(MP4Atom &parentAtom)
        : MP4QosQualifier(parentAtom, MP4MaxGapLossQosTag)
{
    AddProperty( /* 0 */
        new MP4Integer32Property(parentAtom, "maxGapLoss"));
}

MP4MaxAUSizeQosQualifier::MP4MaxAUSizeQosQualifier(MP4Atom &parentAtom)
        : MP4QosQualifier(parentAtom, MP4MaxAUSizeQosTag)
{
    AddProperty( /* 0 */
        new MP4Integer32Property(parentAtom, "maxAUSize"));
}

MP4AvgAUSizeQosQualifier::MP4AvgAUSizeQosQualifier(MP4Atom &parentAtom)
        : MP4QosQualifier(parentAtom, MP4AvgAUSizeQosTag)
{
    AddProperty( /* 0 */
        new MP4Integer32Property(parentAtom, "avgAUSize"));
}

MP4MaxAURateQosQualifier::MP4MaxAURateQosQualifier(MP4Atom &parentAtom)
        : MP4QosQualifier(parentAtom, MP4MaxAURateQosTag)
{
    AddProperty( /* 0 */
        new MP4Integer32Property(parentAtom, "maxAURate"));
}
#endif
MP4UnknownQosQualifier::MP4UnknownQosQualifier(MP4Atom &parentAtom)
        : MP4Descriptor(parentAtom)
{
    AddProperty( /* 0 */
        new MP4BytesProperty(parentAtom, "data"));
}

void MP4UnknownQosQualifier::Read(MP4File& file)
{
    ReadHeader(file);

    /* byte properties need to know how long they are before reading */
    ((MP4BytesProperty*)m_pProperties[0])->SetValueSize(m_size);

    ReadProperties(file);
}

MP4Descriptor* MP4QosQualifierProperty::CreateDescriptor(MP4Atom &parentAtom, uint8_t tag)
{
    MP4Descriptor* pDescriptor = NULL;
    switch (tag) {
    case MP4MaxDelayQosTag:
    case MP4PrefMaxDelayQosTag:
    case MP4LossProbQosTag:
    case MP4MaxGapLossQosTag:
    case MP4MaxAUSizeQosTag:
    case MP4AvgAUSizeQosTag:
    case MP4MaxAURateQosTag:
        pDescriptor = new MP4QosDescriptorBase(parentAtom, tag);
        break;
    default:
        pDescriptor = new MP4UnknownQosQualifier(parentAtom);
        pDescriptor->SetTag(tag);
    }

    return pDescriptor;
}

///////////////////////////////////////////////////////////////////////////////

}
} // namespace mp4v2::impl
