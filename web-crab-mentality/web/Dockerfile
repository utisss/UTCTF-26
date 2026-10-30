FROM python:3.11-slim

WORKDIR /app

RUN pip install flask

COPY main.py .
COPY webfiles/ webfiles/

EXPOSE 5888

CMD ["python", "main.py"]
