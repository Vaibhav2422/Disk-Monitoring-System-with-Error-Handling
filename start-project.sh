#!/bin/bash

echo "Starting V-DISK Project..."
echo

echo "[1/3] Starting Backend Server..."
cd "V-Disk-Management-UI Backend"
./bin/server.exe &
BACKEND_PID=$!
cd ..

echo "[2/3] Waiting for backend to start..."
sleep 3

echo "[3/3] Starting Frontend..."
cd "V-Disk-Management-UI"
npm run dev &
FRONTEND_PID=$!
cd ..

echo
echo "Project started successfully!"
echo
echo "Backend: http://localhost:8080"
echo "Frontend: http://localhost:3000"
echo
echo "Press Ctrl+C to stop both servers"
echo

# Function to cleanup on exit
cleanup() {
    echo "Stopping servers..."
    kill $BACKEND_PID 2>/dev/null
    kill $FRONTEND_PID 2>/dev/null
    exit
}

# Set trap to cleanup on script exit
trap cleanup SIGINT SIGTERM

# Wait for user to stop
wait

