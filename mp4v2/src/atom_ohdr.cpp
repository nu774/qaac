/** \file atom_ohdr.cpp

    \author Danijel Kopcinovic (danijel.kopcinovic@adnecto.net)
*/

#include "src/impl.h"

namespace mp4v2 {
namespace impl {

///////////////////////////////////////////////////////////////////////////////

/*! \brief Patch class for read/write operations when string is 0-length.

    We want to use string property, but mpeg4ip doesn't support ohdr way of
    encoding of string (in ohdr atom we first have 3 lengths of 3 strings and
    then their string values, and it cannot be simulated with any of the
    current mpeg4ip string property parameters), so we have to write our own
    Read() and Write() routines.
*/
class OhdrMP4StringProperty: public MP4StringProperty {
public:
    /*! \brief Constructor.

        \param name                name of the property.
        \param useCountedFormat    counted format flag.
        \param useUnicode          unicode flag.
    */
    OhdrMP4StringProperty(MP4Atom& parentAtom, const char* name, bool useCountedFormat = false,
                          bool useUnicode = false): MP4StringProperty(parentAtom, name, useCountedFormat,
                                          useUnicode) {
    }

    /*! \brief Read property from file.

        \param pFile                input, file handle.
        \param index                input, index to read.
    */
    void Read(MP4File& file, uint32_t index = 0) {
        MP4Free(m_values[index]);
        m_values[index] = (char*)MP4Calloc(m_fixedLength + 1);
        (void)file.ReadBytes((uint8_t*)m_values[index], m_fixedLength);
    }

    /*! \brief Write property to file.

        \param pFile                input, file handle.
        \param index                input, index to write.
    */
    void Write(MP4File& file, uint32_t index = 0) {
        file.WriteBytes((uint8_t*)m_values[index], m_fixedLength);
    }
private:
    OhdrMP4StringProperty();
    OhdrMP4StringProperty ( const OhdrMP4StringProperty &src );
    OhdrMP4StringProperty &operator= ( const OhdrMP4StringProperty &src );
};

/*! \brief OMA DRM headers atom.

    Contained in OMA DRM key management atom. It must contain content identifier.
*/
/*! \brief Constructor.
*/
MP4OhdrAtom::MP4OhdrAtom(MP4File &file): MP4Atom(file, "ohdr") {
    AddVersionAndFlags();

    AddProperty(new MP4Integer8Property(*this, "EncryptionMethod"));
    AddProperty(new MP4Integer8Property(*this, "EncryptionPadding"));
    AddProperty(new MP4Integer64Property(*this, "PlaintextLength"));
    AddProperty(new MP4Integer16Property(*this, "ContentIDLength"));
    AddProperty(new MP4Integer16Property(*this, "RightsIssuerURLLength"));
    AddProperty(new MP4Integer16Property(*this, "TextualHeadersLength"));
    AddProperty(new OhdrMP4StringProperty(*this, "ContentID"));
    AddProperty(new OhdrMP4StringProperty(*this, "RightsIssuerURL"));
    AddProperty(new MP4BytesProperty(*this, "TextualHeaders"));
}

MP4OhdrAtom::~MP4OhdrAtom() {
}

/*! \brief Read atom.
*/
void MP4OhdrAtom::Read() {
    ReadProperties(0, 8);
    MP4Property* lProperty;
    MP4Property* property;
    lProperty = GetProperty(5);
    property = GetProperty(8);
    ((OhdrMP4StringProperty*)property)->SetFixedLength(
        ((MP4Integer16Property*)lProperty)->GetValue());
    lProperty = GetProperty(6);
    property = GetProperty(9);
    ((OhdrMP4StringProperty*)property)->SetFixedLength(
        ((MP4Integer16Property*)lProperty)->GetValue());
    lProperty = GetProperty(7);
    property = GetProperty(10);
    ((MP4BytesProperty*)property)->SetFixedSize(
        ((MP4Integer16Property*)lProperty)->GetValue());
    ReadProperties(8, 3);
}

///////////////////////////////////////////////////////////////////////////////

}
} // namespace mp4v2::impl
