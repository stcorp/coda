@echo off

setlocal

rem Check that all required external environment variables have been properly defined
if not defined VISAN_INSTALLER_DIR goto VISAN_INSTALLER_DIR_NOT_DEFINED
if not defined VISAN_PLATFORM      goto VISAN_PLATFORM_NOT_DEFINED
if not defined VISAN_CONFIG        goto VISAN_CONFIG_NOT_DEFINED

if not exist "%VS90COMNTOOLS%\vsvars32.bat" goto NO_VISUAL_STUDIO

rem Determine the CODA configuration
set SCRIPT_DIR=%~dp0
set CODA_SOURCE_DIR=%SCRIPT_DIR%..
set CODA_BUILD_DIR=%SCRIPT_DIR%build
set CODA_BINARY_DIR=%SCRIPT_DIR%bin
set CODA_PLATFORM=%VISAN_PLATFORM%
set CODA_CONFIG=%VISAN_CONFIG%

rem Define local environment variables (These can be used during building; i.e. in the visual studio project files)
set PYTHONHOME=%VISAN_INSTALLER_DIR%

rem Perform a basic check to make sure the given directories are correct
if not exist "%PYTHONHOME%\include\Python.h"  goto NO_PYTHON
if not exist "%PYTHONHOME%\libs\python26.lib" goto NO_PYTHON

rem Configure the Visual Studio environment
call "%VS90COMNTOOLS%\vsvars32.bat" > nul
if %errorlevel% neq 0 exit /b %errorlevel%

echo.
echo Building CODA-PYTHON interface with the following settings:
echo.
echo 	CODA_SOURCE_DIR = "%CODA_SOURCE_DIR%"
echo 	CODA_BUILD_DIR  = "%CODA_BUILD_DIR%"
echo 	CODA_BINARY_DIR = "%CODA_BINARY_DIR%"
echo 	CODA_PLATFORM   = "%CODA_PLATFORM%"
echo 	CODA_CONFIG     = "%CODA_CONFIG%"
echo 	PYTHONHOME      = "%PYTHONHOME%"
echo.

rem Determine the correct configuration to build based on the environment settings
if "%CODA_PLATFORM%" == "Win32" set CONFIG=%CODA_CONFIG%^|Win32
if "%CODA_PLATFORM%" == "Win64" set CONFIG=%CODA_CONFIG%^|x64

rem Build the CODA-PYTHON interface visual studio project
vcbuild /nologo "%CODA_SOURCE_DIR%\windows\codapython.vcproj" "%CONFIG%"
if %errorlevel% neq 0 exit /b %errorlevel%

goto END

:VISAN_INSTALLER_DIR_NOT_DEFINED
echo. 
echo Environment variable VISAN_INSTALLER_DIR is not defined
echo.
goto VISAN_ENVIRONMENT_INVALID

:VISAN_PLATFORM_NOT_DEFINED
echo. 
echo Environment variable VISAN_PLATFORM is not defined
echo.
goto VISAN_ENVIRONMENT_INVALID

:VISAN_CONFIG_NOT_DEFINED
echo. 
echo Environment variable VISAN_CONFIG is not defined
echo.
goto VISAN_ENVIRONMENT_INVALID

:VISAN_ENVIRONMENT_INVALID
echo Make sure you have called the configure script to properly set the VISAN build environment.
echo.
exit /b 1

:NO_VISUAL_STUDIO
echo.
echo Visual Studio 2008 configuration script not found at "%VS90COMNTOOLS%\vsvars32.bat".
echo.
echo Make sure Visual Studio 2008 is installed and the VS90COMNTOOLS environment variable is properly set.
echo.
exit /b 4

:NO_PYTHON
echo.
echo Python directory not found. Make sure Python is installed and the PYTHONHOME variable in this batch file is properly set.
echo.
exit /b 14

:END

endlocal
