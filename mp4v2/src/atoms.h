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
 * Copyright (C) Cisco Systems Inc. 2001 - 2005.  All Rights Reserved.
 *
 * 3GPP features implementation is based on 3GPP's TS26.234-v5.60,
 * and was contributed by Ximpo Group Ltd.
 *
 * Portions created by Ximpo Group Ltd. are
 * Copyright (C) Ximpo Group Ltd. 2003, 2004.  All Rights Reserved.
 *
 * Contributor(s):
 *      Dave Mackie     dmackie@cisco.com
 *              Ximpo Group Ltd.                mp4v2@ximpo.com
 *              Bill May                wmay@cisco.com
 */

#ifndef MP4V2_IMPL_ATOMS_H
#define MP4V2_IMPL_ATOMS_H

namespace mp4v2 { namespace impl {

///////////////////////////////////////////////////////////////////////////////

class MP4FtypAtom;
class MP4FreeAtom;

/// ISO base media full-atom.
class MP4FullAtom : public MP4Atom
{
public:
    MP4FullAtom( MP4File &file, const char* type );

    MP4Integer8Property&  version;
    MP4Integer24Property& flags;
private:
    MP4FullAtom();
    MP4FullAtom( const MP4FullAtom &src );
    MP4FullAtom &operator= ( const MP4FullAtom &src );
};

// declare all the atom subclasses
// i.e. spare us atom_xxxx.h for all the atoms
//
// The majority of atoms just need their own constructor declared
// Some atoms have a few special needs
// A small minority of atoms need lots of special handling

class MP4RootAtom : public MP4Atom
{
public:
    MP4RootAtom(MP4File &file);
    void BeginWrite(bool use64 = false);
    void Write();
    void FinishWrite(bool use64 = false);

    void BeginOptimalWrite();
    void FinishOptimalWrite();

protected:
    uint32_t GetLastMdatIndex();
    void WriteAtomType(const char* type, bool onlyOne);

private:
    MP4RootAtom();
    MP4RootAtom( const MP4RootAtom &src );
    MP4RootAtom &operator= ( const MP4RootAtom &src );

