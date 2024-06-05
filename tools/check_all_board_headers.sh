#!/bin/bash
for HEADER in src/boards/include/boards/*.h; do
    tools/check_board_header.py $HEADER
    if [[ $? -ne 0 ]]; then
      break
    fi
done
