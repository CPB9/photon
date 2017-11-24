include(CPack)
include(DeployQt5)
include(CMakeParseArguments)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5SerialPort REQUIRED)
find_package(Qt5Network REQUIRED)

set(_PHOTON_MOD_DIRS)
set(PHOTON_GEN_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/_photon_gen_src)
set(PHOTON_GEN_SRC_ONBOARD_DIR ${PHOTON_GEN_SRC_DIR}/onboard)
set(PHOTON_GEN_SRC_GROUNDCONTROL_DIR ${PHOTON_GEN_SRC_DIR}/groundcontrol)


macro(photon_add_mod_dir dir)
    list(APPEND _PHOTON_MOD_DIRS ${dir})
endmacro()

if(GPERFTOOLS)
    find_library(GPERFTOOLS_PROFILER  NAMES profiler)
    set(PHOTON_PROFILER_LIB ${GPERFTOOLS_PROFILER})
endif()

macro(_photon_setup_target target)
    set_target_properties(${target}
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
    if (PHOTON_LOG_LEVEL)
        target_compile_options(${target} PUBLIC -DPHOTON_LOG_LEVEL=${PHOTON_LOG_LEVEL})
    endif()
endmacro()

set(_PHOTON_TESTS_DIR ${CMAKE_BINARY_DIR}/bin/tests)
file(MAKE_DIRECTORY ${_PHOTON_TESTS_DIR})

macro(_photon_add_unit_test target test file)
    add_executable(${test}-${target} ${_PHOTON_DIR}/tests/${file})
    target_link_libraries(${test}-${target}
        ${ARGN}
        gtest
        gtest_main
        photon-${target}
        decode
        photon-gc
    )

    target_include_directories(${test}-${target}
        PRIVATE
        ${_PHOTON_DIR}/thirdparty/gtest/include
    )

    set_target_properties(${test}-${target}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${_PHOTON_TESTS_DIR}
        FOLDER "tests"
    )
    if (NOT MSVC)
        target_compile_options(${test}-${target} PRIVATE -std=c++11)
    endif()
    add_test(${test}-${target} ${_PHOTON_TESTS_DIR}/${test}-${target})
endmacro()

macro(photon_init dir)
    if(PHOTON_ASAN OR ASAN)
        add_definitions(
            -fsanitize=address
            -fsanitize=undefined
            -ggdb
        )
        SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address -fsanitize=undefined")
    endif()

    find_package(Threads)
    #set(_PHOTON_DEPENDS ${PHOTON_GEN_SRC_DIR}/Config.h)
    set(_PHOTON_DEPENDS ${PHOTON_GEN_SRC_GROUNDCONTROL_DIR}/photon/Interface.hpp)
    set(_PHOTON_DEPENDS_H)
    set(_PHOTON_DIR ${dir})

    add_subdirectory(${_PHOTON_DIR}/thirdparty/decode EXCLUDE_FROM_ALL)
    add_subdirectory(${_PHOTON_DIR}/thirdparty/dtacan EXCLUDE_FROM_ALL)

    if (NOT GTEST_INCLUDED)
        add_subdirectory(${_PHOTON_DIR}/thirdparty/gtest EXCLUDE_FROM_ALL)
    endif()
    if (NOT ASIO_INCLUDED)
        add_subdirectory(${_PHOTON_DIR}/thirdparty/asio EXCLUDE_FROM_ALL)
    endif()

    # caf
    if (NOT CAF_INCLUDED) #for parent project
        set(CAF_NO_EXAMPLES 1)
        set(CAF_NO_BENCHMARKS 1)
        set(CAF_NO_PYTHON 1)
        set(CAF_NO_OPENCL 1)
        set(CAF_NO_TOOLS 1)
        set(CAF_NO_UNIT_TESTS 1)
        set(CAF_LOG_LEVEL 0)
        if (UNIX)
            set(CAF_CORE_LIBRARIES libcaf_core_shared)
            set(CAF_IO_LIBRARIES libcaf_io_shared)
        else()
            set(CAF_CORE_LIBRARIES libcaf_core_static)
            set(CAF_IO_LIBRARIES libcaf_io_static)
        endif()
        if (NOT MSVC)
            set(_TMP_FLAGS ${CMAKE_CXX_FLAGS})
            set(CMAKE_CXX_FLAGS "")
        endif()
        add_subdirectory(${_PHOTON_DIR}/thirdparty/caf EXCLUDE_FROM_ALL)
        if (NOT MSVC)
            set(CMAKE_CXX_FLAGS ${_TMP_FLAGS})
        endif()
        target_include_directories(${CAF_CORE_LIBRARIES} SYSTEM INTERFACE ${_PHOTON_DIR}/thirdparty/caf/libcaf_core)
    endif()

    install(FILES $<TARGET_FILE:decode_gen> DESTINATION bin)
    get_target_property(_TARGET_TYPE bmcl TYPE)
    if(_TARGET_TYPE STREQUAL  "SHARED_LIBRARY")
        if(WIN32)
            install(FILES $<TARGET_FILE:bmcl> DESTINATION bin)
        else()
            install(FILES $<TARGET_FILE:bmcl> DESTINATION lib)
        endif()
    elseif(_TARGET_TYPE STREQUAL "EXECUTABLE")
        install(FILES $<TARGET_FILE:bmcl> DESTINATION bin)
    endif()

    qt5_wrap_cpp(PHOTON_UI_TEST_MOC
        ${_PHOTON_DIR}/tests/UiTest.h
    )
    add_library(photon-ui-test
        ${_PHOTON_DIR}/tests/UiTest.cpp
        ${_PHOTON_DIR}/tests/UiTest.h
        ${PHOTON_UI_TEST_MOC}
    )
    target_link_libraries(photon-ui-test
        decode
        photon-gc
        Qt5::Core
    )
    add_executable(test-serialclient
        ${_PHOTON_DIR}/tests/ComTest.cpp
    )
    target_link_libraries(test-serialclient
        decode
        photon-gc
        bmcl
        photon-ui-test
        asio
        ${PHOTON_PROFILER_LIB}
        Qt5::Core
        Qt5::Widgets
        #Qt5::Network
        Qt5::SerialPort
    )
    target_include_directories(test-serialclient
        PRIVATE
        ${_PHOTON_DIR}/thirdparty/decode/thirdparty/tclap/include
    )
    _photon_setup_target(test-serialclient)
    _photon_install_target(test-serialclient)
    if (NOT MSVC)
        target_compile_options(test-serialclient PUBLIC -std=c++11)
    endif()
    add_executable(test-udpclient
        ${_PHOTON_DIR}/tests/UdpTest.cpp
    )
    target_link_libraries(test-udpclient
        decode
        photon-gc
        bmcl
        photon-ui-test
        asio
        ${PHOTON_PROFILER_LIB}
        Qt5::Core
        Qt5::Widgets
        Qt5::Network
    )
    target_include_directories(test-udpclient
        PRIVATE
        ${_PHOTON_DIR}/thirdparty/decode/thirdparty/tclap/include
    )
    _photon_setup_target(test-udpclient)
    _photon_install_target(test-udpclient)
    if (NOT MSVC)
        target_compile_options(test-udpclient PUBLIC -std=c++11)
    endif()

    set(DECODE_GROUNDCONTROL_SRC
        ${_PHOTON_DIR}/src/photon/groundcontrol/AllowUnsafeMessageType.h
        ${_PHOTON_DIR}/src/photon/groundcontrol/Atoms.h
        ${_PHOTON_DIR}/src/photon/groundcontrol/CmdState.cpp
        ${_PHOTON_DIR}/src/photon/groundcontrol/CmdState.h
        ${_PHOTON_DIR}/src/photon/groundcontrol/Crc.cpp
        ${_PHOTON_DIR}/src/photon/groundcontrol/Crc.h
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
    source_group("groundcontrol" FILES ${DECODE_GROUNDCONTROL_SRC})

    set(DECODE_MODEL_SRC
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
        ${_PHOTON_DIR}/src/photon/model/StatusDecoder.cpp
        ${_PHOTON_DIR}/src/photon/model/StatusDecoder.h
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
    source_group("model" FILES ${DECODE_MODEL_SRC})

    set(DECODE_UI_HEADERS
        ${_PHOTON_DIR}/src/photon/ui/FirmwareWidget.h
        ${_PHOTON_DIR}/src/photon/ui/FirmwareStatusWidget.h
        ${_PHOTON_DIR}/src/photon/ui/QModelBase.h
        ${_PHOTON_DIR}/src/photon/ui/QCmdModel.h
        ${_PHOTON_DIR}/src/photon/ui/QNodeModel.h
        ${_PHOTON_DIR}/src/photon/ui/QNodeViewModel.h
    )
    qt5_wrap_cpp(DECODE_UI_MOC
        ${DECODE_UI_HEADERS}
    )
    set(DECODE_UI_SRC
        ${_PHOTON_DIR}/src/photon/ui/FirmwareWidget.cpp
        ${_PHOTON_DIR}/src/photon/ui/FirmwareStatusWidget.cpp
        ${_PHOTON_DIR}/src/photon/ui/QModelBase.cpp
        ${_PHOTON_DIR}/src/photon/ui/QCmdModel.cpp
        ${_PHOTON_DIR}/src/photon/ui/QNodeModel.cpp
        ${_PHOTON_DIR}/src/photon/ui/QNodeViewModel.cpp
        ${DECODE_UI_MOC}
    )
    source_group("ui" FILES ${DECODE_UI_SRC})
    source_group("ui_moc" FILES ${DECODE_UI_MOC})

    add_library(photon-gc
        ${DECODE_UI_SRC}
        ${DECODE_GROUNDCONTROL_SRC}
        ${DECODE_MODEL_SRC}
        ${PHOTON_GEN_SRC_GROUNDCONTROL_DIR}/photon/Interface.hpp
    )

    target_link_libraries(photon-gc decode bmcl Qt5::Widgets ${CAF_CORE_LIBRARIES})
    target_compile_definitions(photon-gc PRIVATE -DBUILDING_PHOTON)

    set_target_properties(photon-gc
        PROPERTIES
        PREFIX "lib"
    )

    if (NOT MSVC)
        target_compile_options(photon-gc PUBLIC -std=c++11)
        target_compile_options(photon-gc PRIVATE -Wall -Wextra -Wno-unused-parameter)
    endif()

    target_include_directories(photon-gc
        PUBLIC
        ${_PHOTON_DIR}/src
        ${_PHOTON_DIR}/thirdparty
        ${PHOTON_GEN_SRC_GROUNDCONTROL_DIR}
    )
    _photon_setup_target(photon-gc)
    _photon_install_target(photon-gc)
endmacro()

function(_photon_install_target target)
    install(TARGETS ${target}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
endfunction()

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
        COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:decode_gen> -p  ${proj} -o ${PHOTON_GEN_SRC_DIR} -c 4 ${_PHOTON_ABS_FLAG}
        DEPENDS decode_gen ${proj} ${_PHOTON_MOD_SOURCES}
    )
    install(DIRECTORY ${PHOTON_GEN_SRC_ONBOARD_DIR}/photon DESTINATION gen)
    install(DIRECTORY ${PHOTON_GEN_SRC_GROUNDCONTROL_DIR}/photon DESTINATION gen)
    install(FILES ${_PHOTON_DEPENDS} ${_PHOTON_DEPENDS_H} DESTINATION gen)

    add_library(_photon-impl EXCLUDE_FROM_ALL ${_PHOTON_MOD_SOURCES})
    target_include_directories(_photon-impl PUBLIC ${_PHOTON_MOD_DIRS} ${PHOTON_GEN_SRC_ONBOARD_DIR})
endmacro()

macro(photon_add_device target)
    string(SUBSTRING ${target} 0 1 _FIRST_LETTER)
    string(TOUPPER ${_FIRST_LETTER} _FIRST_LETTER)
    string(REGEX REPLACE "^.(.*)" "${_FIRST_LETTER}\\1" _SOURCE_NAME "${target}")

    set(_SRC_FILE ${PHOTON_GEN_SRC_ONBOARD_DIR}/Photon${_SOURCE_NAME}.c)
    set(_PHOTON_DEPENDS ${_PHOTON_DEPENDS} ${_SRC_FILE})
    set(_PHOTON_DEPENDS_H ${_PHOTON_DEPENDS_H} ${PHOTON_GEN_SRC_ONBOARD_DIR}/Photon${_SOURCE_NAME}.h)

    add_library(photon-${target}
        ${_SRC_FILE}
    )

    set_target_properties(photon-${target}
        PROPERTIES
        PREFIX "lib"
    )

    if(NOT MSVC)
        target_compile_options(photon-${target} PRIVATE
            -std=c99
            -Wall
            -Wextra
            -Werror=implicit-function-declaration
            -Werror=incompatible-pointer-types
        )
    endif()

    if (PHOTON_ENABLE_STUB)
        target_compile_options(photon-${target} PUBLIC -DPHOTON_STUB)
    endif()

    target_include_directories(photon-${target}
        PUBLIC
        #${IMPL_DIR}
        ${PHOTON_GEN_SRC_ONBOARD_DIR}
    )
    if (PHOTON_USE_ABS_PATHS)
        target_include_directories(photon-${target}
            PUBLIC
            ${_PHOTON_MOD_DIRS}
        )
    endif()

    #TODO: check for other qt modules
    if (Qt5Widgets_FOUND)
        add_executable(test-inproc-${target} ${_PHOTON_DIR}/tests/InprocTest.cpp)
        target_link_libraries(test-inproc-${target} photon-${target}
            bmcl
            decode
            photon-gc
            photon-ui-test
            ${PHOTON_PROFILER_LIB}
            Qt5::Core
        )
        target_include_directories(test-inproc-${target}
            PRIVATE
            ${PHOTON_GEN_SRC_ONBOARD_DIR}
            ${_PHOTON_DIR}/thirdparty/decode/thirdparty/tclap/include
        )
        if(NOT MSVC)
            target_compile_options(test-inproc-${target} PRIVATE -std=c++11)
        endif()

        _photon_setup_target(test-inproc-${target})

        _photon_install_target(test-inproc-${target})
        if(WIN32)
            if(MINGW)
                install_qt_mingw_rt(bin)
            endif()
            install_qt5_platform(bin)
            if((Qt5Core_VERSION_MINOR LESS 7) AND MSVC)
                install_qt5_icu(bin)
            endif()
            install_qt5_lib(bin
                Core
                Gui
                Widgets
                Network
                SerialPort
            )
        endif()
    endif()

    add_executable(test-udpserver-${target} ${_PHOTON_DIR}/tests/Model.cpp)
    target_link_libraries(test-udpserver-${target} photon-${target} bmcl)
    if(NOT MSVC)
        target_compile_options(test-udpserver-${target} PRIVATE -std=c++11)
    endif()

    if(MSVC OR MINGW)
        target_link_libraries(test-udpserver-${target} ws2_32)
    endif()

    target_include_directories(test-udpserver-${target}
        PRIVATE
        ${_PHOTON_DIR}/thirdparty/decode/thirdparty/tclap/include
        ${PHOTON_GEN_SRC_ONBOARD_DIR}
    )

#     add_executable(test-perf-${target} ${_PHOTON_DIR}/tests/PerfTest.cpp)
#     target_link_libraries(test-perf-${target} photon-${target} decode photon-gc ${CMAKE_THREAD_LIBS_INIT})
#     target_include_directories(test-perf-${target}
#         PRIVATE
#         ${_PHOTON_DIR}/thirdparty/decode/thirdparty/tclap/include
#         ${PHOTON_GEN_SRC_ONBOARD_DIR}
#     )

    _photon_setup_target(photon-${target})
    _photon_setup_target(test-udpserver-${target})

    _photon_install_target(test-udpserver-${target})
    _photon_install_target(photon-${target})
    _photon_add_unit_test(${target} fwt-test FwtTest.cpp)
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
            target_link_libraries(test-udpserver-${dev} ${lib})
#            target_link_libraries(test-perf-${dev} ${lib})
            target_link_libraries(test-inproc-${dev} ${lib})
            target_link_libraries(fwt-test-${dev} ${lib})
        endforeach()
    endforeach()
    photon_generate_sources(${_ARGS_PROJECT})
endmacro()

macro(photon_set_device_link_libraries dev)
    set(_PHOTON_${dev}_LINK_LIBRARIES ${ARGN})
endmacro()

