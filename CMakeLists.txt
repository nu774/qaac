cmake_minimum_required(VERSION 3.16)

# CMP0091 lets CMAKE_MSVC_RUNTIME_LIBRARY control /MT vs /MD (see below).
cmake_policy(SET CMP0091 NEW)

project(qaac LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)

if(WIN32)
    if(MSVC)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:DebugDLL>")
    elseif(NOT MINGW)
        add_compile_options(
            $<$<CONFIG:Debug>:-fms-runtime-lib=dll_dbg>
            $<$<NOT:$<CONFIG:Debug>>:-fms-runtime-lib=dll>
        )
    endif()
    add_compile_definitions(UNICODE _UNICODE)
endif()

if(NOT MSVC)
    add_compile_options(-Wno-multichar)
endif()

if(MSVC)
    add_compile_options($<$<CONFIG:Release>:/Zi>)
    add_link_options($<$<CONFIG:Release>:/DEBUG>)
elseif(WIN32)
    add_compile_options($<$<CONFIG:Release>:-g>)
    add_link_options($<$<CONFIG:Release>:-Wl,/DEBUG>)
else()
    add_compile_options($<$<CONFIG:Release>:-g>)
endif()


if(NOT WIN32)
    find_package(Iconv REQUIRED)
endif()

# ---------------------------------------------------------------------------
# alac
# ---------------------------------------------------------------------------
add_library(alac STATIC
    ALAC/ag_dec.c
    ALAC/ag_enc.c
    ALAC/ALACBitUtilities.c
    ALAC/ALACDecoder.cpp
    ALAC/ALACEncoder.cpp
    ALAC/dp_dec.c
    ALAC/dp_enc.c
    ALAC/EndianPortable.c
    ALAC/matrix_dec.c
    ALAC/matrix_enc.c
)
target_compile_definitions(alac PRIVATE TARGET_RT_LITTLE_ENDIAN)
if(WIN32)
    target_compile_definitions(alac PRIVATE TARGET_OS_WIN32)
endif()
if(MSVC)
    target_compile_definitions(alac PRIVATE inline=_inline)
    target_compile_options(alac PRIVATE /W2 /wd4146 /wd4244 /wd4805)
endif()

# ---------------------------------------------------------------------------
# ogg
# ---------------------------------------------------------------------------
include(CheckIncludeFiles)
check_include_files(inttypes.h INCLUDE_INTTYPES_H)
check_include_files(stdint.h INCLUDE_STDINT_H)
check_include_files(sys/types.h INCLUDE_SYS_TYPES_H)
set(SIZE16 int16_t)
set(USIZE16 uint16_t)
set(SIZE32 int32_t)
set(USIZE32 uint32_t)
set(SIZE64 int64_t)
set(USIZE64 uint64_t)
configure_file(
    ogg/include/ogg/config_types.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/ogg_config/ogg/config_types.h
    @ONLY
)

add_library(ogg STATIC
    ogg/src/bitwise.c
    ogg/src/framing.c
)
target_include_directories(ogg PUBLIC ogg/include ${CMAKE_CURRENT_BINARY_DIR}/ogg_config)

