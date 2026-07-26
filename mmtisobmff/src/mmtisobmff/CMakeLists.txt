set(srcCfgRecords
    configdescriptor/hevc_decoderconfigrecord.cpp
    configdescriptor/avc_decoderconfigrecord.cpp
    configdescriptor/mha_decoderconfigrecord.cpp
    configdescriptor/mp4a_decoderconfigrecord.cpp
    configdescriptor/jxs_decoderconfigrecord.cpp
    configdescriptor/vvc_decoderconfigrecord.cpp)

set(srcBoxes
    box/box.h
    box/boxregistryentry.h
    box/box.cpp
    box/containerbox.h
    box/containerbox.cpp
    box/mmpubox.h
    box/mmpubox.cpp
    box/ftypbox.h
    box/ftypbox.cpp
    box/stypbox.h
    box/stypbox.cpp
    box/mdatbox.h
    box/mdatbox.cpp
    box/sampleentry.h
    box/sampleentry.cpp
    box/mhmsampleentry.h
    box/mhmsampleentry.cpp
    box/mhasampleentry.h
    box/mhasampleentry.cpp
    box/hevcsampleentry.h
    box/hevcsampleentry.cpp
    box/avcsampleentry.h
    box/avcsampleentry.cpp
    box/mp4asampleentry.h
    box/mp4asampleentry.cpp
    box/vvcsampleentry.h
    box/vvcsampleentry.cpp
    box/sttsbox.h
    box/sttsbox.cpp
    box/stssbox.h
    box/stssbox.cpp
    box/stcobox.h
    box/stcobox.cpp
    box/co64box.h
    box/co64box.cpp
    box/stscbox.h
    box/stscbox.cpp
    box/mvhdbox.h
    box/mvhdbox.cpp
    box/mdhdbox.h
    box/mdhdbox.cpp
    box/tkhdbox.h
    box/tkhdbox.cpp
    box/trexbox.h
    box/trexbox.cpp
    box/mfhdbox.h
    box/mfhdbox.cpp
    box/tfdtbox.h
    box/tfdtbox.cpp
    box/tfhdbox.h
    box/tfhdbox.cpp
    box/stszbox.h
    box/stszbox.cpp
    box/stz2box.h
    box/stz2box.cpp
    box/hdlrbox.h
    box/hdlrbox.cpp
    box/trunbox.h
    box/trunbox.cpp
    box/smhdbox.h
    box/smhdbox.cpp
    box/vmhdbox.h
    box/vmhdbox.cpp
    box/sidxbox.h
    box/sidxbox.cpp
    box/btrtbox.h
    box/btrtbox.cpp
    box/decoderconfigurationbasebox.h
    box/decoderconfigurationbox.h
    box/decoderconfigurationbox.cpp
    box/decoderconfigurationfullbox.h
    box/decoderconfigurationfullbox.cpp
    box/drefbox.h
    box/drefbox.cpp
    box/urlbox.h
    box/urlbox.cpp
    box/stsdbox.h
    box/stsdbox.cpp
    box/cttsbox.h
    box/cttsbox.cpp
    box/invalidbox.h
    box/invalidbox.cpp
    box/unknownbox.h
    box/unknownbox.cpp
    box/decoderconfigurationprovider.h
    box/elstbox.h
    box/elstbox.cpp
    box/ibox.h
    box/sgpdbox.h
    box/sgpdbox.cpp
    box/samplegroupentry.h
    box/samplegroupentry.cpp
    box/sbgpbox.h
    box/sbgpbox.cpp
    box/loudnessbox.h
    box/loudnessbox.cpp
    box/iodsbox.h
    box/iodsbox.cpp
    box/jxssampleentry.h
    box/jxssampleentry.cpp
    box/jpvibox.h
    box/jpvibox.cpp
    box/jxplbox.h
    box/jxplbox.cpp
    box/colrbox.h
    box/colrbox.cpp
    box/mhapbox.h
    box/mhapbox.cpp
    )

