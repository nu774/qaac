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

MP4BaseDescriptor::MP4BaseDescriptor (MP4Atom& parentAtom, uint8_t tag) : MP4Descriptor(parentAtom, tag)
{
    switch (tag) {
    case MP4ESIDIncDescrTag:
        AddProperty( /* 0 */
            new MP4Integer32Property(parentAtom, "id"));
        break;
    case MP4ESIDRefDescrTag:
        AddProperty( /* 0 */
            new MP4Integer16Property(parentAtom, "refIndex"));
        break;
    case MP4IPIPtrDescrTag:
        AddProperty( /* 0 */
            new MP4Integer16Property(parentAtom, "IPIESId"));
        break;
    case MP4SupplContentIdDescrTag:
        AddProperty( /* 0 */
            new MP4BytesProperty(parentAtom, "languageCode", 3));
        AddProperty( /* 1 */
            new MP4StringProperty(parentAtom, "title", Counted));
        AddProperty( /* 2 */
            new MP4StringProperty(parentAtom, "value", Counted));
        break;
    case MP4IPMPPtrDescrTag:
        AddProperty( /* 0 */
            new MP4Integer8Property(parentAtom, "IPMPDescriptorId"));
        break;
    case MP4ExtProfileLevelDescrTag:
        AddProperty( /* 0 */
            new MP4Integer8Property(parentAtom, "profileLevelIndicationIndex"));
        AddProperty( /* 1 */
            new MP4Integer8Property(parentAtom, "ODProfileLevelIndication"));
        AddProperty( /* 2 */
            new MP4Integer8Property(parentAtom, "sceneProfileLevelIndication"));
        AddProperty( /* 3 */
            new MP4Integer8Property(parentAtom, "audioProfileLevelIndication"));
        AddProperty( /* 4 */
            new MP4Integer8Property(parentAtom, "visualProfileLevelIndication"));
        AddProperty( /* 5 */
            new MP4Integer8Property(parentAtom, "graphicsProfileLevelIndication"));
        AddProperty( /* 6 */
            new MP4Integer8Property(parentAtom, "MPEGJProfileLevelIndication"));
        break;
    default:
        log.errorf("%s: \"%s\": error in base descriptor - tag %u", __FUNCTION__,
                   m_parentAtom.GetFile().GetFilename().c_str(), tag);
        break;

    }
}

MP4BytesDescriptor::MP4BytesDescriptor (MP4Atom& parentAtom, uint8_t tag) : MP4Descriptor(parentAtom, tag)
{
    m_size_offset = 0;
    m_bytes_index = 0;
    if (tag >= MP4ExtDescrTagsStart && tag <= MP4ExtDescrTagsEnd) {
        AddProperty( /* 0 */
            new MP4BytesProperty(parentAtom, "data"));
    } else {
        switch (tag) {
        case MP4DecSpecificDescrTag:
            AddProperty( /* 0 */
                new MP4BytesProperty(parentAtom, "info"));
            // no change to m_size
            break;
        case MP4IPMPDescrTag:
            AddProperty( /* 0 */
                new MP4Integer8Property(parentAtom, "IPMPDescriptorId"));
            AddProperty( /* 1 */
                new MP4Integer16Property(parentAtom, "IPMPSType"));
            AddProperty( /* 2 */
                new MP4BytesProperty(parentAtom, "IPMPData"));
            /* note: if IPMPSType == 0, IPMPData is an URL */
            m_size_offset = 3;
            m_bytes_index = 2;
            break;
        case MP4RegistrationDescrTag:
            AddProperty( /* 0 */
                new MP4Integer32Property(parentAtom, "formatIdentifier"));
            AddProperty( /* 1 */
                new MP4BytesProperty(parentAtom, "additionalIdentificationInfo"));
            m_size_offset = 4;
            m_bytes_index = 1;
            break;
        default:
            log.errorf("%s: \"%s\": error in bytes descriptor - tag %u", __FUNCTION__,
                       m_parentAtom.GetFile().GetFilename().c_str(), tag);
            break;
        }
    }
}

