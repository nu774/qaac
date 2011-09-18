///////////////////////////////////////////////////////////////////////////////
//
//  The contents of this file are subject to the Mozilla Public License
//  Version 1.1 (the "License"); you may not use this file except in
//  compliance with the License. You may obtain a copy of the License at
//  http://www.mozilla.org/MPL/
//
//  Software distributed under the License is distributed on an "AS IS"
//  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
//  License for the specific language governing rights and limitations
//  under the License.
//
//  The Original Code is MPEG4IP.
//
//  The Initial Developer of the Original Code is Cisco Systems Inc.
//  Portions created by Cisco Systems Inc. are
//  Copyright (C) Cisco Systems Inc. 2001 - 2005.  All Rights Reserved.
//
//  3GPP features implementation is based on 3GPP's TS26.234-v5.60,
//  and was contributed by Ximpo Group Ltd.
//
//  Portions created by Ximpo Group Ltd. are
//  Copyright (C) Ximpo Group Ltd. 2003, 2004.  All Rights Reserved.
//
//  Contributors:
//      Dave Mackie, dmackie@cisco.com
//      Alix Marchandise-Franquet, alix@cisco.com
//      Ximpo Group Ltd., mp4v2@ximpo.com
//      Kona Blend, kona8lend@@gmail.com
//
///////////////////////////////////////////////////////////////////////////////

#ifndef MP4V2_IMPL_MP4FILE_H
#define MP4V2_IMPL_MP4FILE_H

namespace mp4v2 { namespace impl {

///////////////////////////////////////////////////////////////////////////////

class MP4Atom;
class MP4Property;
class MP4Float32Property;
class MP4StringProperty;
class MP4BytesProperty;
class MP4Descriptor;
class MP4DescriptorProperty;

class MP4File
{
public:
    static void CopySample(
        MP4File*    srcFile,
        MP4TrackId  srcTrackId,
        MP4SampleId srcSampleId,
        MP4File*    dstFile,
        MP4TrackId  dstTrackId,
        MP4Duration dstSampleDuration );

    static void EncAndCopySample(
        MP4File*      srcFile,
        MP4TrackId    srcTrackId,
        MP4SampleId   srcSampleId,
        encryptFunc_t encfcnp,
        uint32_t      encfcnparam1,
        MP4File*      dstFile,
        MP4TrackId    dstTrackId,
        MP4Duration   dstSampleDuration );

public:
    MP4File();
    ~MP4File();

    ///////////////////////////////////////////////////////////////////////////
    // file ops
    ///////////////////////////////////////////////////////////////////////////

    void Create( const char* fileName,
                 uint32_t    flags,
                 int         add_ftyp = 1,
                 int         add_iods = 1,
                 char*       majorBrand = NULL,
                 uint32_t    minorVersion = 0,
                 char**      supportedBrands = NULL,
                 uint32_t    supportedBrandsCount = 0 );

    const std::string &GetFilename() const;
    void Read( const char* name, const MP4FileProvider* provider );
    bool Modify( const char* fileName );
    void Optimize( const char* srcFileName, const char* dstFileName = NULL );
    bool CopyClose( const string& copyFileName );
    void Dump( bool dumpImplicits = false );
    void Close(uint32_t flags = 0);

    bool Use64Bits(const char *atomName);
    void Check64BitStatus(const char *atomName);
    /* file properties */

    uint64_t GetIntegerProperty(const char* name);
    float GetFloatProperty(const char* name);
    const char* GetStringProperty(const char* name);
    void GetBytesProperty(const char* name,
                          uint8_t** ppValue, uint32_t* pValueSize);

    void SetIntegerProperty(const char* name, uint64_t value);
    void SetFloatProperty(const char* name, float value);
    void SetStringProperty(const char* name, const char* value);
    void SetBytesProperty(const char* name,
                          const uint8_t* pValue, uint32_t valueSize);

    // file level convenience functions

    MP4Duration GetDuration();
    void SetDuration(MP4Duration value);

    uint32_t GetTimeScale();
    void SetTimeScale(uint32_t value);

    uint8_t GetODProfileLevel();
    void SetODProfileLevel(uint8_t value);

    uint8_t GetSceneProfileLevel();
    void SetSceneProfileLevel(uint8_t value);

    uint8_t GetVideoProfileLevel();
    void SetVideoProfileLevel(uint8_t value);