set(srcDescriptors
    descriptor/basedescriptor.cpp
    descriptor/basedescriptor.h
    descriptor/ibasedescriptor.h
    descriptor/decoderconfigdescriptor.cpp
    descriptor/decoderconfigdescriptor.h
    descriptor/decoderspecificinfo.h
    descriptor/genericdecoderspecificinfo.cpp
    descriptor/genericdecoderspecificinfo.h
    descriptor/esdescriptor.cpp
    descriptor/esdescriptor.h
    descriptor/slconfigdescriptor.cpp
    descriptor/slconfigdescriptor.h
    descriptor/iodescriptor.cpp
    descriptor/iodescriptor.h
    descriptor/esidincdescriptor.cpp
    descriptor/esidincdescriptor.h
    )
    
set(srcTools
    common/types.cpp
    common/videohelpertools.cpp
    common/commonhelpertools.cpp
    common/printhelpertools.cpp
    common/logging.h
    common/logging.cpp
    common/bytebuffertools_extension.h
    common/bytebuffertools_extension.cpp
    common/internal_types.h
    service/boxreader.h
    service/boxreader.cpp
    service/factory.h
    service/factory.cpp
    service/boxregistry.h
    service/boxregistry.cpp
    service/servicesingleton.h
    service/servicesingleton.cpp
    tree/boxtree.h
    tree/boxtree.cpp
    tree/tree_parser.h
    )

set(srcReader
    common/tracksampleinfo.h
    reader/reader.cpp
    reader/pimpl.h
    reader/samplereader.h
    reader/samplereader.cpp
    reader/trackreader.cpp
    reader/specificboxinfo.cpp
    reader/readerinfo.h
    reader/readerinfo.cpp
    reader/sample_extractor.h
    reader/sample_extractor.cpp)

set(srcWriter
    writer/sample_store.h
    writer/sample_store.cpp
    writer/treebuilder.h
    writer/initsegment_tree_builder.h
    writer/initsegment_tree_builder.cpp 
    writer/mediafragment_tree_builder.h
    writer/mediafragment_tree_builder.cpp 
    writer/treeenhancer.h
    writer/mpegh_tree_enhancer.h
    writer/mpegh_tree_enhancer.cpp
    writer/avc_tree_enhancer.h
    writer/avc_tree_enhancer.cpp
    writer/hevc_tree_enhancer.h
    writer/hevc_tree_enhancer.cpp
    writer/vvc_tree_enhancer.h
    writer/vvc_tree_enhancer.cpp
    writer/trak_tree_enhancer.h
    writer/trak_tree_enhancer.cpp
    writer/traf_tree_enhancer.h
    writer/traf_tree_enhancer.cpp 
    writer/trak_sample_enhancer.h
    writer/trak_sample_enhancer.cpp
    writer/traf_sample_enhancer.h
    writer/traf_sample_enhancer.cpp
    writer/mp4a_tree_enhancer.h
    writer/mp4a_tree_enhancer.cpp
    writer/jxs_tree_enhancer.h
    writer/jxs_tree_enhancer.cpp
    writer/writer.cpp
    writer/writerpimpl.h
    writer/writerpimpl.cpp
    writer/trackwriter.cpp
    writer/config_verifier.h
    writer/config_verifier.cpp
    writer/trak_samplegroups_enhancer.h
    writer/trak_samplegroups_enhancer.cpp
    writer/traf_samplegroups_enhancer.h
    writer/traf_samplegroups_enhancer.cpp
    writer/trak_editlist_enhancer.h
    writer/trak_editlist_enhancer.cpp
    writer/trak_userdata_enhancer.h
    writer/trak_userdata_enhancer.cpp
)

set(srcCInterface
    mmtisobmff_c.cpp
    ${PROJECT_SOURCE_DIR}/include/mmtisobmff/mmtisobmff_c.h
)