# ---------------------------------------------------------------------------
# mp4v2
# ---------------------------------------------------------------------------
add_library(mp4v2 STATIC
    mp4v2/src/3gp.cpp
    mp4v2/src/atom_ac3.cpp
    mp4v2/src/atom_amr.cpp
    mp4v2/src/atom_avc1.cpp
    mp4v2/src/atom_avcC.cpp
    mp4v2/src/atom_chpl.cpp
    mp4v2/src/atom_colr.cpp
    mp4v2/src/atom_d263.cpp
    mp4v2/src/atom_dac3.cpp
    mp4v2/src/atom_damr.cpp
    mp4v2/src/atom_dref.cpp
    mp4v2/src/atom_elst.cpp
    mp4v2/src/atom_enca.cpp
    mp4v2/src/atom_encv.cpp
    mp4v2/src/atom_free.cpp
    mp4v2/src/atom_ftab.cpp
    mp4v2/src/atom_ftyp.cpp
    mp4v2/src/atom_gmin.cpp
    mp4v2/src/atom_hdlr.cpp
    mp4v2/src/atom_hinf.cpp
    mp4v2/src/atom_hnti.cpp
    mp4v2/src/atom_href.cpp
    mp4v2/src/atom_mdat.cpp
    mp4v2/src/atom_mdhd.cpp
    mp4v2/src/atom_meta.cpp
    mp4v2/src/atom_mp4s.cpp
    mp4v2/src/atom_mp4v.cpp
    mp4v2/src/atom_mvhd.cpp
    mp4v2/src/atom_nmhd.cpp
    mp4v2/src/atom_ohdr.cpp
    mp4v2/src/atom_pasp.cpp
    mp4v2/src/atom_root.cpp
    mp4v2/src/atom_rtp.cpp
    mp4v2/src/atom_s263.cpp
    mp4v2/src/atom_sdp.cpp
    mp4v2/src/atom_sdtp.cpp
    mp4v2/src/atom_smi.cpp
    mp4v2/src/atom_sound.cpp
    mp4v2/src/atom_standard.cpp
    mp4v2/src/atom_stbl.cpp
    mp4v2/src/atom_stdp.cpp
    mp4v2/src/atom_stsc.cpp
    mp4v2/src/atom_stsd.cpp
    mp4v2/src/atom_stsz.cpp
    mp4v2/src/atom_stz2.cpp
    mp4v2/src/atom_text.cpp
    mp4v2/src/atom_tfhd.cpp
    mp4v2/src/atom_tkhd.cpp
    mp4v2/src/atom_treftype.cpp
    mp4v2/src/atom_trun.cpp
    mp4v2/src/atom_tx3g.cpp
    mp4v2/src/atom_udta.cpp
    mp4v2/src/atom_url.cpp
    mp4v2/src/atom_urn.cpp
    mp4v2/src/atom_uuid.cpp
    mp4v2/src/atom_video.cpp
    mp4v2/src/atom_vmhd.cpp
    mp4v2/src/cmeta.cpp
    mp4v2/src/log.cpp
    mp4v2/src/mp4.cpp
    mp4v2/src/qtff/coding.cpp
    mp4v2/src/qtff/ColorParameterBox.cpp
    mp4v2/src/itmf/CoverArtBox.cpp
    mp4v2/src/descriptors.cpp
    mp4v2/libplatform/sys/error.cpp
    mp4v2/src/exception.cpp
    mp4v2/libplatform/io/File.cpp
    mp4v2/libplatform/io/FileSystem.cpp
    mp4v2/src/itmf/generic.cpp
    mp4v2/src/isma.cpp
    mp4v2/src/mp4atom.cpp
    mp4v2/src/mp4container.cpp
    mp4v2/src/mp4descriptor.cpp
    mp4v2/src/mp4file.cpp
    mp4v2/src/mp4file_io.cpp
    mp4v2/src/mp4info.cpp
    mp4v2/src/mp4property.cpp
    mp4v2/src/mp4track.cpp
    mp4v2/src/mp4util.cpp
    mp4v2/src/ocidescriptors.cpp
    mp4v2/src/odcommands.cpp
    mp4v2/libplatform/prog/option.cpp
    mp4v2/src/qtff/PictureAspectRatioBox.cpp
    mp4v2/src/qosqualifiers.cpp
    mp4v2/src/rtphint.cpp
    mp4v2/src/itmf/Tags.cpp
    mp4v2/src/text.cpp
    mp4v2/libplatform/time/time.cpp
    mp4v2/src/itmf/type.cpp
    mp4v2/src/bmff/typebmff.cpp
)
if(WIN32)
    target_sources(mp4v2 PRIVATE mp4v2/libplatform/platform_win32.cpp)
    target_sources(mp4v2 PRIVATE
        mp4v2/libplatform/io/File_win32.cpp
        mp4v2/libplatform/io/FileSystem_win32.cpp
        mp4v2/libplatform/number/random_win32.cpp
        mp4v2/libplatform/time/time_win32.cpp
    )
else()
    target_sources(mp4v2 PRIVATE
        mp4v2/libplatform/io/File_posix.cpp
        mp4v2/libplatform/io/FileSystem_posix.cpp
        mp4v2/libplatform/number/random_posix.cpp
        mp4v2/libplatform/time/time_posix.cpp
    )