    uint8_t GetAudioProfileLevel();
    void SetAudioProfileLevel(uint8_t value);

    uint8_t GetGraphicsProfileLevel();
    void SetGraphicsProfileLevel(uint8_t value);

    const char* GetSessionSdp();
    void SetSessionSdp(const char* sdpString);
    void AppendSessionSdp(const char* sdpString);

    /* track operations */

    MP4TrackId AddTrack(const char* type, uint32_t timeScale = 1000);
    void DeleteTrack(MP4TrackId trackId);

    uint32_t GetNumberOfTracks(const char* type = NULL, uint8_t subType = 0);

    MP4TrackId AllocTrackId();
    MP4TrackId FindTrackId(uint16_t trackIndex,
                           const char* type = NULL, uint8_t subType = 0);
    uint16_t FindTrackIndex(MP4TrackId trackId);
    uint16_t FindTrakAtomIndex(MP4TrackId trackId);

    /* track properties */
    MP4Atom *FindTrackAtom(MP4TrackId trackId, const char *name);
    uint64_t GetTrackIntegerProperty(
        MP4TrackId trackId, const char* name);
    float GetTrackFloatProperty(
        MP4TrackId trackId, const char* name);
    const char* GetTrackStringProperty(
        MP4TrackId trackId, const char* name);
    void GetTrackBytesProperty(
        MP4TrackId trackId, const char* name,
        uint8_t** ppValue, uint32_t* pValueSize);

    void SetTrackIntegerProperty(
        MP4TrackId trackId, const char* name, int64_t value);
    void SetTrackFloatProperty(
        MP4TrackId trackId, const char* name, float value);
    void SetTrackStringProperty(
        MP4TrackId trackId, const char* name, const char* value);
    void SetTrackBytesProperty(
        MP4TrackId trackId, const char* name,
        const uint8_t* pValue, uint32_t valueSize);

    bool GetTrackLanguage( MP4TrackId, char* );
    bool SetTrackLanguage( MP4TrackId, const char* );
    bool GetTrackName( MP4TrackId trackId, char** name );
    bool SetTrackName( MP4TrackId trackId, const char* name);

    /* sample operations */

    uint32_t GetSampleSize(MP4TrackId trackId, MP4SampleId sampleId);

    uint32_t GetTrackMaxSampleSize(MP4TrackId trackId);

    MP4SampleId GetSampleIdFromTime(MP4TrackId trackId,
                                    MP4Timestamp when, bool wantSyncSample = false);

    MP4Timestamp GetSampleTime(
        MP4TrackId trackId, MP4SampleId sampleId);

    MP4Duration GetSampleDuration(
        MP4TrackId trackId, MP4SampleId sampleId);

    MP4Duration GetSampleRenderingOffset(
        MP4TrackId trackId, MP4SampleId sampleId);

    bool GetSampleSync(
        MP4TrackId trackId, MP4SampleId sampleId);

    void ReadSample(
        // input parameters
        MP4TrackId trackId,
        MP4SampleId sampleId,
        // output parameters
        uint8_t**     ppBytes,
        uint32_t*     pNumBytes,
        MP4Timestamp* pStartTime = NULL,
        MP4Duration*  pDuration = NULL,
        MP4Duration*  pRenderingOffset = NULL,
        bool*         pIsSyncSample = NULL,
        bool*         hasDependencyFlags = NULL,
        uint32_t*     dependencyFlags = NULL );

    void WriteSample(
        MP4TrackId     trackId,
        const uint8_t* pBytes,
        uint32_t       numBytes,
        MP4Duration    duration = 0,
        MP4Duration    renderingOffset = 0,
        bool           isSyncSample = true );

    void WriteSampleDependency(
        MP4TrackId     trackId,
        const uint8_t* pBytes,
        uint32_t       numBytes,
        MP4Duration    duration,
        MP4Duration    renderingOffset,
        bool           isSyncSample,
        uint32_t       dependencyFlags );

    void SetSampleRenderingOffset(
        MP4TrackId  trackId,
        MP4SampleId sampleId,
        MP4Duration renderingOffset );

    MP4Duration GetTrackDurationPerChunk( MP4TrackId );
    void        SetTrackDurationPerChunk( MP4TrackId, MP4Duration );

    /* track level convenience functions */

