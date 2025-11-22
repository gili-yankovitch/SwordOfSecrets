#!/usr/bin/python3

from Crypto.Cipher import AES

key = bytes([ 0 for x in range(AES.block_size) ])