void MP4BytesDescriptor::Read(MP4File& file)
{
    ReadHeader(file);

    /* byte properties need to know how long they are before reading */
    ((MP4BytesProperty*)m_pProperties[m_bytes_index])->SetValueSize(m_size - m_size_offset);

    ReadProperties(file);
}
MP4IODescriptor::MP4IODescriptor(MP4Atom& parentAtom)
        : MP4Descriptor(parentAtom, MP4FileIODescrTag)
{
    /* N.B. other member functions depend on the property indicies */
    AddProperty( /* 0 */
        new MP4BitfieldProperty(parentAtom, "objectDescriptorId", 10));
    AddProperty( /* 1 */
        new MP4BitfieldProperty(parentAtom, "URLFlag", 1));
    AddProperty( /* 2 */
        new MP4BitfieldProperty(parentAtom, "includeInlineProfileLevelFlag", 1));
    AddProperty( /* 3 */
        new MP4BitfieldProperty(parentAtom, "reserved", 4));
    AddProperty( /* 4 */
        new MP4StringProperty(parentAtom, "URL", Counted));
    AddProperty( /* 5 */
        new MP4Integer8Property(parentAtom, "ODProfileLevelId"));
    AddProperty( /* 6 */
        new MP4Integer8Property(parentAtom, "sceneProfileLevelId"));
    AddProperty( /* 7 */
        new MP4Integer8Property(parentAtom, "audioProfileLevelId"));
    AddProperty( /* 8 */
        new MP4Integer8Property(parentAtom, "visualProfileLevelId"));
    AddProperty( /* 9 */
        new MP4Integer8Property(parentAtom, "graphicsProfileLevelId"));
    AddProperty( /* 10 */
        new MP4DescriptorProperty(parentAtom, "esIds",
                                  MP4ESIDIncDescrTag, 0, Required, Many));
    AddProperty( /* 11 */
        new MP4DescriptorProperty(parentAtom, "ociDescr",
                                  MP4OCIDescrTagsStart, MP4OCIDescrTagsEnd, Optional, Many));
    AddProperty( /* 12 */
        new MP4DescriptorProperty(parentAtom, "ipmpDescrPtr",
                                  MP4IPMPPtrDescrTag, 0, Optional, Many));
    AddProperty( /* 13 */
        new MP4DescriptorProperty(parentAtom, "extDescr",
                                  MP4ExtDescrTagsStart, MP4ExtDescrTagsEnd, Optional, Many));

    SetReadMutate(2);
}

void MP4IODescriptor::Generate()
{
    ((MP4BitfieldProperty*)m_pProperties[0])->SetValue(1);
    ((MP4BitfieldProperty*)m_pProperties[3])->SetValue(0xF);
    for (uint32_t i = 5; i <= 9; i++) {
        ((MP4Integer8Property*)m_pProperties[i])->SetValue(0xFF);
    }
}

void MP4IODescriptor::Mutate()
{
    bool urlFlag = ((MP4BitfieldProperty*)m_pProperties[1])->GetValue();

    m_pProperties[4]->SetImplicit(!urlFlag);
    for (uint32_t i = 5; i <= 12; i++) {
        m_pProperties[i]->SetImplicit(urlFlag);
    }
}

MP4ODescriptor::MP4ODescriptor(MP4Atom& parentAtom)
        : MP4Descriptor(parentAtom, MP4FileODescrTag)
{
    /* N.B. other member functions depend on the property indicies */
    AddProperty( /* 0 */
        new MP4BitfieldProperty(parentAtom, "objectDescriptorId", 10));
    AddProperty( /* 1 */
        new MP4BitfieldProperty(parentAtom, "URLFlag", 1));
    AddProperty( /* 2 */
        new MP4BitfieldProperty(parentAtom, "reserved", 5));
    AddProperty( /* 3 */
        new MP4StringProperty(parentAtom, "URL", Counted));
    AddProperty( /* 4 */
        new MP4DescriptorProperty(parentAtom, "esIds",
                                  MP4ESIDRefDescrTag, 0, Required, Many));
    AddProperty( /* 5 */
        new MP4DescriptorProperty(parentAtom, "ociDescr",
                                  MP4OCIDescrTagsStart, MP4OCIDescrTagsEnd, Optional, Many));
    AddProperty( /* 6 */
        new MP4DescriptorProperty(parentAtom, "ipmpDescrPtr",
                                  MP4IPMPPtrDescrTag, 0, Optional, Many));
    AddProperty( /* 7 */
        new MP4DescriptorProperty(parentAtom, "extDescr",
                                  MP4ExtDescrTagsStart, MP4ExtDescrTagsEnd, Optional, Many));

    SetReadMutate(2);
}