    MP4TrackId AddSystemsTrack(const char* type, uint32_t timeScale = 1000 );

    MP4TrackId AddODTrack();

    MP4TrackId AddSceneTrack();

    MP4TrackId AddAudioTrack(
        uint32_t timeScale,
        MP4Duration sampleDuration,
        uint8_t audioType);

    MP4TrackId AddULawAudioTrack(
        uint32_t timeScale);

    MP4TrackId AddALawAudioTrack(
        uint32_t timeScale);

    MP4TrackId AddAC3AudioTrack(
        uint32_t samplingRate,
        uint8_t fscod,
        uint8_t bsid,
        uint8_t bsmod,
        uint8_t acmod,
        uint8_t lfeon,
        uint8_t bit_rate_code);

    MP4TrackId AddEncAudioTrack( // ismacryp
        uint32_t timeScale,
        MP4Duration sampleDuration,
        uint8_t  audioType,
        uint32_t scheme_type,
        uint16_t scheme_version,
        uint8_t  key_ind_len,
        uint8_t  iv_len,
        bool      selective_enc,
        const char  *kms_uri,
        bool      use_ismacryp);

    void SetAmrVendor(
        MP4TrackId trackId,
        uint32_t vendor);

    void SetAmrDecoderVersion(
        MP4TrackId trackId,
        uint8_t decoderVersion);

    void SetAmrModeSet(
        MP4TrackId trackId,
        uint16_t modeSet);
    uint16_t GetAmrModeSet(MP4TrackId trackId);

    MP4TrackId AddAmrAudioTrack(
        uint32_t timeScale,
        uint16_t modeSet,
        uint8_t modeChangePeriod,
        uint8_t framesPerSample,
        bool isAmrWB);

    MP4TrackId AddHrefTrack(uint32_t timeScale,
                            MP4Duration sampleDuration,
                            const char *base_url);

    MP4TrackId AddMP4VideoTrack(
        uint32_t timeScale,
        MP4Duration sampleDuration,
        uint16_t width,
        uint16_t height,
        uint8_t videoType);

    MP4TrackId AddEncVideoTrack( // ismacryp
        uint32_t timeScale,
        MP4Duration sampleDuration,
        uint16_t width,
        uint16_t height,
        uint8_t  videoType,
        mp4v2_ismacrypParams *icPp,
        const char *oFormat);

    void SetH263Vendor(
        MP4TrackId trackId,
        uint32_t vendor);

    void SetH263DecoderVersion(
        MP4TrackId trackId,
        uint8_t decoderVersion);

    void SetH263Bitrates(
        MP4TrackId,
        uint32_t avgBitrate,
        uint32_t maxBitrate);

    MP4TrackId AddH263VideoTrack(
        uint32_t timeScale,
        MP4Duration sampleDuration,
        uint16_t width,
        uint16_t height,
        uint8_t h263Level,
        uint8_t h263Profile,
        uint32_t avgBitrate,
        uint32_t maxBitrate);

    MP4TrackId AddH264VideoTrack(
        uint32_t timeScale,
        MP4Duration sampleDuration,
        uint16_t width,
        uint16_t height,
        uint8_t AVCProfileIndication,
        uint8_t profile_compat,
        uint8_t AVCLevelIndication,
        uint8_t sampleLenFieldSizeMinusOne);

    MP4TrackId AddEncH264VideoTrack(
        uint32_t timeScale,
        MP4Duration sampleDuration,
        uint16_t width,
        uint16_t height,
        MP4Atom *srcAtom,
        mp4v2_ismacrypParams *icPp);

    void AddH264SequenceParameterSet(MP4TrackId trackId,
                                     const uint8_t *pSequence,
                                     uint16_t sequenceLen);
    void AddH264PictureParameterSet(MP4TrackId trackId,
                                    const uint8_t *pPicture,
                                    uint16_t pictureLen);
    MP4TrackId AddHintTrack(MP4TrackId refTrackId);

    MP4TrackId AddTextTrack(MP4TrackId refTrackId);

    /** Add a QuickTime chapter track.
     *
     *  This function adds a chapter (text) track.
     *  The optional parameter <b>timescale</b> may be supplied to give the new
     *  chapter a specific timescale. Otherwise the chapter track will have
     *  the same timescale as the reference track defined in parameter refTrackId.
     *
     *  @param refTrackId ID of the track that will reference the chapter track.
     *  @param timescale the timescale of the chapter track or 0 to use the
     *      timescale of track specified by <b>refTrackId</b>.
     *
     *  @return ID of the created chapter track.
     */
    MP4TrackId AddChapterTextTrack(
        MP4TrackId refTrackId,
        uint32_t   timescale = 0 );

