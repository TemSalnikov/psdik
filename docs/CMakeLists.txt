# docs/CMakeLists.txt

if(DOXYGEN_FOUND)
    # Настройка Doxygen
    set(DOXYGEN_INPUT ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/include)
    set(DOXYGEN_OUTPUT ${CMAKE_BINARY_DIR}/docs)
    set(DOXYGEN_HTML_OUTPUT ${DOXYGEN_OUTPUT}/html)
    
    # Создание конфигурационного файла Doxygen
    configure_file(
        ${CMAKE_SOURCE_DIR}/docs/Doxyfile.in
        ${CMAKE_BINARY_DIR}/Doxyfile
        @ONLY
    )
    
    # Цель для генерации документации
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
    
    # Установка документации
    install(
        DIRECTORY ${DOXYGEN_HTML_OUTPUT}
        DESTINATION ${CMAKE_INSTALL_DOCDIR}/html
    )
endif()