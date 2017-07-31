project(photon)

include(CMakeParseArguments)

cmake_minimum_required(VERSION 2.8.11)

enable_testing()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(Photon)

photon_init(${CMAKE_CURRENT_SOURCE_DIR})
photon_add_mod_dir(${CMAKE_CURRENT_SOURCE_DIR}/modules)
photon_add_device(model1)
photon_add_device(model2)
photon_generate_sources(${_PHOTON_DIR}/project.toml)

