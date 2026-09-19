include(GoogleTest)
find_package(Threads REQUIRED)

option(VM_DEBUG_PRINTS "Enable debug prints in VM during tests" OFF)
option(ENABLE_COVERAGE "Enable coverage instrumentation for tests" OFF)

file(GLOB_RECURSE TEST_FILES CONFIGURE_DEPENDS "*.cpp")

add_executable(kites_test
    ${TEST_FILES}
)

target_link_libraries(kites_test PRIVATE
    kites_core
    GTest::gtest
    GTest::gmock_main
    Threads::Threads
)

if(DISABLE_GUI)
    target_compile_definitions(kites_test PRIVATE DISABLE_GUI)
endif()

if(VM_DEBUG_PRINTS)
    target_compile_definitions(kites_test PRIVATE VM_DEBUG_PRINTS)
endif()

gtest_discover_tests(kites_test
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    PROPERTIES
        FAIL_REGULAR_EXPRESSION ".*FAILED.*"
)

# ---------------------------------------------------------------------------
# kites_test_cov — same suite, built with coverage instrumentation.
# ---------------------------------------------------------------------------
if(ENABLE_COVERAGE)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "ENABLE_COVERAGE requires a GCC- or Clang-style compiler")
    endif()

    find_program(GCOVR_EXECUTABLE gcovr REQUIRED)

    add_executable(kites_test_cov
    ${TEST_FILES}
    )

    target_link_libraries(kites_test_cov PRIVATE
    kites_core
    GTest::gtest
    GTest::gmock_main
    Threads::Threads
    )

    if(DISABLE_GUI)
        target_compile_definitions(kites_test_cov PRIVATE DISABLE_GUI)
    endif()

    if(VM_DEBUG_PRINTS)
        target_compile_definitions(kites_test_cov PRIVATE VM_DEBUG_PRINTS)
    endif()

    gtest_discover_tests(kites_test_cov
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        TEST_PREFIX "cov_"
        PROPERTIES
            FAIL_REGULAR_EXPRESSION ".*FAILED.*"
    )

    target_compile_options(kites_test_cov PRIVATE -O0 --coverage)
    target_compile_options(kites_core PRIVATE -O0 --coverage)
    
    target_link_libraries(kites_test_cov PRIVATE gcov)

    set(COVERAGE_DIR ${CMAKE_BINARY_DIR}/coverage)
    add_custom_target(coverage
        COMMAND ${CMAKE_COMMAND} -E make_directory ${COVERAGE_DIR}
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure --test-dir ${CMAKE_BINARY_DIR}
        COMMAND ${GCOVR_EXECUTABLE}
            --root ${CMAKE_SOURCE_DIR}
            --filter ${CMAKE_SOURCE_DIR}/src/.*
            --html --html-details
            --output ${COVERAGE_DIR}/index.html
            --exclude .*/external/.*
            --exclude .*/_deps/.*
            --exclude .*/${PROJECT_NAME}_autogen/.*
        DEPENDS kites_test_cov
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running tests and generating coverage report"
    )
endif()