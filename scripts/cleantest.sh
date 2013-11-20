#!/bin/bash

set -e

cd "$(dirname "$0")"

rm -rf ../build && mkdir ../build
cd ../build
cmake ..
node ../tests/test_server/server.js &> test_server.log &
make clean && make && ctest --output-on-failure .
kill `ps aux | grep test_server | grep -v grep | awk '{ print $2 }'` &> /dev/null
