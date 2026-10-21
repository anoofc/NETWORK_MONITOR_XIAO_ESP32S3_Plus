#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p tools/.build
c++ -std=c++11 -Wall -Wextra -pedantic -Iinclude -Isrc tests/model_tests.cpp -o tools/.build/model-tests
tools/.build/model-tests
