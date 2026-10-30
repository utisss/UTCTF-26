#!/usr/bin/env python3
import random
from Cryptodome.Util.number import getPrime, bytes_to_long

p, q = getPrime(256), getPrime(256)
N = p * q
e = 65537
d = pow(e, -1, (p - 1) * (q - 1))

print("Here's your entrance key!")
print(f"N = {N}")
print(f"e = {e}")

x0 = random.randint(1, N)
x1 = random.randint(1, N)

print("Here are my random x values!")
print(f"x0: {x0}")
print(f"x1: {x1}")

while True:
    try:
        print("Please pick a value k.")
        k = int(input())
        break
    except ValueError:
        print("Invalid value. Please pick an integer.")
        print("Please pick a value k.")
        k = int(input())

v = (x0 + (int(k) ^ e)) % N

k0 = pow(int(v) - x0, d, N)
k1 = pow(int(v) - x1, d, N)

file_content = open("/home/user/flag.txt", 'rb')
flag = file_content.read()

m0 = bytes_to_long(b"hgsynt{Pbatengf! Lbh pnhtug n erq ureevat!}")
m1 = bytes_to_long(flag)

r = random.randint(1, 2)

if (r == 1):
    c0 = (m0 + k0) % N
    c1 = (m1 + k1) % N 
else: 
    c0 = (m1 + k1) % N
    c1 = (m0 + k0) % N 

print("Here are your messages!")
print("Message 1: ", c0)
print("Message 2: ", c1)