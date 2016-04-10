// $Id: test_pic.cpp,v 1.10 2002/06/29 17:25:53 t1mpy Exp $

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "id3/id3lib_streams.h"
#include "id3/tag.h"

int main( int argc, char *argv[])
{
  ID3D_INIT_DOUT();
  ID3D_INIT_WARNING();
  ID3D_INIT_NOTICE();

  ID3_Tag tag;
  ID3_Frame frame;
  
  tag.Link("test-230-picture.tag");
  tag.Strip(ID3TT_ALL);
  tag.Clear();
  
  frame.SetID(ID3FID_TITLE);
  frame.GetField(ID3FN_TEXT)->Set("Aquarium");
  tag.AddFrame(frame);
  
  frame.SetID(ID3FID_CONTENTGROUP);
  frame.GetField(ID3FN_TEXT)->Set("Short fraction of 'Carnival of the Animals: A Grand Zoological Fantasy'");
  tag.AddFrame(frame);
  
  frame.SetID(ID3FID_COMPOSER);
  frame.GetField(ID3FN_TEXT)->Set("Camille Saint-Saëns");
  tag.AddFrame(frame);
  
  frame.SetID(ID3FID_BAND);
  frame.GetField(ID3FN_TEXT)->Set("Slovakia Radio Symphony Orchestra");
  tag.AddFrame(frame);
  
  frame.SetID(ID3FID_CONDUCTOR);
  frame.GetField(ID3FN_TEXT)->Set("Ondrej Lenárd");
  tag.AddFrame(frame);
  
  frame.SetID(ID3FID_COPYRIGHT);
  frame.GetField(ID3FN_TEXT)->Set("1996 HNH international Ltd.");
  tag.AddFrame(frame);
  
  frame.SetID(ID3FID_CONTENTTYPE);
  frame.GetField(ID3FN_TEXT)->Set("(32)");
  tag.AddFrame(frame);
  
  frame.SetID(ID3FID_INVOLVEDPEOPLE);
  frame.GetField(ID3FN_TEXT)->Add("Producer");
  frame.GetField(ID3FN_TEXT)->Add("Martin Sauer");
  frame.GetField(ID3FN_TEXT)->Add("Piano");
  frame.GetField(ID3FN_TEXT)->Add("Peter Toperczer");
  tag.AddFrame(frame);
  
  frame.SetID(ID3FID_PICTURE);
  frame.GetField(ID3FN_MIMETYPE)->Set("image/jpeg");
  frame.GetField(ID3FN_PICTURETYPE)->Set(11);
  frame.GetField(ID3FN_DESCRIPTION)->Set("B/W picture of Saint-Saëns");
  frame.GetField(ID3FN_DATA)->FromFile("composer.jpg");
  tag.AddFrame(frame);
  
  tag.SetPadding(false);
  tag.SetUnsync(true);
  tag.Update(ID3TT_ID3V2);

  return 0;
}

