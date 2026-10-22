@echo off
REM ==========================================================================
REM  SA-MP Freeroam 0.3.7 - plugin-ების ჩამოტვირთვა (Windows)
REM
REM  გაშვება: ორჯერ დააწკაპუნე ამ ფაილზე ან cmd-ში:  download_dependencies.bat
REM
REM  ჩამოტვირთავს: plugins\mysql.dll, plugins\streamer.dll, plugins\sscanf.dll,
REM                libmysql.dll (სერვერის ძირში) და pawno\pawncc.exe-ს
REM ==========================================================================
setlocal
set "ROOT=%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File "%ROOT%download_dependencies.ps1"
set "RC=%ERRORLEVEL%"
echo.
if not "%RC%"=="0" (
    echo [X] ჩამოტვირთვა ვერ დასრულდა. შეამოწმე ინტერნეტი და სცადე ხელახლა.
) else (
    echo [OK] დასრულდა.
)
pause
endlocal & exit /b %RC%
