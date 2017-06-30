project(photon)

include(CMakeParseArguments)

cmake_minimum_required(VERSION 2.8.11)

enable_testing()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(Photon)

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

set(IMPL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/modules)
file(GLOB_RECURSE IMPL_SOURCES "${IMPL_DIR}/*.c" "${IMPL_DIR}/*.h" "${IMPL_DIR}/*.toml" "${IMPL_DIR}/*.decode")
photon_init(${CMAKE_CURRENT_SOURCE_DIR})
photon_add_device(model1)
photon_add_device(model2)
photon_generate_sources(${_PHOTON_DIR}/project.toml ${IMPL_SOURCES})

#add_subdirectory(thirdparty/gtest EXCLUDE_FROM_ALL)
#add_subdirectory(tests)

