# Microsoft Developer Studio Generated NMAKE File, Based on id3lib.dsp

!IF "$(CFG)" == ""

CFG=id3lib - Win32 Debug

!MESSAGE No configuration specified. Defaulting to id3lib - Win32 Debug.

!ENDIF 



!IF "$(CFG)" != "id3lib - Win32 Release" && "$(CFG)" != "id3lib - Win32 Debug"

!MESSAGE Invalid configuration "$(CFG)" specified.

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

!ERROR An invalid configuration is specified.

!ENDIF 



!IF "$(OS)" == "Windows_NT"

NULL=

!ELSE 

NULL=nul

!ENDIF 



CPP=cl.exe

MTL=midl.exe

RSC=rc.exe



!IF  "$(CFG)" == "id3lib - Win32 Release"



OUTDIR=.\Release

INTDIR=.\Release

# Begin Custom Macros

OutDir=.\Release

# End Custom Macros



!IF "$(RECURSE)" == "0" 



ALL : "$(OUTDIR)\id3lib.dll"



!ELSE 



ALL : "$(OUTDIR)\id3lib.dll"



!ENDIF 



CLEAN :

	-@erase "$(INTDIR)\c_wrapper.obj"

	-@erase "$(INTDIR)\error.obj"

	-@erase "$(INTDIR)\field.obj"

	-@erase "$(INTDIR)\field_binary.obj"

	-@erase "$(INTDIR)\field_integer.obj"

	-@erase "$(INTDIR)\field_string_ascii.obj"

	-@erase "$(INTDIR)\field_string_unicode.obj"

	-@erase "$(INTDIR)\frame.obj"

	-@erase "$(INTDIR)\frame_parse.obj"

	-@erase "$(INTDIR)\frame_render.obj"

	-@erase "$(INTDIR)\header.obj"

	-@erase "$(INTDIR)\globals.obj"

	-@erase "$(INTDIR)\header_frame.obj"

	-@erase "$(INTDIR)\header_tag.obj"

	-@erase "$(INTDIR)\int28.obj"

	-@erase "$(INTDIR)\misc_support.obj"

	-@erase "$(INTDIR)\tag.obj"

	-@erase "$(INTDIR)\tag_file.obj"

	-@erase "$(INTDIR)\tag_find.obj"

	-@erase "$(INTDIR)\tag_parse.obj"

	-@erase "$(INTDIR)\tag_parse_lyrics3.obj"

	-@erase "$(INTDIR)\tag_parse_v1.obj"

	-@erase "$(INTDIR)\tag_render.obj"

	-@erase "$(INTDIR)\tag_sync.obj"

	-@erase "$(INTDIR)\vc50.idb"

	-@erase "$(OUTDIR)\id3lib.dll"

	-@erase "$(OUTDIR)\id3lib.exp"

	-@erase "$(OUTDIR)\id3lib.lib"



"$(OUTDIR)" :

    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"



CPP_PROJ=/nologo /MT /W3 /GX /O2 /I ".\\" /I "..\\" /I "..\include" /I\

 "..\include\id3" /I "..\zlib\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS"\

 /D "HAVE_CONFIG_H" /D "__DLL" /Fp"$(INTDIR)\id3lib.pch" /YX /Fo"$(INTDIR)\\"\

 /Fd"$(INTDIR)\\" /FD /c 

CPP_OBJS=.\Release/

CPP_SBRS=.

MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o NUL /win32 

BSC32=bscmake.exe

BSC32_FLAGS=/nologo /o"$(OUTDIR)\id3lib.bsc" 

BSC32_SBRS= \

	

LINK32=link.exe

LINK32_FLAGS=zlib.lib kernel32.lib user32.lib gdi32.lib winspool.lib\

 comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib\

 odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /incremental:no\

 /pdb:"$(OUTDIR)\id3lib.pdb" /machine:I386 /out:"$(OUTDIR)\id3lib.dll"\

 /implib:"$(OUTDIR)\id3lib.lib" /libpath:"..\zlib" 

