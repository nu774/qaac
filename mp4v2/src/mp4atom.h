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
 * Copyright (C) Cisco Systems Inc. 2001 - 2004.  All Rights Reserved.
 *
 * 3GPP features implementation is based on 3GPP's TS26.234-v5.60,
 * and was contributed by Ximpo Group Ltd.
 *
 * Portions created by Ximpo Group Ltd. are
 * Copyright (C) Ximpo Group Ltd. 2003, 2004.  All Rights Reserved.
 *
 * Contributor(s):
 *      Dave Mackie     dmackie@cisco.com
 *              Ximpo Group Ltd.        mp4v2@ximpo.com
 */

#ifndef MP4V2_IMPL_MP4ATOM_H
#define MP4V2_IMPL_MP4ATOM_H

namespace mp4v2 { namespace impl {

///////////////////////////////////////////////////////////////////////////////

class MP4Atom;
MP4ARRAY_DECL(MP4Atom, MP4Atom*);

#define Required    true
#define Optional    false
#define OnlyOne     true
#define Many        false
#define Counted     true

/* helper class */
class MP4AtomInfo {
public:
    MP4AtomInfo() {
        m_name = NULL;
    }
    MP4AtomInfo(const char* name, bool mandatory, bool onlyOne);

    const char* m_name;
    bool m_mandatory;
    bool m_onlyOne;
    uint32_t m_count;
};

MP4ARRAY_DECL(MP4AtomInfo, MP4AtomInfo*);

class MP4Atom
{
public:
    static MP4Atom* ReadAtom( MP4File& file, MP4Atom* pParentAtom );
    static MP4Atom* CreateAtom( MP4File& file, MP4Atom* parent, const char* type );
    static bool IsReasonableType( const char* type );

private:
    static MP4Atom* factory( MP4File &file, MP4Atom* parent, const char* type );
    static bool descendsFrom( MP4Atom* parent, const char* type );

public:
    MP4Atom(MP4File& file, const char* type = NULL);
    virtual ~MP4Atom();

    MP4File& GetFile() {
        return m_File;
    };

    uint64_t GetStart() {
        return m_start;
    };
    void SetStart(uint64_t pos) {
        m_start = pos;
    };

    uint64_t GetEnd() {
        return m_end;
    };
    void SetEnd(uint64_t pos) {
        m_end = pos;
    };

    uint64_t GetSize() {
        return m_size;
    }
    void SetSize(uint64_t size) {
        m_size = size;
    }

    const char* GetType() {
        return m_type;
    };
    void SetType(const char* type) {
        if (type && *type != '\0') {
            // not needed ASSERT(strlen(type) == 4);
            memcpy(m_type, type, 4);
            m_type[4] = '\0';
        } else {
            memset(m_type, 0, 5);
        }
    }

    void GetExtendedType(uint8_t* pExtendedType) {
        memcpy(pExtendedType, m_extendedType, sizeof(m_extendedType));
    };
    void SetExtendedType(uint8_t* pExtendedType) {
        memcpy(m_extendedType, pExtendedType, sizeof(m_extendedType));
    };

    bool IsUnknownType() {
        return m_unknownType;
    }
    void SetUnknownType(bool unknownType = true) {
        m_unknownType = unknownType;
    }

    bool IsRootAtom() {
        return m_type[0] == '\0';
    }

    MP4Atom* GetParentAtom() {
        return m_pParentAtom;
    }
    void SetParentAtom(MP4Atom* pParentAtom) {
        m_pParentAtom = pParentAtom;
    }

    void AddChildAtom(MP4Atom* pChildAtom) {
        pChildAtom->SetParentAtom(this);
        m_pChildAtoms.Add(pChildAtom);
    }

    void InsertChildAtom(MP4Atom* pChildAtom, uint32_t index) {
        pChildAtom->SetParentAtom(this);
        m_pChildAtoms.Insert(pChildAtom, index);
    }

    void DeleteChildAtom(MP4Atom* pChildAtom) {
        for (MP4ArrayIndex i = 0; i < m_pChildAtoms.Size(); i++) {
            if (m_pChildAtoms[i] == pChildAtom) {
                m_pChildAtoms.Delete(i);
                return;
            }
        }
    }

    uint32_t GetNumberOfChildAtoms() {
        return m_pChildAtoms.Size();
    }

    MP4Atom* GetChildAtom(uint32_t index) {
        return m_pChildAtoms[index];
    }

    MP4Property* GetProperty(uint32_t index) {
        return m_pProperties[index];
    }

    uint32_t GetCount() {
        return m_pProperties.Size();
    }

    MP4Atom* FindAtom(const char* name);

    MP4Atom* FindChildAtom(const char* name);

    bool FindProperty(const char* name,
                      MP4Property** ppProperty, uint32_t* pIndex = NULL);

    uint32_t GetFlags();
    void SetFlags(uint32_t flags);

    uint8_t GetDepth();

    void Skip();

    virtual void Generate();
    virtual void Read();
    virtual void BeginWrite(bool use64 = false);
    virtual void Write();
    virtual void Rewrite();
    virtual void FinishWrite(bool use64 = false);
    virtual void Dump(uint8_t indent, bool dumpImplicits);

    bool GetLargesizeMode();

protected:
    void AddProperty(MP4Property* pProperty);

    void AddVersionAndFlags();

    void AddReserved(MP4Atom& parentAtom, const char* name, uint32_t size);

    void ExpectChildAtom(const char* name,
                         bool mandatory, bool onlyOne = true);

    MP4AtomInfo* FindAtomInfo(const char* name);

    bool IsMe(const char* name);

    bool FindContainedProperty(const char* name,
                               MP4Property** ppProperty, uint32_t* pIndex);

    void ReadProperties(
        uint32_t startIndex = 0, uint32_t count = 0xFFFFFFFF);
    void ReadChildAtoms();

    void WriteProperties(
        uint32_t startIndex = 0, uint32_t count = 0xFFFFFFFF);
    void WriteChildAtoms();

    uint8_t GetVersion();
    void SetVersion(uint8_t version);

    void SetLargesizeMode( bool );

protected:
    MP4File&    m_File;
    uint64_t    m_start;
    uint64_t    m_end;
    bool        m_largesizeMode; // true if largesize mode
    uint64_t    m_size;
    char        m_type[5];
    bool        m_unknownType;
    uint8_t m_extendedType[16];

    MP4Atom*    m_pParentAtom;
    uint8_t m_depth;

    MP4PropertyArray    m_pProperties;
    MP4AtomInfoArray    m_pChildAtomInfos;
    MP4AtomArray        m_pChildAtoms;
private:
    MP4Atom();
    MP4Atom( const MP4Atom &src );
    MP4Atom &operator= ( const MP4Atom &src );
};

inline uint32_t ATOMID(const char* type) {
    return STRTOINT32(type);
}

// inverse ATOMID - 32 bit id to string
inline void IDATOM(uint32_t type, char *s) {
    INT32TOSTR(type, s);
}

///////////////////////////////////////////////////////////////////////////////

}} // namespace mp4v2::impl

#endif // MP4V2_IMPL_MP4ATOM_H
