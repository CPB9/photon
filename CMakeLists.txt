project(photon)

include(CMakeParseArguments)

cmake_minimum_required(VERSION 2.8.11)

enable_testing()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(Photon)

photon_init_project(
    PHOTON_ROOT ${CMAKE_CURRENT_SOURCE_DIR}
    MODULE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/modules
    PROJECT ${CMAKE_CURRENT_SOURCE_DIR}/project.toml
    DEVICES master slave1
    ENABLE_STUB
)