LINK32_OBJS= \

	"$(INTDIR)\c_wrapper.obj" \

	"$(INTDIR)\error.obj" \

	"$(INTDIR)\field.obj" \

	"$(INTDIR)\field_binary.obj" \

	"$(INTDIR)\field_integer.obj" \

	"$(INTDIR)\field_string_ascii.obj" \

	"$(INTDIR)\field_string_unicode.obj" \

	"$(INTDIR)\frame.obj" \

	"$(INTDIR)\frame_parse.obj" \

	"$(INTDIR)\frame_render.obj" \

	"$(INTDIR)\globals.obj" \

	"$(INTDIR)\header.obj" \

	"$(INTDIR)\header_frame.obj" \

	"$(INTDIR)\header_tag.obj" \

	"$(INTDIR)\int28.obj" \

	"$(INTDIR)\misc_support.obj" \

	"$(INTDIR)\tag.obj" \

	"$(INTDIR)\tag_file.obj" \

	"$(INTDIR)\tag_find.obj" \

	"$(INTDIR)\tag_parse.obj" \

	"$(INTDIR)\tag_parse_lyrics3.obj" \

	"$(INTDIR)\tag_parse_v1.obj" \

	"$(INTDIR)\tag_render.obj" \

	"$(INTDIR)\tag_sync.obj"



"$(OUTDIR)\id3lib.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)

    $(LINK32) @<<

  $(LINK32_FLAGS) $(LINK32_OBJS)

<<



!ELSEIF  "$(CFG)" == "id3lib - Win32 Debug"



OUTDIR=.\Debug

INTDIR=.\Debug

# Begin Custom Macros

OutDir=.\Debug

# End Custom Macros



!IF "$(RECURSE)" == "0" 



ALL : "$(OUTDIR)\id3lib.dll"



!ELSE 



ALL : "$(OUTDIR)\id3lib.dll"



!ENDIF 



CLEAN :

	-@erase "$(INTDIR)\c_wrapper.obj"

	-@erase "$(INTDIR)\error.obj"

	-@erase "$(INTDIR)\field.obj"

	-@erase "$(INTDIR)\field_binary.obj"

	-@erase "$(INTDIR)\field_integer.obj"

	-@erase "$(INTDIR)\field_string_ascii.obj"

	-@erase "$(INTDIR)\field_string_unicode.obj"

	-@erase "$(INTDIR)\frame.obj"

	-@erase "$(INTDIR)\frame_parse.obj"

	-@erase "$(INTDIR)\frame_render.obj"

	-@erase "$(INTDIR)\globals.obj"

	-@erase "$(INTDIR)\header.obj"

	-@erase "$(INTDIR)\header_frame.obj"

	-@erase "$(INTDIR)\header_tag.obj"

	-@erase "$(INTDIR)\int28.obj"

	-@erase "$(INTDIR)\misc_support.obj"

	-@erase "$(INTDIR)\tag.obj"

	-@erase "$(INTDIR)\tag_file.obj"

	-@erase "$(INTDIR)\tag_find.obj"

	-@erase "$(INTDIR)\tag_parse.obj"

	-@erase "$(INTDIR)\tag_parse_lyrics3.obj"

	-@erase "$(INTDIR)\tag_parse_v1.obj"

	-@erase "$(INTDIR)\tag_render.obj"

	-@erase "$(INTDIR)\tag_sync.obj"

	-@erase "$(INTDIR)\vc50.idb"

	-@erase "$(INTDIR)\vc50.pdb"

	-@erase "$(OUTDIR)\id3lib.dll"

	-@erase "$(OUTDIR)\id3lib.exp"

	-@erase "$(OUTDIR)\id3lib.ilk"

	-@erase "$(OUTDIR)\id3lib.lib"

	-@erase "$(OUTDIR)\id3lib.pdb"



"$(OUTDIR)" :

    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"



CPP_PROJ=/nologo /MDd /W3 /Gm /GX /Zi /Od /I ".\\" /I "..\\" /I "..\include" /I\

 "..\include\id3" /I "..\zlib\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS"\

 /D "HAVE_CONFIG_H" /D "__DLL" /Fp"$(INTDIR)\id3lib.pch" /YX /Fo"$(INTDIR)\\"\

 /Fd"$(INTDIR)\\" /FD /c 

