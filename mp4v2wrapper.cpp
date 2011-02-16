#include "util.h"
#include "mp4v2wrapper.h"

using mp4v2::impl::MP4Atom;
using mp4v2::impl::MP4DataAtom;
using mp4v2::impl::MP4NameAtom;
using mp4v2::impl::MP4MeanAtom;
using mp4v2::impl::MP4Property;
using mp4v2::impl::MP4Integer32Property;
using mp4v2::impl::MP4StringProperty;
using mp4v2::impl::MP4BytesProperty;
namespace itmf = mp4v2::impl::itmf;

std::string format_mp4error(const mp4v2::impl::MP4Error &e)
{
    std::vector<const char*> parts;
    parts.push_back("libmp4v2");
    if (e.m_where) parts.push_back(e.m_where);
    if (e.m_errstring) parts.push_back(e.m_errstring);
    if (e.m_errno) parts.push_back(std::strerror(e.m_errno));
    std::string message;
    for (size_t i = 0; i < parts.size(); ++i) {
	if (i > 0) message += ": ";
	message += parts[i];
    }
    return message;
}

class MP4AlacAtom: public MP4Atom {
public:
    MP4AlacAtom(const char *id): MP4Atom(id)
    {
	AddProperty(new MP4BytesProperty("decoderConfig"));
    }
};


MP4TrackId MP4FileX::AddAlacAudioTrack(uint32_t timeScale,
	const uint8_t *cookie, size_t cookieLength)
{
    MP4TrackId track = AddTrack(MP4_AUDIO_TRACK_TYPE, timeScale);
    AddTrackToOd(track);
    MP4Atom *trackAtom = FindTrackAtom(track, 0);

    SetTrackFloatProperty(track, "tkhd.volume", 1.0);

    InsertChildAtom(MakeTrackName(track, "mdia.minf"), "smhd", 0);
    AddChildAtom(MakeTrackName(track, "mdia.minf.stbl.stsd"), "alac");

    MP4Atom *atom = FindTrackAtom(track, "mdia.minf.stbl.stsd");
    MP4Property *pProp;
    atom->FindProperty("stsd.entryCount", &pProp);
    dynamic_cast<MP4Integer32Property*>(pProp)->IncrementValue();

    atom = atom->FindChildAtom("alac");
    atom->FindProperty("alac.timeScale", &pProp);
    dynamic_cast<MP4Integer32Property*>(pProp)->SetValue(timeScale<<16);
    MP4AlacAtom *alacAtom = new MP4AlacAtom("alac");
    pProp = alacAtom->GetProperty(0);
    dynamic_cast<MP4BytesProperty*>(pProp)->SetValue(cookie, cookieLength, 0);
    atom->AddChildAtom(alacAtom);
    return track;
}

MP4DataAtom *
MP4FileX::CreateMetadataAtom(const char *name, itmf::BasicType typeCode)
{
    MP4Atom *pAtom = AddDescendantAtoms("moov",
	    format("udta.meta.ilst.%s", name).c_str());
    if (!pAtom) return 0;
    MP4DataAtom *pDataAtom = dynamic_cast<MP4DataAtom*>(
	    MP4Atom::CreateAtom(pAtom, "data"));
    if (!pDataAtom) return 0;
    pDataAtom->typeCode.SetValue(typeCode);
    pAtom->AddChildAtom(pDataAtom);

    MP4Atom *pHdlrAtom = FindAtom("moov.udta.meta.hdlr");
    MP4Property *pProp;
    pHdlrAtom->FindProperty("hdlr.reserved2", &pProp);
    static const uint8_t val[12] = { 0x61, 0x70, 0x70, 0x6c, 0 };
    dynamic_cast<MP4BytesProperty*>(pProp)->SetValue(val, 12);

    return pDataAtom;
}

MP4DataAtom *MP4FileX::FindOrCreateMetadataAtom(
	const char *atom, itmf::BasicType typeCode)
{
    std::string atomstring = format("moov.udta.meta.ilst.%s.data", atom);
    MP4DataAtom *pAtom = dynamic_cast<MP4DataAtom*>(
	    FindAtom(atomstring.c_str()));
    if (!pAtom)
	pAtom = CreateMetadataAtom(atom, typeCode);
    return pAtom;
}

bool MP4FileX::SetMetadataString (const char *atom, const char *value)
{
    MP4DataAtom *pAtom = FindOrCreateMetadataAtom(atom, itmf::BT_UTF8);
    if (pAtom)
	pAtom->metadata.SetValue(reinterpret_cast<const uint8_t*>(value),
		std::strlen(value));
    return pAtom != 0;
}

