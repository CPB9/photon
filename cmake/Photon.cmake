include(CPack)
include(DeployQt5)
find_package(Qt5Widgets)
find_package(Qt5SerialPort)
find_package(Qt5Network)

macro(photon_init dir)
    set(PHOTON_GEN_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/_photon_gen_src)
    #set(_PHOTON_DEPENDS ${PHOTON_GEN_SRC_DIR}/Config.h)
    set(_PHOTON_DEPENDS)
    set(_PHOTON_DEPENDS_H)
    set(_PHOTON_DIR ${dir})

    add_subdirectory(${_PHOTON_DIR}/thirdparty/decode EXCLUDE_FROM_ALL)
    add_subdirectory(${_PHOTON_DIR}/thirdparty/dtacan EXCLUDE_FROM_ALL)
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
endmacro()

function(_photon_install_target target)
    install(TARGETS ${target}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
endfunction()

macro(photon_generate_sources proj)
    set(PHOTON_GEN_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/_photon_gen_src)
    add_custom_command(
        OUTPUT ${_PHOTON_DEPENDS}
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${PHOTON_GEN_SRC_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PHOTON_GEN_SRC_DIR}
        COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:decode_gen> -p  ${proj} -o ${PHOTON_GEN_SRC_DIR} -c 4
        DEPENDS decode_gen ${proj} ${ARGN}
    )
    install(DIRECTORY ${PHOTON_GEN_SRC_DIR}/photon DESTINATION gen)
    install(FILES ${_PHOTON_DEPENDS} ${_PHOTON_DEPENDS_H} DESTINATION gen)
endmacro()

macro(_photon_setup_target target)
    set_target_properties(${target}
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endmacro()

macro(photon_add_device target)
    set(PHOTON_GEN_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/_photon_gen_src)
    string(SUBSTRING ${target} 0 1 _FIRST_LETTER)
    string(TOUPPER ${_FIRST_LETTER} _FIRST_LETTER)
    string(REGEX REPLACE "^.(.*)" "${_FIRST_LETTER}\\1" _SOURCE_NAME "${target}")

    set(_SRC_FILE ${PHOTON_GEN_SRC_DIR}/${_SOURCE_NAME}.c)
    set(_PHOTON_DEPENDS ${_PHOTON_DEPENDS} ${_SRC_FILE})
    set(_PHOTON_DEPENDS_H ${_PHOTON_DEPENDS_H} ${PHOTON_GEN_SRC_DIR}/${_SOURCE_NAME}.h)

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

    target_compile_options(photon-${target} PUBLIC -DPHOTON_STUB)

    target_include_directories(photon-${target}
        PUBLIC
        #${IMPL_DIR}
        ${PHOTON_GEN_SRC_DIR}
    )

    #TODO: check for other qt modules
    if (Qt5Widgets_FOUND)
        add_executable(ui-test-${target} ${_PHOTON_DIR}/tests/UiTest.cpp)
        target_link_libraries(ui-test-${target} photon-${target} bmcl dtacan decode Qt5::Widgets Qt5::Network Qt5::SerialPort)
        target_include_directories(ui-test-${target}
            PUBLIC
            ${_PHOTON_DIR}/thirdparty/decode/thirdparty/tclap/include
            PRIVATE
            ${PHOTON_GEN_SRC_DIR}
        )
        if(NOT MSVC)
            target_compile_options(ui-test-${target} PRIVATE -std=c++11)
        endif()

        _photon_setup_target(ui-test-${target})

        _photon_install_target(ui-test-${target})
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

    add_executable(model-${target} ${_PHOTON_DIR}/tests/Model.cpp)
    target_link_libraries(model-${target} photon-${target} bmcl)

    if(MSVC OR MINGW)
        target_link_libraries(model-${target} ws2_32)
    endif()

    target_include_directories(model-${target}
        PUBLIC
        ${_PHOTON_DIR}/thirdparty/decode/thirdparty/tclap/include
        PRIVATE
        ${PHOTON_GEN_SRC_DIR}
    )

    _photon_setup_target(photon-${target})
    _photon_setup_target(model-${target})

    _photon_install_target(model-${target})
    _photon_install_target(photon-${target})
endmacro()

