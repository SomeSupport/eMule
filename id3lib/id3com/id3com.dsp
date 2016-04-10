# Microsoft Developer Studio Project File - Name="id3com" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=id3com - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "id3com.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "id3com.mak" CFG="id3com - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "id3com - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "id3com - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath "Desktop"
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "id3com - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D ID3LIB_LINKOPTION=1 /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\\" /I "..\include" /D "_DEBUG" /D "_MBCS" /D "WIN32" /D "_WINDOWS" /D "HAVE_CONFIG_H" /D ID3LIB_LINKOPTION=1 /Yu"stdafx.h" /FD /GZ /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ../libprj/id3libd.lib ../zlib/zlibd.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"libcmtd" /pdbtype:sept
# ADD LINK32 ../libprj/id3libd.lib ../zlib/zlibd.lib /nologo /subsystem:windows /dll /debug /machine:I386 /nodefaultlib:"libcmtd" /pdbtype:sept
# Begin Custom Build - Performing registration
OutDir=.\Debug
TargetPath=.\Debug\id3com.dll
InputPath=.\Debug\id3com.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "id3com - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "id3com___Win32_Release"
# PROP BASE Intermediate_Dir "id3com___Win32_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O1 /I ".\\" /I "..\\" /I "..\include" /I "..\include\id3" /I "..\zlib\include" /D "NDEBUG" /D "_MBCS" /D "_ATL_STATIC_REGISTRY" /D "WIN32" /D "_WINDOWS" /D "HAVE_CONFIG_H" /D ID3LIB_LINKOPTION=1 /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /W3 /GX /O1 /I "..\\" /I "..\include" /D "NDEBUG" /D "_MBCS" /D "_ATL_STATIC_REGISTRY" /D "WIN32" /D "_WINDOWS" /D "HAVE_CONFIG_H" /D ID3LIB_LINKOPTION=1 /Yu"stdafx.h" /FD /c
# SUBTRACT CPP /Fr
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 ../libprj/id3lib.lib ../zlib/zlib.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib:"libcmt"
# ADD LINK32 ../libprj/id3lib.lib ../zlib/zlib.lib /nologo /subsystem:windows /dll /machine:I386 /nodefaultlib:"libcmt"
# Begin Custom Build - Performing registration
OutDir=.\Release
TargetPath=.\Release\id3com.dll
InputPath=.\Release\id3com.dll
SOURCE="$(InputPath)"

"$(OutDir)\regsvr32.trg" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	regsvr32 /s /c "$(TargetPath)" 
	echo regsvr32 exec. time > "$(OutDir)\regsvr32.trg" 
	
# End Custom Build

!ENDIF 

# Begin Target

# Name "id3com - Win32 Debug"
# Name "id3com - Win32 Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\EnumFields.cpp
# End Source File
# Begin Source File

SOURCE=.\id3com.cpp
# End Source File
# Begin Source File

SOURCE=.\id3com.def
# End Source File
# Begin Source File

SOURCE=.\id3com.idl
# ADD MTL /tlb ".\id3com.tlb" /h "id3com.h" /iid "id3com_i.c" /Oicf
# End Source File
# Begin Source File

SOURCE=.\id3com.rc
# End Source File
# Begin Source File

SOURCE=.\ID3Field.cpp
# End Source File
# Begin Source File

SOURCE=.\ID3Frame.cpp
# End Source File
# Begin Source File

SOURCE=.\ID3Tag.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\EnumFields.h
# End Source File
# Begin Source File

SOURCE=.\ID3Field.h
# End Source File
# Begin Source File

SOURCE=.\ID3Frame.h
# End Source File
# Begin Source File

SOURCE=.\ID3Tag.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\ID3ComFieldLong.rgs
# End Source File
# Begin Source File

SOURCE=.\ID3ComFieldPicture.rgs
# End Source File
# Begin Source File

SOURCE=.\ID3ComFieldText.rgs
# End Source File
# Begin Source File

SOURCE=.\ID3Field.rgs
# End Source File
# Begin Source File

SOURCE=.\ID3Frame.rgs
# End Source File
# Begin Source File

SOURCE=.\ID3Tag.rgs
# End Source File
# Begin Source File

SOURCE=.\TextCollection.rgs
# End Source File
# End Group
# Begin Source File

SOURCE=..\ChangeLog
# End Source File
# Begin Source File

SOURCE=..\News
# End Source File
# Begin Source File

SOURCE=..\Todo
# End Source File
# End Target
# End Project