void MP4ODescriptor::Generate()
{
    ((MP4BitfieldProperty*)m_pProperties[2])->SetValue(0x1F);
}

void MP4ODescriptor::Mutate()
{
    bool urlFlag = ((MP4BitfieldProperty*)m_pProperties[1])->GetValue();

    m_pProperties[3]->SetImplicit(!urlFlag);
    for (uint32_t i = 4; i <= 6; i++) {
        m_pProperties[i]->SetImplicit(urlFlag);
    }
}

MP4ESDescriptor::MP4ESDescriptor(MP4Atom& parentAtom)
        : MP4Descriptor(parentAtom, MP4ESDescrTag)
{
    /* N.B. other class functions depend on the property indicies */
    AddProperty( /* 0 */
        new MP4Integer16Property(parentAtom, "ESID"));
    AddProperty( /* 1 */
        new MP4BitfieldProperty(parentAtom, "streamDependenceFlag", 1));
    AddProperty( /* 2 */
        new MP4BitfieldProperty(parentAtom, "URLFlag", 1));
    AddProperty( /* 3 */
        new MP4BitfieldProperty(parentAtom, "OCRstreamFlag", 1));
    AddProperty( /* 4 */
        new MP4BitfieldProperty(parentAtom, "streamPriority", 5));
    AddProperty( /* 5 */
        new MP4Integer16Property(parentAtom, "dependsOnESID"));
    AddProperty( /* 6 */
        new MP4StringProperty(parentAtom, "URL", Counted));
    AddProperty( /* 7 */
        new MP4Integer16Property(parentAtom, "OCRESID"));
    AddProperty( /* 8 */
        new MP4DescriptorProperty(parentAtom, "decConfigDescr",
                                  MP4DecConfigDescrTag, 0, Required, OnlyOne));
    AddProperty( /* 9 */
        new MP4DescriptorProperty(parentAtom, "slConfigDescr",
                                  MP4SLConfigDescrTag, 0, Required, OnlyOne));
    AddProperty( /* 10 */
        new MP4DescriptorProperty(parentAtom, "ipiPtr",
                                  MP4IPIPtrDescrTag, 0, Optional, OnlyOne));
    AddProperty( /* 11 */
        new MP4DescriptorProperty(parentAtom, "ipIds",
                                  MP4ContentIdDescrTag, MP4SupplContentIdDescrTag, Optional, Many));
    AddProperty( /* 12 */
        new MP4DescriptorProperty(parentAtom, "ipmpDescrPtr",
                                  MP4IPMPPtrDescrTag, 0, Optional, Many));
    AddProperty( /* 13 */
        new MP4DescriptorProperty(parentAtom, "langDescr",
                                  MP4LanguageDescrTag, 0, Optional, Many));
    AddProperty( /* 14 */
        new MP4DescriptorProperty(parentAtom, "qosDescr",
                                  MP4QosDescrTag, 0, Optional, OnlyOne));
    AddProperty( /* 15 */
        new MP4DescriptorProperty(parentAtom, "regDescr",
                                  MP4RegistrationDescrTag, 0, Optional, OnlyOne));
    AddProperty( /* 16 */
        new MP4DescriptorProperty(parentAtom, "extDescr",
                                  MP4ExtDescrTagsStart, MP4ExtDescrTagsEnd, Optional, Many));

    SetReadMutate(5);
}

void MP4ESDescriptor::Mutate()
{
    bool streamDependFlag =
        ((MP4BitfieldProperty*)m_pProperties[1])->GetValue();
    m_pProperties[5]->SetImplicit(!streamDependFlag);

    bool urlFlag =
        ((MP4BitfieldProperty*)m_pProperties[2])->GetValue();
    m_pProperties[6]->SetImplicit(!urlFlag);

    bool ocrFlag =
        ((MP4BitfieldProperty*)m_pProperties[3])->GetValue();
    m_pProperties[7]->SetImplicit(!ocrFlag);
}

