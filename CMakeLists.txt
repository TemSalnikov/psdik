# CMakeLists.txt для Data Server Application

cmake_minimum_required(VERSION 3.14)
project(DataServer VERSION 1.0.0 LANGUAGES CXX)

# Настройка компилятора
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Указываем путь к локальному Boost
set(BOOST_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/contrib/boost)
set(BOOST_INCLUDEDIR ${BOOST_ROOT})
set(BOOST_LIBRARYDIR ${BOOST_ROOT}/libs)

# Политики CMake
if(POLICY CMP0077)
    cmake_policy(SET CMP0077 NEW)
endif()

# Настройки сборки
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Опции проекта
option(BUILD_TESTS "Build tests" ON)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(ENABLE_COVERAGE "Enable code coverage" OFF)

# Пути для установки
include(GNUInstallDirs)

# Поиск зависимостей
find_package(PkgConfig REQUIRED)

# Boost
find_package(Boost REQUIRED 
    COMPONENTS 
    system 
    thread 
    chrono 
    filesystem 
    program_options
    unit_test_framework
)
target_link_libraries(data_server Boost::boost Boost::system Boost::filesystem)

# Threads
find_package(Threads REQUIRED)

# nlohmann_json (может быть установлен через package manager или как submodule)
find_package(nlohmann_json 3.9.0 REQUIRED)

# Проверка дополнительных библиотек для протоколов
find_library(MODBUS_LIBRARY NAMES modbus)
if(MODBUS_LIBRARY)
    message(STATUS "Found libmodbus: ${MODBUS_LIBRARY}")
    set(HAVE_LIBMODBUS 1)
else()
    message(WARNING "libmodbus not found - Modbus TCP support will be limited")
endif()

# Настройки компилятора
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # Предупреждения
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Werror")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnon-virtual-dtor -Woverloaded-virtual")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wold-style-cast -Wcast-align")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wunused -Wconversion -Wsign-conversion")
    
    # Оптимизация
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -DNDEBUG")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -DDEBUG")
    endif()
    
    # Покрытие кода
    if(ENABLE_COVERAGE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
    endif()
    
    # Специфичные для GCC флаги
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wduplicated-cond -Wduplicated-branches -Wlogical-op")
    endif()
    
    # Специфичные для Clang флаги  
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic")
    endif()
endif()

# Покрытие кода
if(ENABLE_COVERAGE)
    find_program(LCOV_PATH lcov)
    find_program(GENHTML_PATH genhtml)
    
    if(LCOV_PATH AND GENHTML_PATH)
        message(STATUS "Code coverage enabled")
    else()
        message(WARNING "Code coverage tools not found - disabling coverage")
        set(ENABLE_COVERAGE OFF)
    endif()
endif()

# Создание конфигурационного файла
configure_file(
    ${CMAKE_SOURCE_DIR}/config/config.json.in
    ${CMAKE_BINARY_DIR}/config.json
    @ONLY
)

# Включение поддиректорий
add_subdirectory(src)

# Тесты
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Документация
find_package(Doxygen)
if(DOXYGEN_FOUND)
    add_subdirectory(docs)
endif()

# Установка
install(
    FILES ${CMAKE_BINARY_DIR}/config.json
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/data_server
)

install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/examples/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/data_server/examples
)

# CPack настройка
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_VENDOR "Industrial Solutions")
set(CPACK_PACKAGE_VERSION_MAJOR "${DataServer_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${DataServer_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${DataServer_VERSION_PATCH}")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

include(CPack)