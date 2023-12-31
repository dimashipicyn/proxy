#!/bin/bash

echo "Configure started..."
cmake -S /code -B build --preset gcc-release
echo "Build started..."
cmake --build build --clean-first

echo "Server started..."
cd code
bin/proxy --host $PROXY_HOST --port $PROXY_PORT --db_host $POSTGRES_HOST --db_port $POSTGRES_PORT
