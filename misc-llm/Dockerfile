FROM python:3.12-slim

ARG FLAG
ENV FLAG=$FLAG

RUN pip install requests

WORKDIR /app

COPY chat.py /app/chat.py
COPY server.py /app/server.py

EXPOSE 9000

ENTRYPOINT ["python3", "/app/server.py"]