    MP4TrackId AddSubtitleTrack(uint32_t timescale,
                                uint16_t width,
                                uint16_t height);

    MP4TrackId AddSubpicTrack(uint32_t timescale,
                                uint16_t width,
                                uint16_t height);

    MP4TrackId AddPixelAspectRatio(MP4TrackId trackId, uint32_t hSpacing, uint32_t vSpacing);
    MP4TrackId AddColr(MP4TrackId trackId, uint16_t pri, uint16_t tran, uint16_t mat);

    /** Add a QuickTime chapter.
     *
     *  @param chapterTrackId ID of chapter track or #MP4_INVALID_TRACK_ID
     *      if unknown.
     *  @param chapterDuration duration (in the timescale of the chapter track).
     *  @param chapterTitle title text for the chapter or NULL to use default
     *      title format ("Chapter %03d", n) where n is the chapter number.
     */
    void AddChapter(
        MP4TrackId  chapterTrackId,
        MP4Duration chapterDuration,
        const char* chapterTitle = 0 );

    /** Add a Nero chapter.
     *
     *  @param chapterStart the start time of the chapter in 100 nanosecond units
     *  @param chapterTitle title text for the chapter or NULL to use default
     *      title format ("Chapter %03d", n) where n is the chapter number.
     */
    void AddNeroChapter(
        MP4Timestamp chapterStart,
        const char*  chapterTitle = 0 );

    /*! Returns the ID of the track referencing the chapter track chapterTrackId.
     *  This function searches for a track of type MP4_AUDIO_TRACK_TYPE that references
     *  the track chapterTrackId through the atom "tref.chap".
     *
     * @param chapterTrackId the ID of the chapter track
     * @param trackName receives the name of the referencing track if not null
     * @param trackNameSize the size of the memory pointed to by trackName
     * @return the ID if the track referencing the chapter track or MP4_INVALID_TRACK_ID
     */
    MP4TrackId FindChapterReferenceTrack(MP4TrackId chapterTrackId,
                                         char *trackName = 0,
                                         int trackNameSize = 0);

    /*! Find the QuickTime chapter track in the current file.
     * This function searches for a track of type text.
     *
     * @param trackName receives the name of the chapter track if not null
     * @param trackNameSize the size of the memory pointed to by trackName
     * @return the ID of the chapter track or MP4_INVALID_TRACK_ID
     */
    MP4TrackId FindChapterTrack(char *trackName = 0, int trackNameSize = 0);

    /** Delete chapters.
     *
     *  @param chapterType the type of chapters to delete:
     *      @li #MP4ChapterTypeAny (delete all known chapter types)
     *      @li #MP4ChapterTypeQt
     *      @li #MP4ChapterTypeNero
     *  @param chapterTrackId ID of the chapter track if known,
     *      or #MP4_INVALID_TRACK_ID.
     *      Only applies when <b>chapterType</b>=#MP4ChapterTypeQt.
     *
     * @return the type of deleted chapters
     */
    MP4ChapterType DeleteChapters(
        MP4ChapterType chapterType = MP4ChapterTypeQt,
        MP4TrackId chapterTrackId = 0 );

    /** Get list of chapters.
     *
     *  @param chapterList address receiving array of chapter items.
     *      If a non-NULL is received the caller is responsible for freeing the
     *      memory with MP4Free().
     *  @param chapterCount address receiving count of items in array.
     *  @param chapterType the type of chapters to read:
     *      @li #MP4ChapterTypeAny (any chapters, searched in order of Qt, Nero)
     *      @li #MP4ChapterTypeQt
     *      @li #MP4ChapterTypeNero
     *
     *  @result the first type of chapters found.
     */

    MP4ChapterType GetChapters(
        MP4Chapter_t** chapterList,
        uint32_t*      chapterCount,
        MP4ChapterType fromChapterType = MP4ChapterTypeQt );

