#include "util.h"
#include "mp4v2wrapper.h"
#include "strcnv.h"
#include "utf8_codecvt_facet.hpp"

using mp4v2::impl::MP4File;
using mp4v2::impl::MP4Track;
using mp4v2::impl::MP4Atom;
using mp4v2::impl::MP4RootAtom;
using mp4v2::impl::MP4DataAtom;
using mp4v2::impl::MP4NameAtom;
using mp4v2::impl::MP4MeanAtom;
using mp4v2::impl::MP4Property;
using mp4v2::impl::MP4Integer16Property;
using mp4v2::impl::MP4Integer32Property;
using mp4v2::impl::MP4StringProperty;
using mp4v2::impl::MP4BytesProperty;
using mp4v2::platform::io::File;
namespace itmf = mp4v2::impl::itmf;

std::string format_mp4error(const mp4v2::impl::Exception &e)
{
    std::wstring wmsg = m2w(e.msg(), utf8_codecvt_facet());
    return format("libmp4v2: %ls", wmsg.c_str());
}

class MP4AlacAtom: public MP4Atom {
public:
    MP4AlacAtom(MP4File &file, const char *id): MP4Atom(file, id)
    {
	AddVersionAndFlags();
	AddProperty(new MP4BytesProperty(*this, "decoderConfig"));
    }
};

class MP4ChanAtom: public MP4Atom {
public:
    MP4ChanAtom(MP4File &file, const char *id): MP4Atom(file, id)
    {
	AddVersionAndFlags();
	AddProperty(new MP4BytesProperty(*this, "channelLayout"));
    }
};

extern FILE *win32_tmpfile(const wchar_t *prefix);

namespace myprovider {

static
void *open(const char *name, MP4FileMode mode)
{
    return win32_tmpfile(m2w(name, utf8_codecvt_facet()).c_str());
}

static
int seek(void *fh, int64_t pos)
{
    FILE *fp = reinterpret_cast<FILE*>(fh);
    return std::fsetpos(fp, static_cast<fpos_t*>(&pos));
}

static
int read(void *fh, void *data, int64_t size, int64_t *nc, int64_t)
{
    FILE *fp = reinterpret_cast<FILE*>(fh);
    size_t n = std::fread(data, 1, size, fp);
    *nc = n;
    return std::ferror(fp);
}

static
int write(void *fh, const void *data, int64_t size, int64_t *nc, int64_t)
{
    FILE *fp = reinterpret_cast<FILE*>(fh);
    size_t n = std::fwrite(data, 1, size, fp);
    *nc = n;
    return std::ferror(fp);
}

static
int close(void *fh)
{
    FILE *fp = reinterpret_cast<FILE*>(fh);
    return std::fclose(fp);
}

} // namespace myprovider


void MP4FileX::CreateTemp(const char *prefix,
	    uint32_t flags, int add_ftyp, int add_iods,
	    char *majorBrand, uint32_t minorVersion,
	    char **supportedBrands, uint32_t supportedBrandsCount)
{
    MP4FileProvider provider = {
	myprovider::open, myprovider::seek, myprovider::read,
	myprovider::write, myprovider::close
    };
    m_createFlags = flags;
    Open(prefix, File::MODE_CREATE, &provider);

    m_pRootAtom = MP4Atom::CreateAtom(*this, NULL, NULL);
    m_pRootAtom->Generate();

    if (add_ftyp)
        MakeFtypAtom(majorBrand, minorVersion,
                     supportedBrands, supportedBrandsCount);
    CacheProperties();
    InsertChildAtom(m_pRootAtom, "mdat", add_ftyp ? 1 : 0);
    m_pRootAtom->BeginWrite();
    if (add_iods != 0) (void)AddChildAtom("moov", "iods");
}