MP4DecConfigDescriptor::MP4DecConfigDescriptor(MP4Atom& parentAtom)
        : MP4Descriptor(parentAtom, MP4DecConfigDescrTag)
{
    AddProperty( /* 0 */
        new MP4Integer8Property(parentAtom, "objectTypeId"));
    AddProperty( /* 1 */
        new MP4BitfieldProperty(parentAtom, "streamType", 6));
    AddProperty( /* 2 */
        new MP4BitfieldProperty(parentAtom, "upStream", 1));
    AddProperty( /* 3 */
        new MP4BitfieldProperty(parentAtom, "reserved", 1));
    AddProperty( /* 4 */
        new MP4BitfieldProperty(parentAtom, "bufferSizeDB", 24));
    AddProperty( /* 5 */
        new MP4Integer32Property(parentAtom, "maxBitrate"));
    AddProperty( /* 6 */
        new MP4Integer32Property(parentAtom, "avgBitrate"));
    AddProperty( /* 7 */
        new MP4DescriptorProperty(parentAtom, "decSpecificInfo",
                                  MP4DecSpecificDescrTag, 0, Optional, OnlyOne));
    AddProperty( /* 8 */
        new MP4DescriptorProperty(parentAtom, "profileLevelIndicationIndexDescr",
                                  MP4ExtProfileLevelDescrTag, 0, Optional, Many));
}

void MP4DecConfigDescriptor::Generate()
{
    ((MP4BitfieldProperty*)m_pProperties[3])->SetValue(1);
}

MP4SLConfigDescriptor::MP4SLConfigDescriptor(MP4Atom& parentAtom)
        : MP4Descriptor(parentAtom, MP4SLConfigDescrTag)
{
    AddProperty( /* 0 */
        new MP4Integer8Property(parentAtom, "predefined"));
    AddProperty( /* 1 */
        new MP4BitfieldProperty(parentAtom, "useAccessUnitStartFlag", 1));
    AddProperty( /* 2 */
        new MP4BitfieldProperty(parentAtom, "useAccessUnitEndFlag", 1));
    AddProperty( /* 3 */
        new MP4BitfieldProperty(parentAtom, "useRandomAccessPointFlag", 1));
    AddProperty( /* 4 */
        new MP4BitfieldProperty(parentAtom, "hasRandomAccessUnitsOnlyFlag", 1));
    AddProperty( /* 5 */
        new MP4BitfieldProperty(parentAtom, "usePaddingFlag", 1));
    AddProperty( /* 6 */
        new MP4BitfieldProperty(parentAtom, "useTimeStampsFlag", 1));
    AddProperty( /* 7 */
        new MP4BitfieldProperty(parentAtom, "useIdleFlag", 1));
    AddProperty( /* 8 */
        new MP4BitfieldProperty(parentAtom, "durationFlag", 1));
    AddProperty( /* 9 */
        new MP4Integer32Property(parentAtom, "timeStampResolution"));
    AddProperty( /* 10 */
        new MP4Integer32Property(parentAtom, "OCRResolution"));
    AddProperty( /* 11 */
        new MP4Integer8Property(parentAtom, "timeStampLength"));
    AddProperty( /* 12 */
        new MP4Integer8Property(parentAtom, "OCRLength"));
    AddProperty( /* 13 */
        new MP4Integer8Property(parentAtom, "AULength"));
    AddProperty( /* 14 */
        new MP4Integer8Property(parentAtom, "instantBitrateLength"));
    AddProperty( /* 15 */
        new MP4BitfieldProperty(parentAtom, "degradationPriortyLength", 4));
    AddProperty( /* 16 */
        new MP4BitfieldProperty(parentAtom, "AUSeqNumLength", 5));
    AddProperty( /* 17 */
        new MP4BitfieldProperty(parentAtom, "packetSeqNumLength", 5));
    AddProperty( /* 18 */
        new MP4BitfieldProperty(parentAtom, "reserved", 2));

    // if durationFlag
    AddProperty( /* 19 */
        new MP4Integer32Property(parentAtom, "timeScale"));
    AddProperty( /* 20 */
        new MP4Integer16Property(parentAtom, "accessUnitDuration"));
    AddProperty( /* 21 */
        new MP4Integer16Property(parentAtom, "compositionUnitDuration"));

    // if !useTimeStampsFlag
    AddProperty( /* 22 */
        new MP4BitfieldProperty(parentAtom, "startDecodingTimeStamp", 64));
    AddProperty( /* 23 */
        new MP4BitfieldProperty(parentAtom, "startCompositionTimeStamp", 64));
}

