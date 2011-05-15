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

#ifndef MP4V2_IMPL_MP4PROPERTY_H
#define MP4V2_IMPL_MP4PROPERTY_H

namespace mp4v2 { namespace impl {

///////////////////////////////////////////////////////////////////////////////

// forward declarations
class MP4Atom;

class MP4Descriptor;
MP4ARRAY_DECL(MP4Descriptor, MP4Descriptor*);

enum MP4PropertyType {
    Integer8Property,
    Integer16Property,
    Integer24Property,
    Integer32Property,
    Integer64Property,
    Float32Property,
    StringProperty,
    BytesProperty,
    TableProperty,
    DescriptorProperty,
    LanguageCodeProperty,
    BasicTypeProperty,
};

class MP4Property {
public:
    MP4Property(MP4Atom& parentAtom, const char *name = NULL);

    virtual ~MP4Property() { }

    MP4Atom& GetParentAtom() {
        return m_parentAtom;
    }

    const char *GetName() {
        return m_name;
    }

    virtual MP4PropertyType GetType() = 0;

    bool IsReadOnly() {
        return m_readOnly;
    }
    void SetReadOnly(bool value = true) {
        m_readOnly = value;
    }

    bool IsImplicit() {
        return m_implicit;
    }
    void SetImplicit(bool value = true) {
        m_implicit = value;
    }

    virtual uint32_t GetCount() = 0;
    virtual void SetCount(uint32_t count) = 0;

    virtual void Generate() { /* default is a no-op */ };

    virtual void Read(MP4File& file, uint32_t index = 0) = 0;

    virtual void Write(MP4File& file, uint32_t index = 0) = 0;

    virtual void Dump(uint8_t indent,
                      bool dumpImplicits, uint32_t index = 0) = 0;

    virtual bool FindProperty(const char* name,
                              MP4Property** ppProperty, uint32_t* pIndex = NULL);

protected:
    MP4Atom& m_parentAtom;
    const char* m_name;
    bool m_readOnly;
    bool m_implicit;

private:
    MP4Property();
    MP4Property ( const MP4Property &src );
    MP4Property &operator= ( const MP4Property &src );
};

MP4ARRAY_DECL(MP4Property, MP4Property*);

class MP4IntegerProperty : public MP4Property {
protected:
    MP4IntegerProperty(MP4Atom& parentAtom, const char* name)
            : MP4Property(parentAtom, name) { };

public:
    uint64_t GetValue(uint32_t index = 0);

    void SetValue(uint64_t value, uint32_t index = 0);

    void InsertValue(uint64_t value, uint32_t index = 0);

    void DeleteValue(uint32_t index = 0);

    void IncrementValue(int32_t increment = 1, uint32_t index = 0);

private:
    MP4IntegerProperty();
    MP4IntegerProperty ( const MP4IntegerProperty &src );
    MP4IntegerProperty &operator= ( const MP4IntegerProperty &src );
};

#define MP4INTEGER_PROPERTY_DECL2(isize, xsize) \
    class MP4Integer##xsize##Property : public MP4IntegerProperty { \
    public: \
        MP4Integer##xsize##Property(MP4Atom& parentAtom, const char* name) \
            : MP4IntegerProperty(parentAtom, name) { \
            SetCount(1); \
            m_values[0] = 0; \
        } \
        \
        MP4PropertyType GetType() { \
            return Integer##xsize##Property; \
        } \
        \
        uint32_t GetCount() { \
            return m_values.Size(); \
        } \
        void SetCount(uint32_t count) { \
            m_values.Resize(count); \
        } \
        \
        uint##isize##_t GetValue(uint32_t index = 0) { \
            return m_values[index]; \
        } \
        \
        void SetValue(uint##isize##_t value, uint32_t index = 0) { \
            if (m_readOnly) { \
                ostringstream msg; \
                msg << "property is read-only: " << m_name; \
                throw new PlatformException(msg.str().c_str(), EACCES, __FILE__, __LINE__, __FUNCTION__); \
            } \
            m_values[index] = value; \
        } \
        void AddValue(uint##isize##_t value) { \
            m_values.Add(value); \
        } \
        void InsertValue(uint##isize##_t value, uint32_t index) { \
            m_values.Insert(value, index); \
        } \
        void DeleteValue(uint32_t index) { \
            m_values.Delete(index); \
        } \
        void IncrementValue(int32_t increment = 1, uint32_t index = 0) { \
            m_values[index] += increment; \
        } \
        void Read(MP4File& file, uint32_t index = 0) { \
            if (m_implicit) { \
                return; \
            } \
            m_values[index] = file.ReadUInt##xsize(); \
        } \
        \
        void Write(MP4File& file, uint32_t index = 0) { \
            if (m_implicit) { \
                return; \
            } \
            file.WriteUInt##xsize(m_values[index]); \
        } \
        void Dump(uint8_t indent, \
            bool dumpImplicits, uint32_t index = 0); \
    \
    protected: \
        MP4Integer##isize##Array m_values; \
    private: \
        MP4Integer##xsize##Property(); \
        MP4Integer##xsize##Property ( const MP4Integer##xsize##Property &src ); \
        MP4Integer##xsize##Property &operator= ( const MP4Integer##xsize##Property &src ); \
    };

