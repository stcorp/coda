# Microsoft Developer Studio Project File - Name="codapython" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=codapython - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "codapython.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "codapython.mak" CFG="codapython - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "codapython - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "codapython - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "codapython - Win32 Release with HDF" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "codapython - Win32 Debug with HDF" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "codapython - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "build\Release\codapython"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "build\Release\codapython"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
LIB32=link.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "." /I "$(PYTHONHOME)\include" /I "$(PYTHONHOME)\Lib\site-packages\numpy\core\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBCODADLL" /D "HAVE_CONFIG_H" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x413 /d "NDEBUG"
# ADD RSC /l 0x413 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 "$(PYTHONHOME)\libs\python26.lib" Release\coda.lib "$(ZLIB_LIB)\zlib.lib" kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"Release\_codac.pyd"

!ELSEIF  "$(CFG)" == "codapython - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "build\Debug\codapython"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "build\Debug\codapython"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
LIB32=link.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "." /I "$(PYTHONHOME)\include" /I "$(PYTHONHOME)\Lib\site-packages\numpy\core\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBCODADLL" /D "HAVE_CONFIG_H" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x413 /d "_DEBUG"
# ADD RSC /l 0x413 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 "$(PYTHONHOME)\libs\python26_d.lib" Debug\coda_d.lib "$(ZLIB_LIB)\zlib.lib" kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"Debug\_codac.pyd" /pdbtype:sept

!ELSEIF  "$(CFG)" == "codapython - Win32 Release with HDF"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release\withhdf"
# PROP BASE Intermediate_Dir "build\Release\codapython\withhdf"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release\withhdf"
# PROP Intermediate_Dir "build\Release\codapython\withhdf"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
LIB32=link.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "." /I "$(PYTHONHOME)\include" /I "$(PYTHONHOME)\Lib\site-packages\numpy\core\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBCODADLL" /D "HAVE_CONFIG_H" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x413 /d "NDEBUG"
# ADD RSC /l 0x413 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 "$(PYTHONHOME)\libs\python26.lib" Release\withhdf\coda.lib "$(ZLIB_LIB)\zlib.lib" kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"Release\withhdf\_codac.pyd"

!ELSEIF  "$(CFG)" == "codapython - Win32 Debug with HDF"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug\withhdf"
# PROP BASE Intermediate_Dir "build\Debug\codapython\withhdf"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug\withhdf"
# PROP Intermediate_Dir "build\Debug\codapython\withhdf"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
LIB32=link.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "." /I "$(PYTHONHOME)\include" /I "$(PYTHONHOME)\Lib\site-packages\numpy\core\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBCODADLL" /D "HAVE_CONFIG_H" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x413 /d "_DEBUG"
# ADD RSC /l 0x413 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 "$(PYTHONHOME)\libs\python26_d.lib" Debug\withhdf\coda_d.lib "$(ZLIB_LIB)\zlib.lib" kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"Debug\withhdf\_codac.pyd" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "codapython - Win32 Release"
# Name "codapython - Win32 Debug"
# Name "codapython - Win32 Release with HDF"
# Name "codapython - Win32 Debug with HDF"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE="..\python\codac.c"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# End Target
# End Project