void MP4SLConfigDescriptor::Generate()
{
    // by default all tracks in an mp4 file
    // use predefined SLConfig descriptor == 2
    ((MP4Integer8Property*)m_pProperties[0])->SetValue(2);

    // which implies UseTimestampsFlag = 1
    ((MP4BitfieldProperty*)m_pProperties[6])->SetValue(1);

    // reserved = 3
    ((MP4BitfieldProperty*)m_pProperties[18])->SetValue(3);
}

void MP4SLConfigDescriptor::Read(MP4File& file)
{
    ReadHeader(file);

    // read the first property, 'predefined'
    ReadProperties(file, 0, 1);

    // if predefined == 0
    if (((MP4Integer8Property*)m_pProperties[0])->GetValue() == 0) {

        /* read the next 18 properties */
        ReadProperties(file, 1, 18);
    }

    // now mutate
    Mutate();

    // and read the remaining properties
    ReadProperties(file, 19);
}

void MP4SLConfigDescriptor::Mutate()
{
    uint32_t i;
    uint8_t predefined =
        ((MP4Integer8Property*)m_pProperties[0])->GetValue();

    if (predefined) {
        // properties 1-18 are implicit
        for (i = 1; i < m_pProperties.Size(); i++) {
            m_pProperties[i]->SetImplicit(true);
        }

        if (predefined == 1) {
            // UseTimestampsFlag = 0
            ((MP4BitfieldProperty*)m_pProperties[6])->SetValue(0);

            // TimestampResolution = 1000
            ((MP4Integer32Property*)m_pProperties[9])->SetValue(1000);

            // TimeStampLength = 32
            ((MP4Integer8Property*)m_pProperties[11])->SetValue(32);

        } else if (predefined == 2) {
            // UseTimestampsFlag = 1
            ((MP4BitfieldProperty*)m_pProperties[6])->SetValue(1);
        }
    } else {
#if 1
        for (i = 1; i <= 18; i++) {
            m_pProperties[i]->SetImplicit(false);
        }
        ((MP4BitfieldProperty*)m_pProperties[18])->SetValue(3);
#endif
    }

    bool durationFlag =
        ((MP4BitfieldProperty*)m_pProperties[8])->GetValue();

    for (i = 19; i <= 21; i++) {
        m_pProperties[i]->SetImplicit(!durationFlag);
    }

    bool useTimeStampsFlag =
        ((MP4BitfieldProperty*)m_pProperties[6])->GetValue();

    for (i = 22; i <= 23; i++) {
        m_pProperties[i]->SetImplicit(useTimeStampsFlag);

        uint8_t timeStampLength = min((uint8_t)64,
                                      ((MP4Integer8Property*)m_pProperties[11])->GetValue());

        ((MP4BitfieldProperty*)m_pProperties[i])->SetNumBits(timeStampLength);

        // handle a nonsensical situation gracefully
        if (timeStampLength == 0) {
            m_pProperties[i]->SetImplicit(true);
        }
    }
}

MP4ContentIdDescriptor::MP4ContentIdDescriptor(MP4Atom& parentAtom)
        : MP4Descriptor(parentAtom, MP4ContentIdDescrTag)
{
    AddProperty( /* 0 */
        new MP4BitfieldProperty(parentAtom, "compatibility", 2));
    AddProperty( /* 1 */
        new MP4BitfieldProperty(parentAtom, "contentTypeFlag", 1));
    AddProperty( /* 2 */
        new MP4BitfieldProperty(parentAtom, "contentIdFlag", 1));
    AddProperty( /* 3 */
        new MP4BitfieldProperty(parentAtom, "protectedContent", 1));
    AddProperty( /* 4 */
        new MP4BitfieldProperty(parentAtom, "reserved", 3));
    AddProperty( /* 5 */
        new MP4Integer8Property(parentAtom, "contentType"));
    AddProperty( /* 6 */
        new MP4Integer8Property(parentAtom, "contentIdType"));
    AddProperty( /* 7 */
        new MP4BytesProperty(parentAtom, "contentId"));
}

