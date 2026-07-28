#!/bin/bash

EXIT_CODE=0

TEMP_CONFIGS=$(mktemp)
tools/extract_configs.py . $TEMP_CONFIGS 2>/dev/null
for HEADER in src/boards/include/boards/*.h; do
    tools/check_board_header.py $HEADER $TEMP_CONFIGS
    if [[ $? -ne 0 ]]; then
      EXIT_CODE=1
    fi
done
rm -f $TEMP_CONFIGS

exit $EXIT_CODE