endif()
target_include_directories(mp4v2 PUBLIC mp4v2 mp4v2/include)
target_compile_definitions(mp4v2 PUBLIC MP4V2_USE_STATIC_LIB MP4V2_NO_STDINT_DEFS)
target_compile_definitions(mp4v2 PRIVATE _CRT_SECURE_NO_DEPRECATE)
if(MSVC)
    target_compile_options(mp4v2 PRIVATE /wd4018 /wd4267)
endif()
if(MSVC AND MSVC_VERSION LESS 1900)
    target_compile_definitions(mp4v2 PRIVATE snprintf=_snprintf)
endif()

# ---------------------------------------------------------------------------
# taglib
# ---------------------------------------------------------------------------
add_library(taglib STATIC
    taglib/ape/apefile.cpp
    taglib/ape/apefooter.cpp
    taglib/ape/apeitem.cpp
    taglib/ape/apeproperties.cpp
    taglib/ape/apetag.cpp
    taglib/audioproperties.cpp
    taglib/flac/flacfile.cpp
    taglib/flac/flacmetadatablock.cpp
    taglib/flac/flacpicture.cpp
    taglib/flac/flacproperties.cpp
    taglib/flac/flacunknownmetadatablock.cpp
    taglib/mpeg/id3v1/id3v1genres.cpp
    taglib/mpeg/id3v1/id3v1tag.cpp
    taglib/mpeg/id3v2/frames/attachedpictureframe.cpp
    taglib/mpeg/id3v2/frames/chapterframe.cpp
    taglib/mpeg/id3v2/frames/commentsframe.cpp
    taglib/mpeg/id3v2/frames/eventtimingcodesframe.cpp
    taglib/mpeg/id3v2/frames/generalencapsulatedobjectframe.cpp
    taglib/mpeg/id3v2/frames/ownershipframe.cpp
    taglib/mpeg/id3v2/frames/podcastframe.cpp
    taglib/mpeg/id3v2/frames/popularimeterframe.cpp
    taglib/mpeg/id3v2/frames/privateframe.cpp
    taglib/mpeg/id3v2/frames/relativevolumeframe.cpp
    taglib/mpeg/id3v2/frames/synchronizedlyricsframe.cpp
    taglib/mpeg/id3v2/frames/tableofcontentsframe.cpp
    taglib/mpeg/id3v2/frames/textidentificationframe.cpp
    taglib/mpeg/id3v2/frames/uniquefileidentifierframe.cpp
    taglib/mpeg/id3v2/frames/unknownframe.cpp
    taglib/mpeg/id3v2/frames/unsynchronizedlyricsframe.cpp
    taglib/mpeg/id3v2/frames/urllinkframe.cpp
    taglib/mpeg/id3v2/id3v2extendedheader.cpp
    taglib/mpeg/id3v2/id3v2footer.cpp
    taglib/mpeg/id3v2/id3v2frame.cpp
    taglib/mpeg/id3v2/id3v2framefactory.cpp
    taglib/mpeg/id3v2/id3v2header.cpp
    taglib/mpeg/id3v2/id3v2synchdata.cpp
    taglib/mpeg/id3v2/id3v2tag.cpp
    taglib/mpeg/mpegfile.cpp
    taglib/mpeg/mpegheader.cpp
    taglib/mpeg/mpegproperties.cpp
    taglib/mpeg/xingheader.cpp
    taglib/ogg/oggfile.cpp
    taglib/ogg/oggpage.cpp
    taglib/ogg/oggpageheader.cpp
    taglib/ogg/opus/opusfile.cpp
    taglib/ogg/opus/opusproperties.cpp
    taglib/ogg/xiphcomment.cpp
    taglib/ogg/flac/oggflacfile.cpp
    taglib/ogg/vorbis/vorbisfile.cpp
    taglib/ogg/vorbis/vorbisproperties.cpp
    taglib/riff/aiff/aifffile.cpp
    taglib/riff/aiff/aiffproperties.cpp
    taglib/riff/rifffile.cpp
    taglib/tag.cpp
    taglib/tagunion.cpp
    taglib/tagutils.cpp
    taglib/toolkit/tbytevector.cpp
    taglib/toolkit/tbytevectorlist.cpp
    taglib/toolkit/tdebug.cpp
    taglib/toolkit/tdebuglistener.cpp
    taglib/toolkit/tfile.cpp
    taglib/toolkit/tfilestream.cpp
    taglib/toolkit/tiostream.cpp
    taglib/toolkit/tpropertymap.cpp
    taglib/toolkit/trefcounter.cpp
    taglib/toolkit/tstring.cpp
    taglib/toolkit/tstringlist.cpp
    taglib/toolkit/tzlib.cpp
)
target_include_directories(taglib PUBLIC
    taglib
    taglib/toolkit
    taglib/ape
    taglib/mpeg
    taglib/mpeg/id3v1
    taglib/mpeg/id3v2
    taglib/riff
    taglib/riff/aiff
    taglib/flac
    taglib/ogg
    taglib/ogg/flac
    taglib/ogg/vorbis
    taglib/ogg/opus
)
target_compile_definitions(taglib PUBLIC TAGLIB_STATIC)
target_compile_definitions(taglib PRIVATE _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE)
if(MSVC)
    target_compile_options(taglib PRIVATE /wd4244 /wd4267)