#define MP4INTEGER_PROPERTY_DECL(size) \
    MP4INTEGER_PROPERTY_DECL2(size, size)

MP4INTEGER_PROPERTY_DECL(8);
MP4INTEGER_PROPERTY_DECL(16);
MP4INTEGER_PROPERTY_DECL2(32, 24);
MP4INTEGER_PROPERTY_DECL(32);
MP4INTEGER_PROPERTY_DECL(64);

class MP4BitfieldProperty : public MP4Integer64Property {
public:
    MP4BitfieldProperty(MP4Atom& parentAtom, const char* name, uint8_t numBits)
            : MP4Integer64Property(parentAtom, name) {
        ASSERT(numBits != 0);
        ASSERT(numBits <= 64);
        m_numBits = numBits;
    }

    uint8_t GetNumBits() {
        return m_numBits;
    }
    void SetNumBits(uint8_t numBits) {
        m_numBits = numBits;
    }

    void Read(MP4File& file, uint32_t index = 0);
    void Write(MP4File& file, uint32_t index = 0);
    void Dump(uint8_t indent,
              bool dumpImplicits, uint32_t index = 0);

protected:
    uint8_t m_numBits;

private:
    MP4BitfieldProperty();
    MP4BitfieldProperty ( const MP4BitfieldProperty &src );
    MP4BitfieldProperty &operator= ( const MP4BitfieldProperty &src );
};

class MP4Float32Property : public MP4Property {
public:
    MP4Float32Property(MP4Atom& parentAtom, const char* name)
            : MP4Property(parentAtom, name) {
        m_useFixed16Format = false;
        m_useFixed32Format = false;
        SetCount(1);
        m_values[0] = 0.0;
    }

    MP4PropertyType GetType() {
        return Float32Property;
    }

    uint32_t GetCount() {
        return m_values.Size();
    }
    void SetCount(uint32_t count) {
        m_values.Resize(count);
    }

    float GetValue(uint32_t index = 0) {
        return m_values[index];
    }

    void SetValue(float value, uint32_t index = 0) {
        if (m_readOnly) {
            ostringstream msg;
            msg << "property is read-only: " << m_name;
            throw new PlatformException(msg.str().c_str(), EACCES, __FILE__, __LINE__, __FUNCTION__);
        }
        m_values[index] = value;
    }

    void AddValue(float value) {
        m_values.Add(value);
    }

    void InsertValue(float value, uint32_t index) {
        m_values.Insert(value, index);
    }

    bool IsFixed16Format() {
        return m_useFixed16Format;
    }

    void SetFixed16Format(bool useFixed16Format = true) {
        m_useFixed16Format = useFixed16Format;
    }

    bool IsFixed32Format() {
        return m_useFixed32Format;
    }

