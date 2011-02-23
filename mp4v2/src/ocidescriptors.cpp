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

MP4ContentClassDescriptor::MP4ContentClassDescriptor(MP4Atom& parentAtom)
        : MP4Descriptor(parentAtom)
{
    AddProperty( /* 0 */
        new MP4Integer32Property(parentAtom, "classificationEntity"));
    AddProperty( /* 1 */
        new MP4Integer16Property(parentAtom, "classificationTable"));
    AddProperty( /* 2 */
        new MP4BytesProperty(parentAtom, "contentClassificationData"));
}

void MP4ContentClassDescriptor::Read(MP4File& file)
{
    ReadHeader(file);

    /* byte properties need to know how long they are before reading */
    ((MP4BytesProperty*)m_pProperties[2])->SetValueSize(m_size - 6);

    ReadProperties(file);
}

MP4KeywordDescriptor::MP4KeywordDescriptor(MP4Atom& parentAtom)
        : MP4Descriptor(parentAtom)
{
    AddProperty( /* 0 */
        new MP4BytesProperty(parentAtom, "languageCode", 3));
    AddProperty( /* 1 */
        new MP4BitfieldProperty(parentAtom, "isUTF8String", 1));
    AddProperty( /* 2 */
        new MP4BitfieldProperty(parentAtom, "reserved", 7));
    MP4Integer8Property* pCount =
        new MP4Integer8Property(parentAtom, "keywordCount");
    AddProperty(pCount); /* 3 */

    MP4TableProperty* pTable = new MP4TableProperty(parentAtom, "keywords", pCount);
    AddProperty(pTable); /* 4 */

    pTable->AddProperty( /* 4, 0 */
        new MP4StringProperty(pTable->GetParentAtom(), "string", Counted));

    SetReadMutate(2);
}

void MP4KeywordDescriptor::Mutate()
{
    bool utf8Flag = ((MP4BitfieldProperty*)m_pProperties[1])->GetValue();
    MP4Property* pProperty =
        ((MP4TableProperty*)m_pProperties[4])->GetProperty(0);
    ASSERT(pProperty);
    ((MP4StringProperty*)pProperty)->SetUnicode(!utf8Flag);
}

MP4RatingDescriptor::MP4RatingDescriptor(MP4Atom& parentAtom)
        : MP4Descriptor(parentAtom)
{
    AddProperty( /* 0 */
        new MP4Integer32Property(parentAtom, "ratingEntity"));
    AddProperty( /* 1 */
        new MP4Integer16Property(parentAtom, "ratingCriteria"));
    AddProperty( /* 2 */
        new MP4BytesProperty(parentAtom, "ratingInfo"));
}

void MP4RatingDescriptor::Read(MP4File& file)
{
    ReadHeader(file);

    /* byte properties need to know how long they are before reading */
    ((MP4BytesProperty*)m_pProperties[2])->SetValueSize(m_size - 6);

    ReadProperties(file);
}

MP4LanguageDescriptor::MP4LanguageDescriptor(MP4Atom& parentAtom)
        : MP4Descriptor(parentAtom)
{
    AddProperty( /* 0 */
        new MP4BytesProperty(parentAtom, "languageCode", 3));
}

MP4ShortTextDescriptor::MP4ShortTextDescriptor(MP4Atom& parentAtom)
        : MP4Descriptor(parentAtom)
{
    AddProperty( /* 0 */
        new MP4BytesProperty(parentAtom, "languageCode", 3));
    AddProperty( /* 1 */
        new MP4BitfieldProperty(parentAtom, "isUTF8String", 1));
    AddProperty( /* 2 */
        new MP4BitfieldProperty(parentAtom, "reserved", 7));
    AddProperty( /* 3 */
        new MP4StringProperty(parentAtom, "eventName", Counted));
    AddProperty( /* 4 */
        new MP4StringProperty(parentAtom, "eventText", Counted));

    SetReadMutate(2);
}