endif()

# ---------------------------------------------------------------------------
# uchardet
# ---------------------------------------------------------------------------
add_library(uchardet STATIC
    uchardet/CharDistribution.cpp
    uchardet/JpCntx.cpp
    uchardet/LangModels/LangArabicModel.cpp
    uchardet/LangModels/LangBelarusianModel.cpp
    uchardet/LangModels/LangBulgarianModel.cpp
    uchardet/LangModels/LangCatalanModel.cpp
    uchardet/LangModels/LangCroatianModel.cpp
    uchardet/LangModels/LangCzechModel.cpp
    uchardet/LangModels/LangDanishModel.cpp
    uchardet/LangModels/LangEnglishModel.cpp
    uchardet/LangModels/LangEsperantoModel.cpp
    uchardet/LangModels/LangEstonianModel.cpp
    uchardet/LangModels/LangFinnishModel.cpp
    uchardet/LangModels/LangFrenchModel.cpp
    uchardet/LangModels/LangGeorgianModel.cpp
    uchardet/LangModels/LangGermanModel.cpp
    uchardet/LangModels/LangGreekModel.cpp
    uchardet/LangModels/LangHebrewModel.cpp
    uchardet/LangModels/LangHindiModel.cpp
    uchardet/LangModels/LangHungarianModel.cpp
    uchardet/LangModels/LangIrishModel.cpp
    uchardet/LangModels/LangItalianModel.cpp
    uchardet/LangModels/LangLatvianModel.cpp
    uchardet/LangModels/LangLithuanianModel.cpp
    uchardet/LangModels/LangMacedonianModel.cpp
    uchardet/LangModels/LangMalteseModel.cpp
    uchardet/LangModels/LangNorwegianModel.cpp
    uchardet/LangModels/LangPolishModel.cpp
    uchardet/LangModels/LangPortugueseModel.cpp
    uchardet/LangModels/LangRomanianModel.cpp
    uchardet/LangModels/LangRussianModel.cpp
    uchardet/LangModels/LangSerbianModel.cpp
    uchardet/LangModels/LangSlovakModel.cpp
    uchardet/LangModels/LangSloveneModel.cpp
    uchardet/LangModels/LangSpanishModel.cpp
    uchardet/LangModels/LangSwedishModel.cpp
    uchardet/LangModels/LangThaiModel.cpp
    uchardet/LangModels/LangTurkishModel.cpp
    uchardet/LangModels/LangUkrainianModel.cpp
    uchardet/LangModels/LangVietnameseModel.cpp
    uchardet/nsBig5Prober.cpp
    uchardet/nsCharSetProber.cpp
    uchardet/nsCJKDetector.cpp
    uchardet/nsEscCharsetProber.cpp
    uchardet/nsEscSM.cpp
    uchardet/nsEUCJPProber.cpp
    uchardet/nsEUCKRProber.cpp
    uchardet/nsEUCTWProber.cpp
    uchardet/nsGB2312Prober.cpp
    uchardet/nsHebrewProber.cpp
    uchardet/nsJohabProber.cpp
    uchardet/nsLanguageDetector.cpp
    uchardet/nsLatin1Prober.cpp
    uchardet/nsMBCSGroupProber.cpp
    uchardet/nsMBCSSM.cpp
    uchardet/nsSBCharSetProber.cpp
    uchardet/nsSBCSGroupProber.cpp
    uchardet/nsSJISProber.cpp
    uchardet/nsUniversalDetector.cpp
    uchardet/nsUTF8Prober.cpp
    uchardet/uchardet.cpp
)
target_compile_definitions(uchardet PRIVATE _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE)

