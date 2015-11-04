@ECHO OFF

SETLOCAL

SET INNOCOMP=C:\Program Files\Inno Setup 4\compil32.exe
SET JAVA_HOME="C:\jdk1.6.0_07"
SET VISANBLDDIR=C:\visanbin
SET HDF4_INCLUDE="%VISANBLDDIR%\VISAN\include\hdf"
SET HDF5_INCLUDE="%VISANBLDDIR%\VISAN\include"
SET ZLIB_INCLUDE="%VISANBLDDIR%\VISAN\include"
SET HDF4_LIB="%VISANBLDDIR%\VISAN\libs"
SET HDF5_LIB="%VISANBLDDIR%\VISAN\libs"
SET ZLIB_LIB="%VISANBLDDIR%\VISAN\libs"
SET SZIP_LIB="%VISANBLDDIR%\VISAN\libs"
SET JPEG_LIB="%VISANBLDDIR%\VISAN\libs"
SET HDF4_DLL="%VISANBLDDIR%\VISAN\bin"
SET HDF5_DLL="%VISANBLDDIR%\VISAN\bin"
SET ZLIB_DLL="%VISANBLDDIR%\VISAN\bin"
SET SZIP_DLL="%VISANBLDDIR%\VISAN\bin"
SET PYTHONHOME="%VISANBLDDIR%\VISAN"

IF NOT EXIST "%INNOCOMP%" GOTO NO_INNOCOMP
IF NOT EXIST "%HDF4_INCLUDE%" GOTO NO_HDF4
IF NOT EXIST "%HDF4_LIB%" GOTO NO_HDF4
IF NOT EXIST "%HDF5_INCLUDE%" GOTO NO_HDF5
IF NOT EXIST "%HDF5_LIB%" GOTO NO_HDF5
IF NOT EXIST "%PYTHONHOME%\include\Python.h" GOTO NO_PYTHON
IF NOT EXIST "%PYTHONHOME%\libs\python26.lib" GOTO NO_PYTHON
IF NOT EXIST "%JAVA_HOME%\include\jni.h" GOTO NO_JAVA
IF NOT EXIST "coda.sln" GOTO NO_SLN


ECHO Setting Visual Studio environment variables
CALL "C:\Program Files\Microsoft Visual Studio 9.0\Common7\Tools\vsvars32.bat"

ECHO Remove build directories
IF EXIST build RMDIR /Q /S build
IF EXIST Release RMDIR /Q /S Release
IF EXIST idl54 RMDIR /Q /S idl54
IF EXIST idl55 RMDIR /Q /S idl55
IF EXIST idl61 RMDIR /Q /S idl61
IF EXIST idl RMDIR /Q /S idl
IF EXIST matlab RMDIR /Q /S matlab
IF EXIST nl RMDIR /Q /S nl

ECHO Building LIBCODA
vcbuild coda.sln "Release with HDF|Win32"

ECHO Building CODA-JAVA
CALL coda_java.bat

ECHO Building CODA-IDL
CALL coda_idl.bat c:\rsi\idl54 5.4
MOVE idl idl54
CALL coda_idl.bat c:\rsi\idl55 5.5
MOVE idl idl55
CALL coda_idl.bat c:\rsi\idl61 6.1
MOVE idl idl61
CALL coda_idl.bat c:\rsi\idl63 6.3

ECHO Building CODA-MATLAB
CALL coda_matlab.bat

ECHO Copying HDF4 DLLs
COPY "%HDF4_DLL%\hd425m.dll" Release\withhdf
COPY "%HDF4_DLL%\hm425m.dll" Release\withhdf

ECHO Copying HDF5 DLLs
COPY "%HDF5_DLL%\hdf5dll.dll" Release\withhdf

ECHO Copying libz and szip DLLs
COPY "%ZLIB_DLL%\zlib1.dll" Release\withhdf
COPY "%SZIP_DLL%\szlibdll.dll" Release\withhdf

ECHO Make Inno Setup Executable
"%INNOCOMP%" /cc removepath.iss
"%INNOCOMP%" /cc coda.iss

ECHO Cleaning up
RMDIR /Q /S build
RMDIR /Q /S Release
RMDIR /Q /S idl54
RMDIR /Q /S idl55
RMDIR /Q /S idl61
RMDIR /Q /S idl
RMDIR /Q /S matlab
RMDIR /Q /S nl
DEL /Q removepath.exe

GOTO END

:NO_INNOCOMP
ECHO.
ECHO Inno Setup Compiler not found. Make sure Inno Setup is installed and that
ECHO the INNOCOMP variable in this batchfile is properly set.
ECHO.
GOTO END

:NO_HDF4
ECHO.
ECHO HDF4 directory not found. Make sure HDF4 is installed and that the
ECHO HDF4_INCLUDE and HDF4_LIB variables in this batchfile are properly set.
ECHO.
GOTO END

:NO_HDF5
ECHO.
ECHO HDF5 directory not found. Make sure HDF5 is installed and that the
ECHO HDF5_INCLUDE and HDF5_LIB variables in this batchfile are properly set.
ECHO.
GOTO END

:NO_PYTHON
ECHO.
ECHO Python directory not found. Make sure Python is installed and that
ECHO the PYTHONHOME variable in this batchfile is properly set.
ECHO.
GOTO END

:NO_JAVA
ECHO.
ECHO Java directory not found. Make sure the Java JDK is installed and that
ECHO the JAVA_HOME variable in this batchfile is properly set.
ECHO.
GOTO END

:NO_SLN
ECHO.
ECHO Unable to locate coda.sln
ECHO.
ECHO You first have to open the coda.dsw file in VS 9.0 and convert it to a
ECHO solution file.
ECHO.

:END

ENDLOCAL
