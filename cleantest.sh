#!/bin/bash

rm -rf ./build && mkdir build
cd build
cmake ..
make clean && make && ctest --output-on-failure .
