from flask import Flask, request, jsonify, send_file, make_response, abort

import time
import hashlib
import os

app = Flask(__name__)
app.secret_key = os.urandom(32)

# { team_token: { "first_seen": float } }
request_log = {}

WINDOW_SECONDS = 300  # 5 minutes

# Absolute paths
APP_DIR = os.path.dirname(os.path.abspath(__file__))   # /app
WEB_DIR = os.path.join(APP_DIR, "webfiles")                 # /app/web  — traversal ceiling


def get_team_token():
    token = request.cookies.get("team_token")
    if not token:
        token = hashlib.sha256(os.urandom(16)).hexdigest()[:16]
    return token


@app.route("/")
def index():
    token = get_team_token()
    resp = make_response(send_file(os.path.join(WEB_DIR, "index.html")))
    resp.set_cookie("team_token", token, httponly=True, samesite="Lax")
    return resp


@app.route("/getFlag")
def get_flag():
    """
    Intended route: /getFlag?f=flag.txt
    Vulnerability:  path traversal within web/ only
                    e.g. /getFlag?f=../index.html  works
                         /getFlag?f=../../main.py  → 403 (above web/)

    flag.txt is only served after the 2-minute window with no competing teams.
    All other reachable files are served freely.
    """
    token = get_team_token()
    filename = request.args.get("f", "")

    # ----------------------------------------------------------------
    # PATH TRAVERSAL VULNERABILITY
    # Base is web/static/ but traversal up to web/ is permitted.
    # Anything above web/ is blocked.
    # ----------------------------------------------------------------
    base_dir = os.path.join(WEB_DIR, "static")
    target = os.path.normpath(os.path.join(base_dir, filename))

    # Jail: must stay within web/
    if not target.startswith(WEB_DIR + os.sep) and target != WEB_DIR:
        abort(403)

    # Only gate flag.txt behind the window logic
    if not target.endswith("flag.txt"):
        if os.path.isfile(target):
            return send_file(target, as_attachment=True)
        abort(404)

    # ---- flag.txt gating logic ----
    now = time.time()

    # Clean up stale entries older than 10 minutes
    stale = [k for k, v in request_log.items() if now - v["first_seen"] > 600]
    for k in stale:
        del request_log[k]

    # Check how many OTHER teams currently have an active window
    active_others = [t for t in request_log if t != token]

    if active_others:
        # Another team is in the queue — cancel EVERYONE
        for t in active_others:
            del request_log[t]
        if token in request_log:
            del request_log[token]
        return jsonify({
            "status": "cancelled",
            "message": "Another team requested the flag at the same time. All requests have been cancelled. Start over."
        }), 409

    if token not in request_log:
        # First time — start their window
        request_log[token] = {"first_seen": now}
        return jsonify({
            "status": "waiting",
            "message": "Request logged. Come back in 5 minutes and click again to claim your flag.",
            "wait_until": int(now + WINDOW_SECONDS)
        }), 202

    entry = request_log[token]
    elapsed = now - entry["first_seen"]

    if elapsed < WINDOW_SECONDS:
        remaining = int(WINDOW_SECONDS - elapsed)
        return jsonify({
            "status": "waiting",
            "message": f"Still waiting. {remaining} seconds remaining.",
            "remaining": remaining
        }), 202

    # Window elapsed and no other teams in queue — deliver the flag
    del request_log[token]
    return send_file(os.path.join(WEB_DIR, "static", "flag.txt"))


@app.route("/status")
def status():
    token = get_team_token()
    now = time.time()

    resp_data = {"has_pending": False}

    if token in request_log:
        entry = request_log[token]
        elapsed = now - entry["first_seen"]
        remaining = max(0, WINDOW_SECONDS - elapsed)
        resp_data = {
            "has_pending": True,
            "remaining": int(remaining),
        }

    resp = make_response(jsonify(resp_data))
    resp.set_cookie("team_token", token, httponly=True, samesite="Lax")
    return resp


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5888, debug=False)
