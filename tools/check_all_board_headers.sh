#!/bin/bash

EXIT_CODE=0

for HEADER in src/boards/include/boards/*.h; do
    tools/check_board_header.py $HEADER
    if [[ $? -ne 0 ]]; then
      EXIT_CODE=1
    fi
done

exit $EXIT_CODE
