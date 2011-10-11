#ifndef _QTMOVIEHELPER_H
#define _QTMOVIEHELPER_H

#include <Movies.h>
#include "qthelper.h"
#include "win32util.h"

class DataReferenceX {
    HandleX m_ref;
    OSType m_type;
public:
    DataReferenceX(Handle h, OSType type): m_ref(h), m_type(type) {}
    static DataReferenceX Open(const std::wstring &path)
    {
	Handle ref;
	OSType type;
	/*
	 * XXX: QuickTime seems to return pathTooLongErr for very long paths.
	 */
	std::wstring fullpath = GetFullPathNameX(path.c_str());
	TRYF(QTNewDataReferenceFromFullPathCFString(
		    W2CF(fullpath), kQTNativeDefaultPathStyle,
		    0, &ref, &type), path);
	return DataReferenceX(ref, type);
    }
    Handle getHandle() const { return m_ref; }
    OSType getType() const { return m_type; }
};

class MovieX: public PropertySupport<MovieX> {
    enum { kAudio = kQTPropertyClass_Audio };
    enum {
	kRenderQuality = kQTAudioPropertyID_RenderQuality,
	kSummaryChannelLayout = kQTAudioPropertyID_SummaryChannelLayout,
	kDeviceChannelLayout = kQTAudioPropertyID_DeviceChannelLayout,
	kDeviceASBD = kQTAudioPropertyID_DeviceASBD,
	kSummaryASBD = kQTAudioPropertyID_SummaryASBD,
    };
    x::shared_ptr<MovieType *> m_instance;
public:
    MovieX() {}
    MovieX(Movie movie) : m_instance(movie, DisposeMovie) {}
    operator Movie() { return m_instance.get(); }
    static MovieX FromFile(const std::wstring &path)
    {
	DirectorySaver __saver__;
	DataReferenceX ref = DataReferenceX::Open(path);
	Movie movie;
	TRYF(NewMovieFromDataRef(
		    &movie,
		    newMovieActive | newMovieDontAskUnresolvedDataRefs,
		    0,
		    ref.getHandle(),
		    ref.getType()), path);
	/*
	 * DataReference handle is closed when exiting this function,
	 * but that seems OK.
	 * cf. scaudiocompress sample
	 */
	return movie;
    }
    void getSummaryChannelLayout(AudioChannelLayoutX *result)
    {
	AudioChannelLayoutX::owner_t value;
	getPointerProperty(kAudio, kSummaryChannelLayout, &value);
	*result = value.get();
    }

    /* for PropertySupport */
    long _getPropertyInfo(
		OSType inPropClass,
		OSType inPropID,
		OSType *outPropType,
		ByteCount *outPropValueSize,
		UInt32 *outPropertyFlags) const
    {
	return QTGetMoviePropertyInfo(
		    m_instance.get(),
		    inPropClass,
		    inPropID,
		    outPropType,
		    outPropValueSize,
		    outPropertyFlags);
    }
    long _getProperty(
		OSType inPropClass,
		OSType inPropID,
		ByteCount inPropValueSize,
		void *outPropValueAddress,
		ByteCount *outPropValueSizeUsed) const
    {
	return QTGetMovieProperty(
		    m_instance.get(),
		    inPropClass,
		    inPropID,
		    inPropValueSize,
		    outPropValueAddress,
		    outPropValueSizeUsed);
    }
    long _setProperty(
		OSType inPropClass,
		OSType inPropID,
		ByteCount inPropValueSize,
		const void *inPropValueAddress) const
    {
	return QTSetMovieProperty(
		    m_instance.get(),
		    inPropClass,
		    inPropID,
		    inPropValueSize,
		    inPropValueAddress);
    }
};

class TrackX: public PropertySupport<TrackX> {
    enum { kAudio = kQTPropertyClass_Audio };
    enum {
	kRenderQuality = kQTAudioPropertyID_RenderQuality,
	kChannelLayout = kQTAudioPropertyID_ChannelLayout,
	kFormatString = kQTAudioPropertyID_FormatString,
	kChannelLayoutString = kQTAudioPropertyID_ChannelLayoutString
    };
    Track m_instance;
public:
    TrackX(Track track) : m_instance(track) {}
    operator Track() { return m_instance; }

    void getChannelLayout(AudioChannelLayoutX *result)
    {
	AudioChannelLayoutX::owner_t value;
	getPointerProperty(kAudio, kChannelLayout, &value);
	*result = value.get();
    }
    /* for PropertySupport */
    long _getPropertyInfo(
		OSType inPropClass,
		OSType inPropID,
		OSType *outPropType,
		ByteCount *outPropValueSize,
		UInt32 *outPropertyFlags) const
    {
	return QTGetTrackPropertyInfo(
		    m_instance,
		    inPropClass,
		    inPropID,
		    outPropType,
		    outPropValueSize,
		    outPropertyFlags);
    }
    long _getProperty(
		OSType inPropClass,
		OSType inPropID,
		ByteCount inPropValueSize,
		void *outPropValueAddress,
		ByteCount *outPropValueSizeUsed) const
    {
	return QTGetTrackProperty(
		    m_instance,
		    inPropClass,
		    inPropID,
		    inPropValueSize,
		    outPropValueAddress,
		    outPropValueSizeUsed);
    }
    long _setProperty(
		OSType inPropClass,
		OSType inPropID,
		ByteCount inPropValueSize,
		const void *inPropValueAddress) const
    {
	return QTSetTrackProperty(
		    m_instance,
		    inPropClass,
		    inPropID,
		    inPropValueSize,
		    inPropValueAddress);
    }
};

