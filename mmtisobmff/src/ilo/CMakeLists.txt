configure_file (
    "${PROJECT_SOURCE_DIR}/src/ilo_config.h.in"
    "${PROJECT_SOURCE_DIR}/include/ilo/version.h"
)

set(publicHeaders
    ${PROJECT_SOURCE_DIR}/include/ilo/version.h
    ${PROJECT_SOURCE_DIR}/include/ilo/logging.h
    ${PROJECT_SOURCE_DIR}/include/ilo/logging_backend.h
    ${PROJECT_SOURCE_DIR}/include/ilo/servicelocator.h
    ${PROJECT_SOURCE_DIR}/include/ilo/node_tree.h
    ${PROJECT_SOURCE_DIR}/include/ilo/singleton.h
    ${PROJECT_SOURCE_DIR}/include/ilo/bytebuffertools.h
    ${PROJECT_SOURCE_DIR}/include/ilo/common_types.h
    ${PROJECT_SOURCE_DIR}/include/ilo/string_utils.h
    ${PROJECT_SOURCE_DIR}/include/ilo/gtest_helper.h
    ${PROJECT_SOURCE_DIR}/include/ilo/memory.h
    ${PROJECT_SOURCE_DIR}/include/ilo/fileio.h
    ${PROJECT_SOURCE_DIR}/include/ilo/async_fileio.h
    ${PROJECT_SOURCE_DIR}/include/ilo/file_utils.h
    ${PROJECT_SOURCE_DIR}/include/ilo/uuid_utils.h
    ${PROJECT_SOURCE_DIR}/include/ilo/bittool_utils.h
    ${PROJECT_SOURCE_DIR}/include/ilo/bitparser.h
    ${PROJECT_SOURCE_DIR}/include/ilo/bitbuffer.h
)

set(srcs
    logging_backend.cpp
    bytebuffertools.cpp
    file_utils.cpp
    fileio.cpp
    uuid_utils.cpp
    common_types.cpp
    ilo_logging.h
    string_utils.cpp
    bitparser.cpp
    bitbuffer.cpp
    async_fileio_not_supported.cpp
)

add_library(ilo STATIC ${srcs} ${publicHeaders})

target_compile_features(ilo PUBLIC cxx_std_11)
set_target_properties(ilo PROPERTIES CXX_EXTENSIONS OFF)

if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
  target_link_libraries(ilo pthread rt)
endif()

# TODO: Do we need to link pthread for WASM?

if(EMSCRIPTEN)
  target_compile_options(ilo PUBLIC "-fexceptions")
endif()

if(ANDROID)
  target_link_libraries(ilo log)
endif()

set_target_properties(ilo PROPERTIES PUBLIC_HEADER "${publicHeaders}")
if("${CMAKE_CXX_FLAGS}" MATCHES "unsigned-integer-overflow")
  # There are some unsigned overflow "errors" reported by UBSAN in some libstdc++ files, which we cannot fix
  target_compile_options(ilo PRIVATE "-fsanitize-blacklist=${CMAKE_CURRENT_SOURCE_DIR}/../sanitizer-ignorelist.txt")
endif()