    /** Set list of chapters.
     *
     *  This functions sets the complete chapter list.
     *  If any chapters of the same type already exist they will first
     *  be deleted.
     *
     *  @param chapterList array of chapters items.
     *  @param chapterCount count of items in array.
     *  @param chapterType type of chapters to write:
     *      @li #MP4ChapterTypeAny (chapters of all types are written)
     *      @li #MP4ChapterTypeQt
     *      @li #MP4ChapterTypeNero
     *
     *  @return the type of chapters written.
     */
    MP4ChapterType SetChapters(
        MP4Chapter_t*  chapterList,
        uint32_t       chapterCount,
        MP4ChapterType toChapterType = MP4ChapterTypeQt );

    /** Convert chapters to another type.
     *
     *  This function converts existing chapters
     *  from one type to another type.
     *  Conversion from Nero to QuickTime or QuickTime to Nero is supported.
     *
     *  @param toChapterType the chapter type to convert to:
     *      @li #MP4ChapterTypeQt (convert from Nero to Qt)
     *      @li #MP4ChapterTypeNero (convert from Qt to Nero)
     *
     *  @return the chapter type before conversion or #MP4ChapterTypeNone
     *      if the source chapters do not exist
     *      or invalid <b>toChapterType</b> was specified.
     */
    MP4ChapterType ConvertChapters(MP4ChapterType toChapterType = MP4ChapterTypeQt);

    /** Change the general timescale.
     *
     *  This function changes the general timescale to the new timescale
     *  <b>value</b> by recalculating all values that depend on the timescale
     *  in "moov.mvhd".
     *
     *  If the timescale is already equal to value nothing is done.
     *
     *  @param value the new timescale.
     */
    void ChangeMovieTimeScale(uint32_t timescale);

    MP4SampleId GetTrackNumberOfSamples(MP4TrackId trackId);

    const char* GetTrackType(MP4TrackId trackId);

    const char *GetTrackMediaDataName(MP4TrackId trackId);
    bool GetTrackMediaDataOriginalFormat(MP4TrackId trackId,
                                         char *originalFormat, uint32_t buflen);
    MP4Duration GetTrackDuration(MP4TrackId trackId);

    uint32_t GetTrackTimeScale(MP4TrackId trackId);
    void SetTrackTimeScale(MP4TrackId trackId, uint32_t value);

    // replacement to GetTrackAudioType and GetTrackVideoType
    uint8_t GetTrackEsdsObjectTypeId(MP4TrackId trackId);

    uint8_t GetTrackAudioMpeg4Type(MP4TrackId trackId);

    MP4Duration GetTrackFixedSampleDuration(MP4TrackId trackId);

    double GetTrackVideoFrameRate(MP4TrackId trackId);

    int GetTrackAudioChannels(MP4TrackId trackId);
    void GetTrackESConfiguration(MP4TrackId trackId,
                                 uint8_t** ppConfig, uint32_t* pConfigSize);
    void SetTrackESConfiguration(MP4TrackId trackId,
                                 const uint8_t* pConfig, uint32_t configSize);

    void GetTrackVideoMetadata(MP4TrackId trackId,
                               uint8_t** ppConfig, uint32_t* pConfigSize);
    void GetTrackH264SeqPictHeaders(MP4TrackId trackId,
                                    uint8_t ***pSeqHeader,
                                    uint32_t **pSeqHeaderSize,
                                    uint8_t ***pPictHeader,
                                    uint32_t **pPictHeaderSize);
    const char* GetHintTrackSdp(MP4TrackId hintTrackId);
    void SetHintTrackSdp(MP4TrackId hintTrackId, const char* sdpString);
    void AppendHintTrackSdp(MP4TrackId hintTrackId, const char* sdpString);

    void MakeFtypAtom(
        char*    majorBrand,
        uint32_t minorVersion,
        char**   compatibleBrands,
        uint32_t compatibleBrandsCount );

    // 3GPP specific functions
    void Make3GPCompliant(const char* fileName,
                          char* majorBrand,
                          uint32_t minorVersion,
                          char** supportedBrands,
                          uint32_t supportedBrandsCount,
                          bool deleteIodsAtom);

    // ISMA specific functions

    // true if media track encrypted according to ismacryp
    bool IsIsmaCrypMediaTrack(MP4TrackId trackId);

    void MakeIsmaCompliant(bool addIsmaComplianceSdp = true);

