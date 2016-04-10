//  The authors have released ID3Lib as Public Domain (PD) and claim no
//  copyright, patent or other intellectual property protection in this work.
//  This means that it may be modified, redistributed and used in commercial
//  and non-commercial software and hardware without restrictions.  ID3Lib is
//  distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
//  express or implied.
//  
//  The ID3Lib authors encourage improvements and optimisations to be sent to
//  the ID3Lib coordinator, currently Dirk Mahoney (dirk@id3.org).  Approved
//  submissions may be altered, and will be included and released under these
//  terms.
//  
//  Mon Nov 23 18:34:01 1998


#ifndef __DLL


#include "id3/id3lib_streams.h"
#include "id3/tag.h"

using std::cout;
using std::endl;
using std::cerr;


void MakeDummyTag(void)
{
  ID3_Tag   myTag("dummy.tag");
  ID3_Frame myFrame[3];

  myFrame[0].SetID(ID3FID_USERTEXT);
  myFrame[0].Field(ID3FN_TEXTENC)     = ID3TE_UNICODE;
  myFrame[0].Field(ID3FN_DESCRIPTION) = "example #1";
  myFrame[0].Field(ID3FN_TEXT)        = "This is the text for example #1";

  myFrame[1].SetID(ID3FID_USERTEXT);
  myFrame[1].Field(ID3FN_TEXTENC)     = ID3TE_ASCII;
  myFrame[1].Field(ID3FN_DESCRIPTION) = "example #2";
  myFrame[1].Field(ID3FN_TEXT)        = "This is the text for example #2";

  myFrame[2].SetID(ID3FID_INVOLVEDPEOPLE);
  myFrame[2].Field(ID3FN_TEXTENC)     = ID3TE_ASCII;
  myFrame[2].Field(ID3FN_TEXT).Add("String 1");
  myFrame[2].Field(ID3FN_TEXT).Add("String 2");
  myFrame[2].Field(ID3FN_TEXT).Add("String 3");
  myFrame[2].Field(ID3FN_TEXT).Add("String 4");

  myTag.AddFrames(myFrame, 3);

  //myTag.SetVersion(3, 0);
  myTag.SetUnsync(false);
  myTag.SetExtendedHeader(false);
  myTag.SetCompression(false);
  myTag.SetPadding(false);

  myTag.Strip();
  myTag.Update();

  return;
}


void StripTags(char *file)
{
  ID3_Tag myTag;

  myTag.Link(file);
  myTag.Strip();

  return;
}


void GetTestTag(void)
{
  ID3_Tag  myTag("dummy.tag");
  ID3_Frame *myFrame;

  if (myFrame = myTag.Find(ID3FID_PICTURE))
  {
    cout << "Found a picture frame!\r\n" << endl;

    char *dada = "output.jpg";

    myFrame->Field(ID3FN_DATA).ToFile(dada);
  }

  if (myFrame = myTag.Find(ID3FID_USERTEXT, ID3FN_DESCRIPTION, "example #1"))
  {
    cout << "Found a user text frame!\r\n" << endl;

    char textBuff[1024];

    myFrame->Field(ID3FN_DESCRIPTION).Get(textBuff, 1024);
    cout << "Desc: " << textBuff << endl;

    myFrame->Field(ID3FN_TEXT).Get(textBuff, 1024);
    cout << "Text: " << textBuff << endl;
  }

  for (luint i = 0; i < myTag.NumFrames(); i++)
    if (myFrame = myTag[i])
      cout << "Frame " << i << " has ID " << (luint) myFrame->GetID() << endl;

  return;
}


int main(int argc, char *argv[])
{
  try
  {
    //  CreateDemoTag1();
    //  MakeDummyTag();
    //  GetTestTag();
    StripTags("c:\\temp.mp3");
  }


  catch(ID3_Error err)
  {
    cout << "Error in ID3Lib!" << endl;
    cout << "Error: '" << err.GetErrorDesc() << "'" << endl;
    cout << " File: '" << err.GetErrorFile() << "'" << endl;
    cout << " Line: " << err.GetErrorLine() << endl;
  }

  return 0;
}


#endif


