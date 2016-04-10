// $Id: get_pic.cpp,v 1.8 2002/06/29 17:25:05 t1mpy Exp $

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "id3/id3lib_streams.h"
#include <stdlib.h>

#include <id3/tag.h>
#include <id3/misc_support.h>

using std::cout;
using std::endl;

int main( int argc, char *argv[])
{
  if (argc != 3)
  {
    cout << "Usage: get_pic <tagfile> <picfilename>" << endl;
    return 1;
  }
  
  ID3_Tag tag(argv[1]);
  const ID3_Frame* frame = tag.Find(ID3FID_PICTURE);
  if (frame && frame->Contains(ID3FN_DATA))
  {
    cout << "*** extracting picture to file \"" << argv[2] << "\"...";
    frame->Field(ID3FN_DATA).ToFile(argv[2]);
    cout << " done!" << endl;
  }
  else
  {
    cout << "*** no picture frame found in \"" << argv[1] << "\"" << endl;
    return 1;
  }

  return 0;
}

