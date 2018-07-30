@echo off

setlocal

rem Check that all required external environment variables have been properly defined
if not defined VISAN_INSTALLER_DIR goto VISAN_INSTALLER_DIR_NOT_DEFINED
if not defined VISAN_PLATFORM      goto VISAN_PLATFORM_NOT_DEFINED
if not defined VISAN_CONFIG        goto VISAN_CONFIG_NOT_DEFINED

if not exist "%INNO4_HOME%\ISCC.exe" goto NO_INNO4_COMPILER

rem Configure the Visual Studio environment
call "%VS90COMNTOOLS%\vsvars32.bat" > nul
if %errorlevel% neq 0 exit /b %errorlevel%

rem Determine the CODA configuration
set SCRIPT_DIR=%~dp0
set CODA_SOURCE_DIR=%SCRIPT_DIR%..
set CODA_BUILD_DIR=%SCRIPT_DIR%build
set CODA_BINARY_DIR=%SCRIPT_DIR%bin
set CODA_INSTALLER_DIR=%SCRIPT_DIR%install
set CODA_PLATFORM=%VISAN_PLATFORM%
set CODA_CONFIG=%VISAN_CONFIG%

rem Determine the correct binary path based on the environment settings
if "%CODA_PLATFORM%" == "Win32" set VC_REDIST_DIR=%VCINSTALLDIR%\redist\x86
if "%CODA_PLATFORM%" == "Win64" set VC_REDIST_DIR=%VCINSTALLDIR%\redist\amd64

rem Define the location of the CODA dependency binaries
set SZIP_BINARY_DIR=%VISAN_INSTALLER_DIR%\bin
set ZLIB_BINARY_DIR=%VISAN_INSTALLER_DIR%\bin
set HDF4_BINARY_DIR=%VISAN_INSTALLER_DIR%\bin
set HDF5_BINARY_DIR=%VISAN_INSTALLER_DIR%\bin

set LOG_FILE=%CODA_INSTALLER_DIR%\create_installer_log.txt

echo Creating installer directory structure...
call "%SCRIPT_DIR%clean_dir.bat" "%CODA_INSTALLER_DIR%" "CODA installer directory"
if %errorlevel% neq 0 exit /b %errorlevel%

mkdir "%CODA_INSTALLER_DIR%\bin"
if %errorlevel% neq 0 exit /b %errorlevel%
mkdir "%CODA_INSTALLER_DIR%\include"
if %errorlevel% neq 0 exit /b %errorlevel%
mkdir "%CODA_INSTALLER_DIR%\lib"
if %errorlevel% neq 0 exit /b %errorlevel%
mkdir "%CODA_INSTALLER_DIR%\doc"
if %errorlevel% neq 0 exit /b %errorlevel%

mkdir "%CODA_INSTALLER_DIR%\idl"
if %errorlevel% neq 0 exit /b %errorlevel%
mkdir "%CODA_INSTALLER_DIR%\java"
if %errorlevel% neq 0 exit /b %errorlevel%
mkdir "%CODA_INSTALLER_DIR%\matlab"
if %errorlevel% neq 0 exit /b %errorlevel%
mkdir "%CODA_INSTALLER_DIR%\python"
if %errorlevel% neq 0 exit /b %errorlevel%
mkdir "%CODA_INSTALLER_DIR%\python\coda"
if %errorlevel% neq 0 exit /b %errorlevel%

echo Copying installer files...

