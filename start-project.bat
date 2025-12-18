@echo off
echo Starting V-DISK Project...
echo.

echo [1/3] Starting Backend Server...
cd /d "%~dp0V-Disk-Management-UI Backend"
start "Backend Server" cmd /k "echo Backend Server Starting... && node server.js"
cd /d "%~dp0"

echo [2/3] Waiting for backend to start...
timeout /t 5 /nobreak > nul

echo [3/3] Starting Frontend (v0)...
cd /d "%~dp0v0-disk-management-ui"
start "Frontend Server" cmd /k "echo Frontend Server Starting (v0)... && npm run dev"
cd /d "%~dp0"

echo.
echo Project started successfully!
echo.
echo Backend: http://localhost:3003
echo Frontend (v0): http://localhost:3000
echo.
echo Wait 10 seconds then open: http://localhost:3000
echo.
echo Press any key to exit...
pause > nul