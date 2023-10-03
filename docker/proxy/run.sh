#!/bin/bash

echo "Configure started..."
cmake -S /code -B build --preset gcc-release
echo "Build started..."
cmake --build build

echo "Server started..."
/code/bin/proxy
