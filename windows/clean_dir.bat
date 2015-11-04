@echo off

setlocal

set DIRECTORY=%~1
set DESCRIPTION=%~2

rem Check if required parameters are given
if "%DIRECTORY%" == "" goto USAGE
if "%DESCRIPTION%" == "" set DESCRIPTION=directory

rem Remove the directory if it already exists (But ask the user for confirmation first)
if not exist "%DIRECTORY%" goto CREATE_DIRECTORY

echo.
echo This script will remove all files in the %DESCRIPTION% "%DIRECTORY%"
echo.

choice /n /m "Do you want to continue? [Y = Yes, N = No]:" 
if not %errorlevel% == 1 goto ABORT

echo.
echo Processing...

del /f /s /q "%DIRECTORY%" >> nul
if %errorlevel% neq 0 exit /b %errorlevel%
rmdir /s /q "%DIRECTORY%"
if %errorlevel% neq 0 exit /b %errorlevel%

rem Safeguard to check if all files have in fact been deleted (e.g. Files may be locked)
if exist "%DIRECTORY%" echo Directory not empty... & exit /b 2

:CREATE_DIRECTORY

mkdir "%DIRECTORY%"
if %errorlevel% neq 0 exit /b %errorlevel%

goto END

:ABORT
echo.
echo Operation was aborted...

exit /b 1

:USAGE
echo.
echo Usage: clean_dir ^<path^> [description]
echo.

:END
