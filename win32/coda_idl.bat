@ECHO OFF

SETLOCAL

REM User definable settings

SET IDL_DIR=C:\rsi\idl63
REM Supported versions are: 5.1, 5.2, 5.3, 5.4, 5.5, 5.6, 6.0, 6.1, 6.2, 6.3
SET IDL_VERSION=6.3

REM End of user definable settings

IF NOT "%1"=="" SET IDL_DIR=%1
IF NOT "%2"=="" SET IDL_VERSION=%2

SET IDL_LIBDIR=%IDL_DIR%\bin\bin.x86
SET OUTDIR=.\idl
SET INTDIR=.\build
SET CODASRCDIR=..\idl
SET LIBCODADIR=Release

IF NOT EXIST "%IDL_DIR%\external\export.h" GOTO NO_EXPORT_H
IF NOT EXIST "%LIBCODADIR%\withhdf\libcoda.lib" GOTO NO_CODA_LIB
IF NOT EXIST %HDF4_LIB% GOTO NO_HDF4
IF NOT EXIST %HDF5_LIB% GOTO NO_HDF5

SET CPPFLAGS=%CPPFLAGS% -nologo -I. -I..\libcoda -I"%CODASRCDIR%" -I"%IDL_DIR%\external" -DWIN32_LEAN_AND_MEAN -DWIN32 -DHAVE_CONFIG_H
SET LDFLAGS=%LDFLAGS%


IF "%IDL_VERSION%"=="5.1" GOTO IDL_VERSION_5_2
IF "%IDL_VERSION%"=="5.2" GOTO IDL_VERSION_5_2
IF "%IDL_VERSION%"=="5.3" GOTO IDL_VERSION_5_3
IF "%IDL_VERSION%"=="5.4" GOTO IDL_VERSION_5_4
IF "%IDL_VERSION%"=="5.5" GOTO IDL_VERSION_5_4
IF "%IDL_VERSION%"=="5.6" GOTO IDL_VERSION_5_4
IF "%IDL_VERSION%"=="6.0" GOTO IDL_VERSION_5_4
IF "%IDL_VERSION%"=="6.1" GOTO IDL_VERSION_5_4
IF "%IDL_VERSION%"=="6.2" GOTO IDL_VERSION_5_4
IF "%IDL_VERSION%"=="6.3" GOTO IDL_VERSION_6_3

GOTO UNSUPPORTED_IDL_VERSION

:IDL_VERSION_6_3

SET IDL_LIB=idl.lib
SET CPPFLAGS=%CPPFLAGS% -DIDL_V5_4

GOTO IDL_VERSION_END

:IDL_VERSION_5_4

SET IDL_LIB=idl32.lib
SET CPPFLAGS=%CPPFLAGS% -DIDL_V5_4

GOTO IDL_VERSION_END

:IDL_VERSION_5_3

SET IDL_LIB=idl32.lib
SET CPPFLAGS=%CPPFLAGS% -DIDL_V5_3

GOTO IDL_VERSION_END

:IDL_VERSION_5_2

SET IDL_LIB=idl32.lib

:IDL_VERSION_END

IF NOT EXIST "%IDL_LIBDIR%\%IDL_LIB%" GOTO NO_IDL_LIB
IF NOT EXIST "%OUTDIR%" mkdir "%OUTDIR%"
IF NOT EXIST "%INTDIR%" mkdir "%INTDIR%"

ECHO Creating coda-idl.obj
cl %CPPFLAGS% /Fo"%INTDIR%\coda-idl.obj" -c "%CODASRCDIR%\coda-idl.c"

ECHO Creating coda-idl.dll
link %LDFLAGS% /DLL /OUT:"%OUTDIR%\coda-idl.dll" /DEF:coda-idl.def "%INTDIR%\coda-idl.obj" "%IDL_LIBDIR%\%IDL_LIB%" "%HDF4_LIB%\hd425.lib" "%HDF4_LIB%\hm425.lib" "%HDF4_LIB%\xdr.lib" "%JPEG_LIB%\libjpeg.lib" "%SZIP_LIB%\szlib.lib" "%ZLIB_LIB%\zlib.lib" "%HDF5_LIB%\hdf5.lib" "%LIBCODADIR%\withhdf\libcoda.lib" ws2_32.lib 

ECHO Copying coda-idl.dlm
COPY %CODASRCDIR%\coda-idl.dlm "%OUTDIR%\coda-idl.dlm" > NUL

DEL "%OUTDIR%\*.lib"
DEL "%OUTDIR%\*.exp"

GOTO END

:NO_IDL_LIB
ECHO.
ECHO Unable to locate %IDL_LIBDIR%\idl32.lib.
ECHO.
GOTO END

:NO_EXPORT_H
ECHO.
ECHO Unable to locate %IDL_DIR%\external\export.h.
ECHO.
GOTO END

:NO_CODA_LIB
ECHO.
ECHO Unable to locate %LIBCODADIR%\withhdf\libcoda.lib.
ECHO.
GOTO END

:UNSUPPORTED_IDL_VERSION
ECHO.
ECHO Version %IDL_VERSION% is not supported.
ECHO.
GOTO END

:NO_HDF4
ECHO.
ECHO HDF4 directory not found. Make sure HDF4 is installed and that
ECHO the system variable HDF4_LIB is properly set.
ECHO.
GOTO END

:NO_HDF5
ECHO.
ECHO HDF5 directory not found. Make sure HDF5 is installed and that
ECHO the system variable HDF5_LIB is properly set.
ECHO.
GOTO END

:END

ENDLOCAL
