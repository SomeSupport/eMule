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
    cout << "Usage: findeng <tagfile>" << endl;
    exit(1);
  }
  ID3_Tag tag(argv[1]);
  const ID3_Frame* frame = tag.Find(ID3FID_COMMENT, ID3FN_LANGUAGE, "eng");
  if (frame)
  {
    char* comment = ID3_GetString(frame, ID3FN_TEXT);
    cout << "*** Found english comment: " << comment << endl;
    delete [] comment;
  }
  else
  {
    cout << "*** No english comment to be found." << endl;
  }
  return 0;
}