    MP4FtypAtom* m_rewrite_ftyp;
    uint64_t     m_rewrite_ftypPosition;
    MP4FreeAtom* m_rewrite_free;
    uint64_t     m_rewrite_freePosition;
};

/***********************************************************************
 * Common atom classes - standard for anything that just contains atoms
 * and non-maleable properties, treftype and url
 ***********************************************************************/
class MP4StandardAtom : public MP4Atom {
public:
    MP4StandardAtom(MP4File &file, const char *name);
private:
    MP4StandardAtom();
    MP4StandardAtom( const MP4StandardAtom &src );
    MP4StandardAtom &operator= ( const MP4StandardAtom &src );
};

class MP4TrefTypeAtom : public MP4Atom {
public:
    MP4TrefTypeAtom(MP4File &file, const char* type);
    void Read();
private:
    MP4TrefTypeAtom();
    MP4TrefTypeAtom( const MP4TrefTypeAtom &src );
    MP4TrefTypeAtom &operator= ( const MP4TrefTypeAtom &src );
};

class MP4UrlAtom : public MP4Atom {
public:
    MP4UrlAtom(MP4File &file, const char *type="url ");
    void Read();
    void Write();
private:
    MP4UrlAtom();
    MP4UrlAtom( const MP4UrlAtom &src );
    MP4UrlAtom &operator= ( const MP4UrlAtom &src );
};

/***********************************************************************
 * Sound and Video atoms - use the generic atoms when possible
 * (MP4SoundAtom and MP4VideoAtom)
 ***********************************************************************/
class MP4SoundAtom : public MP4Atom {
public:
    MP4SoundAtom(MP4File &file, const char *atomid);
    void Generate();
    void Read();
protected:
    void AddProperties(uint8_t version);
private:
    MP4SoundAtom();
    MP4SoundAtom( const MP4SoundAtom &src );
    MP4SoundAtom &operator= ( const MP4SoundAtom &src );
};

class MP4VideoAtom : public MP4Atom {
public:
    MP4VideoAtom(MP4File &file, const char *atomid);
    void Generate();
private:
    MP4VideoAtom();
    MP4VideoAtom( const MP4VideoAtom &src );
    MP4VideoAtom &operator= ( const MP4VideoAtom &src );
};

class MP4AmrAtom : public MP4Atom {
public:
    MP4AmrAtom(MP4File &file, const char *type);
    void Generate();
private:
    MP4AmrAtom();
    MP4AmrAtom( const MP4AmrAtom &src );
    MP4AmrAtom &operator= ( const MP4AmrAtom &src );
};

// H.264 atoms

class MP4Avc1Atom : public MP4Atom {
public:
    MP4Avc1Atom(MP4File &file);
    void Generate();
private:
    MP4Avc1Atom();
    MP4Avc1Atom( const MP4Avc1Atom &src );
    MP4Avc1Atom &operator= ( const MP4Avc1Atom &src );
};

class MP4AvcCAtom : public MP4Atom {
public:
    MP4AvcCAtom(MP4File &file);
    void Generate();
    void Clone(MP4AvcCAtom *dstAtom);
private:
    MP4AvcCAtom();
    MP4AvcCAtom( const MP4AvcCAtom &src );
    MP4AvcCAtom &operator= ( const MP4AvcCAtom &src );
};


class MP4D263Atom : public MP4Atom {
public:
    MP4D263Atom(MP4File &file);
    void Generate();
    void Write();
private:
    MP4D263Atom();
    MP4D263Atom( const MP4D263Atom &src );
    MP4D263Atom &operator= ( const MP4D263Atom &src );
};

class MP4DamrAtom : public MP4Atom {
public:
    MP4DamrAtom(MP4File &file);
    void Generate();
private:
    MP4DamrAtom();
    MP4DamrAtom( const MP4DamrAtom &src );
    MP4DamrAtom &operator= ( const MP4DamrAtom &src );
};

class MP4EncaAtom : public MP4Atom {
public:
    MP4EncaAtom(MP4File &file);
    void Generate();
private:
    MP4EncaAtom();
    MP4EncaAtom( const MP4EncaAtom &src );
    MP4EncaAtom &operator= ( const MP4EncaAtom &src );
};

class MP4EncvAtom : public MP4Atom {
public:
    MP4EncvAtom(MP4File &file);
    void Generate();
private:
    MP4EncvAtom();
    MP4EncvAtom( const MP4EncvAtom &src );
    MP4EncvAtom &operator= ( const MP4EncvAtom &src );
};

class MP4Mp4aAtom : public MP4Atom {
public:
    MP4Mp4aAtom(MP4File &file);
    void Generate();
private:
    MP4Mp4aAtom();
    MP4Mp4aAtom( const MP4Mp4aAtom &src );
    MP4Mp4aAtom &operator= ( const MP4Mp4aAtom &src );
};

class MP4Ac3Atom : public MP4Atom {
public:
    MP4Ac3Atom(MP4File &file);
    void Generate();
private:
    MP4Ac3Atom();
    MP4Ac3Atom( const MP4Ac3Atom &src );
    MP4Ac3Atom &operator= ( const MP4Ac3Atom &src );
};

class MP4DAc3Atom : public MP4Atom {
public:
    MP4DAc3Atom(MP4File &file);
    void Generate();
    void Dump(uint8_t indent, bool dumpImplicits);
private:
    MP4DAc3Atom();
    MP4DAc3Atom( const MP4DAc3Atom &src );
    MP4DAc3Atom &operator= ( const MP4DAc3Atom &src );
};

class MP4Mp4sAtom : public MP4Atom {
public:
    MP4Mp4sAtom(MP4File &file);
    void Generate();
private:
    MP4Mp4sAtom();
    MP4Mp4sAtom( const MP4Mp4sAtom &src );
    MP4Mp4sAtom &operator= ( const MP4Mp4sAtom &src );
};

class MP4Mp4vAtom : public MP4Atom {
public:
    MP4Mp4vAtom(MP4File &file);
    void Generate();
private:
    MP4Mp4vAtom();
    MP4Mp4vAtom( const MP4Mp4vAtom &src );
    MP4Mp4vAtom &operator= ( const MP4Mp4vAtom &src );
};


class MP4S263Atom : public MP4Atom {
public:
    MP4S263Atom(MP4File &file);
    void Generate();
private:
    MP4S263Atom();
    MP4S263Atom( const MP4S263Atom &src );
    MP4S263Atom &operator= ( const MP4S263Atom &src );
};



/************************************************************************
 * Specialized Atoms
 ************************************************************************/

class MP4DrefAtom : public MP4Atom {
public:
    MP4DrefAtom(MP4File &file);
    void Read();
private:
    MP4DrefAtom();
    MP4DrefAtom( const MP4DrefAtom &src );
    MP4DrefAtom &operator= ( const MP4DrefAtom &src );
};

class MP4ElstAtom : public MP4Atom {
public:
    MP4ElstAtom(MP4File &file);
    void Generate();
    void Read();
protected:
    void AddProperties(uint8_t version);
private:
    MP4ElstAtom();
    MP4ElstAtom( const MP4ElstAtom &src );
    MP4ElstAtom &operator= ( const MP4ElstAtom &src );
};

class MP4FreeAtom : public MP4Atom {
public:
    MP4FreeAtom( MP4File &file, const char* = NULL );
    void Read();
    void Write();
private:
    MP4FreeAtom();
    MP4FreeAtom( const MP4FreeAtom &src );
    MP4FreeAtom &operator= ( const MP4FreeAtom &src );
};

class MP4FtypAtom : public MP4Atom {
public:
    MP4FtypAtom(MP4File &file);
    void Generate();
    void Read();

