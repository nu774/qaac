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

#ifndef MP4V2_IMPL_ODCOMMANDS_H
#define MP4V2_IMPL_ODCOMMANDS_H

namespace mp4v2 {
namespace impl {

///////////////////////////////////////////////////////////////////////////////

// OD stream command descriptors
const uint8_t MP4ODUpdateODCommandTag           = 0x01;
const uint8_t MP4ODRemoveODCommandTag           = 0x02;
const uint8_t MP4ESUpdateODCommandTag           = 0x03;
const uint8_t MP4ESRemoveODCommandTag           = 0x04;
const uint8_t MP4IPMPUpdateODCommandTag     = 0x05;
const uint8_t MP4IPMPRemoveODCommandTag     = 0x06;
const uint8_t MP4ESRemoveRefODCommandTag        = 0x07;

class MP4ODUpdateDescriptor : public MP4Descriptor {
public:
    MP4ODUpdateDescriptor(MP4Atom& parentAtom);
private:
    MP4ODUpdateDescriptor();
    MP4ODUpdateDescriptor ( const MP4ODUpdateDescriptor &src );
    MP4ODUpdateDescriptor &operator= ( const MP4ODUpdateDescriptor &src );
};

class MP4ODRemoveDescriptor : public MP4Descriptor {
public:
    MP4ODRemoveDescriptor(MP4Atom& parentAtom);
    void Read(MP4File& file);
private:
    MP4ODRemoveDescriptor();
    MP4ODRemoveDescriptor ( const MP4ODRemoveDescriptor &src );
    MP4ODRemoveDescriptor &operator= ( const MP4ODRemoveDescriptor &src );
};

class MP4ESUpdateDescriptor : public MP4Descriptor {
public:
    MP4ESUpdateDescriptor(MP4Atom& parentAtom);
private:
    MP4ESUpdateDescriptor();
    MP4ESUpdateDescriptor ( const MP4ESUpdateDescriptor &src );
    MP4ESUpdateDescriptor &operator= ( const MP4ESUpdateDescriptor &src );
};

class MP4ESRemoveDescriptor : public MP4Descriptor {
public:
    MP4ESRemoveDescriptor(MP4Atom& parentAtom);
private:
    MP4ESRemoveDescriptor();
    MP4ESRemoveDescriptor ( const MP4ESRemoveDescriptor &src );
    MP4ESRemoveDescriptor &operator= ( const MP4ESRemoveDescriptor &src );
};

MP4Descriptor* CreateODCommand(MP4Atom& parentAtom, uint8_t tag);

///////////////////////////////////////////////////////////////////////////////

}
} // namespace mp4v2::impl

#endif // MP4V2_IMPL_ODCOMMANDS_H
