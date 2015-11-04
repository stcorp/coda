# Microsoft Developer Studio Project File - Name="codalib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=codalib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "codalib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "codalib.mak" CFG="codalib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "codalib - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "codalib - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "codalib - Win32 Release with HDF" (based on "Win32 (x86) Static Library")
!MESSAGE "codalib - Win32 Debug with HDF" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "codalib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "build\Release\codalib"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "build\Release\codalib"
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "." /I "..\libcoda" /I "..\libcoda\expat" /I "..\libcoda\pcre" /I ".." /I "$(ZLIB_INCLUDE)" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "HAVE_CONFIG_H" /YX /FD /Zm400 /c
# ADD BASE RSC /l 0x413 /d "NDEBUG"
# ADD RSC /l 0x413 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Release\libcoda.lib"

!ELSEIF  "$(CFG)" == "codalib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "build\Debug\codalib"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "build\Debug\codalib"
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "." /I "..\libcoda" /I "..\libcoda\expat" /I "..\libcoda\pcre" /I ".." /I "$(ZLIB_INCLUDE)" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "HAVE_CONFIG_H" /FR /YX /FD /GZ /Zm400 /c
# ADD BASE RSC /l 0x413 /d "_DEBUG"
# ADD RSC /l 0x413 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Debug\libcoda_d.lib"

!ELSEIF  "$(CFG)" == "codalib - Win32 Release with HDF"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release\withhdf"
# PROP BASE Intermediate_Dir "build\Release\codalib\withhdf"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release\withhdf"
# PROP Intermediate_Dir "build\Release\codalib\withhdf"
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "." /I "..\libcoda" /I "..\libcoda\expat" /I "..\libcoda\pcre" /I ".." /I "$(ZLIB_INCLUDE)" /I "$(HDF4_INCLUDE)" /I "$(HDF5_INCLUDE)" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "HAVE_HDF4" /D "HAVE_HDF5" /D "HAVE_CONFIG_H" /YX /FD /Zm400 /c
# ADD BASE RSC /l 0x413 /d "NDEBUG"
# ADD RSC /l 0x413 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Release\withhdf\libcoda.lib"

!ELSEIF  "$(CFG)" == "codalib - Win32 Debug with HDF"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug\withhdf"
# PROP BASE Intermediate_Dir "build\Debug\codalib\withhdf"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug\withhdf"
# PROP Intermediate_Dir "build\Debug\codalib\withhdf"
# PROP Target_Dir ""
MTL=midl.exe
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "." /I "..\libcoda" /I "..\libcoda\expat" /I "..\libcoda\pcre" /I ".." /I "$(ZLIB_INCLUDE)" /I "$(HDF4_INCLUDE)" /I "$(HDF5_INCLUDE)" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "HAVE_HDF4" /D "HAVE_HDF5" /D "HAVE_CONFIG_H" /FR /YX /FD /GZ /Zm400 /c
# ADD BASE RSC /l 0x413 /d "_DEBUG"
# ADD RSC /l 0x413 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Debug\withhdf\libcoda_d.lib"

!ENDIF 

# Begin Target

# Name "codalib - Win32 Release"
# Name "codalib - Win32 Debug"
# Name "codalib - Win32 Release with HDF"
# Name "codalib - Win32 Debug with HDF"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE="..\libcoda\coda-ascbin-cursor.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-ascbin.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-ascii-cursor.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-bin-cursor.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-cursor-read.c"
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

SOURCE="..\libcoda\coda-grib-cursor.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-grib-type.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-grib.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-hdf4-cursor.c"

!IF  "$(CFG)" == "codalib - Win32 Release"
# PROP Exclude_From_Build 1
!ELSEIF  "$(CFG)" == "codalib - Win32 Debug"
# PROP Exclude_From_Build 1
!ENDIF 
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-hdf4-type.c"

!IF  "$(CFG)" == "codalib - Win32 Release"
# PROP Exclude_From_Build 1
!ELSEIF  "$(CFG)" == "codalib - Win32 Debug"
# PROP Exclude_From_Build 1
!ENDIF 
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-hdf4.c"