    void CreateIsmaIodFromParams(
        uint8_t videoProfile,
        uint32_t videoBitrate,
        uint8_t* videoConfig,
        uint32_t videoConfigLength,
        uint8_t audioProfile,
        uint32_t audioBitrate,
        uint8_t* audioConfig,
        uint32_t audioConfigLength,
        uint8_t** ppBytes,
        uint64_t* pNumBytes);

    // time convenience functions

    uint64_t ConvertFromMovieDuration(
        MP4Duration duration,
        uint32_t timeScale);

    uint64_t ConvertFromTrackTimestamp(
        MP4TrackId trackId,
        MP4Timestamp timeStamp,
        uint32_t timeScale);

    MP4Timestamp ConvertToTrackTimestamp(
        MP4TrackId trackId,
        uint64_t timeStamp,
        uint32_t timeScale);

    uint64_t ConvertFromTrackDuration(
        MP4TrackId trackId,
        MP4Duration duration,
        uint32_t timeScale);

    MP4Duration ConvertToTrackDuration(
        MP4TrackId trackId,
        uint64_t duration,
        uint32_t timeScale);

    // specialized operations

    void GetHintTrackRtpPayload(
        MP4TrackId hintTrackId,
        char** ppPayloadName = NULL,
        uint8_t* pPayloadNumber = NULL,
        uint16_t* pMaxPayloadSize = NULL,
        char **ppEncodingParams = NULL);

    void SetHintTrackRtpPayload(
        MP4TrackId hintTrackId,
        const char* payloadName,
        uint8_t* pPayloadNumber,
        uint16_t maxPayloadSize,
        const char *encoding_params,
        bool include_rtp_map,
        bool include_mpeg4_esid);

    MP4TrackId GetHintTrackReferenceTrackId(
        MP4TrackId hintTrackId);

    void ReadRtpHint(
        MP4TrackId hintTrackId,
        MP4SampleId hintSampleId,
        uint16_t* pNumPackets = NULL);

    uint16_t GetRtpHintNumberOfPackets(
        MP4TrackId hintTrackId);

    int8_t GetRtpPacketBFrame(
        MP4TrackId hintTrackId,
        uint16_t packetIndex);

    int32_t GetRtpPacketTransmitOffset(
        MP4TrackId hintTrackId,
        uint16_t packetIndex);

    void ReadRtpPacket(
        MP4TrackId hintTrackId,
        uint16_t packetIndex,
        uint8_t** ppBytes,
        uint32_t* pNumBytes,
        uint32_t ssrc = 0,
        bool includeHeader = true,
        bool includePayload = true);

    MP4Timestamp GetRtpTimestampStart(
        MP4TrackId hintTrackId);

    void SetRtpTimestampStart(
        MP4TrackId hintTrackId,
        MP4Timestamp rtpStart);

    void AddRtpHint(
        MP4TrackId hintTrackId,
        bool isBframe,
        uint32_t timestampOffset);

    void AddRtpPacket(
        MP4TrackId hintTrackId,
        bool setMbit,
        int32_t transmitOffset);

    void AddRtpImmediateData(
        MP4TrackId hintTrackId,
        const uint8_t* pBytes,
        uint32_t numBytes);

    void AddRtpSampleData(
        MP4TrackId hintTrackId,
        MP4SampleId sampleId,
        uint32_t dataOffset,
        uint32_t dataLength);

    void AddRtpESConfigurationPacket(
        MP4TrackId hintTrackId);

    void WriteRtpHint(
        MP4TrackId hintTrackId,
        MP4Duration duration,
        bool isSyncSample);

    uint8_t AllocRtpPayloadNumber();

    // edit list related

    char* MakeTrackEditName(
        MP4TrackId trackId,
        MP4EditId editId,
        const char* name);

    MP4EditId AddTrackEdit(
        MP4TrackId trackId,
        MP4EditId editId = MP4_INVALID_EDIT_ID);

    void DeleteTrackEdit(
        MP4TrackId trackId,
        MP4EditId editId);

    uint32_t GetTrackNumberOfEdits(
        MP4TrackId trackId);

    MP4Timestamp GetTrackEditStart(
        MP4TrackId trackId,
        MP4EditId editId);

    MP4Duration GetTrackEditTotalDuration(
        MP4TrackId trackId,
        MP4EditId editId);

    MP4Timestamp GetTrackEditMediaStart(
        MP4TrackId trackId,
        MP4EditId editId);