rem Populate the installer directory structure
xcopy    "%CODA_SOURCE_DIR%\CHANGES"                      "%CODA_INSTALLER_DIR%"             >> "%LOG_FILE%"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy    "%CODA_SOURCE_DIR%\COPYING"                      "%CODA_INSTALLER_DIR%"             >> "%LOG_FILE%"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy    "%CODA_SOURCE_DIR%\LICENSES"                     "%CODA_INSTALLER_DIR%"             >> "%LOG_FILE%"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy    "%CODA_SOURCE_DIR%\FAQ"                          "%CODA_INSTALLER_DIR%"             >> "%LOG_FILE%"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy    "%CODA_SOURCE_DIR%\INSTALL"                      "%CODA_INSTALLER_DIR%"             >> "%LOG_FILE%"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy    "%CODA_SOURCE_DIR%\windows\readme.txt"           "%CODA_INSTALLER_DIR%"             >> "%LOG_FILE%"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy    "%CODA_SOURCE_DIR%\windows\*.iss"                "%CODA_INSTALLER_DIR%"             >> "%LOG_FILE%"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy    "%SZIP_BINARY_DIR%\szip.dll"                     "%CODA_INSTALLER_DIR%\bin"         >> "%LOG_FILE%"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy    "%ZLIB_BINARY_DIR%\zlibdll.dll"                  "%CODA_INSTALLER_DIR%\bin"         >> "%LOG_FILE%"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy    "%HDF4_BINARY_DIR%\hdf.dll"                      "%CODA_INSTALLER_DIR%\bin"         >> "%LOG_FILE%"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy    "%HDF4_BINARY_DIR%\mfhdf.dll"                    "%CODA_INSTALLER_DIR%\bin"         >> "%LOG_FILE%"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy    "%HDF4_BINARY_DIR%\xdr.dll"                      "%CODA_INSTALLER_DIR%\bin"         >> "%LOG_FILE%"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy    "%HDF5_BINARY_DIR%\hdf5.dll"                     "%CODA_INSTALLER_DIR%\bin"         >> "%LOG_FILE%"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy    "%CODA_BINARY_DIR%\coda.dll"                     "%CODA_INSTALLER_DIR%\bin"         >> "%LOG_FILE%"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy    "%CODA_BINARY_DIR%\*.exe"                        "%CODA_INSTALLER_DIR%\bin"         >> "%LOG_FILE%"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy    "%CODA_SOURCE_DIR%\windows\coda.h"               "%CODA_INSTALLER_DIR%\include"     >> "%LOG_FILE%"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy    "%CODA_BINARY_DIR%\coda.exp"                     "%CODA_INSTALLER_DIR%\lib"         >> "%LOG_FILE%"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy    "%CODA_BINARY_DIR%\coda.lib"                     "%CODA_INSTALLER_DIR%\lib"         >> "%LOG_FILE%"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy    "%CODA_BINARY_DIR%\libcoda.lib"                  "%CODA_INSTALLER_DIR%\lib"         >> "%LOG_FILE%"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy /e "%CODA_SOURCE_DIR%\doc\*.*"                      "%CODA_INSTALLER_DIR%\doc"         >> "%LOG_FILE%"
if %errorlevel% neq 0 exit /b %errorlevel%

xcopy    "%CODA_SOURCE_DIR%\idl\*.dlm"                    "%CODA_INSTALLER_DIR%\idl"         >> "%LOG_FILE%"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy    "%CODA_BINARY_DIR%\coda_idl.dll"                 "%CODA_INSTALLER_DIR%\idl"         >> "%LOG_FILE%"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy    "%CODA_SOURCE_DIR%\java\*.java"                  "%CODA_INSTALLER_DIR%\java"        >> "%LOG_FILE%"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy    "%CODA_SOURCE_DIR%\windows\build.xml"            "%CODA_INSTALLER_DIR%\java"        >> "%LOG_FILE%"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy    "%CODA_BINARY_DIR%\coda.jar"                     "%CODA_INSTALLER_DIR%\java"        >> "%LOG_FILE%"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy    "%CODA_BINARY_DIR%\coda_jni.dll"                 "%CODA_INSTALLER_DIR%\java"        >> "%LOG_FILE%"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy    "%CODA_SOURCE_DIR%\matlab\*.m"                   "%CODA_INSTALLER_DIR%\matlab"      >> "%LOG_FILE%"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy    "%CODA_BINARY_DIR%\coda_matlab.mexw*"            "%CODA_INSTALLER_DIR%\matlab"      >> "%LOG_FILE%"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy    "%CODA_SOURCE_DIR%\windows\setup.py"             "%CODA_INSTALLER_DIR%\python"      >> "%LOG_FILE%"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy    "%CODA_SOURCE_DIR%\python\*.py"                  "%CODA_INSTALLER_DIR%\python\coda" >> "%LOG_FILE%"
if %errorlevel% neq 0 exit /b %errorlevel%
xcopy    "%CODA_BINARY_DIR%\_codac.pyd"                   "%CODA_INSTALLER_DIR%\python\coda" >> "%LOG_FILE%"
if %errorlevel% neq 0 exit /b %errorlevel%

echo Creating installer using Inno Setup...

"%INNO4_HOME%\ISCC.exe" "%CODA_INSTALLER_DIR%\removepath.iss"
if %errorlevel% neq 0 exit /b %errorlevel%
if "%CODA_PLATFORM%" == "Win32" "%INNO4_HOME%\ISCC.exe" "%CODA_INSTALLER_DIR%\codawin32.iss"
if %errorlevel% neq 0 exit /b %errorlevel%
if "%CODA_PLATFORM%" == "Win64" "%INNO4_HOME%\ISCC.exe" "%CODA_INSTALLER_DIR%\codawin64.iss"
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

:NO_INNO4_COMPILER
echo.
echo Inno Setup Compiler not found at "%INNO4_HOME%\ISCC.exe".
echo.
echo Make sure Inno Setup is installed and the INNO4_HOME environment variable is properly set.
echo.
exit /b 5

:END

endlocal
