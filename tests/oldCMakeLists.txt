cmake_minimum_required(VERSION 3.14)
project(DataServerTests)

# Настройка компилятора
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Поиск зависимостей
find_package(Boost REQUIRED COMPONENTS system unit_test_framework)
find_package(Threads REQUIRED)

# Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9fdabf8a2def62d6999fdd9.zip
)
FetchContent_MakeAvailable(googletest)

# Компиляция основной программы
add_library(data_server_core
    Logger.cpp
    DataCache.cpp
    ProtocolHandler.cpp
    ModbusTcpHandler.cpp
    DataServer.cpp
)

# Основные тесты
add_executable(run_tests
    test_main.cpp
    test_logger.cpp
    test_datacache.cpp
    test_protocols.cpp
    test_integration.cpp
    test_performance.cpp
)

# Связывание зависимостей
target_link_libraries(data_server_core
    PRIVATE Boost::boost nlohmann_json::nlohmann_json
)

target_link_libraries(run_tests
    PRIVATE
        data_server_core
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        Boost::system
        ${CMAKE_THREAD_LIBS_INIT}
)

# Добавление тестов в CTest
include(GoogleTest)
gtest_discover_tests(run_tests)

# Настройки компиляции
target_compile_options(run_tests PRIVATE -Wall -Wextra -Werror -O2)
target_compile_definitions(run_tests PRIVATE BOOST_ALL_NO_LIB)

# Целевые имена для удобства
add_custom_target(test
    DEPENDS run_tests
    COMMAND ./run_tests
)