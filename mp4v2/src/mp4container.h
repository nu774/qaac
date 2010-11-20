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

#ifndef MP4V2_IMPL_MP4CONTAINER_H
#define MP4V2_IMPL_MP4CONTAINER_H

namespace mp4v2 {
namespace impl {

///////////////////////////////////////////////////////////////////////////////

// base class - container of mp4 properties
class MP4Container {
public:
    MP4Container() { }

    virtual ~MP4Container();

    void AddProperty(MP4Property* pProperty);

    virtual void Read(MP4File* pFile);

    virtual void Write(MP4File* pFile);

    virtual void Dump(FILE* pFile, uint8_t indent, bool dumpImplicits);

    MP4Property* GetProperty(uint32_t index) {
        return m_pProperties[index];
    }

    // LATER MP4Property* GetProperty(const char* name); throw on error
    // LATER MP4Property* FindProperty(const char* name, uint32_t* pIndex = NULL); returns NULL on error

    bool FindProperty(const char* name,
                      MP4Property** ppProperty, uint32_t* pIndex = NULL);

    void FindIntegerProperty(const char* name,
                             MP4Property** ppProperty, uint32_t* pIndex = NULL);

    uint64_t GetIntegerProperty(const char* name);

    void SetIntegerProperty(const char* name, uint64_t value);

    void FindFloatProperty(const char* name,
                           MP4Property** ppProperty, uint32_t* pIndex = NULL);

    float GetFloatProperty(const char* name);

    void SetFloatProperty(const char* name, float value);

    void FindStringProperty(const char* name,
                            MP4Property** ppProperty, uint32_t* pIndex = NULL);

    const char* GetStringProperty(const char* name);

    void SetStringProperty(const char* name, const char* value);

    void FindBytesProperty(const char* name,
                           MP4Property** ppProperty, uint32_t* pIndex = NULL);

    void GetBytesProperty(const char* name,
                          uint8_t** ppValue, uint32_t* pValueSize);

    void SetBytesProperty(const char* name,
                          const uint8_t* pValue, uint32_t valueSize);

protected:
    MP4PropertyArray    m_pProperties;
};

///////////////////////////////////////////////////////////////////////////////

}
} // namespace mp4v2::impl

#endif // MP4V2_IMPL_MP4CONTAINER_H
