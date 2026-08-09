cmake_minimum_required(VERSION 3.16)

# CMP0091 lets CMAKE_MSVC_RUNTIME_LIBRARY control /MT vs /MD (see below).
cmake_policy(SET CMP0091 NEW)

if(NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET AND
   (CMAKE_HOST_APPLE OR CMAKE_SYSTEM_NAME STREQUAL "Darwin"))
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment target" FORCE)
endif()

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
# mmtisobmff
# ---------------------------------------------------------------------------
add_library(mmtisobmff STATIC
    mmtisobmff/src/ilo/async_fileio_not_supported.cpp
    mmtisobmff/src/ilo/bitbuffer.cpp
    mmtisobmff/src/ilo/bitparser.cpp
    mmtisobmff/src/ilo/bytebuffertools.cpp
    mmtisobmff/src/ilo/common_types.cpp
    mmtisobmff/src/ilo/fileio.cpp
    mmtisobmff/src/ilo/file_utils.cpp
    mmtisobmff/src/ilo/logging_backend.cpp
    mmtisobmff/src/ilo/string_utils.cpp
    mmtisobmff/src/ilo/uuid_utils.cpp
    mmtisobmff/src/mmtisobmff/box/ac3sampleentry.cpp
    mmtisobmff/src/mmtisobmff/box/alacsampleentry.cpp
    mmtisobmff/src/mmtisobmff/box/avcsampleentry.cpp
    mmtisobmff/src/mmtisobmff/box/box.cpp
    mmtisobmff/src/mmtisobmff/box/btrtbox.cpp
    mmtisobmff/src/mmtisobmff/box/co64box.cpp
    mmtisobmff/src/mmtisobmff/box/colrbox.cpp
    mmtisobmff/src/mmtisobmff/box/containerbox.cpp
    mmtisobmff/src/mmtisobmff/box/cttsbox.cpp
    mmtisobmff/src/mmtisobmff/box/decoderconfigurationbox.cpp
    mmtisobmff/src/mmtisobmff/box/decoderconfigurationfullbox.cpp
    mmtisobmff/src/mmtisobmff/box/drefbox.cpp
    mmtisobmff/src/mmtisobmff/box/elstbox.cpp
    mmtisobmff/src/mmtisobmff/box/flacsampleentry.cpp
    mmtisobmff/src/mmtisobmff/box/ftypbox.cpp
    mmtisobmff/src/mmtisobmff/box/gminbox.cpp
    mmtisobmff/src/mmtisobmff/box/hdlrbox.cpp
    mmtisobmff/src/mmtisobmff/box/hevcsampleentry.cpp
    mmtisobmff/src/mmtisobmff/box/invalidbox.cpp
    mmtisobmff/src/mmtisobmff/box/iodsbox.cpp
    mmtisobmff/src/mmtisobmff/box/jpvibox.cpp
    mmtisobmff/src/mmtisobmff/box/jxplbox.cpp
    mmtisobmff/src/mmtisobmff/box/jxssampleentry.cpp
    mmtisobmff/src/mmtisobmff/box/loudnessbox.cpp
    mmtisobmff/src/mmtisobmff/box/mdatbox.cpp
    mmtisobmff/src/mmtisobmff/box/mdhdbox.cpp
    mmtisobmff/src/mmtisobmff/box/mfhdbox.cpp
    mmtisobmff/src/mmtisobmff/box/mhapbox.cpp
    mmtisobmff/src/mmtisobmff/box/mhasampleentry.cpp
    mmtisobmff/src/mmtisobmff/box/mhmsampleentry.cpp
    mmtisobmff/src/mmtisobmff/box/mmpubox.cpp
    mmtisobmff/src/mmtisobmff/box/mp4asampleentry.cpp
    mmtisobmff/src/mmtisobmff/box/mvhdbox.cpp
    mmtisobmff/src/mmtisobmff/box/nmhdbox.cpp
    mmtisobmff/src/mmtisobmff/box/opussampleentry.cpp
    mmtisobmff/src/mmtisobmff/box/sampleentry.cpp
    mmtisobmff/src/mmtisobmff/box/samplegroupentry.cpp
    mmtisobmff/src/mmtisobmff/box/sbgpbox.cpp
    mmtisobmff/src/mmtisobmff/box/sgpdbox.cpp
    mmtisobmff/src/mmtisobmff/box/sidxbox.cpp
    mmtisobmff/src/mmtisobmff/box/smhdbox.cpp
    mmtisobmff/src/mmtisobmff/box/stcobox.cpp
    mmtisobmff/src/mmtisobmff/box/stscbox.cpp
    mmtisobmff/src/mmtisobmff/box/stsdbox.cpp
    mmtisobmff/src/mmtisobmff/box/stssbox.cpp
    mmtisobmff/src/mmtisobmff/box/stszbox.cpp
    mmtisobmff/src/mmtisobmff/box/sttsbox.cpp
    mmtisobmff/src/mmtisobmff/box/stypbox.cpp
    mmtisobmff/src/mmtisobmff/box/stz2box.cpp
    mmtisobmff/src/mmtisobmff/box/textbox.cpp
    mmtisobmff/src/mmtisobmff/box/textsampledescription.cpp
    mmtisobmff/src/mmtisobmff/box/tfdtbox.cpp
    mmtisobmff/src/mmtisobmff/box/tfhdbox.cpp
    mmtisobmff/src/mmtisobmff/box/timedtextdescription.cpp
    mmtisobmff/src/mmtisobmff/box/tkhdbox.cpp
    mmtisobmff/src/mmtisobmff/box/trackreferencetypebox.cpp
    mmtisobmff/src/mmtisobmff/box/trexbox.cpp
    mmtisobmff/src/mmtisobmff/box/trunbox.cpp
    mmtisobmff/src/mmtisobmff/box/unknownbox.cpp
    mmtisobmff/src/mmtisobmff/box/urlbox.cpp
    mmtisobmff/src/mmtisobmff/box/vmhdbox.cpp
    mmtisobmff/src/mmtisobmff/box/vvcsampleentry.cpp
    mmtisobmff/src/mmtisobmff/common/bytebuffertools_extension.cpp
    mmtisobmff/src/mmtisobmff/common/commonhelpertools.cpp
    mmtisobmff/src/mmtisobmff/common/logging.cpp
    mmtisobmff/src/mmtisobmff/common/printhelpertools.cpp
    mmtisobmff/src/mmtisobmff/common/types.cpp
    mmtisobmff/src/mmtisobmff/common/videohelpertools.cpp
    mmtisobmff/src/mmtisobmff/configdescriptor/avc_decoderconfigrecord.cpp
    mmtisobmff/src/mmtisobmff/configdescriptor/hevc_decoderconfigrecord.cpp
    mmtisobmff/src/mmtisobmff/configdescriptor/jxs_decoderconfigrecord.cpp
    mmtisobmff/src/mmtisobmff/configdescriptor/mha_decoderconfigrecord.cpp
    mmtisobmff/src/mmtisobmff/configdescriptor/mp4a_decoderconfigrecord.cpp
    mmtisobmff/src/mmtisobmff/configdescriptor/vvc_decoderconfigrecord.cpp
    mmtisobmff/src/mmtisobmff/descriptor/basedescriptor.cpp
    mmtisobmff/src/mmtisobmff/descriptor/decoderconfigdescriptor.cpp
    mmtisobmff/src/mmtisobmff/descriptor/esdescriptor.cpp
    mmtisobmff/src/mmtisobmff/descriptor/esidincdescriptor.cpp
    mmtisobmff/src/mmtisobmff/descriptor/genericdecoderspecificinfo.cpp
    mmtisobmff/src/mmtisobmff/descriptor/iodescriptor.cpp
    mmtisobmff/src/mmtisobmff/descriptor/slconfigdescriptor.cpp
    mmtisobmff/src/mmtisobmff/mmtisobmff_c.cpp
    mmtisobmff/src/mmtisobmff/reader/reader.cpp
    mmtisobmff/src/mmtisobmff/reader/readerinfo.cpp
    mmtisobmff/src/mmtisobmff/reader/samplereader.cpp
    mmtisobmff/src/mmtisobmff/reader/sample_extractor.cpp
    mmtisobmff/src/mmtisobmff/reader/specificboxinfo.cpp
    mmtisobmff/src/mmtisobmff/reader/trackreader.cpp
    mmtisobmff/src/mmtisobmff/service/boxreader.cpp
    mmtisobmff/src/mmtisobmff/service/boxregistry.cpp
    mmtisobmff/src/mmtisobmff/service/factory.cpp
    mmtisobmff/src/mmtisobmff/service/servicesingleton.cpp
    mmtisobmff/src/mmtisobmff/tree/boxtree.cpp
    mmtisobmff/src/mmtisobmff/writer/alac_tree_enhancer.cpp
    mmtisobmff/src/mmtisobmff/writer/avc_tree_enhancer.cpp
    mmtisobmff/src/mmtisobmff/writer/config_verifier.cpp
    mmtisobmff/src/mmtisobmff/writer/hevc_tree_enhancer.cpp
    mmtisobmff/src/mmtisobmff/writer/initsegment_tree_builder.cpp
    mmtisobmff/src/mmtisobmff/writer/jxs_tree_enhancer.cpp
    mmtisobmff/src/mmtisobmff/writer/mediafragment_tree_builder.cpp
    mmtisobmff/src/mmtisobmff/writer/mp4a_tree_enhancer.cpp
    mmtisobmff/src/mmtisobmff/writer/mpegh_tree_enhancer.cpp
    mmtisobmff/src/mmtisobmff/writer/sample_store.cpp
    mmtisobmff/src/mmtisobmff/writer/text_tree_enhancer.cpp
    mmtisobmff/src/mmtisobmff/writer/trackwriter.cpp
    mmtisobmff/src/mmtisobmff/writer/traf_samplegroups_enhancer.cpp
    mmtisobmff/src/mmtisobmff/writer/traf_sample_enhancer.cpp
    mmtisobmff/src/mmtisobmff/writer/traf_tree_enhancer.cpp
    mmtisobmff/src/mmtisobmff/writer/trak_editlist_enhancer.cpp
    mmtisobmff/src/mmtisobmff/writer/trak_reference_enhancer.cpp
    mmtisobmff/src/mmtisobmff/writer/trak_samplegroups_enhancer.cpp
    mmtisobmff/src/mmtisobmff/writer/trak_sample_enhancer.cpp
    mmtisobmff/src/mmtisobmff/writer/trak_tree_enhancer.cpp
    mmtisobmff/src/mmtisobmff/writer/trak_userdata_enhancer.cpp
    mmtisobmff/src/mmtisobmff/writer/vvc_tree_enhancer.cpp
    mmtisobmff/src/mmtisobmff/writer/writer.cpp
    mmtisobmff/src/mmtisobmff/writer/writerpimpl.cpp
)
target_include_directories(mmtisobmff PUBLIC mmtisobmff/include)
target_include_directories(mmtisobmff PRIVATE mmtisobmff/src/mmtisobmff)
target_compile_definitions(mmtisobmff PRIVATE _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE)
if(MSVC)
    target_compile_options(mmtisobmff PRIVATE /wd4244 /wd4267)
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
    filters/KaiserLpf.cpp
    filters/Limiter.cpp
    filters/MatrixMixer.cpp
    filters/Normalizer.cpp
    filters/PipedReader.cpp
    filters/Quantizer.cpp
    filters/SoxLowpassFilter.cpp
    filters/SOXRModule.cpp
    filters/SoxrResampler.cpp
    filters/StreamingConvolver.cpp
    fft4g/fft4g_float.c
    output/CAFSink.cpp
    output/MMTISOBMFFAACSink.cpp
    output/MMTISOBMFFALACSink.cpp
    output/MMTISOBMFFSinkBase.cpp
    output/ADTSSink.cpp
    output/WaveSink.cpp
    bitstream.cpp
    chanmap.cpp
    CompositeSource.cpp
    cuesheet.cpp
    ISource.cpp
    metadata.cpp
    misc.cpp
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
    fft4g
)
if(NOT APPLE)
    target_include_directories(common PRIVATE CoreAudio)
endif()

target_compile_definitions(common PRIVATE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE)
if(MSVC)
    target_compile_options(common PRIVATE /wd4018 /wd4244 /wd4267 /wd4838)
endif()
target_link_libraries(common PUBLIC taglib uchardet ogg mmtisobmff)
target_link_libraries(common PUBLIC libsoundio)

# ---------------------------------------------------------------------------
# qaac
# ---------------------------------------------------------------------------
if(WIN32 OR APPLE)
add_executable(qaac
    input/CAFSource.cpp
    input/CoreAudioPacketDecoder.cpp
    input/ExtAFSource.cpp
    input/MMTISOBMFFSource.cpp
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
target_link_libraries(qaac PRIVATE common taglib mmtisobmff)

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
    input/MMTISOBMFFSource.cpp
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
target_link_libraries(refalac PRIVATE alac common taglib mmtisobmff)
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
