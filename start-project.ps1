Write-Host "Starting V-DISK Project..." -ForegroundColor Green
Write-Host ""

Write-Host "[1/3] Starting Backend Server..." -ForegroundColor Yellow
Set-Location "V-Disk-Management-UI Backend"
Start-Process powershell -ArgumentList "-NoExit", "-Command", "Write-Host 'Backend Server Starting...' -ForegroundColor Green; node server.js"
Set-Location ".."

Write-Host "[2/3] Waiting for backend to start..." -ForegroundColor Yellow
Start-Sleep -Seconds 5

Write-Host "[3/3] Starting Frontend..." -ForegroundColor Yellow
Set-Location "v0-disk-management-ui"
Start-Process powershell -ArgumentList "-NoExit", "-Command", "Write-Host 'Frontend Server Starting...' -ForegroundColor Green; npm run dev"
Set-Location ".."

Write-Host ""
Write-Host "Project started successfully!" -ForegroundColor Green
Write-Host ""
Write-Host "Backend: http://localhost:8080" -ForegroundColor Cyan
Write-Host "Frontend: http://localhost:3000" -ForegroundColor Cyan
Write-Host ""
Write-Host "Wait 10 seconds then open: http://localhost:3000" -ForegroundColor Yellow
Write-Host ""
Write-Host "Press any key to exit..." -ForegroundColor Gray
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")






