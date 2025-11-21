#!/bin/bash

# Сборка и запуск тестов
echo "Building tests..."
mkdir -p build
cd build
cmake ..
make -j4

echo "Running unit tests..."
./run_tests --gtest_output="xml:test_results.xml"

echo "Running performance tests..."
./run_tests --gtest_filter="PerformanceTest*"

echo "Running thread safety tests..."
./run_tests --gtest_filter="ThreadSafetyTest*"

# Генерация отчета о покрытии (если настроено)
if command -v gcov &> /dev/null; then
    echo "Generating coverage report..."
    gcov -r ../*.cpp
fi