void MP4ShortTextDescriptor::Mutate()
{
    bool utf8Flag = ((MP4BitfieldProperty*)m_pProperties[1])->GetValue();
    ((MP4StringProperty*)m_pProperties[3])->SetUnicode(!utf8Flag);
    ((MP4StringProperty*)m_pProperties[4])->SetUnicode(!utf8Flag);
}

MP4ExpandedTextDescriptor::MP4ExpandedTextDescriptor(MP4Atom& parentAtom)
        : MP4Descriptor(parentAtom)
{
    AddProperty( /* 0 */
        new MP4BytesProperty(parentAtom, "languageCode", 3));
    AddProperty( /* 1 */
        new MP4BitfieldProperty(parentAtom, "isUTF8String", 1));
    AddProperty( /* 2 */
        new MP4BitfieldProperty(parentAtom, "reserved", 7));
    MP4Integer8Property* pCount =
        new MP4Integer8Property(parentAtom, "itemCount");
    AddProperty(pCount); /* 3 */

    MP4TableProperty* pTable = new MP4TableProperty(parentAtom, "items", pCount);
    AddProperty(pTable); /* 4 */

    pTable->AddProperty( /* Table 0 */
        new MP4StringProperty(pTable->GetParentAtom(), "itemDescription", Counted));
    pTable->AddProperty( /* Table 1 */
        new MP4StringProperty(pTable->GetParentAtom(), "itemText", Counted));

    AddProperty( /* 5 */
        new MP4StringProperty(parentAtom, "nonItemText"));
    ((MP4StringProperty*)m_pProperties[5])->SetExpandedCountedFormat(true);

    SetReadMutate(2);
}

void MP4ExpandedTextDescriptor::Mutate()
{
    bool utf8Flag = ((MP4BitfieldProperty*)m_pProperties[1])->GetValue();

    MP4Property* pProperty =
        ((MP4TableProperty*)m_pProperties[4])->GetProperty(0);
    ASSERT(pProperty);
    ((MP4StringProperty*)pProperty)->SetUnicode(!utf8Flag);

    pProperty = ((MP4TableProperty*)m_pProperties[4])->GetProperty(1);
    ASSERT(pProperty);
    ((MP4StringProperty*)pProperty)->SetUnicode(!utf8Flag);

    ((MP4StringProperty*)m_pProperties[5])->SetUnicode(!utf8Flag);
}

class MP4CreatorTableProperty : public MP4TableProperty {
public:
    MP4CreatorTableProperty(MP4Atom& parentAtom, const char* name, MP4Integer8Property* pCountProperty) :
            MP4TableProperty(parentAtom, name, pCountProperty) {
    };
protected:
    void ReadEntry(MP4File& file, uint32_t index);
    void WriteEntry(MP4File& file, uint32_t index);
private:
    MP4CreatorTableProperty();
    MP4CreatorTableProperty ( const MP4CreatorTableProperty &src );
    MP4CreatorTableProperty &operator= ( const MP4CreatorTableProperty &src );
};

MP4CreatorDescriptor::MP4CreatorDescriptor(MP4Atom& parentAtom, uint8_t tag)
        : MP4Descriptor(parentAtom, tag)
{
    MP4Integer8Property* pCount =
        new MP4Integer8Property(parentAtom, "creatorCount");
    AddProperty(pCount); /* 0 */

    MP4TableProperty* pTable = new MP4CreatorTableProperty(parentAtom, "creators", pCount);
    AddProperty(pTable); /* 1 */

    pTable->AddProperty( /* Table 0 */
        new MP4BytesProperty(pTable->GetParentAtom(), "languageCode", 3, 3));
    pTable->AddProperty( /* Table 1 */
        new MP4BitfieldProperty(pTable->GetParentAtom(), "isUTF8String", 1));
    pTable->AddProperty( /* Table 2 */
        new MP4BitfieldProperty(pTable->GetParentAtom(), "reserved", 7));
    pTable->AddProperty( /* Table 3 */
        new MP4StringProperty(pTable->GetParentAtom(), "name", Counted));
}

