#!/bin/bash

while true; do
    socat TCP4-LISTEN:9000,fork,reuseaddr EXEC:'python3 /app/chat.py',pty,echo=0,rawer
done