CPP_OBJS=.\Debug/

CPP_SBRS=.

MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o NUL /win32 

BSC32=bscmake.exe

BSC32_FLAGS=/nologo /o"$(OUTDIR)\id3lib.bsc" 

BSC32_SBRS= \

	

LINK32=link.exe

LINK32_FLAGS=zlib.lib kernel32.lib user32.lib gdi32.lib winspool.lib\

 comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib\

 odbc32.lib odbccp32.lib /nologo /subsystem:windows /dll /incremental:yes\

 /pdb:"$(OUTDIR)\id3lib.pdb" /debug /machine:I386 /out:"$(OUTDIR)\id3lib.dll"\

 /implib:"$(OUTDIR)\id3lib.lib" /pdbtype:sept /libpath:"..\zlib" 

LINK32_OBJS= \

	"$(INTDIR)\c_wrapper.obj" \

	"$(INTDIR)\error.obj" \

	"$(INTDIR)\field.obj" \

	"$(INTDIR)\field_binary.obj" \

	"$(INTDIR)\field_integer.obj" \

	"$(INTDIR)\field_string_ascii.obj" \

	"$(INTDIR)\field_string_unicode.obj" \

	"$(INTDIR)\frame.obj" \

	"$(INTDIR)\frame_parse.obj" \

	"$(INTDIR)\frame_render.obj" \

	"$(INTDIR)\globals.obj" \

	"$(INTDIR)\header.obj" \

	"$(INTDIR)\header_frame.obj" \

	"$(INTDIR)\header_tag.obj" \

	"$(INTDIR)\int28.obj" \

	"$(INTDIR)\misc_support.obj" \

	"$(INTDIR)\tag.obj" \

	"$(INTDIR)\tag_file.obj" \

	"$(INTDIR)\tag_find.obj" \

	"$(INTDIR)\tag_parse.obj" \

	"$(INTDIR)\tag_parse_lyrics3.obj" \

	"$(INTDIR)\tag_parse_v1.obj" \

	"$(INTDIR)\tag_render.obj" \

	"$(INTDIR)\tag_sync.obj"



"$(OUTDIR)\id3lib.dll" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)

    $(LINK32) @<<

  $(LINK32_FLAGS) $(LINK32_OBJS)

<<



!ENDIF 



.c{$(CPP_OBJS)}.obj::

   $(CPP) @<<

   $(CPP_PROJ) $< 

<<



.cpp{$(CPP_OBJS)}.obj::

   $(CPP) @<<

   $(CPP_PROJ) $< 

<<



.cxx{$(CPP_OBJS)}.obj::

   $(CPP) @<<

   $(CPP_PROJ) $< 

<<



.c{$(CPP_SBRS)}.sbr::

   $(CPP) @<<

   $(CPP_PROJ) $< 

<<



.cpp{$(CPP_SBRS)}.sbr::

   $(CPP) @<<

   $(CPP_PROJ) $< 

<<



.cxx{$(CPP_SBRS)}.sbr::

   $(CPP) @<<

   $(CPP_PROJ) $< 

<<





!IF "$(CFG)" == "id3lib - Win32 Release" || "$(CFG)" == "id3lib - Win32 Debug"

SOURCE=..\src\c_wrapper.cpp

DEP_CPP_DLL_W=\

	"..\config.h"\

	"..\include\id3\error.h"\

	"..\include\id3\field.h"\

	"..\include\id3\frame.h"\

	"..\include\id3\globals.h"\

	"..\include\id3\header.h"\

	"..\include\id3\header_frame.h"\

	"..\include\id3\header_tag.h"\

	"..\include\id3\int28.h"\

	"..\include\id3\sized_types.h"\

	"..\include\id3\tag.h"\

	



"$(INTDIR)\c_wrapper.obj" : $(SOURCE) $(DEP_CPP_DLL_W) "$(INTDIR)"\

 "..\config.h"

	$(CPP) $(CPP_PROJ) $(SOURCE)