set(publicHeaders
    ${PROJECT_SOURCE_DIR}/include/mmtisobmff/types.h
    ${PROJECT_SOURCE_DIR}/include/mmtisobmff/specificboxinfo.h
    ${PROJECT_SOURCE_DIR}/include/mmtisobmff/reader/reader.h
    ${PROJECT_SOURCE_DIR}/include/mmtisobmff/reader/trackreader.h
    ${PROJECT_SOURCE_DIR}/include/mmtisobmff/reader/input.h
    ${PROJECT_SOURCE_DIR}/include/mmtisobmff/writer/writer.h
    ${PROJECT_SOURCE_DIR}/include/mmtisobmff/writer/trackwriter.h
    ${PROJECT_SOURCE_DIR}/include/mmtisobmff/writer/output.h
    ${PROJECT_SOURCE_DIR}/include/mmtisobmff/writer/advancedwriter.h
    ${PROJECT_SOURCE_DIR}/include/mmtisobmff/helper/videohelpertools.h
    ${PROJECT_SOURCE_DIR}/include/mmtisobmff/helper/commonhelpertools.h
    ${PROJECT_SOURCE_DIR}/include/mmtisobmff/helper/printhelpertools.h
    ${PROJECT_SOURCE_DIR}/include/mmtisobmff/configdescriptor/hevc_decoderconfigrecord.h
    ${PROJECT_SOURCE_DIR}/include/mmtisobmff/configdescriptor/avc_decoderconfigrecord.h
    ${PROJECT_SOURCE_DIR}/include/mmtisobmff/configdescriptor/mha_decoderconfigrecord.h
    ${PROJECT_SOURCE_DIR}/include/mmtisobmff/configdescriptor/mp4a_decoderconfigrecord.h
    ${PROJECT_SOURCE_DIR}/include/mmtisobmff/configdescriptor/jxs_decoderconfigrecord.h
    ${PROJECT_SOURCE_DIR}/include/mmtisobmff/configdescriptor/vvc_decoderconfigrecord.h
    ${PROJECT_SOURCE_DIR}/include/mmtisobmff/logging.h
)

set(publicHeadersC
    ${PROJECT_SOURCE_DIR}/include/mmtisobmff/mmtisobmff_c.h
)

set(emscriptenCompileOptions
    "-fexceptions"
)

set(libraries ilo)

# Target : mmtisobmff C++ Library
add_library(mmtisobmff STATIC ${srcCfgRecords} ${srcBoxes} ${srcDescriptors} ${srcTools} ${srcReader} ${srcWriter} ${publicHeaders})
set_target_properties(mmtisobmff PROPERTIES PUBLIC_HEADER "${publicHeaders}")
target_link_libraries(mmtisobmff ${libraries})
target_compile_features(mmtisobmff PUBLIC cxx_std_11)
set_target_properties(mmtisobmff PROPERTIES CXX_EXTENSIONS OFF)

if (EMSCRIPTEN)
  target_compile_options(mmtisobmff PUBLIC ${emscriptenCompileOptions})
endif()

# Target : mmtisobmff C Library
add_library(mmtisobmff_c STATIC ${srcCInterface})
set_target_properties(mmtisobmff_c PROPERTIES PUBLIC_HEADER "${publicHeadersC}")
target_link_libraries(mmtisobmff_c PRIVATE mmtisobmff)
target_compile_features(mmtisobmff_c PUBLIC cxx_std_11)
set_target_properties(mmtisobmff_c PROPERTIES CXX_EXTENSIONS OFF)

if (EMSCRIPTEN)
  target_compile_options(mmtisobmff_c PUBLIC ${emscriptenCompileOptions})
endif()

# Handle sanitizer ignore list for plain cmake workflow
if("${CMAKE_CXX_FLAGS}" MATCHES "unsigned-integer-overflow")
  # There are some unsigned overflow "errors" reported by UBSAN in some libstdc++ files, which we cannot fix
  target_compile_options(mmtisobmff PRIVATE "-fsanitize-blacklist=${CMAKE_CURRENT_SOURCE_DIR}/../sanitizer-ignorelist.txt")
  target_compile_options(mmtisobmff_c PRIVATE "-fsanitize-blacklist=${CMAKE_CURRENT_SOURCE_DIR}/../sanitizer-ignorelist.txt")
endif()