    MP4StringProperty&    majorBrand;
    MP4Integer32Property& minorVersion;
    MP4StringProperty&    compatibleBrands;
private:
    MP4FtypAtom();
    MP4FtypAtom( const MP4FtypAtom &src );
    MP4FtypAtom &operator= ( const MP4FtypAtom &src );
};

class MP4GminAtom : public MP4Atom {
public:
    MP4GminAtom(MP4File &file);
    void Generate();
private:
    MP4GminAtom();
    MP4GminAtom( const MP4GminAtom &src );
    MP4GminAtom &operator= ( const MP4GminAtom &src );
};

class MP4HdlrAtom : public MP4Atom {
public:
    MP4HdlrAtom(MP4File &file);
    void Read();
private:
    MP4HdlrAtom();
    MP4HdlrAtom( const MP4HdlrAtom &src );
    MP4HdlrAtom &operator= ( const MP4HdlrAtom &src );
};

class MP4HinfAtom : public MP4Atom {
public:
    MP4HinfAtom(MP4File &file);
    void Generate();
private:
    MP4HinfAtom();
    MP4HinfAtom( const MP4HinfAtom &src );
    MP4HinfAtom &operator= ( const MP4HinfAtom &src );
};

class MP4HntiAtom : public MP4Atom {
public:
    MP4HntiAtom(MP4File &file);
    void Read();
private:
    MP4HntiAtom();
    MP4HntiAtom( const MP4HntiAtom &src );
    MP4HntiAtom &operator= ( const MP4HntiAtom &src );
};


class MP4MdatAtom : public MP4Atom {
public:
    MP4MdatAtom(MP4File &file);
    void Read();
    void Write();
private:
    MP4MdatAtom();
    MP4MdatAtom( const MP4MdatAtom &src );
    MP4MdatAtom &operator= ( const MP4MdatAtom &src );
};

class MP4MdhdAtom : public MP4Atom {
public:
    MP4MdhdAtom(MP4File &file);
    void Generate();
    void Read();
protected:
    void AddProperties(uint8_t version);
private:
    MP4MdhdAtom();
    MP4MdhdAtom( const MP4MdhdAtom &src );
    MP4MdhdAtom &operator= ( const MP4MdhdAtom &src );
};

class MP4MvhdAtom : public MP4Atom {
public:
    MP4MvhdAtom(MP4File &file);
    void Generate();
    void Read();
protected:
    void AddProperties(uint8_t version);
private:
    MP4MvhdAtom();
    MP4MvhdAtom( const MP4MvhdAtom &src );
    MP4MvhdAtom &operator= ( const MP4MvhdAtom &src );
};

class MP4OhdrAtom : public MP4Atom {
public:
    MP4OhdrAtom(MP4File &file);
    ~MP4OhdrAtom();
    void Read();
private:
    MP4OhdrAtom();
    MP4OhdrAtom( const MP4OhdrAtom &src );
    MP4OhdrAtom &operator= ( const MP4OhdrAtom &src );
};

class MP4RtpAtom : public MP4Atom {
public:
    MP4RtpAtom(MP4File &file);
    void Generate();
    void Read();
    void Write();

protected:
    void AddPropertiesStsdType();
    void AddPropertiesHntiType();

