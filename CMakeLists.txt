project(photon)

include(CMakeParseArguments)

cmake_minimum_required(VERSION 2.8.11)

enable_testing()

set(PHOTON_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

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
add_subdirectory(thirdparty/dtacan EXCLUDE_FROM_ALL)

get_property(DECODE_GEN_LOC TARGET decode_gen PROPERTY LOCATION)

function(photon_generate_sources proj)
    add_custom_command(
        OUTPUT ${PHOTON_GEN_SRC_DIR}/Model1.c
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${PHOTON_GEN_SRC_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PHOTON_GEN_SRC_DIR}
        COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} ${DECODE_GEN_LOC} -p ${proj} -o ${PHOTON_GEN_SRC_DIR} -c 4
        DEPENDS decode_gen ${photon_generate_sources_SOURCES}
    )
endfunction()

set(IMPL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/modules)
file(GLOB_RECURSE IMPL_SOURCES "${IMPL_DIR}/*.c" "${IMPL_DIR}/*.h")
photon_generate_sources(${CMAKE_CURRENT_SOURCE_DIR}/project.toml)

add_library(photon
    #${IMPL_SOURCES}
    ${PHOTON_GEN_SRC_DIR}/Model1.c
)

set_target_properties(photon
    PROPERTIES
    PREFIX "lib"
)

if(NOT MSVC)
    target_compile_options(photon PRIVATE
        -std=c99
        -Wall
        -Wextra
        -Werror=implicit-function-declaration
        -Werror=incompatible-pointer-types
    )
endif()

target_include_directories(photon
    PUBLIC
    #${IMPL_DIR}
    ${PHOTON_GEN_SRC_DIR}
)

add_subdirectory(thirdparty/gtest EXCLUDE_FROM_ALL)
add_subdirectory(tests)

set_target_properties(photon ui_test
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

