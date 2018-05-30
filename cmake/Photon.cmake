include(CPack)
include(DeployQt5)
include(CMakeParseArguments)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Network REQUIRED)

set(_PHOTON_MOD_DIRS)
set(PHOTON_GEN_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/_photon_gen_src)
set(PHOTON_GEN_SRC_ONBOARD_DIR ${PHOTON_GEN_SRC_DIR})
set(PHOTON_GEN_SRC_GROUNDCONTROL_DIR ${PHOTON_GEN_SRC_DIR})

macro(photon_add_mod_dir dir)
    list(APPEND _PHOTON_MOD_DIRS ${dir})
endmacro()

macro(_photon_setup_target target)
    if (PHOTON_LOG_LEVEL)
        target_compile_options(${target} PUBLIC -DPHOTON_LOG_LEVEL=${PHOTON_LOG_LEVEL})
    endif()
endmacro()

macro(_photon_add_unit_test test target file)
    bmcl_add_unit_test(${test}-${target} ${_PHOTON_DIR}/tests/${file})

    target_link_libraries(${test}-${target}
        photon-target-${target}
        decode
        photon
    )
endmacro()

function(_photon_add_library target)
    bmcl_add_library(${target} ${ARGN})
    _photon_setup_target(${target})
endfunction()

function(_photon_add_executable target)
    bmcl_add_executable(${target} ${ARGN})
    _photon_setup_target(${target})
endfunction()

function(_photon_add_ui_test target)
    _photon_add_executable(${target} ${ARGN})
    target_link_libraries(${target}
        decode
        photon
        bmcl
        photon-uitest
        asio
        tclap
        Qt5::Core
        Qt5::Widgets
    )

    if(MINGW OR MSVS)
        target_link_libraries(${target} ws2_32)
    endif()
endfunction()

function(_photon_add_model_ui_test test target)
    _photon_add_ui_test(${test}-${target} ${ARGN})

    target_include_directories(${test}-${target}
        SYSTEM PRIVATE
        ${PHOTON_GEN_SRC_ONBOARD_DIR}
    )

    target_link_libraries(${test}-${target} photon-target-${target})
endfunction()