    void GenerateStsdType();
    void GenerateHntiType();

    void ReadStsdType();
    void ReadHntiType();

    void WriteHntiType();

private:
    MP4RtpAtom();
    MP4RtpAtom( const MP4RtpAtom &src );
    MP4RtpAtom &operator= ( const MP4RtpAtom &src );
};

class MP4SdpAtom : public MP4Atom {
public:
    MP4SdpAtom(MP4File &file);
    void Read();
    void Write();
private:
    MP4SdpAtom();
    MP4SdpAtom( const MP4SdpAtom &src );
    MP4SdpAtom &operator= ( const MP4SdpAtom &src );
};

// sdtp - Independent and Disposable Samples Atom.
class MP4SdtpAtom : public MP4FullAtom {
public:
    MP4SdtpAtom(MP4File &file);
    void Read();

    // raw bytes; one byte for each sample.
    // number of bytes == stsz.sampleCount.
    MP4BytesProperty& data;
private:
    MP4SdtpAtom();
    MP4SdtpAtom( const MP4SdtpAtom &src );
    MP4SdtpAtom &operator= ( const MP4SdtpAtom &src );
};

class MP4SmiAtom : public MP4Atom {
public:
    MP4SmiAtom(MP4File &file);
    void Read();
private:
    MP4SmiAtom();
    MP4SmiAtom( const MP4SmiAtom &src );
    MP4SmiAtom &operator= ( const MP4SmiAtom &src );
};

class MP4StblAtom : public MP4Atom {
public:
    MP4StblAtom(MP4File &file);
    void Generate();
private:
    MP4StblAtom();
    MP4StblAtom( const MP4StblAtom &src );
    MP4StblAtom &operator= ( const MP4StblAtom &src );
};

class MP4StdpAtom : public MP4Atom {
public:
    MP4StdpAtom(MP4File &file);
    void Read();
private:
    MP4StdpAtom();
    MP4StdpAtom( const MP4StdpAtom &src );
    MP4StdpAtom &operator= ( const MP4StdpAtom &src );
};

class MP4StscAtom : public MP4Atom {
public:
    MP4StscAtom(MP4File &file);
    void Read();
private:
    MP4StscAtom();
    MP4StscAtom( const MP4StscAtom &src );
    MP4StscAtom &operator= ( const MP4StscAtom &src );
};

class MP4StsdAtom : public MP4Atom {
public:
    MP4StsdAtom(MP4File &file);
    void Read();
private:
    MP4StsdAtom();
    MP4StsdAtom( const MP4StsdAtom &src );
    MP4StsdAtom &operator= ( const MP4StsdAtom &src );
};

class MP4StszAtom : public MP4Atom {
public:
    MP4StszAtom(MP4File &file);
    void Read();
    void Write();
private:
    MP4StszAtom();
    MP4StszAtom( const MP4StszAtom &src );
    MP4StszAtom &operator= ( const MP4StszAtom &src );
};

class MP4Stz2Atom : public MP4Atom {
public:
    MP4Stz2Atom(MP4File &file);
    void Read();
private:
    MP4Stz2Atom();
    MP4Stz2Atom( const MP4Stz2Atom &src );
    MP4Stz2Atom &operator= ( const MP4Stz2Atom &src );
};

class MP4TextAtom : public MP4Atom {
public:
    MP4TextAtom(MP4File &file);
    void Generate();
    void Read();
protected:
    void AddPropertiesStsdType();
    void AddPropertiesGmhdType();