bool MP4FileX::SetMetadataTrack(uint16_t track, uint16_t totalTracks)
{
    MP4DataAtom *pAtom = FindOrCreateMetadataAtom("trkn", itmf::BT_IMPLICIT);
    if (pAtom) {
	uint8_t v[8] = { 0 };
	v[2] = track >> 8;
	v[3] = track & 0xff;
	v[4] = totalTracks >> 8;
	v[5] = totalTracks & 0xff;
	pAtom->metadata.SetValue(v, 8);
    }
    return pAtom != 0;
}

bool MP4FileX::SetMetadataDisk(uint16_t disk, uint16_t totalDisks)
{
    MP4DataAtom *pAtom = FindOrCreateMetadataAtom("disk", itmf::BT_IMPLICIT);
    if (pAtom) {
	uint8_t v[6] = { 0 };
	v[2] = disk >> 8;
	v[3] = disk & 0xff;
	v[4] = totalDisks >> 8;
	v[5] = totalDisks & 0xff;
	pAtom->metadata.SetValue(v, 6);
    }
    return pAtom != 0;
}

bool MP4FileX::SetMetadataUint8(const char *atom, uint8_t value)
{
    MP4DataAtom *pAtom = FindOrCreateMetadataAtom(atom, itmf::BT_INTEGER);
    if (pAtom)
	pAtom->metadata.SetValue(&value, 1);
    return pAtom != 0;
}

bool MP4FileX::SetMetadataUint16(const char *atom, uint16_t value)
{
    MP4DataAtom *pAtom = FindOrCreateMetadataAtom(atom, itmf::BT_INTEGER);
    if (pAtom) {
	uint8_t v[2];
	v[0] = value >> 8;
	v[1] = value & 0xff;
	pAtom->metadata.SetValue(v, 2);
    }
    return pAtom != 0;
}

bool MP4FileX::SetMetadataGenre(const char *atom, uint16_t value)
{
    MP4DataAtom *pAtom = FindOrCreateMetadataAtom(atom, itmf::BT_IMPLICIT);
    if (pAtom) {
	uint8_t v[2];
	v[0] = value >> 8;
	v[1] = value & 0xff;
	pAtom->metadata.SetValue(v, 2);
    }
    return pAtom != 0;
}

bool MP4FileX::SetMetadataFreeForm(const char *name, const char *mean,
      const uint8_t* pValue, uint32_t valueSize, itmf::BasicType typeCode)
{
    MP4DataAtom *pDataAtom = 0;
    std::string tagname;
    for (int i = 0;; ++i) {
	tagname = format("moov.udta.meta.ilst.----[%d]", i);
	MP4Atom *pTagAtom = FindAtom(tagname.c_str());
	if (!pTagAtom)	
	    break;
	MP4NameAtom *pNameAtom =
	    dynamic_cast<MP4NameAtom*>(pTagAtom->FindChildAtom("name"));
	if (!pNameAtom || pNameAtom->value.CompareToString(name))
	    continue;
	MP4MeanAtom *pMeanAtom =
	    dynamic_cast<MP4MeanAtom*>(pTagAtom->FindChildAtom("mean"));
	if (!pMeanAtom || pMeanAtom->value.CompareToString(mean))
	    continue;
	pDataAtom =
	    dynamic_cast<MP4DataAtom*>(pTagAtom->FindChildAtom("data"));
	if (!pDataAtom) {
	    pDataAtom = dynamic_cast<MP4DataAtom*>(
		    MP4Atom::CreateAtom(pTagAtom, "data"));
	    pTagAtom->AddChildAtom(pDataAtom);
	}
	break;
    }
    if (!pDataAtom) {
	MP4Atom *pTagAtom = AddDescendantAtoms("moov", tagname.c_str() + 5);
	if (!pTagAtom) return false;

	MP4NameAtom *pNameAtom = dynamic_cast<MP4NameAtom*>(
		MP4Atom::CreateAtom(pTagAtom, "name"));
	if (!pNameAtom) return false;
	pNameAtom->value.SetValue(
		reinterpret_cast<const uint8_t*>(name), std::strlen(name));
	pTagAtom->AddChildAtom(pNameAtom);

	MP4MeanAtom *pMeanAtom = dynamic_cast<MP4MeanAtom*>(
		MP4Atom::CreateAtom(pTagAtom, "mean"));
	if (!pMeanAtom) return false;
	pMeanAtom->value.SetValue(
		reinterpret_cast<const uint8_t*>(mean), std::strlen(mean));
	pTagAtom->AddChildAtom(pMeanAtom);

	pDataAtom = dynamic_cast<MP4DataAtom*>(
		MP4Atom::CreateAtom(pTagAtom, "data"));
	if (!pDataAtom) return false;
	pTagAtom->AddChildAtom(pDataAtom);
    }
    pDataAtom->typeCode.SetValue(typeCode);
    pDataAtom->metadata.SetValue(pValue, valueSize);
    return true;
}

