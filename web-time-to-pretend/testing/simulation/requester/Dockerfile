FROM python:3.11-slim

WORKDIR /app

RUN pip install requests --break-system-packages 2>/dev/null || pip install requests

COPY . .

CMD ["/bin/bash"]