void MP4CreatorTableProperty::ReadEntry(MP4File& file, uint32_t index)
{
    m_pProperties[0]->Read(file, index);
    m_pProperties[1]->Read(file, index);

    bool utf8Flag = ((MP4BitfieldProperty*)m_pProperties[1])->GetValue(index);
    ((MP4StringProperty*)m_pProperties[3])->SetUnicode(!utf8Flag);

    m_pProperties[2]->Read(file, index);
    m_pProperties[3]->Read(file, index);
}

void MP4CreatorTableProperty::WriteEntry(MP4File& file, uint32_t index)
{
    bool utf8Flag = ((MP4BitfieldProperty*)m_pProperties[1])->GetValue(index);
    ((MP4StringProperty*)m_pProperties[3])->SetUnicode(!utf8Flag);

    MP4TableProperty::WriteEntry(file, index);
}

MP4CreationDescriptor::MP4CreationDescriptor(MP4Atom& parentAtom, uint8_t tag)
        : MP4Descriptor(parentAtom, tag)
{
    AddProperty( /* 0 */
        new MP4BitfieldProperty(parentAtom, "contentCreationDate", 40));
}

MP4SmpteCameraDescriptor::MP4SmpteCameraDescriptor(MP4Atom& parentAtom)
        : MP4Descriptor(parentAtom)
{
    MP4Integer8Property* pCount =
        new MP4Integer8Property(parentAtom, "parameterCount");
    AddProperty(pCount);

    MP4TableProperty* pTable = new MP4TableProperty(parentAtom, "parameters", pCount);
    AddProperty(pTable);

    pTable->AddProperty(
        new MP4Integer8Property(parentAtom, "id"));
    pTable->AddProperty(
        new MP4Integer32Property(parentAtom, "value"));
}

MP4UnknownOCIDescriptor::MP4UnknownOCIDescriptor(MP4Atom& parentAtom)
        : MP4Descriptor(parentAtom)
{
    AddProperty( /* 0 */
        new MP4BytesProperty(parentAtom, "data"));
}

void MP4UnknownOCIDescriptor::Read(MP4File& file)
{
    ReadHeader(file);

    /* byte properties need to know how long they are before reading */
    ((MP4BytesProperty*)m_pProperties[0])->SetValueSize(m_size);

    ReadProperties(file);
}

MP4Descriptor* CreateOCIDescriptor(MP4Atom& parentAtom, uint8_t tag)
{
    MP4Descriptor* pDescriptor = NULL;

    switch (tag) {
    case MP4ContentClassDescrTag:
        pDescriptor = new MP4ContentClassDescriptor(parentAtom);
        break;
    case MP4KeywordDescrTag:
        pDescriptor = new MP4KeywordDescriptor(parentAtom);
        break;
    case MP4RatingDescrTag:
        pDescriptor = new MP4RatingDescriptor(parentAtom);
        break;
    case MP4LanguageDescrTag:
        pDescriptor = new MP4LanguageDescriptor(parentAtom);
        break;
    case MP4ShortTextDescrTag:
        pDescriptor = new MP4ShortTextDescriptor(parentAtom);
        break;
    case MP4ExpandedTextDescrTag:
        pDescriptor = new MP4ExpandedTextDescriptor(parentAtom);
        break;
    case MP4ContentCreatorDescrTag:
    case MP4OCICreatorDescrTag:
        pDescriptor = new MP4CreatorDescriptor(parentAtom, tag);
        break;
    case MP4ContentCreationDescrTag:
    case MP4OCICreationDescrTag:
        pDescriptor = new MP4CreationDescriptor(parentAtom, tag);
        break;
    case MP4SmpteCameraDescrTag:
        pDescriptor = new MP4SmpteCameraDescriptor(parentAtom);
        break;
    }

    if (pDescriptor == NULL) {
        if (tag >= MP4OCIDescrTagsStart && tag <= MP4OCIDescrTagsEnd) {
            pDescriptor = new MP4UnknownOCIDescriptor(parentAtom);
            pDescriptor->SetTag(tag);
        }
    }

    return pDescriptor;
}

///////////////////////////////////////////////////////////////////////////////

}
} // namespace mp4v2::impl
