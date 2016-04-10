#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "id3/id3lib_streams.h"
#include "id3/tag.h"
#include "id3/misc_support.h"

using std::cout;
using std::endl;

int main(unsigned argc, char* argv[])
{
  ID3D_INIT_DOUT();
  ID3D_INIT_WARNING();
  ID3D_INIT_NOTICE();

  if (argc != 2)
  {
    cout << "Usage: findstr <tagfile>" << endl;
    exit(1);
  }
  ID3_Tag tag(argv[1]);
  ID3_Frame* first = NULL, *frame = NULL;
  while(NULL != (frame = tag.Find(ID3FID_COMMENT, ID3FN_DESCRIPTION, "")))
  {
    if (frame == first)
    {
      break;
    }
    if (first == NULL)
    {
      first = frame;
    }
    char* comment = ID3_GetString(frame, ID3FN_TEXT);
    cout << "*** Found comment w/o description: " << comment << endl;
    delete [] comment;
  }
  return 0;
}

