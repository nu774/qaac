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

MP4Descriptor::MP4Descriptor(MP4Atom& parentAtom, uint8_t tag)
    : m_parentAtom(parentAtom)
{
    m_tag = tag;
    m_start = 0;
    m_size = 0;
    m_readMutatePoint = 0;
}

MP4Descriptor::~MP4Descriptor()
{
    for (uint32_t i = 0; i < m_pProperties.Size(); i++) {
        delete m_pProperties[i];
    }
}

void MP4Descriptor::AddProperty(MP4Property* pProperty)
{
    ASSERT(pProperty);
    m_pProperties.Add(pProperty);
}

bool MP4Descriptor::FindContainedProperty(const char *name,
        MP4Property** ppProperty, uint32_t* pIndex)
{
    uint32_t numProperties = m_pProperties.Size();

    for (uint32_t i = 0; i < numProperties; i++) {
        if (m_pProperties[i]->FindProperty(name, ppProperty, pIndex)) {
            return true;
        }
    }
    return false;
}

void MP4Descriptor::Generate()
{
    // generate properties
    for (uint32_t i = 0; i < m_pProperties.Size(); i++) {
        m_pProperties[i]->Generate();
    }
}

void MP4Descriptor::Read(MP4File& file)
{
    ReadHeader(file);

    ReadProperties(file, 0, m_readMutatePoint);

    Mutate();

    ReadProperties(file, m_readMutatePoint);

    // flush any leftover read bits
    file.FlushReadBits();
}

void MP4Descriptor::ReadHeader(MP4File& file)
{
    log.verbose1f("\"%s\": ReadDescriptor: pos = 0x%" PRIx64, file.GetFilename().c_str(),
                  file.GetPosition());

    // read tag and length
    uint8_t tag = file.ReadUInt8();
    if (m_tag) {
        ASSERT(tag == m_tag);
    } else {
        m_tag = tag;
    }
    m_size = file.ReadMpegLength();
    m_start = file.GetPosition();

    log.verbose1f("\"%s\": ReadDescriptor: tag 0x%02x data size %u (0x%x)",
                  file.GetFilename().c_str(), m_tag, m_size, m_size);
}

void MP4Descriptor::ReadProperties(MP4File& file,
                                   uint32_t propStartIndex, uint32_t propCount)
{
    uint32_t numProperties = min(propCount,
                                 m_pProperties.Size() - propStartIndex);

    for (uint32_t i = propStartIndex;
            i < propStartIndex + numProperties; i++) {

        MP4Property* pProperty = m_pProperties[i];

        int32_t remaining = m_size - (file.GetPosition() - m_start);

        if (pProperty->GetType() == DescriptorProperty) {
            if (remaining > 0) {
                // place a limit on how far this sub-descriptor looks
                ((MP4DescriptorProperty*)pProperty)->SetSizeLimit(remaining);
                pProperty->Read(file);
            } // else do nothing, empty descriptor
        } else {
            // non-descriptor property
            if (remaining >= 0) {
                pProperty->Read(file);

                MP4LogLevel thisVerbosity =
                    (pProperty->GetType() == TableProperty) ?
                    MP4_LOG_VERBOSE2 : MP4_LOG_VERBOSE1;

                if (log.verbosity >= thisVerbosity) {
                    // log.printf(thisVerbosity,"Read: ");
                    pProperty->Dump(0, true);
                }
            } else {
                log.errorf("%s: \"%s\": Overran descriptor, tag %u data size %u property %u",
                           __FUNCTION__, file.GetFilename().c_str(), m_tag, m_size, i);
                throw new Exception("overran descriptor",__FILE__, __LINE__, __FUNCTION__);
            }
        }
    }
}

void MP4Descriptor::Write(MP4File& file)
{
    // call virtual function to adapt properties before writing
    Mutate();

    uint32_t numProperties = m_pProperties.Size();

    if (numProperties == 0) {
        WARNING(numProperties == 0);
        return;
    }

    // write tag and length placeholder
    file.WriteUInt8(m_tag);
    uint64_t lengthPos = file.GetPosition();
    file.WriteMpegLength(0);
    uint64_t startPos = file.GetPosition();

    for (uint32_t i = 0; i < numProperties; i++) {
        m_pProperties[i]->Write(file);
    }

    // align with byte boundary (rarely necessary)
    file.PadWriteBits();

    // go back and write correct length
    uint64_t endPos = file.GetPosition();
    file.SetPosition(lengthPos);
    file.WriteMpegLength(endPos - startPos);
    file.SetPosition(endPos);
}

void MP4Descriptor::WriteToMemory(MP4File& file,
                                  uint8_t** ppBytes, uint64_t* pNumBytes)
{
    // use memory buffer to save descriptor in memory
    // instead of going directly to disk

    file.EnableMemoryBuffer();

    Write(file);

    file.DisableMemoryBuffer(ppBytes, pNumBytes);
}

void MP4Descriptor::Dump(uint8_t indent, bool dumpImplicits)
{
    // call virtual function to adapt properties before dumping
    Mutate();

    uint32_t numProperties = m_pProperties.Size();

    if (numProperties == 0) {
        WARNING(numProperties == 0);
        return;
    }
    for (uint32_t i = 0; i < numProperties; i++) {
        m_pProperties[i]->Dump(indent, dumpImplicits);
    }
}

uint8_t MP4Descriptor::GetDepth()
{
    return m_parentAtom.GetDepth();
}

///////////////////////////////////////////////////////////////////////////////

}
} // namespace mp4v2::impl
