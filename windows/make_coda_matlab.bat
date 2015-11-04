@echo off

setlocal

rem Check that all required external environment variables have been properly defined
if not defined VISAN_INSTALLER_DIR goto VISAN_INSTALLER_DIR_NOT_DEFINED
if not defined VISAN_PLATFORM      goto VISAN_PLATFORM_NOT_DEFINED
if not defined VISAN_CONFIG        goto VISAN_CONFIG_NOT_DEFINED

if not exist "%VS90COMNTOOLS%\vsvars32.bat"       goto NO_VISUAL_STUDIO
if not exist "%MATLAB_HOME%\extern\include\mex.h" goto NO_MATLAB

rem Determine the CODA configuration
set SCRIPT_DIR=%~dp0
set CODA_SOURCE_DIR=%SCRIPT_DIR%..
set CODA_BUILD_DIR=%SCRIPT_DIR%build
set CODA_BINARY_DIR=%SCRIPT_DIR%bin
set CODA_PLATFORM=%VISAN_PLATFORM%
set CODA_CONFIG=%VISAN_CONFIG%

rem Define local environment variables (These can be used during building; i.e. in the visual studio project files)
set JPEG_LIB=%VISAN_INSTALLER_DIR%\libs
set SZIP_LIB=%VISAN_INSTALLER_DIR%\libs
set ZLIB_LIB=%VISAN_INSTALLER_DIR%\libs
set HDF4_LIB=%VISAN_INSTALLER_DIR%\libs
set HDF5_LIB=%VISAN_INSTALLER_DIR%\libs

rem Perform a basic check to make sure the given directories are correct
if not exist "%JPEG_LIB%" goto NO_JPEG
if not exist "%SZIP_LIB%" goto NO_SZIP
if not exist "%ZLIB_LIB%" goto NO_ZLIB
if not exist "%HDF4_LIB%" goto NO_HDF4
if not exist "%HDF5_LIB%" goto NO_HDF5

rem Configure the Visual Studio environment
call "%VS90COMNTOOLS%\vsvars32.bat" > nul
if %errorlevel% neq 0 exit /b %errorlevel%

echo.
echo Building CODA-MATLAB interface with the following settings:
echo.
echo 	CODA_SOURCE_DIR = "%CODA_SOURCE_DIR%"
echo 	CODA_BUILD_DIR  = "%CODA_BUILD_DIR%"
echo 	CODA_BINARY_DIR = "%CODA_BINARY_DIR%"
echo 	CODA_PLATFORM   = "%CODA_PLATFORM%"
echo 	CODA_CONFIG     = "%CODA_CONFIG%"
echo 	MATLAB_HOME     = "%MATLAB_HOME%"
echo 	JPEG_LIB        = "%JPEG_LIB%"
echo 	SZIP_LIB        = "%SZIP_LIB%"
echo 	ZLIB_LIB        = "%ZLIB_LIB%"
echo 	HDF4_LIB        = "%HDF4_LIB%"
echo 	HDF5_LIB        = "%HDF5_LIB%"
echo.

rem Determine the correct configuration to build based on the environment settings
if "%CODA_PLATFORM%" == "Win32" set CONFIG=%CODA_CONFIG%^|Win32
if "%CODA_PLATFORM%" == "Win64" set CONFIG=%CODA_CONFIG%^|x64

rem Build the CODA-MATLAB interface visual studio project
vcbuild /nologo "%CODA_SOURCE_DIR%\windows\codamatlab.vcproj" "%CONFIG%"
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

:NO_MATLAB
echo.
echo Matlab MEX headers not found at "%MATLAB_HOME%\extern\include\mex.h".
echo.
echo Make sure Matlab is installed and the MATLAB_HOME environment variable is properly set.
echo.
exit /b 7

:NO_JPEG
echo.
echo JPEG directory not found. Make sure JPEG is installed and the JPEG_LIB variable in this batch file is properly set.
echo.
exit /b 9

:NO_SZIP
echo.
echo SZIP directory not found. Make sure SZIP is installed and the SZIP_LIB variable in this batch file is properly set.
echo.
exit /b 10

:NO_ZLIB
echo.
echo ZLIB directory not found. Make sure ZLIB is installed and the ZLIB_LIB variable in this batch file is properly set.
echo.
exit /b 11

:NO_HDF4
echo.
echo HDF4 directory not found. Make sure HDF4 is installed and the HDF4_LIB variable in this batch file is properly set.
echo.
exit /b 12

:NO_HDF5
echo.
echo HDF5 directory not found. Make sure HDF5 is installed and the HDF5_LIB variable in this batch file is properly set.
echo.
exit /b 13

:END

endlocal