# ---------------------------------------------------------------------------
# libsoundio (vendored subset: WASAPI + ALSA + CoreAudio + dummy backends
# only -- see libsoundio/README.md for what was left out and why)
# ---------------------------------------------------------------------------
add_library(libsoundio STATIC
    libsoundio/src/soundio.c
    libsoundio/src/util.c
    libsoundio/src/os.c
    libsoundio/src/dummy.c
    libsoundio/src/channel_layout.c
    libsoundio/src/ring_buffer.c
)
if(WIN32)
    target_sources(libsoundio PRIVATE libsoundio/src/wasapi.c)
    set(SOUNDIO_HAVE_WASAPI true)
endif()
if(NOT WIN32 AND NOT APPLE)
    find_package(ALSA)
    if(ALSA_FOUND)
        target_sources(libsoundio PRIVATE libsoundio/src/alsa.c)
        set(SOUNDIO_HAVE_ALSA true)
        target_include_directories(libsoundio PRIVATE ${ALSA_INCLUDE_DIRS})
        target_link_libraries(libsoundio PUBLIC ${ALSA_LIBRARIES})
    else()
        message(WARNING "ALSA development headers not found -- libsoundio "
                        "will only have the dummy backend on this platform, "
                        "so --play won't produce audible output")
    endif()
endif()
if(APPLE)
    target_sources(libsoundio PRIVATE libsoundio/src/coreaudio.c)
    set(SOUNDIO_HAVE_COREAUDIO true)
    target_link_libraries(libsoundio PUBLIC
        "-framework CoreAudio"
        "-framework AudioUnit"
        "-framework AudioToolbox"
        "-framework CoreFoundation"
    )
endif()
set(LIBSOUNDIO_VERSION_MAJOR 2)
set(LIBSOUNDIO_VERSION_MINOR 0)
set(LIBSOUNDIO_VERSION_PATCH 0)
set(LIBSOUNDIO_VERSION "${LIBSOUNDIO_VERSION_MAJOR}.${LIBSOUNDIO_VERSION_MINOR}.${LIBSOUNDIO_VERSION_PATCH}")
configure_file(
    libsoundio/src/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/libsoundio_config/config.h
)
target_include_directories(libsoundio PUBLIC libsoundio)
target_include_directories(libsoundio PRIVATE
    libsoundio/src
    ${CMAKE_CURRENT_BINARY_DIR}/libsoundio_config
)
if(MSVC)
    target_compile_options(libsoundio PRIVATE /TP)
endif()
if(WIN32)
    target_link_libraries(libsoundio PUBLIC ole32)
    target_compile_definitions(libsoundio PUBLIC SOUNDIO_STATIC_LIBRARY)
    target_compile_definitions(libsoundio PRIVATE _CRT_NONSTDC_NO_DEPRECATE)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(libsoundio PUBLIC Threads::Threads)
endif()

