#!/usr/bin/env python3

import os
import sys
import pathlib
import yaml
import requests

CTFD_URL = os.environ.get("CTFD_URL", "").rstrip("/")
CTFD_TOKEN = os.environ.get("CTFD_TOKEN", "")
CTFD_STATE = os.environ.get("CTFD_STATE", "")

if not CTFD_URL or not CTFD_TOKEN:
    print("Error: CTFD_URL and CTFD_TOKEN environment variables are required")
    sys.exit(2)

session = requests.Session()
session.headers.update({
    "Authorization": f"Token {CTFD_TOKEN}",
    "Content-Type": "application/json",
})


def api_get(path):
    r = session.get(f"{CTFD_URL}{path}")
    r.raise_for_status()
    return r.json()["data"]


def api_post(path, json=None):
    r = session.post(f"{CTFD_URL}{path}", json=json)
    r.raise_for_status()
    data = r.json()
    if not data.get("success"):
        print(f"  API error: {data.get('errors')}")
        return None
    return data["data"]


def api_patch(path, json=None):
    r = session.patch(f"{CTFD_URL}{path}", json=json)
    r.raise_for_status()
    data = r.json()
    if not data.get("success"):
        print(f"  API error: {data.get('errors')}")
        return None
    return data["data"]


def api_delete(path):
    r = session.delete(f"{CTFD_URL}{path}")
    r.raise_for_status()


def upload_files(challenge_id, challdir, filenames):
    for filename in filenames:
        filepath = challdir / filename
        if not filepath.exists():
            print(f"  Warning: file {filepath} not found, skipping")
            continue
        with open(filepath, "rb") as fh:
            r = requests.post(
                f"{CTFD_URL}/api/v1/files",
                files={"file": (filename, fh)},
                data={"challenge_id": challenge_id, "type": "challenge"},
                headers={"Authorization": f"Token {CTFD_TOKEN}"},
            )
        r.raise_for_status()
        if r.json().get("success"):
            print(f"  Uploaded {filename}")
        else:
            print(f"  Failed to upload {filename}: {r.json().get('errors')}")


def get_existing_challenges():
    challenges = api_get("/api/v1/challenges?view=admin")
    return {c["name"]: c for c in challenges}


def clear_challenge_flags(challenge_id):
    flags = api_get(f"/api/v1/flags?challenge_id={challenge_id}")
    for flag in flags:
        api_delete(f"/api/v1/flags/{flag['id']}")


def clear_challenge_files(challenge_id):
    files = api_get(f"/api/v1/challenges/{challenge_id}/files")
    for f in files:
        api_delete(f"/api/v1/files/{f['id']}")


def deploy_challenge(challdir, existing):
    chall_file = challdir / "challenge.yml"
    if not chall_file.exists():
        return

    with open(chall_file) as f:
        data = yaml.safe_load(f)

    name = data["name"]
    print(f"-> {name}")

    challenge_body = {
        "name": name,
        "category": data.get("category", ""),
        "description": data.get("description", ""),
        "type": data.get("type", "standard"),
        "state": CTFD_STATE or data.get("state", "hidden"),
        "connection_info": data.get("connection_info", ""),
        "max_attempts": data.get("max_attempts", 0),
    }

    extra = data.get("extra", {})
    if data.get("type") == "dynamic":
        challenge_body["initial"] = extra.get("initial", 500)
        challenge_body["minimum"] = extra.get("minimum", 100)
        challenge_body["decay"] = extra.get("decay", 50)
        challenge_body["function"] = extra.get("function", "logarithmic")
    else:
        challenge_body["value"] = data.get("value", 0)

    if name in existing:
        challenge_id = existing[name]["id"]
        challenge_body.pop("type", None)
        api_patch(f"/api/v1/challenges/{challenge_id}", json=challenge_body)
        print(f"  Updated (id={challenge_id})")
    else:
        result = api_post("/api/v1/challenges", json=challenge_body)
        if not result:
            return
        challenge_id = result["id"]
        print(f"  Created (id={challenge_id})")

    clear_challenge_flags(challenge_id)
    for flag in data.get("flags", []):
        api_post("/api/v1/flags", json={
            "challenge_id": challenge_id,
            "type": "static",
            "content": flag,
        })
    print(f"  Set {len(data.get('flags', []))} flag(s)")

    clear_challenge_files(challenge_id)
    files = data.get("files", [])
    if files:
        upload_files(challenge_id, challdir, files)


def main():
    challenges_dir = pathlib.Path(__file__).parent
    existing = get_existing_challenges()

    for challdir in sorted(challenges_dir.iterdir()):
        if not challdir.is_dir():
            continue
        if not (challdir / "challenge.yml").exists():
            continue
        try:
            deploy_challenge(challdir, existing)
        except Exception as e:
            print(f"  Error: {e}")

    print("\nDone.")


if __name__ == "__main__":
    main()