SOURCE=..\src\error.cpp

DEP_CPP_ERROR=\

	"..\config.h"\

	



"$(INTDIR)\error.obj" : $(SOURCE) $(DEP_CPP_ERROR) "$(INTDIR)" "..\config.h"

	$(CPP) $(CPP_PROJ) $(SOURCE)





SOURCE=..\src\field.cpp

DEP_CPP_FIELD=\

	"..\config.h"\

	"..\include\id3\error.h"\

	"..\include\id3\field.h"\

	"..\include\id3\globals.h"\

	"..\include\id3\int28.h"\

	"..\include\id3\sized_types.h"\

	



"$(INTDIR)\field.obj" : $(SOURCE) $(DEP_CPP_FIELD) "$(INTDIR)" "..\config.h"

	$(CPP) $(CPP_PROJ) $(SOURCE)





SOURCE=..\src\field_binary.cpp

DEP_CPP_FIELD_=\

	"..\config.h"\

	"..\include\id3\error.h"\

	"..\include\id3\field.h"\

	"..\include\id3\globals.h"\

	"..\include\id3\int28.h"\

	"..\include\id3\sized_types.h"\

	



"$(INTDIR)\field_binary.obj" : $(SOURCE) $(DEP_CPP_FIELD_) "$(INTDIR)"\

 "..\config.h"

	$(CPP) $(CPP_PROJ) $(SOURCE)





SOURCE=..\src\field_integer.cpp

DEP_CPP_FIELD_I=\

	"..\config.h"\

	"..\include\id3\error.h"\

	"..\include\id3\field.h"\

	"..\include\id3\globals.h"\

	"..\include\id3\int28.h"\

	"..\include\id3\sized_types.h"\

	



"$(INTDIR)\field_integer.obj" : $(SOURCE) $(DEP_CPP_FIELD_I) "$(INTDIR)"\

 "..\config.h"

	$(CPP) $(CPP_PROJ) $(SOURCE)





SOURCE=..\src\field_string_ascii.cpp

DEP_CPP_FIELD_S=\

	"..\config.h"\

	"..\include\id3\error.h"\

	"..\include\id3\field.h"\

	"..\include\id3\frame.h"\

	"..\include\id3\globals.h"\

	"..\include\id3\header.h"\

	"..\include\id3\header_frame.h"\

	"..\include\id3\header_tag.h"\

	"..\include\id3\int28.h"\

	"..\include\id3\misc_support.h"\

	"..\include\id3\sized_types.h"\

	"..\include\id3\tag.h"\

	



"$(INTDIR)\field_string_ascii.obj" : $(SOURCE) $(DEP_CPP_FIELD_S) "$(INTDIR)"\

 "..\config.h"

	$(CPP) $(CPP_PROJ) $(SOURCE)





SOURCE=..\src\field_string_unicode.cpp

DEP_CPP_FIELD_ST=\

	"..\config.h"\

	"..\include\id3\error.h"\

	"..\include\id3\field.h"\

	"..\include\id3\frame.h"\

	"..\include\id3\globals.h"\

	"..\include\id3\header.h"\

	"..\include\id3\header_frame.h"\

	"..\include\id3\header_tag.h"\

	"..\include\id3\int28.h"\

	"..\include\id3\misc_support.h"\

	"..\include\id3\sized_types.h"\

	"..\include\id3\tag.h"\

	



"$(INTDIR)\field_string_unicode.obj" : $(SOURCE) $(DEP_CPP_FIELD_ST)\

 "$(INTDIR)" "..\config.h"

	$(CPP) $(CPP_PROJ) $(SOURCE)





SOURCE=..\src\frame.cpp

DEP_CPP_FRAME=\

	"..\config.h"\

	"..\include\id3\error.h"\

	"..\include\id3\field.h"\

	"..\include\id3\frame.h"\

	"..\include\id3\globals.h"\

	"..\include\id3\header.h"\

	"..\include\id3\header_frame.h"\

	"..\include\id3\header_tag.h"\

	"..\include\id3\int28.h"\

	"..\include\id3\sized_types.h"\

	"..\include\id3\tag.h"\

	



