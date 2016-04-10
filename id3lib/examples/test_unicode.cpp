// $Id: test_unicode.cpp,v 1.8 2002/06/29 17:26:18 t1mpy Exp $

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "id3/id3lib_streams.h"
#include <id3/tag.h>

int main( int argc, char *argv[])
{
  ID3_Tag tag;
  ID3_Frame frame;
  
  tag.Link("test-230-unicode.tag");
  tag.Strip(ID3TT_ALL);
  tag.Clear();
  
  frame.SetID(ID3FID_USERTEXT);
  frame.GetField(ID3FN_DESCRIPTION)->Set("example text frame");
  frame.GetField(ID3FN_TEXT)->Set("This text and the description should be in Unicode.");
  frame.GetField(ID3FN_TEXTENC)->Set(ID3TE_UNICODE);
  tag.AddFrame(frame);
  
  tag.SetPadding(false);
  tag.SetUnsync(false);
  tag.Update(ID3TT_ID3V2);

  return 0;
}

