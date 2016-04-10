// $Id: demo_convert.cpp,v 1.15 2002/06/27 12:46:55 t1mpy Exp $
//
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>
#include "id3/id3lib_streams.h"
#include "id3/tag.h"
#include "demo_convert_options.h"

using std::cout;
using std::endl;

static const char* VERSION_NUMBER = "$Revision: 1.15 $";

void PrintUsage(const char *sName)
{
  cout << "Converts between id3v1 and id3v2 tags of an mp3 file." << endl;
  cout << endl;
  cout << "Will render both types of tag by default.  Only the last" << endl
       << "tag type indicated in the option list will be used.  Non-" << endl
       << "rendered will remain unchanged in the original file.  Will" << endl
       << "also parse and convert Lyrics3 v2.0 frames, but will not" << endl
       << "render them." << endl;
}

void PrintVersion(const char *sName)
{
  size_t nIndex;
  cout << sName << " ";
  for (nIndex = 0; nIndex < strlen(VERSION_NUMBER); nIndex++)
  {
    if (VERSION_NUMBER[nIndex] == ' ') 
    {
      break;
    }
  }
  nIndex++;
  for (; nIndex < strlen (VERSION_NUMBER); nIndex++)
  {
    if (VERSION_NUMBER[nIndex] == ' ') 
    {
      break;
    }
    cout << VERSION_NUMBER[nIndex];
  }
  cout << endl;
  cout << "Uses " << ID3LIB_FULL_NAME << endl << endl;

  cout << "This program converts and strips ID3v1/1.1 and Lyrics3 v2.0" << endl;
  cout << "tags to ID3v2 tags." << endl << endl;
}

void DisplayTags(ostream &os, luint nTags)
{
  if (!((nTags & ID3TT_ID3V1) || (nTags & ID3TT_ID3V2)))
  {
    os << "no tag";
  }
  if (nTags & ID3TT_ID3V1)
  {
    os << "v1";
  }
  if ((nTags & ID3TT_ID3V1) && (nTags & ID3TT_ID3V2))
  {
    os << " and ";
  }
  if (nTags & ID3TT_ID3V2)
  {
    os << "v2";
  }
}

int main( unsigned int argc, char * const argv[])
{
  flags_t ulFlag = ID3TT_ALL;
  gengetopt_args_info args;

  if (cmdline_parser(argc, argv, &args) != 0)
  {
    exit(1);
  }

#if defined ID3_ENABLE_DEBUG
  if (args.warning_flag)
  {
    ID3D_INIT_WARNING();
    ID3D_WARNING ( "warnings turned on" );
  }
  if (args.notice_flag)
  {
    ID3D_INIT_NOTICE();
    ID3D_NOTICE ( "notices turned on" );
  }
#endif

  if (args.v1tag_flag)
  {
    ulFlag = ID3TT_ID3V1;
  }

  if (args.v2tag_flag)
  {
    ulFlag = ID3TT_ID3V2;
  }


  const char* filename = NULL;
  for (size_t i = 0; i < args.inputs_num; ++i)
  {
    filename = args.inputs[i];
    ID3_Tag myTag;
    
    if (args.strip_given)
    {
      cout << "Stripping ";
    }
    else
    {
      cout << "Converting ";
    }
    cout << filename << ": ";
    
    myTag.Clear();
    myTag.Link(filename, ID3TT_ALL);
    myTag.SetPadding(args.padding_flag);
    
    cout << "attempting ";
    DisplayTags(cout, ulFlag);
    luint nTags;
    
    if (args.strip_flag)
    {
      nTags = myTag.Strip(ulFlag);
      cout << ", stripped ";
    }
    else
    {
      nTags = myTag.Update(ulFlag);
      cout << ", converted ";
    }
    
    DisplayTags(cout, nTags);
    cout << endl;
  }
  
  return 0;
}