!IF  "$(CFG)" == "codalib - Win32 Release"
# PROP Exclude_From_Build 1
!ELSEIF  "$(CFG)" == "codalib - Win32 Debug"
# PROP Exclude_From_Build 1
!ENDIF 
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-hdf5-cursor.c"

!IF  "$(CFG)" == "codalib - Win32 Release"
# PROP Exclude_From_Build 1
!ELSEIF  "$(CFG)" == "codalib - Win32 Debug"
# PROP Exclude_From_Build 1
!ENDIF 
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-hdf5-type.c"

!IF  "$(CFG)" == "codalib - Win32 Release"
# PROP Exclude_From_Build 1
!ELSEIF  "$(CFG)" == "codalib - Win32 Debug"
# PROP Exclude_From_Build 1
!ENDIF 
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-hdf5.c"

!IF  "$(CFG)" == "codalib - Win32 Release"
# PROP Exclude_From_Build 1
!ELSEIF  "$(CFG)" == "codalib - Win32 Debug"
# PROP Exclude_From_Build 1
!ENDIF 
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-mem-cursor.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-mem-type.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-mem.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-netcdf-cursor.c"
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

SOURCE="..\libcoda\coda-rinex.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-sp3c.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-time.c"
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
# Begin Source File

SOURCE="..\libcoda\pcre\pcre_chartables.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\pcre\pcre_compile.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\pcre\pcre_config.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\pcre\pcre_dfa_exec.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\pcre\pcre_exec.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\pcre\pcre_fullinfo.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\pcre\pcre_get.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\pcre\pcre_globals.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\pcre\pcre_info.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\pcre\pcre_maketables.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\pcre\pcre_newline.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\pcre\pcre_ord2utf8.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\pcre\pcre_refcount.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\pcre\pcre_study.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\pcre\pcre_tables.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\pcre\pcre_try_flipped.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\pcre\pcre_ucd.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\pcre\pcre_valid_utf8.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\pcre\pcre_version.c"
# End Source File
# Begin Source File

SOURCE="..\libcoda\pcre\pcre_xclass.c"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE="..\libcoda\coda-ascbin-internal.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-ascbin.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-ascii.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-bin.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-definition.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-expr.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-filefilter.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-grib-internal.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-grib.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-hdf4-internal.h"

!IF  "$(CFG)" == "codalib - Win32 Release"
# PROP Exclude_From_Build 1
!ELSEIF  "$(CFG)" == "codalib - Win32 Debug"
# PROP Exclude_From_Build 1
!ENDIF 
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-hdf4.h"

!IF  "$(CFG)" == "codalib - Win32 Release"
# PROP Exclude_From_Build 1
!ELSEIF  "$(CFG)" == "codalib - Win32 Debug"
# PROP Exclude_From_Build 1
!ENDIF 
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-hdf5-internal.h"

!IF  "$(CFG)" == "codalib - Win32 Release"
# PROP Exclude_From_Build 1
!ELSEIF  "$(CFG)" == "codalib - Win32 Debug"
# PROP Exclude_From_Build 1
!ENDIF 
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-hdf5.h"

!IF  "$(CFG)" == "codalib - Win32 Release"
# PROP Exclude_From_Build 1
!ELSEIF  "$(CFG)" == "codalib - Win32 Debug"
# PROP Exclude_From_Build 1
!ENDIF 
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-internal.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-mem-internal.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-mem.h"
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

SOURCE="..\libcoda\coda-rinex.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\coda-sp3c.h"
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
# Begin Source File

SOURCE="..\libcoda\pcre\coda_pcre_config.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\pcre\coda_pcre_mangle.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\pcre\pcre_internal.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\pcre\pcre.h"
# End Source File
# Begin Source File

SOURCE="..\libcoda\pcre\ucp.h"
# End Source File
# End Group
# End Target
# End Project
