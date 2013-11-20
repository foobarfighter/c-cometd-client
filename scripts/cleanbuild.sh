#!/bin/bash

set -e

cd "$(dirname "$0")"

rm -rf ../build && mkdir ../build
cd ../build
cmake ..
make clean && make