void MP4ContentIdDescriptor::Read(MP4File& file)
{
    ReadHeader(file);

    /* read the first property, 'compatiblity' */
    ReadProperties(file, 0, 1);

    /* if compatiblity != 0 */
    if (((MP4Integer8Property*)m_pProperties[0])->GetValue() != 0) {
        /* we don't understand it */
        log.verbose1f("incompatible content id descriptor");
        return;
    }

    /* read the next four properties */
    ReadProperties(file, 1, 4);

    /* which allows us to reconfigure ourselves */
    Mutate();

    bool contentTypeFlag = ((MP4BitfieldProperty*)m_pProperties[1])->GetValue();

    bool contentIdFlag = ((MP4BitfieldProperty*)m_pProperties[2])->GetValue();

    if (contentIdFlag) {

        uint32_t cIdOffset = 2;

        if (contentTypeFlag) {

            cIdOffset++;

        }

        ((MP4BytesProperty*)m_pProperties[7])->SetValueSize(m_size - cIdOffset);

    }



    /* read the remaining properties */
    ReadProperties(file, 5);
}

void MP4ContentIdDescriptor::Mutate()
{
    bool contentTypeFlag = ((MP4BitfieldProperty*)m_pProperties[1])->GetValue();
    m_pProperties[5]->SetImplicit(!contentTypeFlag);

    bool contentIdFlag = ((MP4BitfieldProperty*)m_pProperties[2])->GetValue();
    m_pProperties[6]->SetImplicit(!contentIdFlag);
    m_pProperties[7]->SetImplicit(!contentIdFlag);

}

MP4Descriptor* MP4DescriptorProperty::CreateDescriptor(MP4Atom& parentAtom, uint8_t tag)
{
    MP4Descriptor* pDescriptor = NULL;

    switch (tag) {
    case MP4ESDescrTag:
        pDescriptor = new MP4ESDescriptor(parentAtom);
        break;
    case MP4DecConfigDescrTag:
        pDescriptor = new MP4DecConfigDescriptor(parentAtom);
        break;
    case MP4DecSpecificDescrTag:
    case MP4IPMPDescrTag:
    case MP4RegistrationDescrTag:
        pDescriptor = new MP4BytesDescriptor(parentAtom, tag);
        break;
    case MP4SLConfigDescrTag:
        pDescriptor = new MP4SLConfigDescriptor(parentAtom);
        break;
    case MP4ContentIdDescrTag:
        pDescriptor = new MP4ContentIdDescriptor(parentAtom);
        break;
    case MP4ESIDIncDescrTag:
    case MP4ESIDRefDescrTag:
    case MP4IPIPtrDescrTag:
    case MP4SupplContentIdDescrTag:
    case MP4IPMPPtrDescrTag:
    case MP4ExtProfileLevelDescrTag:
        pDescriptor = new MP4BaseDescriptor(parentAtom, tag);
        break;
    case MP4QosDescrTag:
        pDescriptor = new MP4QosDescriptorBase(parentAtom, MP4QosDescrTag);
        break;
    case MP4IODescrTag:
    case MP4FileIODescrTag:
        pDescriptor = new MP4IODescriptor(parentAtom);
        pDescriptor->SetTag(tag);
        break;
    case MP4ODescrTag:
    case MP4FileODescrTag:
        pDescriptor = new MP4ODescriptor(parentAtom);
        pDescriptor->SetTag(tag);
        break;
    }

    if (pDescriptor == NULL) {
        if (tag >= MP4OCIDescrTagsStart && tag <= MP4OCIDescrTagsEnd) {
            pDescriptor = CreateOCIDescriptor(parentAtom, tag);
        }

        if (tag >= MP4ExtDescrTagsStart && tag <= MP4ExtDescrTagsEnd) {
            pDescriptor = new MP4BytesDescriptor(parentAtom, tag);
        }
    }

    return pDescriptor;
}

///////////////////////////////////////////////////////////////////////////////

}
} // namespace mp4v2::impl