    void SetFixed32Format(bool useFixed32Format = true) {
        m_useFixed32Format = useFixed32Format;
    }

    void Read(MP4File& file, uint32_t index = 0);
    void Write(MP4File& file, uint32_t index = 0);
    void Dump(uint8_t indent,
              bool dumpImplicits, uint32_t index = 0);

protected:
    bool m_useFixed16Format;
    bool m_useFixed32Format;
    MP4Float32Array m_values;

private:
    MP4Float32Property();
    MP4Float32Property ( const MP4Float32Property &src );
    MP4Float32Property &operator= ( const MP4Float32Property &src );
};

class MP4StringProperty : public MP4Property {
public:
    MP4StringProperty(MP4Atom& parentAtom, const char* name,
                      bool useCountedFormat = false, bool useUnicode = false, bool arrayMode = false);

    ~MP4StringProperty();

    MP4PropertyType GetType() {
        return StringProperty;
    }

    uint32_t GetCount() {
        return m_values.Size();
    }

    void SetCount(uint32_t count);

    const char* GetValue(uint32_t index = 0) {
        return m_values[index];
    }

    void SetValue(const char* value, uint32_t index = 0);

    void AddValue(const char* value) {
        uint32_t count = GetCount();
        SetCount(count + 1);
        SetValue(value, count);
    }

    bool IsCountedFormat() {
        return m_useCountedFormat;
    }

    void SetCountedFormat(bool useCountedFormat) {
        m_useCountedFormat = useCountedFormat;
    }

    bool IsExpandedCountedFormat() {
        return m_useExpandedCount;
    }

    void SetExpandedCountedFormat(bool useExpandedCount) {
        m_useExpandedCount = useExpandedCount;
    }

    bool IsUnicode() {
        return m_useUnicode;
    }

    void SetUnicode(bool useUnicode) {
        m_useUnicode = useUnicode;
    }

    uint32_t GetFixedLength() {
        return m_fixedLength;
    }

    void SetFixedLength(uint32_t fixedLength) {
        m_fixedLength = fixedLength;
    }

    void Read(MP4File& file, uint32_t index = 0);
    void Write(MP4File& file, uint32_t index = 0);
    void Dump(uint8_t indent,
              bool dumpImplicits, uint32_t index = 0);

protected:
    bool m_arrayMode; // during read/write ignore index and read/write full array
    bool m_useCountedFormat;
    bool m_useExpandedCount;
    bool m_useUnicode;
    uint32_t m_fixedLength;

    MP4StringArray m_values;

private:
    MP4StringProperty();
    MP4StringProperty ( const MP4StringProperty &src );
    MP4StringProperty &operator= ( const MP4StringProperty &src );
};

class MP4BytesProperty : public MP4Property {
public:
    MP4BytesProperty(MP4Atom& parentAtom, const char* name, uint32_t valueSize = 0,
                     uint32_t defaultValueSize = 0);

    ~MP4BytesProperty();

    MP4PropertyType GetType() {
        return BytesProperty;
    }

    uint32_t GetCount() {
        return m_values.Size();
    }

    void SetCount(uint32_t count);

    void GetValue(uint8_t** ppValue, uint32_t* pValueSize,
                  uint32_t index = 0) {
        // N.B. caller must free memory
        *ppValue = (uint8_t*)MP4Malloc(m_valueSizes[index]);
        memcpy(*ppValue, m_values[index], m_valueSizes[index]);
        *pValueSize = m_valueSizes[index];
    }

    char* GetValueStringAlloc( uint32_t index = 0 ) {
        char* buf = (char*)MP4Malloc( m_valueSizes[index] + 1 );
        memcpy( buf, m_values[index], m_valueSizes[index] );
        buf[m_valueSizes[index]] = '\0';
        return buf;
    }

    bool CompareToString( const string& s, uint32_t index = 0 ) {
        return string( (const char*)m_values[index], m_valueSizes[index] ) != s;
    }

    void CopyValue(uint8_t* pValue, uint32_t index = 0) {
        // N.B. caller takes responsbility for valid pointer
        // and sufficient memory at the destination
        memcpy(pValue, m_values[index], m_valueSizes[index]);
    }