macro(photon_init dir)
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${PHOTON_GEN_SRC_DIR})
    include(${dir}/thirdparty/decode/thirdparty/bmcl/cmake/Bmcl.cmake)

    #set(_PHOTON_DEPENDS ${PHOTON_GEN_SRC_DIR}/Config.h)
    set(_PHOTON_DEPENDS
        ${PHOTON_GEN_SRC_GROUNDCONTROL_DIR}/Photon.hpp
        ${PHOTON_GEN_SRC_GROUNDCONTROL_DIR}/Photon.cpp
        ${PHOTON_GEN_SRC_ONBOARD_DIR}/Photon.c
        ${PHOTON_GEN_SRC_ONBOARD_DIR}/Photon.h
    )
    set(_PHOTON_DEPENDS_H)
    set(_PHOTON_DIR ${dir})

    bmcl_add_dep_gtest(${_PHOTON_DIR}/thirdparty/gtest)
    bmcl_add_dep_tclap(${_PHOTON_DIR}/thirdparty/decode/thirdparty/tclap)
    bmcl_add_dep_caf(${_PHOTON_DIR}/thirdparty/caf)
    bmcl_add_dep_asio(${_PHOTON_DIR}/thirdparty/asio)

    add_subdirectory(${_PHOTON_DIR}/thirdparty/decode EXCLUDE_FROM_ALL)
    add_subdirectory(${_PHOTON_DIR}/thirdparty/dtacan EXCLUDE_FROM_ALL)

    set(PHOTON_GROUNDCONTROL_SRC
        ${_PHOTON_DIR}/src/photon/groundcontrol/AllowUnsafeMessageType.h
        ${_PHOTON_DIR}/src/photon/groundcontrol/Atoms.h
        ${_PHOTON_DIR}/src/photon/groundcontrol/CmdState.cpp
        ${_PHOTON_DIR}/src/photon/groundcontrol/CmdState.h
        ${_PHOTON_DIR}/src/photon/groundcontrol/Crc.cpp
        ${_PHOTON_DIR}/src/photon/groundcontrol/Crc.h
        ${_PHOTON_DIR}/src/photon/groundcontrol/DfuState.cpp
        ${_PHOTON_DIR}/src/photon/groundcontrol/DfuState.h
        ${_PHOTON_DIR}/src/photon/groundcontrol/Exchange.cpp
        ${_PHOTON_DIR}/src/photon/groundcontrol/Exchange.h
        ${_PHOTON_DIR}/src/photon/groundcontrol/FwtState.cpp
        ${_PHOTON_DIR}/src/photon/groundcontrol/FwtState.h
        ${_PHOTON_DIR}/src/photon/groundcontrol/GcInterface.cpp
        ${_PHOTON_DIR}/src/photon/groundcontrol/GcInterface.h
        ${_PHOTON_DIR}/src/photon/groundcontrol/GcStructs.h
        ${_PHOTON_DIR}/src/photon/groundcontrol/GroundControl.cpp
        ${_PHOTON_DIR}/src/photon/groundcontrol/GroundControl.h
        ${_PHOTON_DIR}/src/photon/groundcontrol/MemIntervalSet.cpp
        ${_PHOTON_DIR}/src/photon/groundcontrol/MemIntervalSet.h
        ${_PHOTON_DIR}/src/photon/groundcontrol/ProjectUpdate.cpp
        ${_PHOTON_DIR}/src/photon/groundcontrol/ProjectUpdate.h
        ${_PHOTON_DIR}/src/photon/groundcontrol/TmParamUpdate.h
        ${_PHOTON_DIR}/src/photon/groundcontrol/TmState.cpp
        ${_PHOTON_DIR}/src/photon/groundcontrol/TmState.h
    )
    source_group("groundcontrol" FILES ${PHOTON_GROUNDCONTROL_SRC})

    set(PHOTON_MODEL_SRC
        ${_PHOTON_DIR}/src/photon/model/CmdModel.cpp
        ${_PHOTON_DIR}/src/photon/model/CmdModel.h
        ${_PHOTON_DIR}/src/photon/model/CmdNode.cpp
        ${_PHOTON_DIR}/src/photon/model/CmdNode.h
        ${_PHOTON_DIR}/src/photon/model/Decoder.cpp
        ${_PHOTON_DIR}/src/photon/model/Decoder.h
        ${_PHOTON_DIR}/src/photon/model/Encoder.cpp
        ${_PHOTON_DIR}/src/photon/model/Encoder.h
        ${_PHOTON_DIR}/src/photon/model/FieldsNode.cpp
        ${_PHOTON_DIR}/src/photon/model/FieldsNode.h
        ${_PHOTON_DIR}/src/photon/model/FindNode.cpp
        ${_PHOTON_DIR}/src/photon/model/FindNode.h
        ${_PHOTON_DIR}/src/photon/model/Node.cpp
        ${_PHOTON_DIR}/src/photon/model/Node.h
        ${_PHOTON_DIR}/src/photon/model/NodeView.cpp
        ${_PHOTON_DIR}/src/photon/model/NodeView.h
        ${_PHOTON_DIR}/src/photon/model/NodeViewStore.cpp
        ${_PHOTON_DIR}/src/photon/model/NodeViewStore.h
        ${_PHOTON_DIR}/src/photon/model/NodeViewUpdate.cpp
        ${_PHOTON_DIR}/src/photon/model/NodeViewUpdate.h
        ${_PHOTON_DIR}/src/photon/model/NodeViewUpdater.cpp
        ${_PHOTON_DIR}/src/photon/model/NodeViewUpdater.h
        ${_PHOTON_DIR}/src/photon/model/OnboardTime.cpp
        ${_PHOTON_DIR}/src/photon/model/OnboardTime.h
        ${_PHOTON_DIR}/src/photon/model/TmMsgDecoder.cpp
        ${_PHOTON_DIR}/src/photon/model/TmMsgDecoder.h
        ${_PHOTON_DIR}/src/photon/model/TmModel.cpp
        ${_PHOTON_DIR}/src/photon/model/TmModel.h
        ${_PHOTON_DIR}/src/photon/model/Value.cpp
        ${_PHOTON_DIR}/src/photon/model/Value.h
        ${_PHOTON_DIR}/src/photon/model/ValueInfoCache.cpp
        ${_PHOTON_DIR}/src/photon/model/ValueInfoCache.h
        ${_PHOTON_DIR}/src/photon/model/ValueKind.h
        ${_PHOTON_DIR}/src/photon/model/ValueNode.cpp
        ${_PHOTON_DIR}/src/photon/model/ValueNode.h
    )
    source_group("model" FILES ${PHOTON_MODEL_SRC})

    set(PHOTON_UI_HEADERS
        ${_PHOTON_DIR}/src/photon/ui/FirmwareWidget.h
        ${_PHOTON_DIR}/src/photon/ui/FirmwareStatusWidget.h
        ${_PHOTON_DIR}/src/photon/ui/QModelBase.h
        ${_PHOTON_DIR}/src/photon/ui/QCmdModel.h
        ${_PHOTON_DIR}/src/photon/ui/QNodeModel.h
        ${_PHOTON_DIR}/src/photon/ui/QNodeViewModel.h
    )
    qt5_wrap_cpp(PHOTON_UI_MOC
        ${PHOTON_UI_HEADERS}
    )

    set(PHOTON_UI_SRC
        ${_PHOTON_DIR}/src/photon/ui/FirmwareWidget.cpp
        ${_PHOTON_DIR}/src/photon/ui/FirmwareStatusWidget.cpp
        ${_PHOTON_DIR}/src/photon/ui/QModelBase.cpp
        ${_PHOTON_DIR}/src/photon/ui/QCmdModel.cpp
        ${_PHOTON_DIR}/src/photon/ui/QNodeModel.cpp
        ${_PHOTON_DIR}/src/photon/ui/QNodeViewModel.cpp
        ${PHOTON_UI_MOC}
    )
    source_group("ui" FILES ${PHOTON_UI_SRC})
    source_group("ui_moc" FILES ${PHOTON_UI_MOC})

    _photon_add_library(photon
        ${PHOTON_UI_SRC}
        ${PHOTON_GROUNDCONTROL_SRC}
        ${PHOTON_MODEL_SRC}
        ${PHOTON_GEN_SRC_GROUNDCONTROL_DIR}/Photon.cpp
    )

    add_dependencies(photon photon-gen-src)

    target_link_libraries(photon
        decode
        bmcl
        Qt5::Widgets
        caf-core
    )

    target_compile_definitions(photon PRIVATE -DBUILDING_PHOTON)

    target_include_directories(photon
        PUBLIC
        ${_PHOTON_DIR}/src
        ${_PHOTON_DIR}/thirdparty
        ${PHOTON_GEN_SRC_GROUNDCONTROL_DIR}
    )

    qt5_wrap_cpp(PHOTON_UI_TEST_MOC
        ${_PHOTON_DIR}/tests/UiTest.h
    )
    _photon_add_library(photon-uitest
        ${_PHOTON_DIR}/tests/UiTest.cpp
        ${_PHOTON_DIR}/tests/UiTest.h
        ${PHOTON_UI_TEST_MOC}
    )
    target_link_libraries(photon-uitest
        decode
        photon
        Qt5::Core
    )

    _photon_add_ui_test(photon-client-serial
        ${_PHOTON_DIR}/tests/ComTest.cpp
    )

    _photon_add_ui_test(photon-client-udp
        ${_PHOTON_DIR}/tests/UdpTest.cpp
    )

    _photon_add_ui_test(photon-client
    ${_PHOTON_DIR}/tests/Client.cpp
    )
