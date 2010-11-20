#ifndef _MOVIEAUDIO_H
#define _MOVIEAUDIO_H

#include "qthelper.h"

class MovieAudioExtractionX : public PropertySupport<MovieAudioExtractionX> {
    MovieX m_movie;
    std::tr1::shared_ptr<MovieAudioExtractionRefRecord> m_instance;
    enum {
	kMovie = kQTPropertyClass_MovieAudioExtraction_Movie,
	kAudio = kQTPropertyClass_MovieAudioExtraction_Audio
    };
    enum {
	kCurrentTime = kQTMovieAudioExtractionMoviePropertyID_CurrentTime,
	kAllChannelsDiscrete =
	    kQTMovieAudioExtractionMoviePropertyID_AllChannelsDiscrete,
	kRenderQuality =
	    kQTMovieAudioExtractionAudioPropertyID_RenderQuality
    };
    enum {
	kAudioStreamBasicDescription = 
	    kQTMovieAudioExtractionAudioPropertyID_AudioStreamBasicDescription,
	kAudioChannelLayout =
	    kQTMovieAudioExtractionAudioPropertyID_AudioChannelLayout,
	kRemainingAudioDuration =
	    kQTMovieAudioExtractionAudioPropertyID_RemainingAudioDuration
    };
    enum {
	kMaxSampleSize = FOUR_CHAR_CODE('mssz')
    };
public:
    MovieAudioExtractionX() {}
    MovieAudioExtractionX(MovieX &movie, MovieAudioExtractionRef mae):
	m_movie(movie),
	m_instance(mae, MovieAudioExtractionEnd)
    {}
    static MovieAudioExtractionX Begin(MovieX &movie, UInt32 flags)
    {
	MovieAudioExtractionRef ref;
	TRYE(MovieAudioExtractionBegin(movie, flags, &ref));
	return MovieAudioExtractionX(movie, ref);
    }
    operator MovieAudioExtractionRef() { return m_instance.get(); }

    void getCurrentTime(TimeRecord *value)
    {
	getPodProperty(kMovie, kCurrentTime, value);
    }
    void setCurrentTime(const TimeRecord &value)
    {
	setPodProperty(kMovie, kCurrentTime, value);
    }
    Boolean getAllChannelsDiscrete()
    {
	return getPodProperty<Boolean>(kMovie, kAllChannelsDiscrete);
    }
    void setAllChannelsDiscrete(Boolean value)
    {
	setPodProperty(kMovie, kAllChannelsDiscrete, value);
    }
    UInt32 getRenderQuality()
    {
	return getPodProperty<UInt32>(kAudio, kRenderQuality);
    }
    void setRenderQuality(UInt32 value)
    {
	return setPodProperty(kAudio, kRenderQuality, value);
    }
    void getAudioStreamBasicDescription(AudioStreamBasicDescription *asbd)
    {
	getPodProperty(kAudio, kAudioStreamBasicDescription, asbd);
    }
    void setAudioStreamBasicDescription(
	    const AudioStreamBasicDescription &asbd)
    {
	setPodProperty(kAudio, kAudioStreamBasicDescription, asbd);
    }
    void getAudioChannelLayout(AudioChannelLayoutX *result)
    {
	AudioChannelLayoutX::owner_t value;
	getPointerProperty(kAudio, kAudioChannelLayout, &value);
	*result = value.get();
    }
    void setAudioChannelLayout(const AudioChannelLayout *acl)
    {
	ByteCount size = AudioChannelLayout_length(acl);
	setProperty(kAudio, kAudioChannelLayout, size, acl);
    }
    void getRemainingAudioDuration(TimeRecord *result)
    {
	getPodProperty(kAudio, kRemainingAudioDuration, result);
    }
    UInt32 getMaxSampleSize()
    {
	return getPodProperty<UInt32>(kAudio, kMaxSampleSize);
    }
    /* for PropertySupport */
    long _getPropertyInfo(
		OSType inPropClass,
		OSType inPropID,
		OSType *outPropType,
		ByteCount *outPropValueSize,
		UInt32 *outPropertyFlags) const
    {
	return MovieAudioExtractionGetPropertyInfo(
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
	return MovieAudioExtractionGetProperty(
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
	return MovieAudioExtractionSetProperty(
		    m_instance.get(),
		    inPropClass,
		    inPropID,
		    inPropValueSize,
		    inPropValueAddress);
    }
};

#endif