# ---------------------------------------------------------------------------
# common (shared code linked into both qaac and refalac)
# ---------------------------------------------------------------------------
add_library(common STATIC
    input/CAFFile.cpp
    input/FLACModule.cpp
    input/FLACPacketDecoder.cpp
    input/FLACSource.cpp
    input/LibSndfileSource.cpp
    input/LPCMPacketDecoder.cpp
    input/MP4Edits.cpp
    input/OggIndex.cpp
    input/OggSource.cpp
    input/OpusPacketDecoder.cpp
    input/RawSource.cpp
    input/VorbisPacketDecoder.cpp
    input/WaveSource.cpp
    input/WavpackSource.cpp
    filters/ChannelMapper.cpp
    filters/Compressor.cpp
    filters/Limiter.cpp
    filters/MatrixMixer.cpp
    filters/Normalizer.cpp
    filters/PipedReader.cpp
    filters/Quantizer.cpp
    filters/SoXConvolverModule.cpp
    filters/SoxLowpassFilter.cpp
    filters/SOXRModule.cpp
    filters/SoxrResampler.cpp
    output/CAFSink.cpp
    output/sink.cpp
    output/WaveSink.cpp
    bitstream.cpp
    chanmap.cpp
    CompositeSource.cpp
    cuesheet.cpp
    ISource.cpp
    metadata.cpp
    misc.cpp
    mp4v2wrapper.cpp
    Subprocess.cpp
    strutil.cpp
    util.cpp
    SeekableInputStream.cpp
    platformutil.cpp
)
if(WIN32)
    target_sources(common PRIVATE
        wicimage.cpp
        output/WaveOutSink.cpp
        input/AvisynthSource.cpp
        input/TakSource.cpp
        win32/getopt.cpp
    )
endif()
target_sources(common PRIVATE
    output/ConsoleInputParser.cpp
    output/PlaybackNotifier.cpp
    output/SoundIoOutDevice.cpp
)
target_include_directories(common PRIVATE
    .
    input
    output
    filters
    include
    ALAC
    uchardet
    include/opus
)
if(NOT APPLE)
    target_include_directories(common PRIVATE CoreAudio)
endif()

target_compile_definitions(common PRIVATE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)
if(MSVC)
    target_compile_options(common PRIVATE /wd4018 /wd4244 /wd4267 /wd4838)
endif()
target_link_libraries(common PUBLIC mp4v2 taglib uchardet ogg)
target_link_libraries(common PUBLIC libsoundio)

# ---------------------------------------------------------------------------
# qaac
# ---------------------------------------------------------------------------
if(WIN32 OR APPLE)
add_executable(qaac
    input/CAFSource.cpp
    input/CoreAudioPacketDecoder.cpp
    input/ExtAFSource.cpp
    input/MP4Source.cpp
    input/MPAHeader.cpp
    input/InputFactory.cpp
    filters/CoreAudioResampler.cpp
    AudioConverterX.cpp
    AudioConverterXX.cpp
    ascutil.cpp
    cautil.cpp
    CoreAudioEncoder.cpp
    CoreAudioPaddedEncoder.cpp
    lpc.c
    main.cpp
    options.cpp
    version.cpp
    windows7.manifest
)
if(MINGW)
    target_sources(qaac PRIVATE windows7.rc)
endif()
target_include_directories(qaac PRIVATE
    .
    input
    output
    filters
    include
    ALAC
    include/opus
)
if(NOT APPLE)
    target_include_directories(qaac PRIVATE CoreAudio)
endif()
target_compile_definitions(qaac PRIVATE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE QAAC)
if(MSVC)
    target_compile_options(qaac PRIVATE /wd4018 /wd4091 /wd4244 /wd4267 /wd4838)
endif()
target_link_libraries(qaac PRIVATE common mp4v2 taglib)

if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(CORE_AUDIO_LIBS CoreAudioToolbox64.lib CoreFoundation64.lib)
        set_target_properties(qaac PROPERTIES OUTPUT_NAME qaac64)
    else()
        set(CORE_AUDIO_LIBS CoreAudioToolbox.lib CoreFoundation.lib)
    endif()

    target_link_directories(qaac PRIVATE ${CMAKE_SOURCE_DIR}/implib)
    target_link_libraries(qaac PRIVATE
        ${CORE_AUDIO_LIBS}
        delayimp.lib
        version.lib
        shlwapi.lib
        advapi32.lib
        winmm.lib
    )
elseif(APPLE)
    # Real system frameworks, linked directly -- unlike Windows, there's no
    # separately-installed/delay-loaded CoreAudioToolbox.dll to worry about.
    target_link_libraries(qaac PRIVATE
        "-framework AudioToolbox"
        "-framework CoreFoundation"
        "-framework CoreServices"
    )
    find_package(Threads REQUIRED)
    target_link_libraries(qaac PRIVATE ${CMAKE_DL_LIBS} Threads::Threads)
    target_link_libraries(qaac PRIVATE Iconv::Iconv)