endmacro()

macro(photon_generate_sources proj)

    foreach(dir ${_PHOTON_MOD_DIRS})
        file(GLOB_RECURSE _DIR_SOURCES "${dir}/*.c" "${dir}/*.h" "${dir}/*.toml" "${dir}/*.decode")
        list(APPEND _PHOTON_MOD_SOURCES ${_DIR_SOURCES})
    endforeach()

    if(PHOTON_USE_ABS_PATHS)
        set(_PHOTON_ABS_FLAG "-a")
    endif()
    add_custom_command(
        OUTPUT ${_PHOTON_DEPENDS}
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${PHOTON_GEN_SRC_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PHOTON_GEN_SRC_DIR}
        COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:decode-gen> -p  ${proj} -o ${PHOTON_GEN_SRC_DIR} -c 4 ${_PHOTON_ABS_FLAG}
        DEPENDS decode-gen ${proj} ${_PHOTON_MOD_SOURCES}
    )
    add_custom_target(photon-gen-src DEPENDS ${_PHOTON_DEPENDS})
    install(DIRECTORY ${PHOTON_GEN_SRC_ONBOARD_DIR}/photon DESTINATION gen)
    install(DIRECTORY ${PHOTON_GEN_SRC_GROUNDCONTROL_DIR}/photon DESTINATION gen)
    install(FILES ${_PHOTON_DEPENDS} ${_PHOTON_DEPENDS_H} DESTINATION gen)
