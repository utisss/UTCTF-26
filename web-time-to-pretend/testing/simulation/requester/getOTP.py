import requests
import time
import random
import sys

URL = "http://"+sys.argv[1]+"/debug/getOTP"

# Load usernames from file
with open("usernames.txt", "r") as f:
    all_usernames = [line.strip() for line in f if line.strip()]

# Randomly choose 100 usernames
selected_usernames = random.sample(all_usernames, min(100, len(all_usernames)))

for username in selected_usernames:
    tim = int(time.time())
    payload = {
        "username": username,
        "epoch": tim
    }

    response = requests.post(URL, json=payload)
    print(f"[{tim}] {username}: {response.json()}")

    # Random delay between requests (0.5 to 3 seconds)
    delay = random.uniform(0.5, 3.0)
    time.sleep(delay)



# VALID_A = [1,3,5,7,9,11,15,17,19,21,23,25]

# def affine_encrypt(text, a, b):
#     out = ""
#     for c in text:
#         x = ord(c) - 97
#         y = (a*x + b) % 26
#         out += chr(y + 97)
#     return out

# def generate(epoch):
#     a = VALID_A[epoch % len(VALID_A)]
#     b = epoch % 26
#     otp = affine_encrypt("admin", a, b)
#     return {"a": a, "b": b, "otp": otp}