"$(INTDIR)\frame.obj" : $(SOURCE) $(DEP_CPP_FRAME) "$(INTDIR)" "..\config.h"

	$(CPP) $(CPP_PROJ) $(SOURCE)





SOURCE=..\src\frame_parse.cpp

DEP_CPP_FRAME_=\

	"..\zlib\include\zconf.h"\

	"..\zlib\include\zlib.h"\

	"..\config.h"\

	"..\include\id3\error.h"\

	"..\include\id3\field.h"\

	"..\include\id3\frame.h"\

	"..\include\id3\globals.h"\

	"..\include\id3\header.h"\

	"..\include\id3\header_frame.h"\

	"..\include\id3\header_tag.h"\

	"..\include\id3\int28.h"\

	"..\include\id3\sized_types.h"\

	{$(INCLUDE)}"sys\types.h"\

	



"$(INTDIR)\frame_parse.obj" : $(SOURCE) $(DEP_CPP_FRAME_) "$(INTDIR)"\

 "..\config.h"

	$(CPP) $(CPP_PROJ) $(SOURCE)





SOURCE=..\src\frame_render.cpp

DEP_CPP_FRAME_R=\

	"..\zlib\include\zconf.h"\

	"..\zlib\include\zlib.h"\

	"..\config.h"\

	"..\include\id3\error.h"\

	"..\include\id3\field.h"\

	"..\include\id3\frame.h"\

	"..\include\id3\globals.h"\

	"..\include\id3\header.h"\

	"..\include\id3\header_frame.h"\

	"..\include\id3\header_tag.h"\

	"..\include\id3\int28.h"\

	"..\include\id3\misc_support.h"\

	"..\include\id3\sized_types.h"\

	"..\include\id3\tag.h"\

	{$(INCLUDE)}"sys\types.h"\

	



"$(INTDIR)\frame_render.obj" : $(SOURCE) $(DEP_CPP_FRAME_R) "$(INTDIR)"\

 "..\config.h"

	$(CPP) $(CPP_PROJ) $(SOURCE)





SOURCE=..\src\globals.cpp

DEP_CPP_HEADE=\

	"..\config.h"\

	"..\include\id3\globals.h"\

	"..\include\id3\sized_types.h"\

	



"$(INTDIR)\globals.obj" : $(SOURCE) $(DEP_CPP_HEADE) "$(INTDIR)" "..\config.h"

	$(CPP) $(CPP_PROJ) $(SOURCE)





SOURCE=..\src\header.cpp

DEP_CPP_HEADE=\

	"..\config.h"\

	"..\include\id3\globals.h"\

	"..\include\id3\header.h"\

	"..\include\id3\int28.h"\

	"..\include\id3\sized_types.h"\

	



"$(INTDIR)\header.obj" : $(SOURCE) $(DEP_CPP_HEADE) "$(INTDIR)" "..\config.h"

	$(CPP) $(CPP_PROJ) $(SOURCE)





SOURCE=..\src\header_frame.cpp

DEP_CPP_HEADER=\

	"..\config.h"\

	"..\include\id3\error.h"\

	"..\include\id3\field.h"\

	"..\include\id3\globals.h"\

	"..\include\id3\header.h"\

	"..\include\id3\header_frame.h"\

	"..\include\id3\header_tag.h"\

	"..\include\id3\int28.h"\

	"..\include\id3\sized_types.h"\

	



"$(INTDIR)\header_frame.obj" : $(SOURCE) $(DEP_CPP_HEADER) "$(INTDIR)"\

 "..\config.h"

	$(CPP) $(CPP_PROJ) $(SOURCE)





SOURCE=..\src\header_tag.cpp

DEP_CPP_HEADER_=\

	"..\config.h"\

	"..\include\id3\globals.h"\

	"..\include\id3\header.h"\

	"..\include\id3\header_tag.h"\

	"..\include\id3\int28.h"\

	"..\include\id3\sized_types.h"\

	



"$(INTDIR)\header_tag.obj" : $(SOURCE) $(DEP_CPP_HEADER_) "$(INTDIR)"\

 "..\config.h"

	$(CPP) $(CPP_PROJ) $(SOURCE)





