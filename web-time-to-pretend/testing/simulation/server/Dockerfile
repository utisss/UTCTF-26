FROM python:3.11-slim

WORKDIR /app

RUN pip install flask --break-system-packages 2>/dev/null || pip install flask

COPY . .

CMD ["/bin/bash"]
