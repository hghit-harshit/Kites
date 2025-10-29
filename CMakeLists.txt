cmake_minimum_required(VERSION 3.19)
project(RISC_V_Simulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
#tod change it to fetch content later
set( CMAKE_PREFIX_PATH "D:/QT/6.9.2/mingw_64/lib/cmake")
set(MINGW_PATH "D:/Qt/Tools/mingw1310_64")
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# This makes GTest's targets (like GTest::gtest_main) available
FetchContent_MakeAvailable(googletest)

qt_standard_project_setup()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(SRC_DIR "src")
set(INCLUDE_DIR "include")
set(TEST_DIR "test")

set(CMAKE_AUTOUIC_SEARCH_PATHS ${INCLUDE_DIR} ${SRC_DIR}/ui)

file(GLOB_RECURSE SRC_FILES "${SRC_DIR}/*.cpp")
file(GLOB_RECURSE TEST_FILES "${TEST_DIR}/*.cpp")
file(GLOB_RECURSE UI_FILES "${SRC_DIR}/*.ui")
file(GLOB_RECURSE HEADER_FILES
    "${INCLUDE_DIR}/*.h"
    "${SRC_DIR}/*.h")

qt_add_executable(RISC_V_Simulator
    WIN32 MACOSX_BUNDLE
    main.cpp
    src/ui/editortab.cpp
    src/ui/kitestab.cpp
    src/ui/registercontainer.cpp
    ${HEADER_FILES}
    ${SRC_FILES}
    ${UI_FILES}
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    <cstdint>
    <string>
    <array>
)


target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIR} "${PROJECT_BINARY_DIR}/${PROJECT_NAME}_autogen/include")
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -frounding-math -ffloat-store -g -O3)
target_link_libraries(RISC_V_Simulator
    PRIVATE
        Qt::Core
        Qt::Widgets
)

include(GNUInstallDirs)

install(TARGETS RISC_V_Simulator
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET RISC_V_Simulator
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

# tests
option(ENABLE_TESTS "Build tests" OFF)

if(ENABLE_TESTS)
    #find_package(GTest REQUIRED)
    include_directories(${GTEST_INCLUDE_DIRS})
    #list(REMOVE_ITEM SRC_FILES "${CMAKE_SOURCE_DIR}/main.cpp")
    add_executable(tests 
    ${HEADER_FILES}
    ${SRC_FILES}
    ${UI_FILES}
    ${TEST_FILES})
    target_include_directories(tests PRIVATE ${INCLUDE_DIR} "${PROJECT_BINARY_DIR}/${PROJECT_NAME}_autogen/include")
    target_link_libraries(tests 
    GTest::gtest 
    GTest::gmock_main
    Qt::Core
    Qt::Widgets
    pthread)

    add_custom_command(TARGET tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:Qt6::Core>"
        "$<TARGET_FILE:Qt6::Widgets>"
        "$<TARGET_FILE:Qt6::Gui>"
        "${MINGW_PATH}/bin/libstdc++-6.dll"
        "${MINGW_PATH}/bin/libgcc_s_seh-1.dll"
        "${MINGW_PATH}/bin/libwinpthread-1.dll"
        "$<TARGET_FILE_DIR:tests>"
    )


    add_custom_target(test_run
        COMMAND ./tests
        DEPENDS tests
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()