endmacro()

macro(photon_add_device target)
    string(SUBSTRING ${target} 0 1 _FIRST_LETTER)
    string(TOUPPER ${_FIRST_LETTER} _FIRST_LETTER)
    string(TOUPPER ${target} _TARGET_UPPER)
    string(REGEX REPLACE "^.(.*)" "${_FIRST_LETTER}\\1" _SOURCE_NAME "${target}")

    set(_SRC_FILE ${PHOTON_GEN_SRC_ONBOARD_DIR}/Photon${_SOURCE_NAME}.c)
    execute_process(COMMAND ${CMAKE_COMMAND} -E touch ${_SRC_FILE})
    set(_PHOTON_DEPENDS_H ${_PHOTON_DEPENDS_H} ${PHOTON_GEN_SRC_ONBOARD_DIR}/Photon${_SOURCE_NAME}.h)

    _photon_add_library(photon-target-${target}
        ${_SRC_FILE}
    )

    add_dependencies(photon-target-${target} photon-gen-src)

    if(NOT MSVC)
        target_compile_options(photon-target-${target} PRIVATE
            -Werror=implicit-function-declaration
            -Werror=incompatible-pointer-types
        )
    endif()

    if (PHOTON_ENABLE_STUB)
        target_compile_options(photon-target-${target} PUBLIC -DPHOTON_STUB)
    endif()
    target_compile_options(photon-target-${target} PUBLIC -DPHOTON_DEVICE_${_TARGET_UPPER})

    target_include_directories(photon-target-${target}
        PUBLIC
        ${PHOTON_GEN_SRC_ONBOARD_DIR}
    )
    if (PHOTON_USE_ABS_PATHS)
        target_include_directories(photon-target-${target}
            PUBLIC
            ${_PHOTON_MOD_DIRS}
        )
    endif()

    _photon_add_model_ui_test(photon-model-inproc ${target} ${_PHOTON_DIR}/tests/InprocTest.cpp)
    _photon_add_model_ui_test(photon-model-udpserver ${target} ${_PHOTON_DIR}/tests/Model.cpp)

    #_photon_add_unit_test(photon-test-fwt ${target} FwtTest.cpp)
endmacro()

macro(photon_init_project)
    set(options ENABLE_STUB)
    set(oneValueArgs PHOTON_ROOT PROJECT)
    set(multiValueArgs MODULE_DIRS DEVICES)
    cmake_parse_arguments(_ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    if (${_ARGS_ENABLE_STUB})
        set(PHOTON_ENABLE_STUB 1)
    endif()
    photon_init(${_ARGS_PHOTON_ROOT})
    foreach(dir ${_ARGS_MODULE_DIRS})
        photon_add_mod_dir(${dir})
    endforeach()
    foreach(dev ${_ARGS_DEVICES})
        photon_add_device(${dev})
        foreach (lib ${_PHOTON_${dev}_LINK_LIBRARIES})
            target_link_libraries(photon-model-inproc-${dev} ${lib})
            target_link_libraries(photon-model-udpserver-${dev} ${lib})
            #target_link_libraries(photon-test-fwt-${dev} ${lib})
        endforeach()
    endforeach()
    photon_generate_sources(${_ARGS_PROJECT})
endmacro()

macro(photon_set_device_link_libraries dev)
    set(_PHOTON_${dev}_LINK_LIBRARIES ${ARGN})
endmacro()