    void SetValue(const uint8_t* pValue, uint32_t valueSize,
                  uint32_t index = 0);

    void AddValue(const uint8_t* pValue, uint32_t valueSize) {
        uint32_t count = GetCount();
        SetCount(count + 1);
        SetValue(pValue, valueSize, count);
    }

    uint32_t GetValueSize( uint32_t index = 0 ) {
        return m_valueSizes[index];
    }

    void SetValueSize(uint32_t valueSize, uint32_t index = 0);

    uint32_t GetFixedSize() {
        return m_fixedValueSize;
    }

    void SetFixedSize(uint32_t fixedSize);

    void Read(MP4File& file, uint32_t index = 0);
    void Write(MP4File& file, uint32_t index = 0);
    void Dump(uint8_t indent,
              bool dumpImplicits, uint32_t index = 0);

protected:
    uint32_t        m_fixedValueSize;
    uint32_t        m_defaultValueSize;
    MP4Integer32Array   m_valueSizes;
    MP4BytesArray       m_values;

private:
    MP4BytesProperty();
    MP4BytesProperty ( const MP4BytesProperty &src );
    MP4BytesProperty &operator= ( const MP4BytesProperty &src );
};

class MP4TableProperty : public MP4Property {
public:
    MP4TableProperty(MP4Atom& parentAtom, const char* name, MP4IntegerProperty* pCountProperty);

    ~MP4TableProperty();

    MP4PropertyType GetType() {
        return TableProperty;
    }

    void AddProperty(MP4Property* pProperty);

    MP4Property* GetProperty(uint32_t index) {
        return m_pProperties[index];
    }

    virtual uint32_t GetCount() {
        return m_pCountProperty->GetValue();
    }
    virtual void SetCount(uint32_t count) {
        m_pCountProperty->SetValue(count);
    }

    void Read(MP4File& file, uint32_t index = 0);
    void Write(MP4File& file, uint32_t index = 0);
    void Dump(uint8_t indent,
              bool dumpImplicits, uint32_t index = 0);

    bool FindProperty(const char* name,
                      MP4Property** ppProperty, uint32_t* pIndex = NULL);

protected:
    virtual void ReadEntry(MP4File& file, uint32_t index);
    virtual void WriteEntry(MP4File& file, uint32_t index);

    bool FindContainedProperty(const char* name,
                               MP4Property** ppProperty, uint32_t* pIndex);

protected:
    MP4IntegerProperty* m_pCountProperty;
    MP4PropertyArray    m_pProperties;

private:
    MP4TableProperty();
    MP4TableProperty ( const MP4TableProperty &src );
    MP4TableProperty &operator= ( const MP4TableProperty &src );
};

class MP4DescriptorProperty : public MP4Property {
public:
    MP4DescriptorProperty(MP4Atom& parentAtom, const char* name = NULL,
                          uint8_t tagsStart = 0, uint8_t tagsEnd = 0,
                          bool mandatory = false, bool onlyOne = false);

    ~MP4DescriptorProperty();

    MP4PropertyType GetType() {
        return DescriptorProperty;
    }

    void SetParentAtom(MP4Atom* pParentAtom);

    void SetSizeLimit(uint64_t sizeLimit) {
        m_sizeLimit = sizeLimit;
    }

    uint32_t GetCount() {
        return m_pDescriptors.Size();
    }
    void SetCount(uint32_t count) {
        m_pDescriptors.Resize(count);
    }

    void SetTags(uint8_t tagsStart, uint8_t tagsEnd = 0) {
        m_tagsStart = tagsStart;
        m_tagsEnd = tagsEnd ? tagsEnd : tagsStart;
    }

    MP4Descriptor* AddDescriptor(uint8_t tag);

    void AppendDescriptor(MP4Descriptor* pDescriptor) {
        m_pDescriptors.Add(pDescriptor);
    }

    void DeleteDescriptor(uint32_t index);

