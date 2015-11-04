@echo off

setlocal

rem Check that all required external environment variables have been properly defined
if not defined VISAN_INSTALLER_DIR goto VISAN_INSTALLER_DIR_NOT_DEFINED
if not defined VISAN_PLATFORM      goto VISAN_PLATFORM_NOT_DEFINED
if not defined VISAN_CONFIG        goto VISAN_CONFIG_NOT_DEFINED

if not exist "%VS90COMNTOOLS%\vsvars32.bat"             goto NO_VISUAL_STUDIO
if not exist "%JAVA_HOME%\include\jni.h"                goto NO_JAVA
if not exist "%IDL_HOME%\external\include\idl_export.h" goto NO_IDL
if not exist "%MATLAB_HOME%\extern\include\mex.h"       goto NO_MATLAB

rem Determine the CODA configuration
set SCRIPT_DIR=%~dp0
set CODA_SOURCE_DIR=%SCRIPT_DIR%..
set CODA_BUILD_DIR=%SCRIPT_DIR%build
set CODA_BINARY_DIR=%SCRIPT_DIR%bin
set CODA_PLATFORM=%VISAN_PLATFORM%
set CODA_CONFIG=%VISAN_CONFIG%

rem Define local environment variables (These can be used during building; i.e. in the visual studio project files)
set ZLIB_INCLUDE_DIR=%VISAN_INSTALLER_DIR%\include
set HDF4_INCLUDE_DIR=%VISAN_INSTALLER_DIR%\include\hdf
set HDF5_INCLUDE_DIR=%VISAN_INSTALLER_DIR%\include

set JPEG_LIB=%VISAN_INSTALLER_DIR%\libs
set SZIP_LIB=%VISAN_INSTALLER_DIR%\libs
set ZLIB_LIB=%VISAN_INSTALLER_DIR%\libs
set HDF4_LIB=%VISAN_INSTALLER_DIR%\libs
set HDF5_LIB=%VISAN_INSTALLER_DIR%\libs

set PYTHONHOME=%VISAN_INSTALLER_DIR%

rem Perform a basic check to make sure the given directories are correct
if not exist "%ZLIB_INCLUDE_DIR%"             goto NO_ZLIB
if not exist "%HDF4_INCLUDE_DIR%"             goto NO_HDF4
if not exist "%HDF5_INCLUDE_DIR%"             goto NO_HDF5
if not exist "%JPEG_LIB%"                     goto NO_JPEG
if not exist "%SZIP_LIB%"                     goto NO_SZIP
if not exist "%ZLIB_LIB%"                     goto NO_ZLIB
if not exist "%HDF4_LIB%"                     goto NO_HDF4
if not exist "%HDF5_LIB%"                     goto NO_HDF5
if not exist "%PYTHONHOME%\include\Python.h"  goto NO_PYTHON
if not exist "%PYTHONHOME%\libs\python26.lib" goto NO_PYTHON

rem Configure the Visual Studio environment
call "%VS90COMNTOOLS%\vsvars32.bat" > nul
if %errorlevel% neq 0 exit /b %errorlevel%

echo.
echo Building CODA libraries, CODA tools and CODA interfaces with the following settings:
echo.
echo 	CODA_SOURCE_DIR  = "%CODA_SOURCE_DIR%"
echo 	CODA_BUILD_DIR   = "%CODA_BUILD_DIR%"
echo 	CODA_BINARY_DIR  = "%CODA_BINARY_DIR%"
echo 	CODA_PLATFORM    = "%CODA_PLATFORM%"
echo 	CODA_CONFIG      = "%CODA_CONFIG%"
echo 	IDL_HOME         = "%IDL_HOME%"
echo 	JAVA_HOME        = "%JAVA_HOME%"
echo 	MATLAB_HOME      = "%MATLAB_HOME%"
echo 	PYTHONHOME       = "%PYTHONHOME%"
echo 	ZLIB_INCLUDE_DIR = "%ZLIB_INCLUDE_DIR%"
echo 	HDF4_INCLUDE_DIR = "%HDF4_INCLUDE_DIR%"
echo 	HDF5_INCLUDE_DIR = "%HDF5_INCLUDE_DIR%"
echo 	JPEG_LIB         = "%JPEG_LIB%"
echo 	SZIP_LIB         = "%SZIP_LIB%"
echo 	ZLIB_LIB         = "%ZLIB_LIB%"
echo 	HDF4_LIB         = "%HDF4_LIB%"
echo 	HDF5_LIB         = "%HDF5_LIB%"
echo.

rem Determine the correct configuration to build based on the environment settings
if "%CODA_PLATFORM%" == "Win32" set CONFIG=%CODA_CONFIG%^|Win32
if "%CODA_PLATFORM%" == "Win64" set CONFIG=%CODA_CONFIG%^|x64

rem Build CODA visual studio solution
vcbuild /nologo "%CODA_SOURCE_DIR%\windows\coda.sln" "%CONFIG%"
if %errorlevel% neq 0 exit /b %errorlevel%

rem Build the CODA Java interface
pushd "%CODA_SOURCE_DIR%\java\nl\stcorp\coda"
if %errorlevel% neq 0 exit /b %errorlevel%
"%JAVA_HOME%\bin\javac.exe" -d "%CODA_BUILD_DIR%" *.java
if %errorlevel% neq 0 exit /b %errorlevel%
popd

rem Create CODA Java interface archive
"%JAVA_HOME%\bin\jar.exe" -cf "%CODA_BINARY_DIR%\coda.jar" "%CODA_BUILD_DIR%\nl"
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

:NO_IDL
echo.
echo IDL external headers not found at "%IDL_HOME%\external\include\idl_export.h".
echo.
echo Make sure IDL is installed and the IDL_HOME environment variable is properly set.
echo.
exit /b 6

:NO_MATLAB
echo.
echo Matlab MEX headers not found at "%MATLAB_HOME%\extern\include\mex.h".
echo.
echo Make sure Matlab is installed and the MATLAB_HOME environment variable is properly set.
echo.
exit /b 7

:NO_JAVA
echo.
echo Java JNI headers not found at "%JAVA_HOME%\include\jni.h".
echo.
echo Make sure the Java JDK is installed and the JAVA_HOME environment variable is properly set.
echo.
exit /b 8

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
echo ZLIB directory not found. Make sure ZLIB is installed and the ZLIB_INCLUDE_DIR and ZLIB_LIB variables in this batch file are properly set.
echo.
exit /b 11

:NO_HDF4
echo.
echo HDF4 directory not found. Make sure HDF4 is installed and the HDF4_INCLUDE_DIR and HDF4_LIB variables in this batch file are properly set.
echo.
exit /b 12

:NO_HDF5
echo.
echo HDF5 directory not found. Make sure HDF5 is installed and the HDF5_INCLUDE_DIR and HDF5_LIB variables in this batch file are properly set.
echo.
exit /b 13

:NO_PYTHON
echo.
echo Python directory not found. Make sure Python is installed and the PYTHONHOME variable in this batch file is properly set.
echo.
exit /b 14

:END

endlocal