    void SetTrackEditMediaStart(
        MP4TrackId trackId,
        MP4EditId editId,
        MP4Timestamp startTime);

    MP4Duration GetTrackEditDuration(
        MP4TrackId trackId,
        MP4EditId editId);

    void SetTrackEditDuration(
        MP4TrackId trackId,
        MP4EditId editId,
        MP4Duration duration);

    bool GetTrackEditDwell(
        MP4TrackId trackId,
        MP4EditId editId);

    void SetTrackEditDwell(
        MP4TrackId trackId,
        MP4EditId editId,
        bool dwell);

    MP4SampleId GetSampleIdFromEditTime(
        MP4TrackId trackId,
        MP4Timestamp when,
        MP4Timestamp* pStartTime = NULL,
        MP4Duration* pDuration = NULL);

    /* "protected" interface to be used only by friends in library */

    uint64_t GetPosition( File* file = NULL );
    void SetPosition( uint64_t pos, File* file = NULL );
    uint64_t GetSize( File* file = NULL );

    void ReadBytes( uint8_t* buf, uint32_t bufsiz, File* file = NULL );
    void PeekBytes( uint8_t* buf, uint32_t bufsiz, File* file = NULL );

    uint64_t ReadUInt(uint8_t size);
    uint8_t ReadUInt8();
    uint16_t ReadUInt16();
    uint32_t ReadUInt24();
    uint32_t ReadUInt32();
    uint64_t ReadUInt64();
    float ReadFixed16();
    float ReadFixed32();
    float ReadFloat();
    char* ReadString();
    char* ReadCountedString(
        uint8_t charSize = 1, bool allowExpandedCount = false, uint8_t fixedLength = 0);
    uint64_t ReadBits(uint8_t numBits);
    void FlushReadBits();
    uint32_t ReadMpegLength();


    void WriteBytes( uint8_t* buf, uint32_t bufsiz, File* file = NULL );
    void WriteUInt8(uint8_t value);
    void WriteUInt16(uint16_t value);
    void WriteUInt24(uint32_t value);
    void WriteUInt32(uint32_t value);
    void WriteUInt64(uint64_t value);
    void WriteFixed16(float value);
    void WriteFixed32(float value);
    void WriteFloat(float value);
    void WriteString(char* string);
    void WriteCountedString(char* string,
                            uint8_t charSize = 1,
                            bool allowExpandedCount = false,
                            uint32_t fixedLength = 0);
    void WriteBits(uint64_t bits, uint8_t numBits);
    void PadWriteBits(uint8_t pad = 0);
    void FlushWriteBits();
    void WriteMpegLength(uint32_t value, bool compact = false);

    void EnableMemoryBuffer(
        uint8_t* pBytes = NULL, uint64_t numBytes = 0);
    void DisableMemoryBuffer(
        uint8_t** ppBytes = NULL, uint64_t* pNumBytes = NULL);

    bool IsWriteMode();

    MP4Track* GetTrack(MP4TrackId trackId);

    void UpdateDuration(MP4Duration duration);

    MP4Atom* FindAtom(const char* name);

    MP4Atom* AddChildAtom(
        const char* parentName,
        const char* childName);

    MP4Atom* AddChildAtom(
        MP4Atom* pParentAtom,
        const char* childName);

    MP4Atom* InsertChildAtom(
        const char* parentName,
        const char* childName,
        uint32_t index);

    MP4Atom* InsertChildAtom(
        MP4Atom* pParentAtom,
        const char* childName,
        uint32_t index);

    MP4Atom* AddDescendantAtoms(
        const char* ancestorName,
        const char* childName);

    MP4Atom* AddDescendantAtoms(
        MP4Atom* pAncestorAtom,
        const char* childName);

protected:
    void Init();
    void Open( const char* name, File::Mode mode, const MP4FileProvider* provider );
    void ReadFromFile();
    void GenerateTracks();
    void BeginWrite();
    void FinishWrite(uint32_t options);
    void CacheProperties();
    void RewriteMdat( File& src, File& dst );
    bool ShallHaveIods();

    void Rename(const char* existingFileName, const char* newFileName);

    void ProtectWriteOperation(const char* file, int line, const char *func);