class MovieStorageX {
    x::shared_ptr<ComponentInstanceRecord> m_instance;
public:
    MovieStorageX() {}
    MovieStorageX(DataHandler dh) : m_instance(dh, CloseMovieStorage) {}
    operator DataHandler() { return m_instance.get(); }
    static MovieStorageX Create(const std::wstring &path, MovieX *outMovie)
    {
	DataReferenceX ref = DataReferenceX::Open(path);
	DataHandler dh;
	Movie movie;
	TRYF(CreateMovieStorage(
		    ref.getHandle(),
		    ref.getType(),
		    'TVOD',
		    0,
		    createMovieFileDeleteCurFile |
		    	createMovieFileDontCreateResFile,
		    &dh,
		    &movie), path);
	*outMovie = MovieX(movie);
	return dh;
    }
};

class QTAtomContainerX {
    x::shared_ptr<Ptr> m_instance;
public:
    QTAtomContainerX() 
    {
	QTAtomContainer ac;
	TRYE(QTNewAtomContainer(&ac));
	m_instance = x::shared_ptr<Ptr>(ac, QTDisposeAtomContainer);
    }
    QTAtomContainerX(QTAtomContainer ac)
	: m_instance(ac, QTDisposeAtomContainer)
    {}
    operator QTAtomContainer() { return m_instance.get(); }
};

class QTContainerLockerX {
    QTAtomContainer m_container;
public:
    explicit QTContainerLockerX(QTAtomContainer container)
	: m_container(container)
    {
	QTLockContainer(m_container);
    }
    ~QTContainerLockerX()
    {
	QTUnlockContainer(m_container);
    }
};

class SoundDescriptionX : public PropertySupport<SoundDescriptionX> {
    enum { kSelf = kQTPropertyClass_SoundDescription };
    enum {
	kAudioChannelLayout = kQTSoundDescriptionPropertyID_AudioChannelLayout,
	kMagicCookie = kQTSoundDescriptionPropertyID_MagicCookie,
	kAudioStreamBasicDescription
	    = kQTSoundDescriptionPropertyID_AudioStreamBasicDescription,
	kBitRate = kQTSoundDescriptionPropertyID_BitRate,
	kUserReadableText = kQTSoundDescriptionPropertyID_UserReadableText
    };
    x::shared_ptr<SoundDescriptionPtr> m_instance;
public:
    SoundDescriptionX():
   	m_instance(reinterpret_cast<SoundDescriptionHandle>(NewHandle(0)),
		DisposeHandleX) {}
    SoundDescriptionX(SoundDescriptionHandle handle):
	m_instance(handle, DisposeHandleX) {}
    operator SoundDescriptionHandle() { return m_instance.get(); }
    operator SampleDescriptionHandle()
    {
	return reinterpret_cast<SampleDescriptionHandle>(m_instance.get());
    }
    static SoundDescriptionX FromBasicDescription(
	    AudioStreamBasicDescription &format)
    {
	SoundDescriptionHandle sdh = 0;
	TRYE(QTSoundDescriptionCreate(&format, 0, 0, 0, 0,
		kQTSoundDescriptionKind_Movie_LowestPossibleVersion, &sdh));
	return SoundDescriptionX(sdh);
    }
    void getAudioChannelLayout(AudioChannelLayoutX *result)
    {
	AudioChannelLayoutX::owner_t value;
	getPointerProperty(kSelf, kAudioChannelLayout, &value);
	*result = value.get();
    }
    void setAudioChannelLayout(const AudioChannelLayout *acl)
    {
	ByteCount size = AudioChannelLayout_length(acl);
	setProperty(kSelf, kAudioChannelLayout, size, acl);
    }
    void getMagicCookie(std::vector<char> *result)
    {
	getVectorProperty(kSelf, kMagicCookie, result);
    }
    void setMagicCookie(const void *cookie, ByteCount size)
    {
	setProperty(kSelf, kMagicCookie, size, cookie);
    }
    void getAudioStreamBasicDescription(AudioStreamBasicDescription *asbd)
    {
	getPodProperty(kSelf, kAudioStreamBasicDescription, asbd);
    }

    /* for PropertySupport */
    long _getPropertyInfo(
		OSType inPropClass,
		OSType inPropID,
		OSType *outPropType,
		ByteCount *outPropValueSize,
		UInt32 *outPropertyFlags) const
    {
	return QTSoundDescriptionGetPropertyInfo(
		    m_instance.get(),
		    inPropClass,
		    inPropID,
		    outPropType,
		    outPropValueSize,
		    outPropertyFlags);
    }
    long _getProperty(
		OSType inPropClass,
		OSType inPropID,
		ByteCount inPropValueSize,
		void *outPropValueAddress,
		ByteCount *outPropValueSizeUsed) const
    {
	return QTSoundDescriptionGetProperty(
		    m_instance.get(),
		    inPropClass,
		    inPropID,
		    inPropValueSize,
		    outPropValueAddress,
		    outPropValueSizeUsed);
    }
    long _setProperty(
		OSType inPropClass,
		OSType inPropID,
		ByteCount inPropValueSize,
		const void *inPropValueAddress) const
    {
	return QTSoundDescriptionSetProperty(
		    m_instance.get(),
		    inPropClass,
		    inPropID,
		    inPropValueSize,
		    inPropValueAddress);
    }
};

#endif
