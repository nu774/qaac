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

template<class type> class MP4Array {
public:
    MP4Array() {
        m_elements = NULL;
        m_numElements = 0;
        m_maxNumElements = 0;
    }

    ~MP4Array() {
        MP4Free(m_elements);
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

    inline void Add(type newElement) {
        Insert(newElement, m_numElements);
    }

    void Insert(type newElement, MP4ArrayIndex newIndex) {
        if (newIndex > m_numElements) {
            throw new PLATFORM_EXCEPTION("illegal array index", ERANGE);
        }
        if (m_numElements == m_maxNumElements) {
            MP4ArrayIndex newSize = max(m_maxNumElements, (MP4ArrayIndex)1) * 2;
            m_elements = (type*)MP4Realloc(m_elements,
                newSize * sizeof(type));
            m_maxNumElements = newSize;
        }
        memmove(&m_elements[newIndex + 1], &m_elements[newIndex],
            (m_numElements - newIndex) * sizeof(type));
        m_elements[newIndex] = newElement;
        m_numElements++;
    }

    void Delete(MP4ArrayIndex index) {
        if (!ValidIndex(index)) {
            ostringstream msg;
            msg << "illegal array index: " << index << " of " << m_numElements;
            throw new PLATFORM_EXCEPTION(msg.str().c_str(), ERANGE);
        }
        m_numElements--;
        if (index < m_numElements) {
            memmove(&m_elements[index], &m_elements[index + 1],
                (m_numElements - index) * sizeof(type));
        }
    }

    void Resize(MP4ArrayIndex newSize) {
        if ( (uint64_t) newSize * sizeof(type) > 0xFFFFFFFF )
            throw new PLATFORM_EXCEPTION("requested array size exceeds 4GB", ERANGE); /* prevent overflow */
        m_elements = (type*)MP4Realloc(m_elements,
        newSize * sizeof(type));
        m_numElements = newSize;
        m_maxNumElements = newSize;
    }

    type& operator[](MP4ArrayIndex index) {
        if (ValidIndex(index)) {
            return m_elements[index];
        }
        else {
            ostringstream msg;
            msg << "illegal array index: " << index << " of " << m_numElements;
            throw new PLATFORM_EXCEPTION(msg.str().c_str(), ERANGE);
        }
    }

protected:
    type*           m_elements;
    MP4ArrayIndex   m_numElements;
    MP4ArrayIndex   m_maxNumElements;
};

typedef MP4Array<uint8_t> MP4Integer8Array;
typedef MP4Array<uint16_t> MP4Integer16Array;
typedef MP4Array<uint32_t> MP4Integer32Array;
typedef MP4Array<uint64_t> MP4Integer64Array;
typedef MP4Array<float> MP4Float32Array;
typedef MP4Array<double> MP4Float64Array;
typedef MP4Array<char*> MP4StringArray;
typedef MP4Array<uint8_t*> MP4BytesArray;

///////////////////////////////////////////////////////////////////////////////

}
} // namespace mp4v2::impl

#endif // MP4V2_IMPL_MP4ARRAY_H