SOURCE=..\src\int28.cpp

DEP_CPP_INT28=\

	"..\config.h"\

	"..\include\id3\globals.h"\

	"..\include\id3\int28.h"\

	"..\include\id3\sized_types.h"\

	



"$(INTDIR)\int28.obj" : $(SOURCE) $(DEP_CPP_INT28) "$(INTDIR)" "..\config.h"

	$(CPP) $(CPP_PROJ) $(SOURCE)





SOURCE=..\src\misc_support.cpp

DEP_CPP_MISC_=\

	"..\config.h"\

	"..\include\id3\error.h"\

	"..\include\id3\field.h"\

	"..\include\id3\frame.h"\

	"..\include\id3\globals.h"\

	"..\include\id3\header.h"\

	"..\include\id3\header_frame.h"\

	"..\include\id3\header_tag.h"\

	"..\include\id3\int28.h"\

	"..\include\id3\misc_support.h"\

	"..\include\id3\sized_types.h"\

	"..\include\id3\tag.h"\

	



"$(INTDIR)\misc_support.obj" : $(SOURCE) $(DEP_CPP_MISC_) "$(INTDIR)"\

 "..\config.h"

	$(CPP) $(CPP_PROJ) $(SOURCE)





SOURCE=..\src\tag.cpp

DEP_CPP_TAG_C=\

	"..\config.h"\

	"..\include\id3\error.h"\

	"..\include\id3\field.h"\

	"..\include\id3\frame.h"\

	"..\include\id3\globals.h"\

	"..\include\id3\header.h"\

	"..\include\id3\header_frame.h"\

	"..\include\id3\header_tag.h"\

	"..\include\id3\int28.h"\

	"..\include\id3\sized_types.h"\

	"..\include\id3\tag.h"\

	



"$(INTDIR)\tag.obj" : $(SOURCE) $(DEP_CPP_TAG_C) "$(INTDIR)" "..\config.h"

	$(CPP) $(CPP_PROJ) $(SOURCE)





SOURCE=..\src\tag_file.cpp

DEP_CPP_TAG_F=\

	"..\config.h"\

	"..\include\id3\error.h"\

	"..\include\id3\field.h"\

	"..\include\id3\frame.h"\

	"..\include\id3\globals.h"\

	"..\include\id3\header.h"\

	"..\include\id3\header_frame.h"\

	"..\include\id3\header_tag.h"\

	"..\include\id3\int28.h"\

	"..\include\id3\sized_types.h"\

	"..\include\id3\tag.h"\

	



"$(INTDIR)\tag_file.obj" : $(SOURCE) $(DEP_CPP_TAG_F) "$(INTDIR)" "..\config.h"

	$(CPP) $(CPP_PROJ) $(SOURCE)





SOURCE=..\src\tag_find.cpp

DEP_CPP_TAG_FI=\

	"..\config.h"\

	"..\include\id3\error.h"\

	"..\include\id3\field.h"\

	"..\include\id3\frame.h"\

	"..\include\id3\globals.h"\

	"..\include\id3\header.h"\

	"..\include\id3\header_frame.h"\

	"..\include\id3\header_tag.h"\

	"..\include\id3\int28.h"\

	"..\include\id3\misc_support.h"\

	"..\include\id3\sized_types.h"\

	"..\include\id3\tag.h"\

	



"$(INTDIR)\tag_find.obj" : $(SOURCE) $(DEP_CPP_TAG_FI) "$(INTDIR)"\

 "..\config.h"

	$(CPP) $(CPP_PROJ) $(SOURCE)





SOURCE=..\src\tag_parse.cpp

DEP_CPP_TAG_P=\

	"..\zlib\include\zconf.h"\

	"..\zlib\include\zlib.h"\

	"..\config.h"\

	"..\include\id3\error.h"\

	"..\include\id3\field.h"\

	"..\include\id3\frame.h"\

	"..\include\id3\globals.h"\

	"..\include\id3\header.h"\

	"..\include\id3\header_frame.h"\

	"..\include\id3\header_tag.h"\

	"..\include\id3\int28.h"\

	"..\include\id3\sized_types.h"\

	"..\include\id3\tag.h"\

	{$(INCLUDE)}"sys\types.h"\

	



