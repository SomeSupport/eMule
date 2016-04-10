# Copyright (C) 1999 Scott Thomas Haug <scott@id3.org>
# Copyright (C) 2002 Thijmen Klok <thijmen@id3lib.org>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

LT_VERSION = $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)

if ID3_NEEDZLIB
zlib_include = -I$(top_srcdir)/zlib/include
else
zlib_include = 
endif

INCLUDES = \
  @ID3LIB_DEBUG_FLAGS@ -I$(top_srcdir)/include/id3 -I$(top_srcdir)/include $(zlib_include)

noinst_HEADERS =                \
  field_def.h                   \
  field_impl.h                  \
  flags.h                       \
  frame_def.h                   \
  frame_impl.h                  \
  header.h                      \
  header_frame.h                \
  header_tag.h                  \
  mp3_header.h                  \
  tag_impl.h                    \
  spec.h                        

id3lib_sources =                \
  c_wrapper.cpp                 \
  field.cpp                     \
  field_binary.cpp              \
  field_integer.cpp             \
  field_string_ascii.cpp        \
  field_string_unicode.cpp      \
  frame.cpp                     \
  frame_impl.cpp                \
  frame_parse.cpp               \
  frame_render.cpp              \
  globals.cpp                   \
  header.cpp                    \
  header_frame.cpp              \
  header_tag.cpp                \
  helpers.cpp                   \
  io.cpp                        \
  io_decorators.cpp             \
  io_helpers.cpp                \
  misc_support.cpp              \
  mp3_parse.cpp                 \
  readers.cpp                   \
  spec.cpp                      \
  tag.cpp                       \
  tag_file.cpp                  \
  tag_find.cpp                  \
  tag_impl.cpp                  \
  tag_parse.cpp                 \
  tag_parse_lyrics3.cpp         \
  tag_parse_musicmatch.cpp      \
  tag_parse_v1.cpp              \
  tag_render.cpp                \
  utils.cpp                     \
  writers.cpp                   

lib_LTLIBRARIES   = libid3.la 

libid3_la_SOURCES = $(id3lib_sources)

if ID3_NEEDZLIB
LDADD        = $(top_builddir)/zlib/src/libz.la
endif

libid3_la_LDFLAGS = \
  -version-info $(LT_VERSION) \
  -release $(LT_RELEASE) \
  -export-dynamic