MP4TrackId
MP4FileX::AddAlacAudioTrack(const uint8_t *alac, const uint8_t *chan)
{
    uint32_t bitsPerSample = alac[5];
    uint32_t nchannels = alac[9];
    uint32_t timeScale;
    std::memcpy(&timeScale, alac + 20, 4);
    timeScale = b2host32(timeScale);

    SetTimeScale(timeScale);
    MP4TrackId track = AddTrack(MP4_AUDIO_TRACK_TYPE, timeScale);
    AddTrackToOd(track);

    SetTrackFloatProperty(track, "tkhd.volume", 1.0);

    InsertChildAtom(MakeTrackName(track, "mdia.minf"), "smhd", 0);
    AddChildAtom(MakeTrackName(track, "mdia.minf.stbl.stsd"), "alac");

    MP4Atom *atom = FindTrackAtom(track, "mdia.minf.stbl.stsd");
    MP4Property *pProp;
    atom->FindProperty("stsd.entryCount", &pProp);
    dynamic_cast<MP4Integer32Property*>(pProp)->IncrementValue();

    atom = atom->FindChildAtom("alac");

    /* XXX
       Would overflow when samplerate >= 65536, so we shift down here.
       Anyway, iTunes seems to always set 44100 to stsd samplerate for ALAC.
     */
    while (timeScale & 0xffff0000)
	timeScale >>= 1;
    atom->FindProperty("alac.timeScale", &pProp);
    dynamic_cast<MP4Integer32Property*>(pProp)->SetValue(timeScale<<16);
    atom->FindProperty("alac.sampleSize", &pProp);
    dynamic_cast<MP4Integer16Property*>(pProp)->SetValue(bitsPerSample);
    atom->FindProperty("alac.channels", &pProp);
    dynamic_cast<MP4Integer16Property*>(pProp)->SetValue(nchannels);

    MP4AlacAtom *alacAtom = new MP4AlacAtom(*this, "alac");
    pProp = alacAtom->GetProperty(2);
    dynamic_cast<MP4BytesProperty*>(pProp)->SetValue(alac, 24, 0);
    atom->AddChildAtom(alacAtom);

    if (chan) {
	MP4ChanAtom *chanAtom = new MP4ChanAtom(*this, "chan");
	pProp = chanAtom->GetProperty(2);
	dynamic_cast<MP4BytesProperty*>(pProp)->SetValue(chan, 12, 0);
	atom->AddChildAtom(chanAtom);
    }
    return track;
}

MP4DataAtom *
MP4FileX::CreateMetadataAtom(const char *name, itmf::BasicType typeCode)
{
    MP4Atom *pAtom = AddDescendantAtoms("moov",
	    format("udta.meta.ilst.%s", name).c_str());
    if (!pAtom) return 0;
    MP4DataAtom *pDataAtom = AddChildAtomT(pAtom, "data");
    pDataAtom->typeCode.SetValue(typeCode);

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
    MP4DataAtom *pAtom = FindAtomT(atomstring.c_str());
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

bool MP4FileX::SetMetadataArtwork(const char *atom,
	const char *data, size_t size, itmf::BasicType typeCode)
{
    MP4Atom *covr = FindAtom("moov.udta.meta.ilst.covr");
    if (!covr) {
	AddDescendantAtoms("moov", "udta.meta.ilst.covr");
	covr = FindAtom("moov.udta.meta.ilst.covr");
	if (!covr) return false;
    }
    MP4DataAtom *pDataAtom = AddChildAtomT(covr, "data");
    if (typeCode == itmf::BT_UNDEFINED)
	typeCode = itmf::computeBasicType(data, size);
    pDataAtom->typeCode.SetValue(typeCode);
    pDataAtom->metadata.SetValue(
	reinterpret_cast<const uint8_t *>(data), size);
    return true;
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
	MP4NameAtom *pNameAtom = FindChildAtomT(pTagAtom, "name");
	if (!pNameAtom || pNameAtom->value.CompareToString(name))
	    continue;
	MP4MeanAtom *pMeanAtom = FindChildAtomT(pTagAtom, "mean");
	if (!pMeanAtom || pMeanAtom->value.CompareToString(mean))
	    continue;
	MP4DataAtom *pDataAtom = FindChildAtomT(pTagAtom, "data");
	if (!pDataAtom)
	    pDataAtom = AddChildAtomT(pTagAtom, "data");
	break;
    }
    if (!pDataAtom) {
	MP4Atom *pTagAtom = AddDescendantAtoms("moov", tagname.c_str() + 5);
	if (!pTagAtom) return false;

	MP4NameAtom *pNameAtom = AddChildAtomT(pTagAtom, "name");
	pNameAtom->value.SetValue(
		reinterpret_cast<const uint8_t*>(name), std::strlen(name));

	MP4MeanAtom *pMeanAtom = AddChildAtomT(pTagAtom, "mean");
	pMeanAtom->value.SetValue(
		reinterpret_cast<const uint8_t*>(mean), std::strlen(mean));

	pDataAtom = AddChildAtomT(pTagAtom, "data");
    }
    pDataAtom->typeCode.SetValue(typeCode);
    pDataAtom->metadata.SetValue(pValue, valueSize);
    return true;
}