"$(INTDIR)\tag_parse.obj" : $(SOURCE) $(DEP_CPP_TAG_P) "$(INTDIR)"\

 "..\config.h"

	$(CPP) $(CPP_PROJ) $(SOURCE)





SOURCE=..\src\tag_parse_lyrics3.cpp

DEP_CPP_TAG_PA=\

	"..\config.h"\

	"..\include\id3\error.h"\

	"..\include\id3\field.h"\

	"..\include\id3\frame.h"\

	"..\include\id3\globals.h"\

	"..\include\id3\header.h"\

	"..\include\id3\header_frame.h"\

	"..\include\id3\header_tag.h"\

	"..\include\id3\int28.h"\

	"..\include\id3\misc_support.h"\

	"..\include\id3\sized_types.h"\

	"..\include\id3\tag.h"\

	



"$(INTDIR)\tag_parse_lyrics3.obj" : $(SOURCE) $(DEP_CPP_TAG_PA) "$(INTDIR)"\

 "..\config.h"

	$(CPP) $(CPP_PROJ) $(SOURCE)





SOURCE=..\src\tag_parse_v1.cpp

DEP_CPP_TAG_PAR=\

	"..\config.h"\

	"..\include\id3\error.h"\

	"..\include\id3\field.h"\

	"..\include\id3\frame.h"\

	"..\include\id3\globals.h"\

	"..\include\id3\header.h"\

	"..\include\id3\header_frame.h"\

	"..\include\id3\header_tag.h"\

	"..\include\id3\int28.h"\

	"..\include\id3\misc_support.h"\

	"..\include\id3\sized_types.h"\

	"..\include\id3\tag.h"\

	



"$(INTDIR)\tag_parse_v1.obj" : $(SOURCE) $(DEP_CPP_TAG_PAR) "$(INTDIR)"\

 "..\config.h"

	$(CPP) $(CPP_PROJ) $(SOURCE)





SOURCE=..\src\tag_render.cpp

DEP_CPP_TAG_R=\

	"..\config.h"\

	"..\include\id3\error.h"\

	"..\include\id3\field.h"\

	"..\include\id3\frame.h"\

	"..\include\id3\globals.h"\

	"..\include\id3\header.h"\

	"..\include\id3\header_frame.h"\

	"..\include\id3\header_tag.h"\

	"..\include\id3\int28.h"\

	"..\include\id3\misc_support.h"\

	"..\include\id3\sized_types.h"\

	"..\include\id3\tag.h"\

	



"$(INTDIR)\tag_render.obj" : $(SOURCE) $(DEP_CPP_TAG_R) "$(INTDIR)"\

 "..\config.h"

	$(CPP) $(CPP_PROJ) $(SOURCE)





SOURCE=..\src\tag_sync.cpp

DEP_CPP_TAG_S=\

	"..\config.h"\

	"..\include\id3\error.h"\

	"..\include\id3\field.h"\

	"..\include\id3\frame.h"\

	"..\include\id3\globals.h"\

	"..\include\id3\header.h"\

	"..\include\id3\header_frame.h"\

	"..\include\id3\header_tag.h"\

	"..\include\id3\int28.h"\

	"..\include\id3\sized_types.h"\

	"..\include\id3\tag.h"\

	



"$(INTDIR)\tag_sync.obj" : $(SOURCE) $(DEP_CPP_TAG_S) "$(INTDIR)" "..\config.h"

	$(CPP) $(CPP_PROJ) $(SOURCE)





SOURCE=..\config.win32



!IF  "$(CFG)" == "id3lib - Win32 Release"



InputPath=..\config.win32



"..\config.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"

	copy ..\config.win32 ..\config.h



!ELSEIF  "$(CFG)" == "id3lib - Win32 Debug"



InputPath=..\config.win32



"..\config.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"

	copy ..\config.win32 ..\config.h



!ENDIF 





!ENDIF 



