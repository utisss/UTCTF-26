FROM amazoncorretto:11-alpine

WORKDIR /app

# Copy the uber JAR
COPY app.jar app.jar

# Copy static files and keys
COPY public/ public/
COPY admin.html admin.html
COPY jwt.pem jwt.pem
COPY jwt_public.pem jwt_public.pem

EXPOSE 5926

ENTRYPOINT ["java", "-jar", "app.jar"]
