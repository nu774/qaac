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

#ifndef MP4V2_IMPL_MP4ARRAY_H
#define MP4V2_IMPL_MP4ARRAY_H

namespace mp4v2 {
namespace impl {

///////////////////////////////////////////////////////////////////////////////

typedef uint32_t MP4ArrayIndex;

class MP4Array {
public:
    MP4Array() {
        m_numElements = 0;
        m_maxNumElements = 0;
    }

    inline bool ValidIndex(MP4ArrayIndex index) {
        return (index < m_numElements);
    }

    inline MP4ArrayIndex Size(void) {
        return m_numElements;
    }

    inline MP4ArrayIndex MaxSize(void) {
        return m_maxNumElements;
    }

protected:
    MP4ArrayIndex   m_numElements;
    MP4ArrayIndex   m_maxNumElements;
};

// macro to generate subclasses
// we use this as an alternative to templates
// due to the excessive compile time price of extensive template usage

#define MP4ARRAY_DECL(name, type) \
    class name##Array : public MP4Array { \
    public: \
        name##Array() { \
            m_elements = NULL; \
        } \
        \
        ~name##Array() { \
            MP4Free(m_elements); \
        } \
        \
        inline void Add(type newElement) { \
            Insert(newElement, m_numElements); \
        } \
        \
        void Insert(type newElement, MP4ArrayIndex newIndex) { \
            if (newIndex > m_numElements) { \
                  throw new PlatformException("illegal array index", ERANGE, __FILE__, __LINE__, __FUNCTION__); \
            } \
            if (m_numElements == m_maxNumElements) { \
                m_maxNumElements = max(m_maxNumElements, (MP4ArrayIndex)1) * 2; \
                m_elements = (type*)MP4Realloc(m_elements, \
                    m_maxNumElements * sizeof(type)); \
            } \
            memmove(&m_elements[newIndex + 1], &m_elements[newIndex], \
                (m_numElements - newIndex) * sizeof(type)); \
            m_elements[newIndex] = newElement; \
            m_numElements++; \
        } \
        \
        void Delete(MP4ArrayIndex index) { \
            if (!ValidIndex(index)) { \
                ostringstream msg; \
                msg << "illegal array index: " << index << " of " << m_numElements; \
                throw new PlatformException(msg.str().c_str(), ERANGE, __FILE__, __LINE__, __FUNCTION__); \
            } \
            m_numElements--; \
            if (index < m_numElements) { \
              memmove(&m_elements[index], &m_elements[index + 1], \
                  (m_numElements - index) * sizeof(type)); \
            } \
        } \
        void Resize(MP4ArrayIndex newSize) { \
            m_numElements = newSize; \
            m_maxNumElements = newSize; \
            m_elements = (type*)MP4Realloc(m_elements, \
                m_maxNumElements * sizeof(type)); \
        } \
        \
        type& operator[](MP4ArrayIndex index) { \
            if (ValidIndex(index)) { \
                return m_elements[index]; \
            } \
            else { \
                ostringstream msg; \
                msg << "illegal array index: " << index << " of " << m_numElements; \
                throw new PlatformException(msg.str().c_str(), ERANGE, __FILE__, __LINE__, __FUNCTION__ ); \
            } \
        } \
        \
    protected: \
        type*   m_elements; \
    };

MP4ARRAY_DECL(MP4Integer8, uint8_t)

MP4ARRAY_DECL(MP4Integer16, uint16_t)

MP4ARRAY_DECL(MP4Integer32, uint32_t)

MP4ARRAY_DECL(MP4Integer64, uint64_t)

MP4ARRAY_DECL(MP4Float32, float)

MP4ARRAY_DECL(MP4Float64, double)

MP4ARRAY_DECL(MP4String, char*)

MP4ARRAY_DECL(MP4Bytes, uint8_t*)

///////////////////////////////////////////////////////////////////////////////

}
} // namespace mp4v2::impl

#endif // MP4V2_IMPL_MP4ARRAY_H
