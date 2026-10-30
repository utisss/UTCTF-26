#!/usr/bin/env python3
import os
import signal
import socketserver
import subprocess
import threading
import time

MAX_MESSAGES_PER_IP = int(os.environ.get("MAX_MESSAGES_PER_IP", "10"))
RATE_WINDOW_SECONDS = int(os.environ.get("RATE_WINDOW_SECONDS", "60"))

message_log: dict[str, list[float]] = {}
lock = threading.Lock()


def is_rate_limited(ip: str) -> bool:
    now = time.time()
    with lock:
        timestamps = message_log.get(ip, [])
        timestamps = [t for t in timestamps if now - t < RATE_WINDOW_SECONDS]
        message_log[ip] = timestamps
        if len(timestamps) >= MAX_MESSAGES_PER_IP:
            return True
        timestamps.append(now)
        return False


class ChatHandler(socketserver.BaseRequestHandler):
    def handle(self):
        ip = self.client_address[0]

        proc = subprocess.Popen(
            ["python3", "/app/chat.py"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )

        def forward_output():
            try:
                while True:
                    data = proc.stdout.read(1)
                    if not data:
                        break
                    self.request.sendall(data)
            except (BrokenPipeError, ConnectionResetError, OSError):
                proc.kill()

        out_thread = threading.Thread(target=forward_output, daemon=True)
        out_thread.start()

        buf = b""
        try:
            while True:
                data = self.request.recv(4096)
                if not data:
                    break
                buf += data
                while b"\n" in buf:
                    line, buf = buf.split(b"\n", 1)
                    if is_rate_limited(ip):
                        self.request.sendall(
                            b"Rate limited. Please wait before sending more messages.\n\n> "
                        )
                        continue
                    proc.stdin.write(line + b"\n")
                    proc.stdin.flush()
        except (BrokenPipeError, ConnectionResetError, OSError):
            pass
        finally:
            proc.kill()
            proc.wait()


class ThreadedTCPServer(socketserver.ThreadingMixIn, socketserver.TCPServer):
    allow_reuse_address = True
    daemon_threads = True


if __name__ == "__main__":
    signal.signal(signal.SIGCHLD, signal.SIG_IGN)
    server = ThreadedTCPServer(("0.0.0.0", 9000), ChatHandler)
    server.serve_forever()
