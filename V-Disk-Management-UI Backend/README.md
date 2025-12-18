# Disk Management Simulator Backend (Pure C)

A minimal REST-style HTTP server written in ANSI C (C99) with POSIX sockets that simulates disk operations. No external libraries, no JSON/HTTP frameworks — everything is hand-rolled for clarity and portability.

## Features

- In-memory disk model with `DISK_MAX_BLOCKS=512` blocks
- Allocate files: contiguous, fragmented, and custom strategies (first-fit, best-fit, worst-fit)
- Logical delete and undelete last
- Defragmentation (compacts used blocks to the front)
- Mark random bad sectors and repair
- Fragmentation percentage, stats, files list, state dump, and operation logs
- Simple persistence to a human-readable JSON-like file
- Single-threaded HTTP/1.1 handler with manual routing and JSON responses
- Plain C tests without external frameworks

## Build

- Prerequisites: gcc and make (Linux recommended)
- Build:
  - `make build` (outputs `bin/server`)
- Run:
  - `PORT=8080 DATA_FILE=disk_state.json make run`
- Test:
  - `make test`

## Docker

\`\`\`
docker build -t disk-sim-c .
docker run --rm -p 8080:8080 -e PORT=8080 -e DATA_FILE=/data/disk_state.json -v $(pwd)/data:/data disk-sim-c
\`\`\`

## API

All responses follow:
\`\`\`
{ "success": 1|0, "data": <object|null>, "error": "<string|null>" }
\`\`\`

- POST /allocate/contiguous
  - Body: `{ "size": 10 }`
- POST /allocate/fragmented
  - Body: `{ "size": 10 }`
- POST /allocate/custom
  - Body: `{ "size": 10, "strategy": "first-fit|best-fit|worst-fit" }`
- DELETE /file/:id
- POST /undelete/last
- POST /defragment
- POST /mark-bad
  - Body: `{ "count": 5 }`
- GET /fragmentation
- GET /disk/state
- GET /disk/files
- GET /disk/stats
- GET /disk/logs
- POST /disk/reset
- POST /repair

## Example curl

\`\`\`
curl -s -X POST http://localhost:8080/allocate/contiguous -d '{"size":8}'
curl -s -X POST http://localhost:8080/allocate/fragmented -d '{"size":8}'
curl -s -X POST http://localhost:8080/allocate/custom -d '{"size":8,"strategy":"best-fit"}'
curl -s -X DELETE http://localhost:8080/file/1
curl -s -X POST http://localhost:8080/undelete/last
curl -s -X POST http://localhost:8080/defragment
curl -s -X POST http://localhost:8080/mark-bad -d '{"count":5}'
curl -s http://localhost:8080/fragmentation
curl -s http://localhost:8080/disk/state
curl -s http://localhost:8080/disk/files
curl -s http://localhost:8080/disk/stats
curl -s http://localhost:8080/disk/logs
curl -s -X POST http://localhost:8080/disk/reset
curl -s -X POST http://localhost:8080/repair
\`\`\`

## Notes and Limitations

- Single-process, single-threaded server for simplicity; adequate for demos/tests.
- Minimal HTTP parsing: request line, headers, Content-Length, and body; no chunked encoding, no TLS.
- Persistence uses a simple JSON-like file with naive parsing (format must be compatible with our writer).
- Tested on Linux. Other POSIX systems may work with minor changes.
- Block size is conceptual (1 unit = 1 block). Adjust `DISK_MAX_BLOCKS` in `disk.h` if needed.

## Project Structure

\`\`\`
src/
  disk.c, disk.h      # disk simulation core
  server.c            # HTTP server + routing
  utils.c, utils.h    # string builder, file IO, parsing helpers
tests/
  test_runner.c       # plain C tests
Makefile
Dockerfile
README.md
.env.example
\`\`\`

## Security and Safety

- Not production-hardened. No authentication, no TLS, naive JSON parsing.
- Accepts only small payloads and simple JSON key/value fields.
