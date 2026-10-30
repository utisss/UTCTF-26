FROM python:3.11-slim

WORKDIR /app

RUN pip install flask

COPY main.py index.html portal.html URGENT.txt ./

EXPOSE 9382

CMD ["python", "main.py"]
