#!/bin/bash

cd "$(dirname "$0")"

export CK_FORK=no

cd ../build && make && cd tests && gdb check_cometd
