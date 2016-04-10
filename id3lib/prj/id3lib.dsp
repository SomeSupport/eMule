# Microsoft Developer Studio Project File - Name="id3lib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=id3lib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "id3lib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "id3lib.mak" CFG="id3lib - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "id3lib - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "id3lib - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath "H/PC Ver. 2.00"
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "id3lib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D ID3LIB_LINKOPTION=2 /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I ".\\" /I "..\\" /I "..\include" /I "..\include\id3" /I "..\zlib\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "__DLL" /D "HAVE_CONFIG_H" /D ID3LIB_LINKOPTION=2 /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 ..\zlib\zlib.lib /nologo /subsystem:windows /dll /incremental:yes /machine:I386
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "id3lib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D ID3LIB_LINKOPTION=2 /YX /FD /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I ".\\" /I "..\\" /I "..\include" /I "..\include\id3" /I "..\zlib\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "__DLL" /D "HAVE_CONFIG_H" /D ID3LIB_LINKOPTION=2 /FR /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ..\zlib\zlibD.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "id3lib - Win32 Release"
# Name "id3lib - Win32 Debug"
# Begin Group "source"

# PROP Default_Filter "c;cpp"
# Begin Source File

SOURCE=..\src\c_wrapper.cpp
# End Source File
# Begin Source File

SOURCE=..\src\field.cpp
# End Source File
# Begin Source File

SOURCE=..\src\field_binary.cpp
# End Source File
# Begin Source File

SOURCE=..\src\field_integer.cpp
# End Source File
# Begin Source File

SOURCE=..\src\field_string_ascii.cpp
# End Source File
# Begin Source File

SOURCE=..\src\field_string_unicode.cpp
# End Source File
# Begin Source File

SOURCE=..\src\frame.cpp
# End Source File
# Begin Source File

SOURCE=..\src\frame_impl.cpp
# End Source File
# Begin Source File

SOURCE=..\src\frame_parse.cpp
# End Source File
# Begin Source File

SOURCE=..\src\frame_render.cpp
# End Source File
# Begin Source File

SOURCE=..\src\globals.cpp
# End Source File
# Begin Source File

SOURCE=..\src\header.cpp
# End Source File
# Begin Source File

SOURCE=..\src\header_frame.cpp
# End Source File
# Begin Source File

SOURCE=..\src\header_tag.cpp
# End Source File
# Begin Source File

SOURCE=..\src\helpers.cpp
# End Source File
# Begin Source File

SOURCE=..\src\io.cpp
# End Source File
# Begin Source File

SOURCE=..\src\io_decorators.cpp
# End Source File
# Begin Source File

SOURCE=..\src\io_helpers.cpp
# End Source File
# Begin Source File

SOURCE=..\src\misc_support.cpp
# End Source File
# Begin Source File

SOURCE=..\src\mp3_parse.cpp
# End Source File
# Begin Source File

SOURCE=..\src\readers.cpp
# End Source File
# Begin Source File

SOURCE=..\src\spec.cpp
# End Source File
# Begin Source File

SOURCE=..\src\tag.cpp
# End Source File
# Begin Source File

SOURCE=..\src\tag_file.cpp
# End Source File
# Begin Source File

SOURCE=..\src\tag_find.cpp
# End Source File
# Begin Source File

SOURCE=..\src\tag_impl.cpp
# End Source File
# Begin Source File

SOURCE=..\src\tag_parse.cpp
# End Source File
# Begin Source File

SOURCE=..\src\tag_parse_lyrics3.cpp
# End Source File
# Begin Source File

SOURCE=..\src\tag_parse_musicmatch.cpp
# End Source File
# Begin Source File

SOURCE=..\src\tag_parse_v1.cpp
# End Source File
# Begin Source File

SOURCE=..\src\tag_render.cpp
# End Source File
# Begin Source File

SOURCE=..\src\utils.cpp
# End Source File
# Begin Source File

SOURCE=..\src\writers.cpp
# End Source File
# End Group
# Begin Group "include"

# PROP Default_Filter "h;hpp"
# Begin Source File

SOURCE=..\include\id3\field.h
# End Source File
# Begin Source File

SOURCE=..\src\field_def.h
# End Source File
# Begin Source File

SOURCE=..\src\field_impl.h
# End Source File
# Begin Source File

SOURCE=..\src\flags.h
# End Source File
# Begin Source File

SOURCE=..\include\id3\id3lib_frame.h
# End Source File
# Begin Source File

SOURCE=..\src\frame_def.h
# End Source File
# Begin Source File

SOURCE=..\src\frame_impl.h
# End Source File
# Begin Source File

SOURCE=..\include\id3\globals.h
# End Source File
# Begin Source File

SOURCE=..\src\header.h
# End Source File
# Begin Source File

SOURCE=..\src\header_frame.h
# End Source File
# Begin Source File

SOURCE=..\src\header_tag.h
# End Source File
# Begin Source File

SOURCE=..\include\id3\helpers.h
# End Source File
# Begin Source File

SOURCE=..\include\id3.h
# End Source File
# Begin Source File

SOURCE=..\include\id3\id3lib_streams.h
# End Source File
# Begin Source File

SOURCE=..\include\id3\id3lib_strings.h
# End Source File
# Begin Source File

SOURCE=..\include\id3\io_decorators.h
# End Source File
# Begin Source File

SOURCE=..\include\id3\io_helpers.h
# End Source File
# Begin Source File

SOURCE=..\include\id3\io_strings.h
# End Source File
# Begin Source File

SOURCE=..\include\id3\misc_support.h
# End Source File
# Begin Source File

SOURCE=..\src\mp3_header.h
# End Source File
# Begin Source File

SOURCE=..\include\id3\reader.h
# End Source File
# Begin Source File

SOURCE=..\include\id3\readers.h
# End Source File
# Begin Source File

SOURCE=..\include\id3\sized_types.h
# End Source File
# Begin Source File

SOURCE=..\src\spec.h
# End Source File
# Begin Source File

SOURCE=..\include\id3\tag.h
# End Source File
# Begin Source File

SOURCE=..\src\tag_impl.h
# End Source File
# Begin Source File

SOURCE=..\include\id3\utils.h
# End Source File
# Begin Source File

SOURCE=..\include\id3\writer.h
# End Source File
# Begin Source File

SOURCE=..\include\id3\writers.h
# End Source File
# Begin Source File

SOURCE=..\zlib\include\zconf.h
# End Source File
# Begin Source File

SOURCE=..\zlib\include\zlib.h
# End Source File
# End Group
# Begin Group "config"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\config.h.win32
# End Source File
# End Group
# Begin Group "definition file"

# PROP Default_Filter "def"
# Begin Source File

SOURCE=.\Id3lib.def
# End Source File
# End Group
# Begin Group "Resources"

# PROP Default_Filter "rc"
# Begin Source File

SOURCE=.\version.rc
# End Source File
# End Group
# End Target
# End Project
