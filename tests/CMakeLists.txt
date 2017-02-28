if(NOT MSVC)
    add_definitions(-std=c++11)
endif()

add_executable(model Model.cpp)
target_link_libraries(model photon bmcl)

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
        photon-gc
        gtest
        gtest_main
    )

    set_target_properties(${test}
        PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${TESTS_DIR}
        FOLDER "tests"
    )
    add_test(${test} ${TESTS_DIR}/${test})
endmacro()

#add_unit_test(memintervalset_tests MemIntervalSet.cpp)