    void GenerateStsdType();
    void GenerateGmhdType();
private:
    MP4TextAtom();
    MP4TextAtom( const MP4TextAtom &src );
    MP4TextAtom &operator= ( const MP4TextAtom &src );
};

class MP4Tx3gAtom : public MP4Atom {
public:
    MP4Tx3gAtom(MP4File &file);
    void Generate();
private:
    MP4Tx3gAtom();
    MP4Tx3gAtom( const MP4Tx3gAtom &src );
    MP4Tx3gAtom &operator= ( const MP4Tx3gAtom &src );
};

class MP4FtabAtom : public MP4Atom {
public:
    MP4FtabAtom(MP4File &file);
private:
    MP4FtabAtom();
    MP4FtabAtom( const MP4FtabAtom &src );
    MP4FtabAtom &operator= ( const MP4FtabAtom &src );
};

class MP4TfhdAtom : public MP4Atom {
public:
    MP4TfhdAtom(MP4File &file);
    void Read();
protected:
    void AddProperties(uint32_t flags);
private:
    MP4TfhdAtom();
    MP4TfhdAtom( const MP4TfhdAtom &src );
    MP4TfhdAtom &operator= ( const MP4TfhdAtom &src );
};

class MP4TkhdAtom : public MP4Atom {
public:
    MP4TkhdAtom(MP4File &file);
    void Generate();
    void Read();
protected:
    void AddProperties(uint8_t version);
private:
    MP4TkhdAtom();
    MP4TkhdAtom( const MP4TkhdAtom &src );
    MP4TkhdAtom &operator= ( const MP4TkhdAtom &src );
};

class MP4TrunAtom : public MP4Atom {
public:
    MP4TrunAtom(MP4File &file);
    void Read();
protected:
    void AddProperties(uint32_t flags);
private:
    MP4TrunAtom();
    MP4TrunAtom( const MP4TrunAtom &src );
    MP4TrunAtom &operator= ( const MP4TrunAtom &src );
};

class MP4UdtaAtom : public MP4Atom {
public:
    MP4UdtaAtom(MP4File &file);
    void Read();
private:
    MP4UdtaAtom();
    MP4UdtaAtom( const MP4UdtaAtom &src );
    MP4UdtaAtom &operator= ( const MP4UdtaAtom &src );
};

class MP4UrnAtom : public MP4Atom {
public:
    MP4UrnAtom(MP4File &file);
    void Read();
private:
    MP4UrnAtom();
    MP4UrnAtom( const MP4UrnAtom &src );
    MP4UrnAtom &operator= ( const MP4UrnAtom &src );
};

class MP4VmhdAtom : public MP4Atom {
public:
    MP4VmhdAtom(MP4File &file);
    void Generate();
private:
    MP4VmhdAtom();
    MP4VmhdAtom( const MP4VmhdAtom &src );
    MP4VmhdAtom &operator= ( const MP4VmhdAtom &src );
};

class MP4HrefAtom : public MP4Atom {
public:
    MP4HrefAtom(MP4File &file);
    void Generate(void);
private:
    MP4HrefAtom();
    MP4HrefAtom( const MP4HrefAtom &src );
    MP4HrefAtom &operator= ( const MP4HrefAtom &src );
};

class MP4PaspAtom : public MP4Atom {
public:
    MP4PaspAtom(MP4File &file);
    void Generate();
private:
    MP4PaspAtom();
    MP4PaspAtom( const MP4PaspAtom &src );
    MP4PaspAtom &operator= ( const MP4PaspAtom &src );
};

class MP4ColrAtom : public MP4Atom {
public:
    MP4ColrAtom(MP4File &file);
    void Generate();
private:
    MP4ColrAtom();
    MP4ColrAtom( const MP4ColrAtom &src );
    MP4ColrAtom &operator= ( const MP4ColrAtom &src );
};

class IPodUUIDAtom : public MP4Atom {
public:
    IPodUUIDAtom(MP4File &file);
private:
    IPodUUIDAtom();
    IPodUUIDAtom( const IPodUUIDAtom &src );
    IPodUUIDAtom &operator= ( const IPodUUIDAtom &src );
};

class MP4NmhdAtom : public MP4Atom {
public:
    MP4NmhdAtom(MP4File &file);
private:
    MP4NmhdAtom();
    MP4NmhdAtom( const MP4NmhdAtom &src );
    MP4NmhdAtom &operator= ( const MP4NmhdAtom &src );
};

/*! Nero Chapter List.
 * This atom defines the structure of a Nero chapter list.
 * Although it is not completely clear if this structure is
 * correct it is complete enough to successfully read and write
 * the chapter list so that even Nero's software accepts it.
 *
 * The assumed format is as follows:
 * - MP4Integer8Property("version")
 * - MP4Integer24Property("flags")
 * - MP4BytesProperty("reserved", 1)
 * - MP4Integer32Property("chaptercount")\n
 * - MP4TableProperty("chapters", "ref to chaptercount");
 *     - MP4Integer64Property("starttime")\n
 *       The start time of the chapter expressed in 100 nanosecond units
 *     - MP4StringProperty("title", true)\n
 *       The title of the chapter encoded in UTF-8
 *
 * The chapter title only accepts strings of 255 bytes so if a string
 * only contains simple (two-byte) UTF-8 chars the maximum length is
 * 127 chars.
 */
class MP4ChplAtom : public MP4Atom {
public:
    MP4ChplAtom(MP4File &file);
    void Generate();
private:
    MP4ChplAtom();
    MP4ChplAtom( const MP4ChplAtom &src );
    MP4ChplAtom &operator= ( const MP4ChplAtom &src );
};

///////////////////////////////////////////////////////////////////////////////

/// iTMF hdlr-atom.
class MP4ItmfHdlrAtom : public MP4FullAtom
{
public:
    MP4ItmfHdlrAtom(MP4File &file);
    void Read();

