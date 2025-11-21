#!/bin/bash

set -e

# Параметры сборки
BUILD_TYPE="${1:-Release}"
BUILD_DIR="build_${BUILD_TYPE}"
INSTALL_PREFIX="${2:-/usr/local}"

echo "Building Data Server in ${BUILD_TYPE} mode..."
echo "Install prefix: ${INSTALL_PREFIX}"

# Создание директории сборки
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

# Конфигурация CMake
cmake .. \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} \
    -DBUILD_TESTS=ON \
    -DENABLE_COVERAGE=OFF

# Сборка
make -j$(nproc)

# Тесты
echo "Running tests..."
make test

# Установка (опционально)
if [ "$3" == "install" ]; then
    echo "Installing to ${INSTALL_PREFIX}"
    sudo make install
fi

echo "Build completed successfully!"