MP4FileCopy::MP4FileCopy(MP4File *file)
	: m_mp4file(reinterpret_cast<MP4FileX*>(file)),
	  m_src(reinterpret_cast<MP4FileX*>(file)->m_file),
	  m_dst(0)
{
    m_nchunks = 0;
    size_t numTracks = file->GetNumberOfTracks();
    for (size_t i = 0; i < numTracks; ++i) {
	ChunkInfo ci;
	ci.current = 1;
	ci.final = m_mp4file->m_pTracks[i]->GetNumberOfChunks();
	ci.time = MP4_INVALID_TIMESTAMP;
	m_state.push_back(ci);
	m_nchunks += ci.final;
    }
}

void MP4FileCopy::start(const char *path)
{
    m_mp4file->m_file = 0;
    try {
	m_mp4file->Open(path, File::MODE_CREATE, 0);
    } catch (...) {
	m_mp4file->ResetFile();
	throw;
    }
    m_dst = m_mp4file->m_file;
    m_mp4file->SetIntegerProperty("moov.mvhd.modificationTime",
	mp4v2::impl::MP4GetAbsTimestamp());
    dynamic_cast<MP4RootAtom*>(m_mp4file->m_pRootAtom)->BeginOptimalWrite();
}

void MP4FileCopy::finish()
{
    if (!m_dst)
	return;
    try {
	MP4RootAtom *root = dynamic_cast<MP4RootAtom*>(m_mp4file->m_pRootAtom);
	root->FinishOptimalWrite();
    } catch (...) {
	delete m_src;
	delete m_dst;
	m_dst = 0;
	m_mp4file->m_file = 0;
	throw;
    }
    delete m_src;
    delete m_dst;
    m_dst = 0;
    m_mp4file->m_file = 0;
}

bool MP4FileCopy::copyNextChunk()
{
    uint32_t nextTrack = -1;
    MP4Timestamp nextTime = MP4_INVALID_TIMESTAMP;
    size_t numTracks = m_mp4file->GetNumberOfTracks();
    for (size_t i = 0; i < numTracks; ++i) {
	MP4Track *track = m_mp4file->m_pTracks[i];
	if (m_state[i].current > m_state[i].final)
	    continue;
	if (m_state[i].time == MP4_INVALID_TIMESTAMP) {
	    MP4Timestamp time = track->GetChunkTime(m_state[i].current);
	    m_state[i].time = mp4v2::impl::MP4ConvertTime(time,
		    track->GetTimeScale(), m_mp4file->GetTimeScale());
	}
	if (m_state[i].time > nextTime)
	    continue;
	if (m_state[i].time == nextTime &&
		std::strcmp(track->GetType(), MP4_HINT_TRACK_TYPE))
	    continue;
	nextTime = m_state[i].time;
	nextTrack = i;
    }
    if (nextTrack == -1) return false;
    MP4Track *track = m_mp4file->m_pTracks[nextTrack];
    m_mp4file->m_file = m_src;
    uint8_t *chunk;
    uint32_t size;
    track->ReadChunk(m_state[nextTrack].current, &chunk, &size);
    m_mp4file->m_file = m_dst;
    track->RewriteChunk(m_state[nextTrack].current, chunk, size);
    MP4Free(chunk);
    m_state[nextTrack].current++;
    m_state[nextTrack].time = MP4_INVALID_TIMESTAMP;
    return true;
}

