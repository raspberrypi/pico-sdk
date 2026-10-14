#!/bin/bash

if [[ $# -ge 2 ]]; then
    echo "error - usage is: ./partition_for_ffs.sh [image.uf2]"
    exit 2
fi

SLEEP_DELAY=3  # Time to ensure Pico is responsive after a reboot
PARTITION_FILES="ffs_pt"

echo "Attempting Pico reboot"
picotool reboot -uf
if [[ $? -ne 0 ]]; then
    echo "error - picotool failed to connect to device, exiting"
    exit 1
fi

sleep $SLEEP_DELAY

# Workaround lack of E10 errata handling in picotool:
# Need to request an `info` before requesting the erase.
echo "erasing DUT"
picotool info
picotool erase --range 0x10000000 0x10400000

echo "rebooting DUT"
picotool reboot -uf
sleep $SLEEP_DELAY

echo "partitioning DUT"
picotool partition create $PARTITION_FILES.json $PARTITION_FILES.uf2
picotool load -v $PARTITION_FILES.uf2

echo "rebooting DUT"
picotool reboot -uf  # essential to ensure the partition table is loaded
sleep $SLEEP_DELAY

picotool partition info

if [ $# -eq 1 ]; then
    echo "programming DUT with image: $1"
    picotool load -xv $1
fi

exit 0