    void FindIntegerProperty(const char* name,
                             MP4Property** ppProperty, uint32_t* pIndex = NULL);
    void FindFloatProperty(const char* name,
                           MP4Property** ppProperty, uint32_t* pIndex = NULL);
    void FindStringProperty(const char* name,
                            MP4Property** ppProperty, uint32_t* pIndex = NULL);
    void FindBytesProperty(const char* name,
                           MP4Property** ppProperty, uint32_t* pIndex = NULL);

    bool FindProperty(const char* name,
                      MP4Property** ppProperty, uint32_t* pIndex = NULL);

    MP4TrackId AddVideoTrackDefault(
        uint32_t timeScale,
        MP4Duration sampleDuration,
        uint16_t width,
        uint16_t height,
        const char *videoType);
    MP4TrackId AddCntlTrackDefault(
        uint32_t timeScale,
        MP4Duration sampleDuration,
        const char *videoType);
    void AddTrackToIod(MP4TrackId trackId);

    void RemoveTrackFromIod(MP4TrackId trackId, bool shallHaveIods = true);

    void AddTrackToOd(MP4TrackId trackId);

    void RemoveTrackFromOd(MP4TrackId trackId);

    void GetTrackReferenceProperties(const char* trefName,
                                     MP4Property** ppCountProperty, MP4Property** ppTrackIdProperty);

    void AddTrackReference(const char* trefName, MP4TrackId refTrackId);

    uint32_t FindTrackReference(const char* trefName, MP4TrackId refTrackId);

    void RemoveTrackReference(const char* trefName, MP4TrackId refTrackId);

    void AddDataReference(MP4TrackId trackId, const char* url);

    char* MakeTrackName(MP4TrackId trackId, const char* name);

    uint8_t ConvertTrackTypeToStreamType(const char* trackType);

    void CreateIsmaIodFromFile(
        MP4TrackId odTrackId,
        MP4TrackId sceneTrackId,
        MP4TrackId audioTrackId,
        MP4TrackId videoTrackId,
        uint8_t** ppBytes,
        uint64_t* pNumBytes);

    void CreateESD(
        MP4DescriptorProperty* pEsProperty,
        uint32_t esid,
        uint8_t objectType,
        uint8_t streamType,
        uint32_t bufferSize,
        uint32_t bitrate,
        const uint8_t* pConfig,
        uint32_t configLength,
        char* url);

    void CreateIsmaODUpdateCommandFromFileForFile(
        MP4TrackId odTrackId,
        MP4TrackId audioTrackId,
        MP4TrackId videoTrackId,
        uint8_t** ppBytes,
        uint64_t* pNumBytes);

    void CreateIsmaODUpdateCommandFromFileForStream(
        MP4TrackId audioTrackId,
        MP4TrackId videoTrackId,
        uint8_t** ppBytes,
        uint64_t* pNumBytes);

    void CreateIsmaODUpdateCommandForStream(
        MP4DescriptorProperty* pAudioEsdProperty,
        MP4DescriptorProperty* pVideoEsdProperty,
        uint8_t** ppBytes,
        uint64_t* pNumBytes);

    void CreateIsmaSceneCommand(
        bool hasAudio,
        bool hasVideo,
        uint8_t** ppBytes,
        uint64_t* pNumBytes);

protected:
    File*    m_file;
    uint64_t m_fileOriginalSize;
    uint32_t m_createFlags;

    MP4Atom*          m_pRootAtom;
    MP4Integer32Array m_trakIds;
    MP4TrackArray     m_pTracks;
    MP4TrackId        m_odTrackId;
    bool              m_useIsma;

    // cached properties
    MP4IntegerProperty*     m_pModificationProperty;
    MP4Integer32Property*   m_pTimeScaleProperty;
    MP4IntegerProperty*     m_pDurationProperty;

    // read/write in memory
    uint8_t*    m_memoryBuffer;
    uint64_t    m_memoryBufferPosition;
    uint64_t    m_memoryBufferSize;

    // bit read/write buffering
    uint8_t m_numReadBits;
    uint8_t m_bufReadBits;
    uint8_t m_numWriteBits;
    uint8_t m_bufWriteBits;

    char m_trakName[1024];
    char *m_editName;

 private:
    MP4File ( const MP4File &src );
    MP4File &operator= ( const MP4File &src );
};

///////////////////////////////////////////////////////////////////////////////

}} // namespace mp4v2::impl

#endif // MP4V2_IMPL_MP4FILE_H