    MP4Integer32Property& reserved1;
    MP4BytesProperty&     handlerType;
    MP4BytesProperty&     reserved2;
    MP4BytesProperty&     name;
private:
    MP4ItmfHdlrAtom();
    MP4ItmfHdlrAtom( const MP4ItmfHdlrAtom &src );
    MP4ItmfHdlrAtom &operator= ( const MP4ItmfHdlrAtom &src );
};

/// iTMF item-atom.
class MP4ItemAtom : public MP4Atom
{
public:
    MP4ItemAtom( MP4File &file, const char* type );
private:
    MP4ItemAtom();
    MP4ItemAtom( const MP4ItemAtom &src );
    MP4ItemAtom &operator= ( const MP4ItemAtom &src );
};

/// iTMF meaning-atom.
class MP4MeanAtom : public MP4FullAtom
{
public:
    MP4MeanAtom(MP4File &file);
    void Read();

    MP4BytesProperty& value;
private:
    MP4MeanAtom();
    MP4MeanAtom( const MP4MeanAtom &src );
    MP4MeanAtom &operator= ( const MP4MeanAtom &src );
};

/// iTMF name-atom.
class MP4NameAtom : public MP4FullAtom
{
public:
    MP4NameAtom(MP4File &file);
    void Read();

    MP4BytesProperty& value;
private:
    MP4NameAtom();
    MP4NameAtom( const MP4NameAtom &src );
    MP4NameAtom &operator= ( const MP4NameAtom &src );
};

/// iTMF data-atom.
class MP4DataAtom : public MP4Atom
{
public:
    MP4DataAtom(MP4File &file);
    void Read();

    MP4Integer16Property& typeReserved;
    MP4Integer8Property&  typeSetIdentifier;
    MP4BasicTypeProperty& typeCode;
    MP4Integer32Property& locale;
    MP4BytesProperty&     metadata;
private:
    MP4DataAtom();
    MP4DataAtom( const MP4DataAtom &src );
    MP4DataAtom &operator= ( const MP4DataAtom &src );
};

///////////////////////////////////////////////////////////////////////////////

/// QTFF udta data element-atom.
class MP4UdtaElementAtom : public MP4Atom
{
public:
    MP4UdtaElementAtom( MP4File &file, const char* type );
    void Read();

    MP4BytesProperty& value;
private:
    MP4UdtaElementAtom();
    MP4UdtaElementAtom( const MP4UdtaElementAtom &src );
    MP4UdtaElementAtom &operator= ( const MP4UdtaElementAtom &src );
};

///////////////////////////////////////////////////////////////////////////////

}} // namespace mp4v2::impl

#endif // MP4V2_IMPL_ATOMS_H
