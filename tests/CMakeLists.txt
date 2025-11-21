# tests/CMakeLists.txt

# Включение Google Test
include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)

FetchContent_MakeAvailable(googletest)

# Тестовые исходные файлы
set(TEST_SOURCES
    test_main.cpp
    test_Logger.cpp
    test_DataCache.cpp
    test_ProtocolHandler.cpp
    test_ModbusTcpHandler.cpp
    test_DataServer.cpp
    test_Integration.cpp
    test_Performance.cpp
    test_ThreadSafety.cpp
    test_utilities.cpp
)

# Создание тестового исполняемого файла
add_executable(run_tests ${TEST_SOURCES})

# Связывание зависимостей для тестов
target_link_libraries(run_tests
    PRIVATE
        data_server_lib
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        Boost::unit_test_framework
        nlohmann_json::nlohmann_json
        Threads::Threads
)

# Компиляционные определения для тестов
target_compile_definitions(run_tests
    PRIVATE
        TEST_BUILD
        BOOST_TEST_DYN_LINK
)

# Включение директорий
target_include_directories(run_tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_BINARY_DIR}
        ${googletest_SOURCE_DIR}/googletest/include
        ${googletest_SOURCE_DIR}/googlemock/include
)

# Добавление тестов в CTest
include(GoogleTest)
gtest_discover_tests(run_tests
    EXTRA_ARGS
        --gtest_output=xml:${CMAKE_BINARY_DIR}/test_results.xml
)

# Цели для удобства
add_custom_target(test
    DEPENDS run_tests
    COMMAND ${CMAKE_BINARY_DIR}/run_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_custom_target(test-verbose
    DEPENDS run_tests
    COMMAND ${CMAKE_BINARY_DIR}/run_tests --gtest_color=yes
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Покрытие кода
if(ENABLE_COVERAGE)
    add_custom_target(coverage
        DEPENDS run_tests
        COMMAND lcov --capture --directory . --output-file coverage.info
        COMMAND lcov --remove coverage.info '/usr/*' '${CMAKE_SOURCE_DIR}/tests/*' --output-file coverage.info
        COMMAND genhtml coverage.info --output-directory ${CMAKE_BINARY_DIR}/coverage_report
        COMMAND echo "Coverage report generated in ${CMAKE_BINARY_DIR}/coverage_report"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()