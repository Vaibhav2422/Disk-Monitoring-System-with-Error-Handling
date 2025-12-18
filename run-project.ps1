# Script to start both backend and frontend servers

Write-Host "Starting V-DISK Project..." -ForegroundColor Green
Write-Host ""

# Start Backend Server
Write-Host "[1/2] Starting Backend Server..." -ForegroundColor Yellow
Start-Process powershell -ArgumentList "-NoExit", "-Command", "Set-Location 'V:\AIDS(2nd)\SEM 3\EDAI-3\project\V-DISK\V-Disk-Management-UI Backend'; Write-Host 'Backend Server Starting...' -ForegroundColor Cyan; node server.js" -WindowStyle Normal

# Wait for backend to start
Write-Host "Waiting for backend to start..." -ForegroundColor Yellow
Start-Sleep -Seconds 5

# Start Frontend Server
Write-Host "[2/2] Starting Frontend Server..." -ForegroundColor Yellow
Start-Process powershell -ArgumentList "-NoExit", "-Command", "Set-Location 'V:\AIDS(2nd)\SEM 3\EDAI-3\project\V-DISK\v0-disk-management-ui'; Write-Host 'Frontend Server Starting...' -ForegroundColor Cyan; npm run dev" -WindowStyle Normal

Write-Host ""
Write-Host "Project started successfully!" -ForegroundColor Green
Write-Host ""
Write-Host "Backend: http://localhost:3003" -ForegroundColor Blue
Write-Host "Frontend: http://localhost:3000" -ForegroundColor Blue
Write-Host ""
Write-Host "Wait 10 seconds then open: http://localhost:3000" -ForegroundColor Magenta
Write-Host ""