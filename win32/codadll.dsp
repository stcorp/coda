# Microsoft Developer Studio Project File - Name="codadll" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=codadll - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "codadll.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "codadll.mak" CFG="codadll - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "codadll - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "codadll - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "codadll - Win32 Release with HDF" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "codadll - Win32 Debug with HDF" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "codadll - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "build\Release\codadll"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "build\Release\codadll"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
LIB32=link.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "." /I "..\libcoda" /I "..\libcoda\expat" /I ".." /I "$(ZLIB_INCLUDE)" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBCODADLL_EXPORTS" /D "LIBCODADLL" /D "HAVE_CONFIG_H" /YX /FD /Zm400 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x413 /d "NDEBUG"
# ADD RSC /l 0x413 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 "$(ZLIB_LIB)\zlib1.lib" kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"Release\coda.dll"

!ELSEIF  "$(CFG)" == "codadll - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "build\Debug\codadll"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "build\Debug\codadll"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
LIB32=link.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "." /I "..\libcoda" /I "..\libcoda\expat" /I ".." /I "$(ZLIB_INCLUDE)" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBCODADLL_EXPORTS" /D "LIBCODADLL" /D "HAVE_CONFIG_H" /YX /FD /GZ /Zm400 /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x413 /d "_DEBUG"
# ADD RSC /l 0x413 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 "$(ZLIB_LIB)\zlib1.lib" kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"Debug\coda_d.dll" /pdbtype:sept

!ELSEIF  "$(CFG)" == "codadll - Win32 Release with HDF"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "build\Release\codadll\withhdf"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release\withhdf"
# PROP Intermediate_Dir "build\Release\codadll\withhdf"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
LIB32=link.exe
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "." /I "..\libcoda" /I "..\libcoda\expat" /I ".." /I "$(ZLIB_INCLUDE)" /I "$(HDF4_INCLUDE)" /I "$(HDF5_INCLUDE)" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBCODADLL_EXPORTS" /D "LIBCODADLL" /D "HAVE_HDF4" /D "HAVE_HDF5" /D "_HDFDLL_" /D "_HDF5USEDLL_" /D "HAVE_CONFIG_H" /YX /FD /Zm400 /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x413 /d "NDEBUG"
# ADD RSC /l 0x413 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 "$(ZLIB_LIB)\zlib1.lib" "$(HDF4_LIB)\hd423m.lib" "$(HDF4_LIB)\hm423m.lib" "$(HDF5_LIB)\hdf5dll.lib" kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"Release\withhdf\coda.dll"

!ELSEIF  "$(CFG)" == "codadll - Win32 Debug with HDF"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "build\Debug\codadll\withhdf"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug\withhdf"
# PROP Intermediate_Dir "build\Debug\codadll\withhdf"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
LIB32=link.exe
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "." /I "..\libcoda" /I "..\libcoda\expat" /I ".." /I "$(ZLIB_INCLUDE)" /I "$(HDF4_INCLUDE)" /I "$(HDF5_INCLUDE)" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBCODADLL_EXPORTS" /D "LIBCODADLL" /D "HAVE_HDF4" /D "HAVE_HDF5" /D "_HDFDLL_" /D "_HDF5USEDLL_" /D "HAVE_CONFIG_H" /YX /FD /GZ /Zm400 /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x413 /d "_DEBUG"
# ADD RSC /l 0x413 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 "$(ZLIB_LIB)\zlib1.lib" "$(HDF4_LIB)\hd423m.lib" "$(HDF4_LIB)\hm423m.lib" "$(HDF5_LIB)\hdf5dll.lib" kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"Debug\withhdf\coda_d.dll" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "codadll - Win32 Release"
# Name "codadll - Win32 Debug"
# Name "codadll - Win32 Release with HDF"
# Name "codadll - Win32 Debug with HDF"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE="..\libcoda\coda-ascbin-cursor.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-ascbin-definition.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-ascbin-type.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-ascbin.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-ascii-cursor.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-ascii-definition.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-ascii-type.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-ascii.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-bin-cursor.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-bin-definition.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-bin-type.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-bin.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-cursor.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-definition-parse.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-definition.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-errno.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-expr.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-filefilter.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-hdf4-cursor.c"

