if(NOT MSVC)
    add_definitions(-std=c++11)
endif()

add_executable(model Model.cpp)
target_link_libraries(model photon bmcl)

find_package(Qt5Widgets)
find_package(Qt5SerialPort)
find_package(Qt5Network)

if (Qt5Widgets_FOUND)
    add_executable(ui_test UiTest.cpp)
    target_link_libraries(ui_test photon bmcl dtacan decode Qt5::Widgets Qt5::Network Qt5::SerialPort)
endif()

if(MSVC OR MINGW)
    target_link_libraries(model ws2_32)
endif()

include_directories(
    SYSTEM
    ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/decode/thirdparty/gtest/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/decode/thirdparty/tclap/include
)

set(TESTS_DIR ${CMAKE_BINARY_DIR}/bin/tests)
file(MAKE_DIRECTORY ${TESTS_DIR})

macro(add_unit_test test file)
    add_executable(${test} ${file})
    target_link_libraries(${test}
        ${ARGN}
        gtest
        gtest_main
        photon
        decode
    )

    set_target_properties(${test}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${TESTS_DIR}
        FOLDER "tests"
    )
    add_test(${test} ${TESTS_DIR}/${test})
endmacro()

if (WIN32)
    add_custom_command(
        TARGET ui_test
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:bmcl>
        ${CMAKE_CURRENT_BINARY_DIR}
    )
endif()

#add_unit_test(memintervalset_tests MemIntervalSet.cpp)
add_unit_test(fwt_test Fwt.cpp)
