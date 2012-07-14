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

#ifndef MP4V2_IMPL_QOSQUALIFIERS_H
#define MP4V2_IMPL_QOSQUALIFIERS_H

namespace mp4v2 {
namespace impl {

///////////////////////////////////////////////////////////////////////////////

const uint8_t MP4QosDescrTag                = 0x0C;
const uint8_t MP4QosTagsStart               = 0x01;
const uint8_t MP4MaxDelayQosTag         = 0x01;
const uint8_t MP4PrefMaxDelayQosTag     = 0x02;
const uint8_t MP4LossProbQosTag         = 0x03;
const uint8_t MP4MaxGapLossQosTag           = 0x04;
const uint8_t MP4MaxAUSizeQosTag            = 0x41;
const uint8_t MP4AvgAUSizeQosTag            = 0x42;
const uint8_t MP4MaxAURateQosTag            = 0x43;
const uint8_t MP4QosTagsEnd             = 0xFF;

class MP4QosDescriptorBase : public MP4Descriptor {
public:
    MP4QosDescriptorBase(MP4Atom &parentAtom, uint8_t tag);
private:
    MP4QosDescriptorBase();
    MP4QosDescriptorBase ( const MP4QosDescriptorBase &src );
    MP4QosDescriptorBase &operator= ( const MP4QosDescriptorBase &src );
};

class MP4UnknownQosQualifier : public MP4Descriptor {
public:
    MP4UnknownQosQualifier(MP4Atom &parentAtom);
    void Read(MP4File& file);
private:
    MP4UnknownQosQualifier();
    MP4UnknownQosQualifier ( const MP4UnknownQosQualifier &src );
    MP4UnknownQosQualifier &operator= ( const MP4UnknownQosQualifier &src );
};

///////////////////////////////////////////////////////////////////////////////

}
} // namespace mp4v2::impl

#endif // MP4V2_IMPL_QOSQUALIFIERS_H
