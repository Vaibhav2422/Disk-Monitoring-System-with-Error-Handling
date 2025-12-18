# Disk Management Simulator - Dockerfile
FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y --no-install-recommends gcc make libc6-dev && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . /app

RUN make build

ENV PORT=8080
ENV DATA_FILE=/app/disk_state.json

EXPOSE 8080
CMD ["make", "run"]
