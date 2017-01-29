project(photon)

cmake_minimum_required(VERSION 2.8.11)

macro(enable_sanitizer)
    add_definitions(
        -fsanitize=address
        -fsanitize=undefined
        -ggdb
    )
    SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address -fsanitize=undefined")
endmacro()

if(ASAN)
    enable_sanitizer()
endif()

set(PHOTON_GEN_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/_gen_src)

add_subdirectory(thirdparty/decode EXCLUDE_FROM_ALL)

function(photon_generate_sources dir)
    add_custom_command(
        OUTPUT ${PHOTON_GEN_SRC_DIR}/photon/Photon.c
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${PHOTON_GEN_SRC_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PHOTON_GEN_SRC_DIR}
        COMMAND decode_gen -i ${dir} -o ${PHOTON_GEN_SRC_DIR}
        DEPENDS decode_gen ${ARGN}
    )
endfunction()

set(ONBOARD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/onboard)
file(GLOB ONBOARD_SOURCES ${ONBOARD_DIR})
photon_generate_sources(${ONBOARD_DIR} ${ONBOARD_SOURCES})

add_library(photon
    src/photon/Config.h
    src/photon/core/Assert.h
    src/photon/core/Crc.c
    src/photon/core/Crc.h
    src/photon/core/Endian.h
    src/photon/core/Reader.c
    src/photon/core/RingBuf.c
    src/photon/core/Try.h
    src/photon/core/Writer.c
    src/photon/tm/Tm.c
    src/photon/tm/StatusMessage.c
    src/photon/exc/Exc.c
    src/photon/exc/DataPacket.c
    src/photon/exc/ReceiptPacket.c
    src/photon/exc/Utils.h
    ${PHOTON_GEN_SRC_DIR}/photon/Photon.c
    ${ONBOARD_SOURCES}
)

set_target_properties(photon
    PROPERTIES
    PREFIX "lib"
)

if(NOT MSVC)
    target_compile_options(photon PRIVATE -std=c99 -Wall -Wextra)
endif()

target_include_directories(photon
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${PHOTON_GEN_SRC_DIR}
)

#add_subdirectory(thirdparty/gtest EXCLUDE_FROM_ALL)
add_subdirectory(tests)
