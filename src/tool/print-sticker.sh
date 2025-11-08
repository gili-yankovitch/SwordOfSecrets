#!/bin/bash

brother_ql -b pyusb -p usb://0x04f9:0x2020 --model QL-1050 print -l 102 $1
