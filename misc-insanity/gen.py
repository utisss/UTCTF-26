import random
import string


flag = "utflag{I'm_not_a_robot_I_promise}"
otp = "".join(random.choices(string.ascii_lowercase + string.digits, k=len(flag)))


def encrypt(flag, otp):
    encrypted = []
    for f, o in zip(flag, otp):
        encrypted.append(ord(f) ^ ord(o))
    return encrypted


for char in flag:
    print(ord(char), end=", ")
print()

for byte in otp:
    print(ord(byte), end=", ")
print()

for byte in encrypt(flag, otp):
    print(byte, end=", ")
print()

print(f"{random.randint(0, 2 ** 32)}.html")
print(f"{random.randint(0, 2 ** 32)}.html")
