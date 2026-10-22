@echo off
REM ==========================================================================
REM  SA-MP Freeroam 0.3.7 - ერთი ბრძანებით მომზადება (Windows)
REM
REM  გაშვება: ორჯერ დააწკაპუნე ამ ფაილზე ან cmd-ში:  setup.bat
REM
REM  აკეთებს: plugin-ების ჩამოტვირთვა + gamemode-ის კომპილაცია
REM ==========================================================================
setlocal
set "ROOT=%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File "%ROOT%setup.ps1"
set "RC=%ERRORLEVEL%"
echo.
if not "%RC%"=="0" (
    echo [X] setup ვერ დასრულდა. ნახე შეტყობინებები ზემოთ.
) else (
    echo [OK] setup დასრულდა.
)
pause
endlocal & exit /b %RC%
