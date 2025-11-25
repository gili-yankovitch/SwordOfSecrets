#!/bin/bash

function fail() {
    echo "Failed to flash"
    exit 1
}

echo "Erasing device and flashing..."

minichlink -p && make firmware.bin flash || fail

echo "Waiting for device to boot..."
sleep 3

echo "Performing self-test..."
./src/tool/sanity_test.py || fail
