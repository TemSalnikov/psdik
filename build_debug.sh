#!/bin/bash

# Сборка в режиме отладки с покрытием кода
BUILD_DIR="build_debug"

mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_TESTS=ON \
    -DENABLE_COVERAGE=ON

make -j$(nproc)

# Запуск тестов и генерация отчета о покрытии
make coverage

echo "Debug build with coverage completed!"
echo "Coverage report: ${BUILD_DIR}/coverage_report/index.html"