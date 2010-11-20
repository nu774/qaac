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

#ifndef MP4V2_IMPL_MP4DESCRIPTOR_H
#define MP4V2_IMPL_MP4DESCRIPTOR_H

namespace mp4v2 {
namespace impl {

///////////////////////////////////////////////////////////////////////////////

class MP4Descriptor {
public:
    MP4Descriptor(uint8_t tag = 0);

    virtual ~MP4Descriptor();

    uint8_t GetTag() {
        return m_tag;
    }
    void SetTag(uint8_t tag) {
        m_tag = tag;
    }

    void SetParentAtom(MP4Atom* pParentAtom) {
        m_pParentAtom = pParentAtom;
        for (uint32_t i = 0; i < m_pProperties.Size(); i++) {
            m_pProperties[i]->SetParentAtom(pParentAtom);
        }
    }

    void AddProperty(MP4Property* pProperty);

    virtual void Generate();
    virtual void Read(MP4File* pFile);
    virtual void Write(MP4File* pFile);
    virtual void Dump(FILE* pFile, uint8_t indent, bool dumpImplicits);

    MP4Property* GetProperty(uint32_t index) {
        return m_pProperties[index];
    }

    // use with extreme caution
    void SetProperty(uint32_t index, MP4Property* pProperty) {
        m_pProperties[index] = pProperty;
    }

    bool FindProperty( const char* name, MP4Property** ppProperty,
                       uint32_t* pIndex = NULL)
    {
        return FindContainedProperty(name, ppProperty, pIndex);
    }

    void WriteToMemory(MP4File* pFile,
                       uint8_t** ppBytes, uint64_t* pNumBytes);

protected:
    void SetReadMutate(uint32_t propIndex) {
        m_readMutatePoint = propIndex;
    }

    void ReadHeader(MP4File* pFile);
    void ReadProperties(MP4File* pFile,
                        uint32_t startIndex = 0, uint32_t count = 0xFFFFFFFF);

    virtual void Mutate() {
        // default is a no-op
    };

    bool FindContainedProperty(const char* name,
                               MP4Property** ppProperty, uint32_t* pIndex);

    uint8_t GetDepth();

protected:
    MP4Atom*            m_pParentAtom;
    uint8_t             m_tag;
    uint64_t            m_start;
    uint32_t            m_size;
    MP4PropertyArray    m_pProperties;
    uint32_t            m_readMutatePoint;
};

///////////////////////////////////////////////////////////////////////////////

}
} // namespace mp4v2::impl

#endif // MP4V2_IMPL_MP4DESCRIPTOR_H
