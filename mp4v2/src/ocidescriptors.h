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

#ifndef MP4V2_IMPL_OCIDESCRIPTORS_H
#define MP4V2_IMPL_OCIDESCRIPTORS_H

namespace mp4v2 {
namespace impl {

///////////////////////////////////////////////////////////////////////////////

const uint8_t MP4OCIDescrTagsStart          = 0x40;
const uint8_t MP4ContentClassDescrTag       = 0x40;
const uint8_t MP4KeywordDescrTag            = 0x41;
const uint8_t MP4RatingDescrTag             = 0x42;
const uint8_t MP4LanguageDescrTag           = 0x43;
const uint8_t MP4ShortTextDescrTag          = 0x44;
const uint8_t MP4ExpandedTextDescrTag       = 0x45;
const uint8_t MP4ContentCreatorDescrTag = 0x46;
const uint8_t MP4ContentCreationDescrTag    = 0x47;
const uint8_t MP4OCICreatorDescrTag     = 0x48;
const uint8_t MP4OCICreationDescrTag        = 0x49;
const uint8_t MP4SmpteCameraDescrTag        = 0x4A;
const uint8_t MP4OCIDescrTagsEnd            = 0x5F;

class MP4ContentClassDescriptor : public MP4Descriptor {
public:
    MP4ContentClassDescriptor(MP4Atom& parentAtom);
    void Read(MP4File& file);
private:
    MP4ContentClassDescriptor();
    MP4ContentClassDescriptor ( const MP4ContentClassDescriptor &src );
    MP4ContentClassDescriptor &operator= ( const MP4ContentClassDescriptor &src );
};

class MP4KeywordDescriptor : public MP4Descriptor {
public:
    MP4KeywordDescriptor(MP4Atom& parentAtom);
protected:
    void Mutate();
private:
    MP4KeywordDescriptor();
    MP4KeywordDescriptor ( const MP4KeywordDescriptor &src );
    MP4KeywordDescriptor &operator= ( const MP4KeywordDescriptor &src );
};

class MP4RatingDescriptor : public MP4Descriptor {
public:
    MP4RatingDescriptor(MP4Atom& parentAtom);
    void Read(MP4File& file);
private:
    MP4RatingDescriptor();
    MP4RatingDescriptor ( const MP4RatingDescriptor &src );
    MP4RatingDescriptor &operator= ( const MP4RatingDescriptor &src );
};

class MP4LanguageDescriptor : public MP4Descriptor {
public:
    MP4LanguageDescriptor(MP4Atom& parentAtom);
private:
    MP4LanguageDescriptor();
    MP4LanguageDescriptor ( const MP4LanguageDescriptor &src );
    MP4LanguageDescriptor &operator= ( const MP4LanguageDescriptor &src );
};

class MP4ShortTextDescriptor : public MP4Descriptor {
public:
    MP4ShortTextDescriptor(MP4Atom& parentAtom);
protected:
    void Mutate();
private:
    MP4ShortTextDescriptor();
    MP4ShortTextDescriptor ( const MP4ShortTextDescriptor &src );
    MP4ShortTextDescriptor &operator= ( const MP4ShortTextDescriptor &src );
};

class MP4ExpandedTextDescriptor : public MP4Descriptor {
public:
    MP4ExpandedTextDescriptor(MP4Atom& parentAtom);
protected:
    void Mutate();
private:
    MP4ExpandedTextDescriptor();
    MP4ExpandedTextDescriptor ( const MP4ExpandedTextDescriptor &src );
    MP4ExpandedTextDescriptor &operator= ( const MP4ExpandedTextDescriptor &src );
};

class MP4CreatorDescriptor : public MP4Descriptor {
public:
    MP4CreatorDescriptor(MP4Atom& parentAtom, uint8_t tag);
private:
    MP4CreatorDescriptor();
    MP4CreatorDescriptor ( const MP4CreatorDescriptor &src );
    MP4CreatorDescriptor &operator= ( const MP4CreatorDescriptor &src );
};

class MP4CreationDescriptor : public MP4Descriptor {
public:
    MP4CreationDescriptor(MP4Atom& parentAtom, uint8_t tag);
private:
    MP4CreationDescriptor();
    MP4CreationDescriptor ( const MP4CreationDescriptor &src );
    MP4CreationDescriptor &operator= ( const MP4CreationDescriptor &src );
};

class MP4SmpteCameraDescriptor : public MP4Descriptor {
public:
    MP4SmpteCameraDescriptor(MP4Atom& parentAtom);
private:
    MP4SmpteCameraDescriptor();
    MP4SmpteCameraDescriptor ( const MP4SmpteCameraDescriptor &src );
    MP4SmpteCameraDescriptor &operator= ( const MP4SmpteCameraDescriptor &src );
};

class MP4UnknownOCIDescriptor : public MP4Descriptor {
public:
    MP4UnknownOCIDescriptor(MP4Atom& parentAtom);
    void Read(MP4File& file);
private:
    MP4UnknownOCIDescriptor();
    MP4UnknownOCIDescriptor ( const MP4UnknownOCIDescriptor &src );
    MP4UnknownOCIDescriptor &operator= ( const MP4UnknownOCIDescriptor &src );
};


extern MP4Descriptor *CreateOCIDescriptor(MP4Atom& parentAtom, uint8_t tag);

///////////////////////////////////////////////////////////////////////////////

}
} // namespace mp4v2::impl

#endif // MP4V2_IMPL_OCIDESCRIPTORS_H
