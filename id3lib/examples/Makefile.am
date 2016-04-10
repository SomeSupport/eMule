# Copyright (C) 1999 Scott Thomas Haug <scott@id3.org>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

if ID3_NEEDDEBUG
ID3_DEBUG_LIBS =  -lcwd -lbfd -liberty
else
ID3_DEBUG_LIBS =
endif

if ID3_NEEDZLIB
zlib_lib = $(top_builddir)/zlib/src/libz.la
zlib_include = -I$(top_srcdir)/zlib/include
else
zlib_lib = -lz
zlib_include =
endif

if ID3_NEEDGETOPT_LONG
getopt_lib = getopt.o getopt1.o
else
getopt_lib = 
endif

LDADD =  $(top_builddir)/src/libid3.la $(zlib_lib) $(ID3_DEBUG_LIBS) $(getopt_lib)

INCLUDES = @ID3LIB_DEBUG_FLAGS@ -I$(top_srcdir)/include

bin_PROGRAMS            = id3info id3convert id3tag id3cp
check_PROGRAMS          = \
  id3simple               \
  testpic                 \
  testunicode             \
  testcompression         \
  testremove              \
  testio                  \
  get_pic                 \
  findstr                 \
  findeng

id3cp_SOURCES           = demo_copy_options.c    demo_copy.cpp

id3info_SOURCES         = demo_info_options.c    demo_info.cpp

id3convert_SOURCES      = demo_convert_options.c demo_convert.cpp

id3tag_SOURCES          = demo_tag_options.c     demo_tag.cpp

id3simple_SOURCES       = demo_simple.cpp
testpic_SOURCES         = test_pic.cpp
testunicode_SOURCES     = test_unicode.cpp
testcompression_SOURCES = test_compression.cpp
testremove_SOURCES      = test_remove.cpp
testio_SOURCES          = test_io.cpp
get_pic_SOURCES         = get_pic.cpp
findeng_SOURCES         = findeng.cpp
findstr_SOURCES         = findstr.cpp

tag_files =             \
  composer.jpg          \
  demo_main.cpp         \
  221-compressed.tag    \
  230-compressed.tag    \
  230-picture.tag       \
  230-unicode.tag       \
  230-syncedlyrics.tag  \
  thatspot.tag          \
  ozzy.tag              \
  crc53865.mp3 

getopt_files =          \
  getopt.c              \
  getopt.h              \
  getopt1.c             

header_files =          \
  demo_tag_options.h    \
  demo_copy_options.h   \
  demo_info_options.h   \
  demo_convert_options.h

EXTRA_DIST =            \
  $(tag_files)          \
  $(getopt_files)       \
  $(header_files)       

PROGNAME = gengetopt

%.c: $(srcdir)/%.ggo
	$(PROGNAME) --file-name=$* --unamed-opts --input=$<

demo_%.cpp: demo_%_options.c
