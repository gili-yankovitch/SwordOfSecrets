#!/usr/bin/python3

from Crypto.Cipher import AES

key = bytes([ x for x in range(AES.block_size) ])
