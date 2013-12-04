#!/bin/bash

cd "$(dirname "$0")"

export CK_FORK=no

cd ../build && make && gdb tests/check_cometd