    void Generate();
    void Read(MP4File& file, uint32_t index = 0);
    void Write(MP4File& file, uint32_t index = 0);
    void Dump(uint8_t indent,
              bool dumpImplicits, uint32_t index = 0);

    bool FindProperty(const char* name,
                      MP4Property** ppProperty, uint32_t* pIndex = NULL);

protected:
    virtual MP4Descriptor* CreateDescriptor(MP4Atom& parentAtom, uint8_t tag);

    bool FindContainedProperty(const char* name,
                               MP4Property** ppProperty, uint32_t* pIndex);

protected:
    uint8_t         m_tagsStart;
    uint8_t         m_tagsEnd;
    uint64_t            m_sizeLimit;
    bool                m_mandatory;
    bool                m_onlyOne;
    MP4DescriptorArray  m_pDescriptors;

private:
    MP4DescriptorProperty();
    MP4DescriptorProperty ( const MP4DescriptorProperty &src );
    MP4DescriptorProperty &operator= ( const MP4DescriptorProperty &src );
};

class MP4QosQualifierProperty : public MP4DescriptorProperty {
public:
    MP4QosQualifierProperty(MP4Atom& parentAtom, const char* name = NULL,
                            uint8_t tagsStart = 0, uint8_t tagsEnd = 0,
                            bool mandatory = false, bool onlyOne = false) :
            MP4DescriptorProperty(parentAtom, name, tagsStart, tagsEnd, mandatory, onlyOne) { }

protected:
    MP4Descriptor* CreateDescriptor(MP4Atom& parentAtom, uint8_t tag);

private:
    MP4QosQualifierProperty();
    MP4QosQualifierProperty ( const MP4QosQualifierProperty &src );
    MP4QosQualifierProperty &operator= ( const MP4QosQualifierProperty &src );
};

///////////////////////////////////////////////////////////////////////////////

/// ISO-639-2/T language code.
/// Language codes are 3-alpha (always lowercase) codes which are then
/// offset using 0x60 and packed as 5-bit values into 16-bits, most
/// significant bit is zero-padding.

class MP4LanguageCodeProperty : public MP4Property {
private:
    bmff::LanguageCode _value;

public:
    explicit MP4LanguageCodeProperty( MP4Atom& parentAtom, const char* , bmff::LanguageCode = bmff::ILC_UND );
    MP4LanguageCodeProperty( MP4Atom& parentAtom, const char* , const string& );

    MP4PropertyType GetType();
    uint32_t        GetCount();
    void            SetCount( uint32_t );
    void            Read( MP4File&, uint32_t = 0 );
    void            Write( MP4File&, uint32_t = 0 );
    void            Dump( uint8_t, bool, uint32_t = 0 );

    bmff::LanguageCode GetValue();
    void               SetValue( bmff::LanguageCode );

private:
    MP4LanguageCodeProperty();
    MP4LanguageCodeProperty ( const MP4LanguageCodeProperty &src );
    MP4LanguageCodeProperty &operator= ( const MP4LanguageCodeProperty &src );
};

///////////////////////////////////////////////////////////////////////////////

class MP4BasicTypeProperty : public MP4Property {
private:
    itmf::BasicType _value;

public:
    explicit MP4BasicTypeProperty( MP4Atom& parentAtom, const char* , itmf::BasicType = itmf::BT_UNDEFINED );

    MP4PropertyType GetType();
    uint32_t        GetCount();
    void            SetCount( uint32_t );
    void            Read( MP4File&, uint32_t = 0 );
    void            Write( MP4File&, uint32_t = 0 );
    void            Dump( uint8_t, bool, uint32_t = 0 );
    itmf::BasicType GetValue();
    void            SetValue( itmf::BasicType );

private:
    MP4BasicTypeProperty();
    MP4BasicTypeProperty ( const MP4BasicTypeProperty &src );
    MP4BasicTypeProperty &operator= ( const MP4BasicTypeProperty &src );
};

///////////////////////////////////////////////////////////////////////////////

}} // namespace mp4v2::impl

#endif // MP4V2_IMPL_MP4PROPERTY_H
