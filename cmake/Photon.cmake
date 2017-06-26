find_package(Qt5Widgets)
find_package(Qt5SerialPort)
find_package(Qt5Network)

macro(photon_init dir)
    set(PHOTON_GEN_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/_photon_gen_src)
    #set(_PHOTON_DEPENDS ${PHOTON_GEN_SRC_DIR}/Config.h)
    #set(_PHOTON_DEPENDS)
    set(_PHOTON_DIR ${dir})

    add_subdirectory(${_PHOTON_DIR}/thirdparty/decode EXCLUDE_FROM_ALL)
    add_subdirectory(${_PHOTON_DIR}/thirdparty/dtacan EXCLUDE_FROM_ALL)
endmacro()

macro(photon_generate_sources proj)
    set(PHOTON_GEN_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/_photon_gen_src)
    add_custom_command(
        OUTPUT ${_PHOTON_DEPENDS}
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${PHOTON_GEN_SRC_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PHOTON_GEN_SRC_DIR}
        COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:decode_gen> -p  ${proj} -o ${PHOTON_GEN_SRC_DIR} -c 4
        DEPENDS decode_gen ${proj} ${ARGN}
    )
endmacro()

macro(photon_add_library target)
    set(PHOTON_GEN_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/_photon_gen_src)
    string(SUBSTRING ${target} 0 1 _FIRST_LETTER)
    string(TOUPPER ${_FIRST_LETTER} _FIRST_LETTER)
    string(REGEX REPLACE "^.(.*)" "${_FIRST_LETTER}\\1" _SOURCE_NAME "${target}")

    set(_SRC_FILE ${PHOTON_GEN_SRC_DIR}/${_SOURCE_NAME}.c)
    set(_PHOTON_DEPENDS ${_PHOTON_DEPENDS} ${_SRC_FILE})

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

        set_target_properties(ui-test-${target}
            PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        )
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

    set_target_properties(photon-${target} model-${target}
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endmacro()

