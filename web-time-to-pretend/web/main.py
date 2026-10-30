from flask import Flask, request, session, redirect, url_for, send_file, abort, make_response, jsonify
import time
from math import gcd
app = Flask(__name__)
app.secret_key = "f5b3593ea8578c1343d7efcc24ecd469ee7d4f259b8ac3d0"

VALID_A = [1, 3, 5, 7, 9, 11, 15, 17, 19, 21, 23, 25]

def affine_keys_from_epoch(epoch: int) -> tuple[int, int]:
    a = VALID_A[(epoch) % len(VALID_A)]
    b = epoch % 26
    return a, b

def encrypt(plaintext: str, a: int, b: int) -> str:
    result = []
    for ch in plaintext:
        if ch.isalpha():
            base = ord('A') if ch.isupper() else ord('a')
            result.append(chr((a * (ord(ch) - base) + b) % 26 + base))
        else:
            result.append(ch)
    return ''.join(result)

def decrypt_at_time(ciphertext: str, epoch: int) -> str:
    a, b = affine_keys_from_epoch(epoch)

    # find modular inverse of a mod 26
    a_inv = next(i for i in range(26) if (a * i) % 26 == 1)

    result = []
    for ch in ciphertext:
        if ch.isalpha():
            base = ord('A') if ch.isupper() else ord('a')
            result.append(chr((a_inv * ((ord(ch) - base) - b)) % 26 + base))
        else:
            result.append(ch)
    return ''.join(result)

def decrypt_brute(ciphertext: str, window_seconds: int = 7200) -> list[tuple[int, str]]:
    """
    Try all epochs within window_seconds of now.
    Returns a list of (epoch, plaintext) for every distinct decryption result.
    Useful when you know roughly when the ciphertext was produced.
    """
    now = int(time.time())
    seen = set()
    results = []

    for epoch in range(now - window_seconds, now + 1):
        a, b = affine_keys_from_epoch(epoch)
        key = (a, b)
        if key in seen:
            continue
        seen.add(key)
        results.append((epoch, decrypt_at_time(ciphertext, epoch)))

    return results

@app.route("/")
def index():
    return send_file("index.html")

@app.route("/urgent.txt")
def urgent():
    return send_file("URGENT.txt")

@app.route("/portal")
def portal():
    if not session.get("authenticated"):
        abort(403)
    response = make_response(send_file("portal.html"))
    response.headers["X-Admin-Access"] = "granted"
    return response

@app.route("/auth", methods=["POST"])
def auth():
    if not request.is_json:
        return jsonify({"error": "Invalid JSON"}), 400

    data = request.get_json(silent=True)
    if not data or "username" not in data or "otp" not in data:
        return jsonify({"error": "Missing fields"}), 400

    username = data["username"]
    otp = data["otp"]

    if username != "timothy":
        return jsonify({"error": "Authentication failed"}), 401

    # Check current epoch and ±3 seconds for clock skew tolerance
    now = int(time.time())
    for epoch in range(now - 3, now + 4):
        a, b = affine_keys_from_epoch(epoch)
        expected = encrypt("timothy", a, b)
        if otp == expected:
            session["authenticated"] = True
            return jsonify({"message": "Access granted"}), 200

    return jsonify({"error": "Authentication failed"}), 401

# not meant for actual exploitation, this was for http traffic generation
# @app.route("/debug/getOTP", methods=["POST"])
# def get_otp():
#     if not request.is_json:
#         return jsonify({"error": "Invalid JSON"}), 400

#     data = request.get_json(silent=True)
#     if not data or "username" not in data:
#         return jsonify({"error": "Missing 'username' field"}), 400

#     username = data["username"]
#     epoch = data["epoch"]
#     a, b = affine_keys_from_epoch(int(epoch))
#     otp = encrypt(username, a, b)
#     return jsonify({"otp": otp, "mult": a, "add": b})

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=9382, debug=True)