endif()
if(MSVC)
    target_link_options(qaac PRIVATE
        /DELAYLOAD:CoreAudioToolbox.dll
        /DELAYLOAD:CoreFoundation.dll
        $<$<CONFIG:Release>:/OPT:REF>
        $<$<CONFIG:Release>:/OPT:ICF>
        # /PROFILE is a cl.exe/link.exe-only feature; lld-link (clang-cl's default
        # linker) may not support it, so skip it there.
        $<$<AND:$<CONFIG:Release>,$<NOT:$<CXX_COMPILER_ID:Clang>>>:/PROFILE>
        wsetargv.obj
    )
elseif(WIN32)
    # Same linker (lld-link) and same flags as the clang-cl branch above, but
    # routed through -Wl, since the plain GNU-style driver -- unlike
    # cl.exe/clang-cl -- doesn't forward bare /flags to the linker itself.
    # /PROFILE is skipped unconditionally here (see comment above; this
    # branch is never cl.exe). wsetargv.obj also needs -Wl,: as a bare
    # filename it would otherwise be resolved by the clang driver itself
    # (relative to the build directory, so "file not found") rather than by
    # lld-link, which is what actually searches the LIB env var for it.
    target_link_options(qaac PRIVATE
        -fuse-ld=lld
        -municode
        -Wl,/DELAYLOAD:CoreAudioToolbox.dll
        -Wl,/DELAYLOAD:CoreFoundation.dll
        $<$<CONFIG:Release>:-Wl,/OPT:REF>
        $<$<CONFIG:Release>:-Wl,/OPT:ICF>
    )
    if(MINGW)
	target_link_libraries(qaac PRIVATE ksguid.lib)
    else()
        target_link_options(qaac PRIVATE -Wl,wsetargv.obj)
    endif()
endif()
endif() # WIN32 (qaac)

# ---------------------------------------------------------------------------
# refalac
# ---------------------------------------------------------------------------
add_executable(refalac
    input/CAFSource.cpp
    input/ALACPacketDecoder.cpp
    input/InputFactory.cpp
    input/MP4Source.cpp
    ALACEncoderX.cpp
    ascutil.cpp
    cautil.cpp
    main.cpp
    options.cpp
    version.cpp
)
if(WIN32)
    target_sources(refalac PRIVATE windows7.manifest)
    if(MINGW)
        target_sources(refalac PRIVATE windows7.rc)
    endif()
endif()
target_include_directories(refalac PRIVATE
    .
    input
    output
    filters
    include
    ALAC
    include/opus
)
if(NOT APPLE)
    target_include_directories(refalac PRIVATE CoreAudio)
endif()
target_compile_definitions(refalac PRIVATE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE REFALAC)
if(MSVC)
    target_compile_options(refalac PRIVATE /wd4018 /wd4091 /wd4244 /wd4267 /wd4838)
endif()
target_link_libraries(refalac PRIVATE alac common mp4v2 taglib)
if(WIN32)
    target_link_libraries(refalac PRIVATE
        shlwapi.lib
        advapi32.lib
        winmm.lib
    )
else()
    find_package(Threads REQUIRED)
    target_link_libraries(refalac PRIVATE ${CMAKE_DL_LIBS} Threads::Threads)
    target_link_libraries(refalac PRIVATE Iconv::Iconv)
endif()

if(MSVC)
    target_link_options(refalac PRIVATE
        $<$<CONFIG:Release>:/OPT:REF>
        $<$<CONFIG:Release>:/OPT:ICF>
        $<$<AND:$<CONFIG:Release>,$<NOT:$<CXX_COMPILER_ID:Clang>>>:/PROFILE>
        wsetargv.obj
    )
elseif(WIN32)
    target_link_options(refalac PRIVATE
        -municode
        $<$<CONFIG:Release>:-Wl,/OPT:REF>
        $<$<CONFIG:Release>:-Wl,/OPT:ICF>
    )
    if (MINGW)
	target_link_libraries(refalac PRIVATE ksguid.lib)
    else()
        target_link_options(refalac PRIVATE -Wl,wsetargv.obj)
    endif()
endif()

if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    set_target_properties(refalac PROPERTIES OUTPUT_NAME refalac64)
endif()