!IF  "$(CFG)" == "codadll - Win32 Release"
# PROP Exclude_From_Build 1
!ELSEIF  "$(CFG)" == "codadll - Win32 Debug"
# PROP Exclude_From_Build 1
!ENDIF
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-hdf4-definition.c"

!IF  "$(CFG)" == "codadll - Win32 Release"
# PROP Exclude_From_Build 1
!ELSEIF  "$(CFG)" == "codadll - Win32 Debug"
# PROP Exclude_From_Build 1
!ENDIF
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-hdf4-type.c"

!IF  "$(CFG)" == "codadll - Win32 Release"
# PROP Exclude_From_Build 1
!ELSEIF  "$(CFG)" == "codadll - Win32 Debug"
# PROP Exclude_From_Build 1
!ENDIF
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-hdf5-cursor.c"

!IF  "$(CFG)" == "codadll - Win32 Release"
# PROP Exclude_From_Build 1
!ELSEIF  "$(CFG)" == "codadll - Win32 Debug"
# PROP Exclude_From_Build 1
!ENDIF
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-hdf5-definition.c"

!IF  "$(CFG)" == "codadll - Win32 Release"
# PROP Exclude_From_Build 1
!ELSEIF  "$(CFG)" == "codadll - Win32 Debug"
# PROP Exclude_From_Build 1
!ENDIF
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-hdf5-type.c"

!IF  "$(CFG)" == "codadll - Win32 Release"
# PROP Exclude_From_Build 1
!ELSEIF  "$(CFG)" == "codadll - Win32 Debug"
# PROP Exclude_From_Build 1
!ENDIF
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-netcdf-cursor.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-netcdf-dynamic.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-netcdf-type.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-netcdf.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-product.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-type.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-utils.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-xml-cursor.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-xml-definition.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-xml-dynamic.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-xml-parser.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-xml-type.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-xml.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\hashtable.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\ziparchive.c"
# End Source File
# Begin Source File

SOURCE="..\coda-expr-parser.c"
# End Source File
# Begin Source File

SOURCE="..\coda-expr-tokenizer.c"
# ADD CPP /D "YY_NO_UNISTD_H"
# End Source File
# Begin Source File

SOURCE="..\libcoda\expat\xmlparse.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\expat\xmlrole.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\expat\xmltok.c"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE="..\libcoda\coda-ascbin-definition.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-ascbin-internal.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-ascbin.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-ascii-definition.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-ascii-internal.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-ascii.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-bin-definition.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-bin-internal.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-bin.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-definition.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-expr-internal.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-expr.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-filefilter.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-hdf4-internal.h"

!IF  "$(CFG)" == "codadll - Win32 Release"
# PROP Exclude_From_Build 1
!ELSEIF  "$(CFG)" == "codadll - Win32 Debug"
# PROP Exclude_From_Build 1
!ENDIF
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-hdf4.h"

!IF  "$(CFG)" == "codadll - Win32 Release"
# PROP Exclude_From_Build 1
!ELSEIF  "$(CFG)" == "codadll - Win32 Debug"
# PROP Exclude_From_Build 1
!ENDIF
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-hdf5-internal.h"

!IF  "$(CFG)" == "codadll - Win32 Release"
# PROP Exclude_From_Build 1
!ELSEIF  "$(CFG)" == "codadll - Win32 Debug"
# PROP Exclude_From_Build 1
!ENDIF
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-hdf5.h"

!IF  "$(CFG)" == "codadll - Win32 Release"
# PROP Exclude_From_Build 1
!ELSEIF  "$(CFG)" == "codadll - Win32 Debug"
# PROP Exclude_From_Build 1
!ENDIF
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-internal.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-netcdf-internal.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-netcdf.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-path.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-xml-definition.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-xml-dynamic.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-xml-internal.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-xml.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\hashtable.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\ziparchive.h"
# End Source File
# Begin Source File

SOURCE="coda.h"
# End Source File
# Begin Source File

SOURCE="coda-expr-parser.h"
# End Source File
# Begin Source File

SOURCE="config.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\expat\ascii.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\expat\asciitab.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\expat\coda_expat_mangle.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\expat\expat.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\expat\expat_external.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\expat\iasciitab.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\expat\internal.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\expat\latin1tab.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\expat\nametab.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\expat\utf8tab.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\expat\xmlrole.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\expat\xmltok.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\expat\xmltol_impl.h"
# End Source File
# End Group
# End Target
# End Project
