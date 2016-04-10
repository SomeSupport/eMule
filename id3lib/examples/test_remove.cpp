// $Id: test_remove.cpp,v 1.13 2003/03/02 15:17:21 t1mpy Exp $

#if defined(HAVE_CONFIG_H)
# include "config.h"
#endif

#include "id3/id3lib_streams.h"
#include "id3/tag.h"
#include "id3/misc_support.h"
#include "id3/id3lib_strings.h"

using std::cout;
using std::endl;
using std::cerr;

using namespace dami;

typedef const char* LPCTSTR;

/* CSharedTag is a wrapper I made for some features I need */
/* LPCTSTR means const char * */
size_t RemoveFrame(ID3_Tag& pTag, ID3_FrameID fID, LPCTSTR sDescription)
{
  size_t nCount = 0;
  const ID3_Frame * frame = NULL;

  do {
    if (!sDescription)
    {
      cerr << "*** description is null" << endl;
      frame = pTag.Find(fID);
    }
    else
    {
      cerr << "*** description is \"" << sDescription << "\"" << endl;
      frame = pTag.Find(fID, ID3FN_DESCRIPTION, sDescription);
    }

    if (frame)
    {
      ID3_Field* fld = frame->GetField(ID3FN_TEXT);
      String text(fld->GetRawText(), fld->Size());
      cerr << "*** delete frame with text \"" << text << "\"" << endl;
      /* pTag is an ID3_Tag */
      delete pTag.RemoveFrame(frame);
      nCount++;
    }
  } while (frame != NULL);

  return nCount;
}

int main( int argc, char *argv[])
{
  ID3_Tag tag;
  ID3_Frame frame;
  
  if (argc == 2)
  {
    tag.Link(argv[1]);
    cerr << "removed " << RemoveFrame(tag, ID3FID_COMMENT, "") << " descriptionless comment frames" << endl;
    tag.Update();
    
  }
  else
  {
    tag.Link("test-remove.tag");
    tag.Strip(ID3TT_ALL);
    tag.Clear();
    
    frame.SetID(ID3FID_TITLE);
    frame.GetField(ID3FN_TEXT)->Set("Test title");
    tag.AddFrame(frame);
    
    frame.SetID(ID3FID_COMPOSER);
    frame.GetField(ID3FN_TEXT)->Set("Test composer");
    tag.AddFrame(frame);
    
    frame.SetID(ID3FID_BAND);
    frame.GetField(ID3FN_TEXT)->Set("Test band");
    tag.AddFrame(frame);
    
    frame.SetID(ID3FID_CONDUCTOR);
    frame.GetField(ID3FN_TEXT)->Set("Test conductor");
    tag.AddFrame(frame);
    
    frame.SetID(ID3FID_COMMENT);
    frame.GetField(ID3FN_LANGUAGE)->Set("eng");
    frame.GetField(ID3FN_TEXT)->Set("Test comment");
    frame.GetField(ID3FN_DESCRIPTION)->Set("A Description");
    tag.AddFrame(frame);
    
    frame.SetID(ID3FID_COMMENT);
    frame.GetField(ID3FN_LANGUAGE)->Set("eng");
    frame.GetField(ID3FN_TEXT)->Set("Test comment 2");
    frame.GetField(ID3FN_DESCRIPTION)->Set("");
    tag.AddFrame(frame);
    
    frame.SetID(ID3FID_COMMENT);
    frame.GetField(ID3FN_LANGUAGE)->Set("eng");
    frame.GetField(ID3FN_TEXT)->Set("ID3v1 comment text?");
    frame.GetField(ID3FN_DESCRIPTION)->Set(STR_V1_COMMENT_DESC);
    tag.AddFrame(frame);
    
    tag.SetPadding(false);
    tag.Update(ID3TT_ID3V2);
    
    cerr << "removed " << ID3_RemoveArtists(&tag) << " artist frames" << endl;
    tag.Update();
    cerr << "removed " << ID3_RemoveTitles(&tag) << " title frames" << endl;
    tag.Update();
    cerr << "removed " << RemoveFrame(tag, ID3FID_COMMENT, "") << " descriptionless comment frames" << endl;
    tag.Update();
  }

  return 0;
}

