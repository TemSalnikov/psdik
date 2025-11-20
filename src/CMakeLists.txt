# Исходные файлы
# set(SOURCES
#     Logger.cpp
#     DataCache.cpp
#     ProtocolHandler.cpp
#     ModbusTcpHandler.cpp
#     IEC104Handler.cpp
#     SNMPHandler.cpp
#     SubscriptionManager.cpp
#     DataServer.cpp
#     main.cpp
# )

set(SOURCES
    ./src/psdik.cpp
)

# Заголовочные файлы
# set(HEADERS
#     Logger.h
#     DataCache.h
#     ProtocolHandler.h
#     ModbusTcpHandler.h
#     IEC104Handler.h
#     SNMPHandler.h
#     SubscriptionManager.h
#     DataServer.h
#     version.h
# )

set(HEADERS
    ./include/psdik.h
)

# Создание библиотеки (опционально)
add_library(data_server_lib STATIC ${SOURCES})

# Основной исполняемый файл
add_executable(data_server main.cpp)

# Связывание зависимостей
target_link_libraries(data_server
    PRIVATE
        data_server_lib
        Boost::boost
        Boost::system
        Boost::thread
        Boost::chrono
        Boost::filesystem
        Boost::program_options
        nlohmann_json::nlohmann_json
        Threads::Threads
)

# Дополнительные библиотеки для протоколов
if(HAVE_LIBMODBUS)
    target_link_libraries(data_server PRIVATE ${MODBUS_LIBRARY})
    target_compile_definitions(data_server PRIVATE HAVE_LIBMODBUS)
endif()

# Установка
install(TARGETS data_server
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES ${HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/data_server
)

# Компиляционные определения
target_compile_definitions(data_server
    PRIVATE
        BOOST_ALL_NO_LIB
        BOOST_ASIO_STANDALONE
        _WEBSOCKETPP_CPP11_STL_
)

# Включение директорий
target_include_directories(data_server
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}
)

# Свойства цели
set_target_properties(data_server PROPERTIES
    OUTPUT_NAME "data-server"
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Генерация файла версии
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_BINARY_DIR}/version.h
    @ONLY
)