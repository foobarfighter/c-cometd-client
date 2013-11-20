#!/bin/bash

cd "$(dirname "$0")"

export G_SLICE=always-malloc
export CK_FORK=no

node ../tests/test_server/server.js &> ../build/test_server.log &

valgrind --tool=memcheck \
         --leak-check=full \
         --leak-resolution=high \
         --suppressions=./cometd_suppressions.supp \
         --gen-suppressions=yes \
         ../build/tests/check_cometd

kill `ps aux | grep test_server | grep -v grep | \
      awk '{ print $2 }'` &> /dev/null
