<?php
$subsection=0;

function printHeader($string) {
    global $subsection;
    echo  "<FONT SIZE=\"+";
    echo 3-$subsection ;
    echo  "\">$string</FONT>\n";
    echo  "<P>";
    $subsection++;
    return;
}

function startSection() {
    $subsection++;
    echo  "<OL>\n";
}

function endSection() {
    global $subsection;
    echo  "</OL>\n";
    echo  "<P>\n";
    $subsection--;
}

function startCode() {
    global $subsection;

/*
<TABLE WIDTH="100%" BORDER="0" CELLSPACING="0" CELLPADDING="3"><TR><TD ALIGN="center" BGCOLOR="#EEEEEE"><B><FONT FACE="Lucida,Verdana,Helvetica,Arial"><FONT COLOR="#000000">navigator</FONT></FONT></B></TD></TR><TR><TD BGCOLOR="#FFFFFF"><SMALL><FONT FACE="Lucida,Verdana,Helvetica,Arial">
*/ 

    echo  "<TABLE WIDTH=\"";
    echo 100-(($subsection+1)*5);
    echo  "%\" BORDER=\"0\" CELLSPACING=\"0\" CELLPADDING=\"0\"><TR><TD BGCOLOR=\"000000\" align=center>";
    echo  "<TABLE WIDTH=\"100%\" BORDER=\"0\" CELLSPACING=\"1\" CELLPADDING=\"3\"><TR><TD BGCOLOR=\"b0b0b0\">\n";
    echo  "<TT><PRE>";
}

function endCode() {
    global $subsection;
    echo  "</PRE></TT>\n";
    echo  "</TD></TR></TABLE>";
    echo  "</TD></TR></TABLE>\n";
}

?>
<HEAD>
<TITLE>Documentation for ID3Lib 3.05</TITLE>
</HEAD>
<BODY BGCOLOR="#ffffff">
<CENTER>
<FONT SIZE="+3">
Documentation for<BR>
ID3Lib 3.05<P>
</FONT>

<FONT SIZE="+1">
Copyright Dirk Mahoney<BR>
22 November 1998
</FONT>
</CENTER>

<BLOCKQUOTE>
ID3Lib is a C++ class library for manipulating ID3v2 tags.  ID3Lib 3.05 conforms to all ID3v2 standards up to and including the ID3v2-3.0 informal standard.
</BLOCKQUOTE>

<P>
<?php printHeader( "Introduction"); ?>
<?php startSection() ?>
    No intro. Let's go!
<?php endSection() ?>

<?php printHeader( "License Agreement"); ?>
<?php startSection() ?>
The authors have released ID3Lib as Public Domain (PD) and claim no copyright, patent or other intellectual property protection in this work.  This means that it may be modified, redistributed and used in commercial and non-commercial software or hardware without restrictions.  ID3Lib is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.
<P>
The ID3Lib authors encourage improvements and optimisations to be sent to the ID3Lib coordinator, currently Dirk Mahoney (dirk@id3.org).  Approved submissions may be altered, and will be included and released under these terms.
<?php endSection() ?>

<?php printHeader( "The ID3_Tag Class"); ?>
<?php startSection() ?>
<P>
This is the "container" class for everything else.  It is through an ID3_Tag object that most of the productive stuff happens.  Let's look at what's required to start using ID3v2 tags.

<?php startCode(); ?>
#include "id3_tag.h"
<?php endCode(); ?>

This simple #include does it all.  In order to read an existing tag

<?php startCode(); ?>
ID3_Tag myTag;

myTag.Link ( "something.mp3" );
<?php endCode(); ?>

That is all there is to it.  Now all you have to do is use the ::Find() member function to locate the frames you are interested in:

<?php startCode(); ?>
ID3_Frame *myFrame;

if ( myFrame = myTag.Find ( ID3FID_TITLE ) )
{
      char title[ 1024 ];

      myFrame->Field ( ID3FN_TEXT ).Get ( title, 1024 );

      cout << "Title: " << title << endl;
}
<?php endCode(); ?>

This code snippet locates the TITLE frame and copies the contents of the text field into a buffer and displays the buffer.  Not difficult eh?

When using the ::Link() facility of an ID3_Tag object, you automatically gain access to any ID3v1/1.1, ID3v2 and Lyrics3 v2.0 tags present in the file.  The class will automatically parse and convert any of these foreign tag formats into ID3v2 tags.  Also, ID3Lib will correctly parse any correctly formatted 'CDM' frames from the unreleased 2.01 draft that ID3Lib 2.16 supports.
<P>
Okay, let's analyse the ID3_Tag class function-by-function to see what an application can do with it.
<P>
<?php printHeader( "The ID3_Tag Constructor"); ?>
<?php startSection() ?>
The ID3_Tag constructor can be called without any parameters.  Alternatively, it can accept a single parameter that is a pointer to a file name.  If this file exists, it will be opened and all ID3Lib-supported tags will be parsed and converted to ID3v2 if necessary.  After the conversion, the file will remain unchanged, and will continue to do so until you use the ID3_Tag::Update() function on the tag (if you choose to ID3_Tag::Update() at all).  This will be discussed later.
<?php endSection() ?>

<?php printHeader( "ID3_Tag::Clear()"); ?>
<?php startSection() ?>
This function takes no parameters and simply clears the object and disassociates it from any files.  It frees any resources for which the object is responsible, and the object is now free to be used again for any new or existing tag.
<?php endSection() ?>

<?php printHeader( "ID3_Tag::HasChanged()"); ?>
<?php startSection() ?>
This function takes no parameters but returns a boolean indicating whether the tag has been altered since the last parse, render or update.  If you have a tag linked to a file, you do not need this function since the ID3_Tag::Update() function will check for changes before writing the tag.
<P>
This function is primarily intended as a status indicator for applications and for applications that use the ID3_Tag::Parse() and ID3_Tag::Render() functions.
<P>
Setting a field, changing the ID of an attached frame, setting or changing grouping or encryption IDs and clearing a frame or field all constitute a change to the tag, as do calls to the ID3_Tag::SetUnsync(), ID3_Tag::SetExtendedHeader() and ID3_Tag::SetPadding() and ID3_Tag::SetCompression() functions.

<?php startCode(); ?>
if ( myTag.HasChanged() )
{
    // render and output the tag
    ...
}
<?php endCode(); ?>
<?php endSection() ?>

<?php printHeader( "ID3_Tag::SetVersion()"); ?>
<?php startSection(); ?>
This function has been removed from 3.05.  ID3Lib will now always create tags using the latest version of the standard.
<?php endSection(); ?>

<?php printHeader( "ID3_Tag::SetUnsync()"); ?>
<?php startSection(); ?>
This call takes a single boolean parameter and tells the object whether you wish the resulting binary tag to be unsynchronised if unsync is required.  If you call this function with 'false' as the parameter, the binary tag will not be unsync'ed, regardless of whether the tag should be.  This option is useful when the file is only going to be used by ID3v2-compliant software.  See the ID3v2 standard document for further details on unsync.
<P>
By default, tags are created without unsync.

<?php startCode(); ?>
myTag.SetUnsync ( false );
<?php endCode(); ?>
<?php endSection(); ?>

<?php printHeader( "ID3_Tag::SetExtendedHeader()"); ?>
<?php startSection(); ?>
This function tells the tag whether to create an extended header when rendering the tag.  This option is currently ignored as ID3Lib doesn't yet create extended headers.  This option only applies when rendering tags for ID3v2 versions that support extended headers.
<P>
By default, ID3Lib will generate extended headers for all tags in which extended headers are supported.

<?php startCode(); ?>
myTag.SetExtendedHeader ( true );
<?php endCode(); ?>
<?php endSection(); ?>

<?php printHeader( "ID3_Tag::SetCompression()"); ?>
<?php startSection(); ?>
Under ID3v2-3.0, there is support for the compression of data in tags.  The current standard defines compression on a frame-by-frame basis using the zlib data compression standard.  Using this call, you tell the object whether to attempt to compress frames or not.  If a frame doesn't compress well, it will not be compressed, regardless of this setting.  This function takes a single boolean parameter.  By default, ID3Lib will attempt to compress all the frames in a tag.

<?php startCode(); ?>
myTag.SetCompression ( true );
<?php endCode(); ?>

When creating tags for a version of ID3v2 in which compression isn't defined, the tags are simply rendered without compression to ensure compliance to the relevant version of the standard.
<?php endSection(); ?>

<?php printHeader( "ID3_Tag::SetPadding()"); ?>
<?php startSection(); ?>
When using ID3v2 tags in association with files, ID3Lib can optionally add padding to the tags to ensure minimal file write times when updating the tag in the future.
<P> 
When the padding option is switched on, ID3Lib automatically creates padding according the 'ID3v2 Programming Guidelines'.  Specifically, enough padding will be added to round out the entire file (song plus tag) to an even multiple of 2K.  Padding will only be created when the tag is attached to a file and that file is not empty (aside from a pre-existing tag).
<P>
My own addition to the guidelines for padding, and that which ID3Lib follows is that if frames are removed from a pre-existing tag (or the tag simply shrinks because of other reasons), the new tag will continue to stay the same size as the old tag (with padding making the difference of course) until such time as the padding is greater than 4K.  When this happens, the padding will be reduced and the new tag will be smaller than the old.
<P>
By default, padding is switched on.
<P>
<?php startCode(); ?>
myTag.SetPadding ( false );
<?php endCode(); ?>

An alternate form of the SetPadding() function has been removed from 3.05.

<?php startCode(); ?>
myTag.SetPadding ( true ); // switch padding on
myTag.SetPadding ( 1024 ); // add 1K of padding
<?php endCode(); ?>

<?php endSection(); ?>

<?php printHeader( "ID3_Tag::SetFileSize()"); ?>
<?php startSection(); ?>
This function has been removed from 3.05.
<?php endSection(); ?>

<?php printHeader( "ID3_Tag::AddFrame() and operator<<()"); ?>
<?php startSection(); ?>
To attach a frame object (see later) to a tag object, you can use the ID3_Tag::AddFrame() function or the << operator.  To use the ID3_Tag::AddFrame() function, simply supply as it's sole parameter a pointer to the ID3_Frame object you wish to attach.  Even though the 'id3_tag.h' file says that ID3_Tag::AddFrame also takes a boolean parameter, this parameter is for internal use only – do not use it.
<P>
<?php startCode(); ?>
ID3_Frame myFrame;
myTag.AddFrame ( &myFrame );
<?php endCode(); ?>

Alternatively, use the << operator to attach a frame to a tag.  The << operator supports the addition of a pointer to a frame object, or the frame object itself.

<?php startCode(); ?>
ID3_Frame *framePoint, frameObj;
myTag << framePoint;
myTag << frameObj;
<?php endCode(); ?>

Both these functions simply attach the frames to the tag – the tag does not create it's own copy of the frame.  Frames created by an application must exist until the frame is removed or the tag is finished with.
<?php endSection(); ?>

<?php printHeader( "ID3_Tag::AddFrames()"); ?>
<?php startSection(); ?>
This function takes a pointer to an array of ID3_Frame objects and a number that specifies the number of objects in the array.  ID3Lib will then go and add all the frames to the tag. Even though the 'id3_tag.h' file says that ID3_Tag::AddFrames also takes a boolean parameter, this parameter is for internal use only – do not use it.

<?php startCode(); ?>
ID3_Frame myFrame[ 10 ];
myTag.AddFrames ( myFrame, 10 );
<?php endCode(); ?>
<?php endSection(); ?>

<?php printHeader( "ID3_Tag::RemoveFrame()"); ?>
<?php StartSection(); ?>
If you wish to remove a frame from a tag, use this function.  If you already own the frame object in question, then you should already have a pointer to the frame you want to delete.  If not, or if you wish to delete a pre-existing frame (from a tag you have parsed, for example), then use one of the ID3_Tag::Find() functions to obtain a frame pointer to pass to this function.

<?php startCode(); ?>
ID3_Frame *someFrame;

if ( ( someFrame = myTag.Find ( ID3FID_TITLE ) )
    myTag.RemoveFrame ( someFrame );
<?php endCode(); ?>
<?php endSection(); ?>

<?php printHeader( "ID3_Tag::Size()"); ?>
<?php startSection(); ?>
This function returns an over estimate of the number of bytes required to store a binary version of a tag.  When using ID3_Tag::Render() to render a binary tag to a memory buffer, first use the result of this call to allocate a buffer of unsigned chars.

<?php startCode(); ?>
luint tagSize;
uchar *buffer;

if ( myTag.HasChanged() )
{
    if ( ( tagSize = myTag.Size() ) > 0 )
    {
        if ( buffer = new uchar[ tagSize ] )
        {
            luint actualSize;

            actualSize = myTag.Render ( buffer );

            // do something useful with the first
            // 'actualSize' bytes of the buffer,
            // like push it down a socket
            ...

            delete[] buffer;
        }
    }
}
<?php endCode(); ?>
<?php endSection(); ?>

<?php printHeader( "ID3_Tag::Render()"); ?>
<?php startSection(); ?>
This renders a binary image of the tag into the supplied buffer.  See ID3_Tag::Size() for an example.  This function returns the actual number of the bytes of the buffer used to store the tag.  This will be less that the size of the buffer itself, because ID3_Tag::Size() over estimates the required buffer size.
<P>
Before calling this function, it is advisable to call ID3_Tag::HasChanged() first as this will let you know whether you should bother rendering the tag.
<?php endSection(); ?>

<?php printHeader( "ID3_IsTagHeader()"); ?>
<?php startSection(); ?>
This function isn't a member of the ID3_Tag class, but is presented here because it is used before the actual ID3_Tag::Parse() function when parsing a tag from a memory buffer.
<P>
When you are scanning a stream for a tag, you should pass ID3_TAGHEADERSIZE bytes of unsigned chars into this function and it will return the number of extra bytes to read in order to obtain the rest of the tag.  If the supplied ID3_TAGHEADERSIZE bytes don't constitute a valid ID3v2 tag header, this function will return –1.
<P>
Use this function only if you are using ID3_Tag::Parse()/Render() instead of ID3_Tag::Link()/Update().

<?php startCode(); ?>
uchar buffer[ ID3_TAGHEADERSIZE ];

// get ID3_TAGHEADERSIZE bytes from a socket or somewhere
...

luint tagSize;

if ( ( tagSize = ID3_IsTagHeader ( buffer ) ) > -1 )
{
     // read a further 'tagSize' bytes in
     // from our data source
     ...

     // now we will call ID3_Tag::Parse()
     // with these values (explained later)
     ...
}
<?php endCode(); ?>
<?php endSection(); ?>


<?php printHeader( "ID3_Tag::Parse()") ?>
<?php startSection(); ?>
This will turn a binary tag into a series of ID3_Frame objects attached to the specified tag.  This function takes two parameters, the first is the ID3_TAGHEADERSIZE byte header you read in from your data source.  The second is a pointer to an unsigned char buffer which is the rest of the tag (not including the header) you read from the data source.

<?php startCode(); ?>
ID3_Tag myTag;
uchar header[ ID3_TAGHEADERSIZE ];
uchar *buffer;
luint tagSize;

// get ID3_TAGHEADERSIZE from a socket or somewhere
...

if ( ( tagSize = ID3_IsTagHeader ( ourSourceBuffer ) ) > -1 )
{
    // read a further 'tagSize' bytes in
    // from our data source
    ...

    if ( buffer = new uchar[ tagSize ] )
    {
        // now we will call ID3_Tag::Parse()
        // with these values (explained later)
        myTag.Parse ( header, buffer );

        // do something with the objects,
        // like look for titles, artists etc
        ...

        // free the buffer
        delete[] buffer;
    }
}
<?php endCode(); ?>
<?php endSection(); ?>

<?php printHeader( "ID3_Tag::Link()");; ?>
<?php startSection(); ?>
If you created your ID3_Tag object without supplying a parameter to the constructor (maybe you create an array of ID3_Tag pointers), you can use this function to attach the tag to a file.  This is the preferred method of interacting with files, since ID3Lib can automatically do things like parse foreign tag formats and handle padding when linked to a file.  When a tag is linked to a file, you do not need to use the ::Size(), ::Render(), ::Parse() or ID3_IsTagHeader() functions – ID3Lib will take care of all that stuff for you.  The single parameter is a pointer to a file name.
<P>
Link() returns a 'luint' which is the byte position within the file that the audio starts (ie. where the ID3v2 tag ends).

<?php startCode(); ?>
ID3_Tag *myTag;

if ( myTag = new ID3_Tag )
{
    myTag->Link ( "mysong.mp3" );

    // do whatever we want with the tag
    ...

    // setup all our rendering parameters
    myTag->SetUnsync ( false );
    myTag->SetExtendedHeader ( true );
    myTag->SetCompression ( true );
    myTag->SetPadding ( true );

    // write any changes to the file
    myTag->Update();

    // free the tag
    delete myTag;
}
<?php endCode(); ?>
<?php endSection(); ?>


<?php printHeader( "ID3_Tag::Update()");; ?>
<?php startSection(); ?>
This little function takes no parameters, returns no results and simply renders the tag and writes it to the file to which this tag is attached.  Make sure you setup your rendering parameters (including version number) before calling this function.  See ID3_Tag::Link() for an example of this function.
<?php endSection(); ?>

<?php printHeader( "ID3_Tag::Strip()");; ?>
<?php startSection(); ?>
This function takes no parameters and returns no results.  When attached to a file, this function will strip any ID3v2 tag from the file.  If you supply a 'true' boolean parameter then this tells the tag to strip any ID3v1/1.1 and Lyrics3 v2.0 tags from the file as well.
<?php startCode(); ?>
myTag.Strip()
myTag.Strip ( true );
<?php endCode(); ?>
<?php endSection(); ?>

<?php printHeader( "ID3_Tag::Find()");; ?>
<?php startSection(); ?>
This function takes a required ID3_FrameID parameter that specifies the ID of the frame you wish to locate.  If present in the tag, the first such frame will be returned as a pointer to an ID3_Frame object.  If no frame is found, this returns NULL.  If there are multiple frames in the tag with the same ID (which, for some frames, is allowed), then subsequent calls to ::Find() will return subsequent frame pointers, until it wraps around again.
 
<?php startCode(); ?>
ID3_Frame *myFrame;

if ( myFrame = myTag.Find ( ID3FID_TITLE ) )
{
    // do something with the frame, like copy
    // the contents into a buffer, display the
    // contents in a window etc
    ...
}
<?php endCode(); ?>

You may optionally supply two more parameters to this function, being an ID3_FieldID and a value of some sort.  Depending on the field name/ID you supply, you may supply an integer, a char * or a wchar_t * as the third parameter.  If you supply an ID3_FrameID, you must also supply a data value to compare with.
<P>
This function will then return the first frame that has a matching frame ID, and which has a field with the same name as that which you supplied in the second parameter, whose value matches that which you supplied as the third parameter.  For example:

<?php startCode(); ?>
ID3_Frame *myFrame;

if ( myFrame = myTag.Find ( ID3FID_TITLE, ID3FN_TEXT, "Nirvana" ) )
{
    // found it, do something with it
    ...
}
<?php endCode(); ?>

This example with return the first frame that is a TITLE frame and whose TEXT field is 'Nirvana'.  I emphasis 'is' because currently there is no provision for things like 'contains', 'greater than' or 'less than'.  If there happens to be more than one of these frames, subsequent calls to the ::Find() function will return subsequent frames and will wrap around to the beginning.
<P> 
Another example:

<?php startCode(); ?>
ID3_Frame *myFrame;

if ( myFrame = myTag.Find ( ID3FID_COMMENT, ID3FN_TEXTENC, (luint) ID3TE_UNICODE ) )
{
    // do something
    ...
}

<?php endCode(); ?>
This returns the first COMMENT frame that uses Unicode as it's text encoding.
<?php endSection(); ?>

<?php printHeader( "ID3_Tag::NumFrames()");; ?>
<?php startSection(); ?>
This returns the number of frames present in the tag object.  This includes only those frames that ID3Lib recognises.  This is used as the upper bound on calls to the ID3_Tag::GetFrame() and operator[] functions.
<?php endSection(); ?>

<?php printHeader( "ID3_Tag::GetFrameNum() and operator[]");; ?>
<?php startSection(); ?>
In 3.04 and earlier, this function was called ::GetFrame().
<P>
These return pointers to the given frame number, or NULL if the specified frame number doesn't exist.  Numbering starts at 0 (that is, the first frame is number 0, and the last frame in a tag that holds 34 frames is 33).
<P>
If you wish to have a more complex searching facility, then at least for now, you will have to devise it yourself and implement it using these functions.
<?php endSection(); ?>
<?php endSection(); ?>


<?php printHeader( "The ID3_Frame Class");; ?>
<?php startSection(); ?>

ID3Lib defines frames in a funny way.  Using some nice C++ conventions, ID3_Frame class objects appear to be quite polymorphic, that is, they can take on many forms.  The same ID3_Frame class provides the facilities for the implementation of a complex APIC frame and for a simple text frame.  Here's how to use an ID3_Frame object:
<P>
<?php printHeader( "The ID3_Frame Constructor");; ?>
<?php startSection(); ?>
The optional parameter to the ID3_Frame constructor is of the type ID3_FrameID, which will internally set the frame's structure.  See ID3_Frame::SetID() for more details.
<?php endSection(); ?>

<?php printHeader( "ID3_Frame::Clear()");; ?>
<?php startSection(); ?>
Like it's ID3_Tag counterpart, ID3_Frame::Clear() takes no parameters and returns nothing.  It just clears the frame of all data and resets the frame such that it can take on the form of any ID3v2 frame that ID3Lib supports.
<?php endSection(); ?>


<?php printHeader( "ID3_Frame::SetID()");; ?>
<?php startSection(); ?>
This function sets up the internal structure of an ID3_Frame object so that it represents any ID3v2 frame that ID3Lib supports.  Given an ID3_FrameID (a list of which is found in 'id3_field.h'), ::SetID() will structure the object according to the frame you wish to implement.
<P>
Either using this call or via the constructor, this must be the first command performed on an ID3_Frame object.
<?php startCode(); ?>
myFrame.SetID ( ID3FID_TITLE );
<?php endCode(); ?>
<?php endSection(); ?>

<?php printHeader( "ID3_Frame::GetID()");; ?>
<?php startSection(); ?>
Useful in conjunction with the ID3_Tag::GetFrame() function, this function returns the type of frame that the object represents, just so you know what to expect.
<?php endSection(); ?>

<?php printHeader( "ID3_Frame::Field()");; ?>
<?php startSection(); ?>
This is a go-between function that returns a pointer to one of the frame's internal fields.  A list of fields that are in given frames appears in 'id3_field.cpp'.  This function returns a reference to the field in question, so that the result can be used as though it were a field itself:

<?php startCode(); ?>
ID3_TextEnc    enc;
enc = (ID3_TextEnc) myFrame.Field ( ID3FN_TEXTENC ).Get();
<?php endCode(); ?>

As far as frames go, that's all there is.  The next step is to find out how to manipulate the fields contained in the frames.
<?php endSection(); ?>
<?php endSection(); ?>

<?php printHeader( "The ID3_Field Class");; ?>
<?php startSection(); ?>
As a general rule, you need never create an object of this type.  ID3Lib uses them internally as part of the ID3_Frame class.  You must know how to interact with these objects though, and that's what this section is about.
<P>
The ID3_Field contains many overloaded functions to provide these facilities for four different data types.
<P>
As a general implementation note, you should be prepared to support all fields in an ID3Lib frame, even if all fields in the ID3Lib version of the frame aren't present in the ID3v2 version.  This is because of frames like the picture frame, which changed slightly from one version of the ID3v2 standard to the next (the IMAGEFORMAT field in 2.0 changed to a MIMETYPE in 3.0).  If you support all ID3Lib fields in a given frame, ID3Lib can generate the correct ID3v2 frame for the ID3v2 version you wish to support.  Alternatively, just support the fields you know will be used in, say, 3.0 if you only plan to generate 3.0 tags.
<P>
<?php printHeader( "ID3_Field::Clear()");; ?>
<?php startSection(); ?>
Regardless of the type of field, this function will clear any data and free any memory associated with the field.  This function takes no parameters and returns no results.
<?php endSection(); ?>

<?php printHeader( "ID3_Field::Size()");; ?>
<?php startSection(); ?>
This function returns the size of a field.
<P>
The value returned is dependent on the type of the field.  For ASCII strings, this returns the number of characters in the field, not including any NULL-terminator.  The same holds true for Unicode – it returns the number of characters in the field, not bytes and this does not include the Unicode BOM, which isn't put in a Unicode string obtained by the ::Get() function anyway.  For binary and integer fields, this returns the number of bytes in the field.

<?php startCode(); ?>
luint howBig;
howBig = myFrame.Field ( ID3FN_DATA ).Size();
<?php endCode(); ?>
<?php endSection(); ?>

<?php printHeader( "Integer Fields");; ?>
<?php startSection(); ?>
This next bit talks about the ID3_Field class as it relates to integers only.
<P>
<?php printHeader( "ID3_Field::Set()");; ?>
<?php startSection(); ?>
This function takes a single parameter that is an integer.  It sets the value of this field to the specified integer.

<?php startCode(); ?>
myFrame.Field ( ID3FN_PICTURETYPE ).Set ( 0x0B );
ID3_Field::Get()
<?php endCode(); ?>

Taking no parameters, this function returns the value of the integer field.

<?php startCode(); ?>
luint picType;
picType = myFrame.Field ( ID3FN_PICTURETYPE ).Get();
<?php endCode(); ?>
<?php endSection(); ?>

<?php printHeader( "ID3_Field::operator=()"); ?>
<?php startSection(); ?>

This functions as you would expect.

<?php startCode(); ?>
myFrame.Field ( ID3FN_PICTURETYPE ) = 0x0B;
<?php endCode(); ?>
<?php endSection(); ?>
<?php endSection(); ?>

<?php printHeader( "General String Fields"); ?>
<?php startSection(); ?>
<?php printHeader( "ID3_Field::GetNumTextItems()");; ?>
<?php startSection(); ?>
In 3.04 and earlier, this function was called ::GetNumItems().
<P>
This function returns the number of items in a text list.  The numbering of items in the list begins at one (1).
<?php startCode(); ?>
luint numItems;
numItems = myFrame.Field ( ID3FN_TEXT ).GetNumItems();
<?php endCode(); ?>
<?php endSection(); ?>
<?php endSection(); ?>

<?php printHeader( "ASCII String Fields"); ?>
<?php startSection(); ?>
This next bit talks about ID3_Field objects in reference to ASCII strings.
<P>
<?php printHeader( "ID3_Field::Set()");; ?>
<?php startSection(); ?>
This function takes a single parameter that is a pointer to an ASCII string.  It copies the supplied string (as opposed to simply keeping a pointer to it), so you may dispose of the source string after a call to this function.

<?php startCode(); ?>
myFrame.Field ( ID3FN_TEXT ).Set ( "ID3Lib is very cool!" );
<?php endCode(); ?>
<?php endSection(); ?>

<?php printHeader( "ID3_Field::operator=()");; ?>
<?php startSection(); ?>
This functions much as you would expect;

<?php startCode(); ?>
myFrame.Field ( ID3FN_TEXT ) = "ID3Lib is very cool!";
<?php endCode(); ?>

Like ID3_Field::Set(), this function copies the supplied string.
<?php endSection(); ?>

<?php printHeader( "ID3_Field::Add()");; ?>
<?php startSection(); ?>
This adds a string to the list of strings currently in the field.  This useful for using ID3v2 frames such as the involved people list, composer and part of set.  You can use the ID3_Field::GetNumItems() to find out how many such items are in a list.

<?php startCode(); ?>
myFrame.Field ( ID3FN_TEXT ).Add ( "this is a test" );
<?php endCode(); ?>
<?php endSection(); ?>

<?php printHeader( "ID3_Field::Get()");; ?>
<?php startSection(); ?>
This takes a pointer to a char buffer and an integer that says how big (in characters) the supplied buffer is. Optionally as the third parameter is the item number you wish to retrieve.  This is useful when using text lists (see ID3_Field::Add() for more details).  The default value for this third parameter is 1, which returns the entire string if the field contains only one item.
<P>
It returns the number of characters (not bytes necessarily, and not including any NULL terminator) of the supplied buffer that are now used.

<?php startCode(); ?>
char myBuffer[ 1024 ];
luint charsUsed;

charUsed = myFrame.Field ( ID3FN_TEXT ).Get ( buffer, 1024 );
<?php endCode(); ?>

It fills the buffer with as much data from the field as is present in the field, or as large as the buffer, whichever is smaller.

<?php startCode(); ?>
char myBuffer[ 1024 ];
luint charsUsed;

charUsed = myFrame.Field ( ID3FN_TEXT ).Get ( buffer, 1024, 3 );
<?php endCode(); ?>

This fills the buffer with up to the first 1024 characters from the third element of the text list.
<?php endSection(); ?>
<?php endSection();; ?>



<?php printHeader( "Unicode String Fields"); ?>
<?php startSection(); ?>
Without elaborating, the Unicode functions behave exactly as it's ASCII counterparts, taking wchar_t pointers in place of char pointers.  The ::Get() function still takes a max size in characters (not bytes).
 
<?php startCode(); ?>
wchar_t buffer[ 1024 ];
char charsUsed;

charsUsed = myFrame.Field ( ID3FN_TEXT ).Get ( buffer, 1024 );
<?php endCode(); ?>

All strings in ID3Lib are handled internally as Unicode.  This means that when you set a field with an ASCII source type, it will be converted and stored internally as a Unicode string.  ID3Lib will handle all necessary conversions when parsing, rendering and ::Get()ing.  If you set a field as an ASCII string, then try to read the string into a wchar_t buffer, ID3Lib will automatically convert the string into Unicode so this will function as expected.  The same holds true in reverse.
<P>
Of course, when converting from Unicode to ASCII, you will experience problems when the Unicode string contains characters that don't map to ISO-8859-1.
<?php endSection(); ?>

<?php printHeader( "Binary Fields"); ?>
<?php startSection(); ?>
The last type field that ID3Lib provides support for is a binary field.  This holds miscellaneous data that can't be easily described any other way, such a JPEG image.
<P>
<?php printHeader( "ID3_Field::Set()");; ?>
<?php startSection(); ?>
This takes a pointer to an unsigned char buffer and an integer that specifies how big the data is in bytes.  Again, like the string types, the binary ::Set() function copies the data so you may dispose of the source data after a call to this function.

<?php startCode(); ?>
uchar myBuffer[ 1024 ];
myFrame.Field ( ID3FN_DATA ).Set ( myBuffer, 1024 );
<?php endCode(); ?>
<?php endSection(); ?>

<?php printHeader( "ID3_Field::Get()");; ?>
<?php startSection(); ?>
This takes the same parameters as the ::Set function.  It copies the data in the field into the buffer, for as many bytes as the field contains, or the size of buffer, whichever is smaller.

<?php startCode(); ?>
uchar buffer[ 1024 ];
myFrame.Field ( ID3FN_DATA ).Get ( buffer, sizeof ( buffer ) );
<?php endCode(); ?>
<?php endSection(); ?>

<?php printHeader( "ID3_Field::FromFile()");; ?>
<?php startSection(); ?>
Taking a single char pointer as a parameter, this function takes the data in the specified file and stores it in the field.  The file may be deleted after a call to this function as the source data is copied into the field.

<?php startCode(); ?>
myFrame.Field ( ID3FN_DATA ).FromFile ( "mypic.jpg" );
<?php endCode(); ?>
<?php endSection(); ?>

<?php printHeader( "ID3_Field::ToFile()");; ?>
<?php startSection(); ?>
The exact reverse of ::FromFile(), this function copies the data in the field to the specified file.

<?php startCode(); ?>
myFrame.Field ( ID3FN_DATA ).ToFile ( "output.bin" );
<?php endCode(); ?>
<?php endSection(); ?>

<?php endSection(); ?>
<?php endSection(); ?>

<?php printHeader( "The ID3_Error Class and ID3Lib Exceptions"); ?>
<?php startSection(); ?>
When ID3Lib encounters a nasty error, it throws an exception of type ID3_Error.  A function that calls an ID3Lib routine can place the call in a try block and provide an appropriate catch block.

<?php startCode(); ?>
try
{
    // call some ID3Lib routine
    ID3_Tag myTag ( "mySong.mp3" );
    ID3_Frame *myFrame = NULL;

    // this will generate an exception
    myTag << myFrame;
}

catch ( ID3_Error err )
{
    // handle the error
    ...
}
<?php endCode(); ?>

Let's have a look at the error handling functions provided by the ID3_Error class.
<P>
<?php printHeader( "ID3_Error::GetErrorID()");; ?>
<?php startSection(); ?>
This function takes no parameters but returns an ID3_Err value, which represents the ID of the error that caused the exception.
<?php endSection(); ?>


<?php printHeader( "ID3_Error::GetErrorDesc()");; ?>
<?php startSection(); ?>
Again, this function takes no parameters but returns a character pointer that is an English string that describes the error.
<?php endSection(); ?>

<?php printHeader( "ID3_Error::GetErrorFile()");; ?>
<?php startSection(); ?>
This function returns a pointer to a string of characters that is the name of the ID3Lib source file that generated the exception.  When submitting bug reports, it is useful to include this.

<?php startCode(); ?>
cout << "Exception in file '" << err.GetErrorFile() << "'" << endl;
<?php endCode(); ?>
<?php endSection(); ?>

<?php printHeader( "ID3_Error::GetErrorLine()");; ?>
<?php startSection(); ?>
This function returns the line number in the ID3Lib source file that threw the exception.

<?php startCode(); ?>
cout << "Line # " << err.GetErrorLine() << endl;
<?php endCode(); ?>
<?php endSection(); ?>
<?php endSection(); ?>


<?php printHeader( "Known Bugs and Limitations");; ?>
<?php startSection(); ?>
Version 3.05 of ID3Lib has some known limitations:
<P>

<LI>Firstly, contrary to good programming ideas and contrary to the 'ID3v2 Programming Guidelines', ID3Lib will explode in a ball of brilliant blue frame if asked to parse an invalid ID3v2 tag.  This will change.
<LI>Incorrect handling of unknown frames.  This means that when ID3Lib encounters an unknown frame, it is currently ignored.  It is neither re-written to the tag when re-rendered nor are the file or tag alter frame flags observed.
<LI>No support for the read-only frame flag.  It is currently ignored – such frames can be altered at will.
<LI>No support as yet for the verification of frames before rendering.
<LI>Does not yet render 3.0 extended headers, although the function to select this is present but the setting is ignored for now.
<LI>Does not yet parse 3.0 extended headers.  They are quite adequately ignored and the rest of the tag is parsed correctly.
<LI>ID3Lib currently has no direct support for things like the language and currency fields.  It is up to the application to generate the data for these fields manually.  Soon, ID3Lib will have these things assigned to IDs so that the applications programmer will not have to remember the ISO tables for the actual strings.
<?php endSection(); ?>

<?php printHeader( "Appendix A - Class Reference");; ?>
<?php startSection(); ?>
<?php printHeader( "Application Useable Definitions");; ?>
<?php startSection(); ?>
ID3Lib defines some preprocessor symbols that applications are free to use.  They are used to identify the version and date of the ID3Lib that is currently being used.  All these definitions are strings, defined in 'id3_version.h'.

<?php startCode(); ?>
ID3LIB_NAME            defines the name of ID3Lib
ID3LIB_VERSION        defines which version of ID3Lib is being used (string)
ID3LIB_VER            defines which version of ID3Lib is being used (int)
ID3LIB_REV            defines which revision of ID3Lib is being used (int)
ID3LIB_DATE            the date which this version of ID3Lib was released
ID3LIB_VERSTRING        an amalgamation of most of the above
<?php endCode(); ?>

For example:

<?php startCode(); ?>
cout << "Using " << ID3LIB_VERSTRING << endl;
<?php endCode(); ?>
<?php endSection(); ?>


<?php printHeader( "The ID3_Tag Class");; ?>
<?php startSection(); ?>
<?php startCode(); ?>
lsint        ID3_IsTagHeader    ( uchar header[ ID3_TAGHEADERSIZE ] );

        ID3_Tag        ( char *fileName = NULL );
void        Clear            ( void );
bool        HasChanged        ( void );
void        SetUnsync        ( bool newSync );
void        SetExtendedHeader    ( bool ext );
void        SetCompression    ( bool comp );
void        SetPadding        ( bool pad );
void        SetPadding        ( luint fixedPaddingSize );
void        AddFrame        ( ID3_Frame *newFrame );
void        AddFrames
( ID3_Frame *newFrames, luint numFrames, bool
freeWhenDone = false );

void        RemoveFrame        ( ID3_Frame *oldFrame );
luint        Size            ( void );
luint        Render            ( uchar *buffer );
void        Parse 
( uchar header[ ID3_TAGHEADERSIZE ], uchar *buffer );

luint        Link            ( char *name );
void        Update            ( void );
void        Strip            ( bool v1Also = true );
ID3_Frame    *Find            ( ID3_FrameID id );
ID3_Frame    *Find
( ID3_FrameID id, ID3_FieldID fld, luint data );

ID3_Frame    *Find
        ( ID3_FrameID id, ID3_FieldID fld, char *data );

ID3_Frame    *Find
            ( ID3_FrameID id, ID3_FieldID fld, wchar_t *data );

luint        NumFrames        ( void );
ID3_Frame    *GetFrameNum        ( luint num );
ID3_Frame    *operator[]        ( luint num );
<?php endCode(); ?>
<?php endSection(); ?>


<?php printHeader( "The ID3_Frame Class");; ?>
<?php startSection(); ?>
<?php startCode(); ?>
ID3_Frame        ( ID3_FrameID id = ID3FID_NOFRAME );

void        Clear            ( void );

void        SetID            ( ID3_FrameID id );
ID3_FrameID    GetID            ( void );

ID3_Field&    Field            ( ID3_FieldID name );

<?php endCode(); ?>
<?php endSection(); ?>
<?php printHeader( "The ID3_Field Class");; ?>
<?php startSection(); ?>
<?php startCode(); ?>
void        Clear            ( void );
luint        Size            ( void );
luint        GetNumTextItems    ( void );

// integer field functions
ID3_Field&    operator=        ( luint newData );
void        Set            ( luint newData );
luint        Get            ( void );

// Unicode string field functions
ID3_Field&    operator=        ( wchar_t *string );
void        Set            ( wchar_t *string );
luint        Get
( wchar_t *buffer, luint maxChars, luint itemNum = 1 );

void        Add            ( wchar_t *string );

// ASCII string field functions
ID3_Field&    operator=        ( char *string );
void        Set            ( char *string );
luint        Get
( wchar_t *buffer, luint maxChars, luint itemNum = 1 );

void        Add            ( char *string );

// binary field functions
void        Set            ( uchar *newData, luint newSize );
void        Get            ( uchar *buffer, luint buffLength );
void        FromFile        ( char *fileName );
void        ToFile            ( char *fileName );
<?php endCode(); ?>
<?php endSection(); ?>

<?php printHeader( "The ID3_Error Class");; ?>
<?php startSection(); ?>
<?php startCode(); ?>
ID3_Err        GetErrorID        ( void );
char        *GetErrorDesc        ( void );
char        *GetErrorFile        ( void );
luint        GetErrorLine        ( void );
<?php endCode(); ?>
<?php endSection(); ?>

<?php printHeader( "The ID3Lib DLL (C-wrapper functions)");; ?>
<?php startSection(); ?>
<?php startCode(); ?>
void        ID3_GetVersionInfo    ( ID3_VerInfo *info );

lsint        ID3_IsTagHeader    ( uchar header[ ID3_TAGHEADERSIZE ] );

ID3_Tag        *ID3Tag_New        ( void );
void        ID3Tag_Delete        ( ID3_Tag *tag );
void        ID3Tag_Clear        ( ID3_Tag *tag );
bool        ID3Tag_HasChanged    ( ID3_Tag *tag );
void        ID3Tag_SetUnsync    ( ID3_Tag *tag, bool unsync );
void        ID3Tag_SetExtendedHeader
( ID3_Tag *tag, bool ext );

void        ID3Tag_SetCompression
( ID3_Tag *tag, bool comp );

void        ID3Tag_SetPadding    ( ID3_Tag *tag, bool pad );
void        ID3Tag_AddFrame    ( ID3_Tag *tag, ID3_Frame *frame );
void        ID3Tag_AddFrames
( ID3_Tag *tag, ID3_Frame *frames, luint num );

void        ID3Tag_RemoveFrame    ( ID3_Tag *tag, ID3_Frame *frame );
void        ID3Tag_Parse
( ID3_Tag *tag, uchar header[ ID3_TAGHEADERSIZE ],
uchar *buffer );

luint        ID3Tag_Link        ( ID3_Tag *tag, char *fileName );
void        ID3Tag_Update        ( ID3_Tag *tag );
void        ID3Tag_Strip        ( ID3_Tag *tag, bool v1Also );
ID3_Frame    *ID3Tag_FindFrameWithID
    ( ID3_Tag *tag, ID3_FrameID id );

ID3_Frame    *ID3Tag_FindFrameWithINT
( ID3_Tag *tag, ID3_FrameID id, ID3_FieldID fld,
luint data );

ID3_Frame    *ID3Tag_FindFrameWithASCII
( ID3_Tag *tag, ID3_FrameID id, ID3_FieldID fld,
char *data );

ID3_Frame    *ID3Tag_FindFrameWithUNICODE
( ID3_Tag *tag, ID3_FrameID id, ID3_FieldID fld,
wchar_t *data );

luint        ID3Tag_NumFrames    ( ID3_Tag *tag );
ID3_Frame    *ID3Tag_GetFrameNum    ( ID3_Tag *tag, luint num );

void        ID3Frame_Clear    ( ID3_Frame *frame );
void        ID3Frame_SetID    ( ID3_Frame *frame, ID3_FrameID id );
ID3_FrameID    ID3Frame_GetID    ( ID3_Frame *frame );
ID3_Field    *ID3Frame_GetField    ( ID3_Frame *frame, ID3_FieldID name );

void        ID3Field_Clear    ( ID3_Field *field );
luint        ID3Field_Size        ( ID3_Field *field );
luint        ID3Field_GetNumTextItems
        ( ID3_Field *field );
void        ID3Field_SetINT    ( ID3_Field *field, luint data );
luint        ID3Field_GetINT    ( ID3_Field *field );
void        ID3Field_SetUNICODE    ( ID3_Field *field, wchar_t *string );
luint        ID3Field_GetUNICODE
( ID3_Field *field, wchar_t *buffer, luint maxChars,
luint itemNum );

void        ID3Field_AddUNICODE ( ID3_Field *field, wchar_t *string );

void        ID3Field_SetASCII    ( ID3_Field *field, char *string );
luint        ID3Field_GetASCII    ( ID3_Field *field, char *buffer,
luint maxChars, luint itemNum );

void        ID3Field_AddASCII    ( ID3_Field *field, char *string );
void        ID3Field_SetBINARY
        ( ID3_Field *field, uchar *data, luint size );

void        ID3Field_GetBINARY
( ID3_Field *field, uchar *buffer, luint buffLength );

void        ID3Field_FromFile    ( ID3_Field *field, char *fileName );
void        ID3Field_ToFile    ( ID3_Field *field, char *fileName );
<?php endCode(); ?>
<?php endSection(); ?>
<?php endSection(); ?>

<?php printHeader( "Contact Information");; ?>
<?php startSection(); ?>
I can be contacted by e-mail at dirk@id3.org or on ICQ UIN 808 7113.
<P>
The ID3Lib mailing list is a general ID3Lib announcements list.  To subscribe to it, send an e-mail message to dirk@id3.org with ID3LibSub as the subject and the automatic newsletter software will add you to the list.  Same again but with ID3LibUnsub as the subject will remove you from the list.
<P>
When you subscribe, please include a brief message about why you are subscribing.  If you are writing software, tell me what you are writing and what platforms you will support.  If ID3Lib is lacking in a certain area, I can look to see who is doing what and can ask for advice and ideas from the appropriate people.
<?php endSection(); ?>

<?php printHeader( "Special Thanks and Credits");; ?>
<?php startSection(); ?>
I would like to extend my many thanks to the people who have contributed to the ID3Lib project.  The show of support has been tremendous.  I consider ID3Lib to be a very 'international' product, as contributions have come from almost literally every corner of the globe.  If I have missed you, please forgive my lapse of memory.
 
<UL>
<LI>Jean-loup Gailly and Mark Adler – for their great zlib compression library and for making it free.
<LI>Tord Jansson – for much help with teaching me how to make and use DLLs.
<LI>Slava Karpenko – for creating the MacOS static link libraries for the PowerPC and CodeWarrior.
<LI>Bob Kohn – for his advice, input and generally creating the ID3Lib licence agreement.
<LI>Eng-Keong Lee – for finding a few bugs and for extensively testing ID3Lib 2.xx.
<LI>James Lin – for his 'ID3v2 Programming Guidelines', and many helpful suggestions.
<LI>Michael Mutschler – for prompting me to write the Unicode support and for his input on the ID3Lib calling convention.
<LI>Martin Nilsson – for ID3v2, his support of the ID3Lib web page, for many, many suggestions, debates, pointers, URLs, documents and brightly coloured fish.
<LI>Chris Nunn – for the 3D animated ID3v2 logos which appear in the ID3Lib web page and in the distribution.
<LI>Lachlan Pitts – for general implementation ideas and his brief but helpful work on the up-coming genre tree.
<LI>Jukka Poikolainen – for prompting to implement error handling via the C++ exception handling mechanism instead of the old 2.xx-style of using an error handling function.  Also for compiling the Borland static link libraries and greatly helping with the DLL.
<LI>Carlos Puchol – for his help with some minor Linux compilation hassles.
<LI>Andreas Sigfridsson – for his initial code for the unsync/resync support and for his very valuable input in long brainstorming sessions.
<LI>Michael Robertson – for helping support ID3Lib by posting announcements on MP3.com.
<LI>Ilana Rudnik – for bug finding and suggestions.
<LI>Chuck Zenkus – for his support of ID3v2 and ID3Lib by providing us with a mirror in the United States and for his bug finding and suggestions.
</UL>
<P>
And last but by no means least, all the others who support ID3Lib by subscribing to the mailing list and to the contributors to the discussions and debates in the ID3v2 discussion group.
<P>
Without the help of all these people, ID3Lib would not be as good as it is, and I dare say might not even exist if they all weren't around to provide motivation to continue to write the thing!
<P>
- Dirk Mahoney<BR>
22 November 1998<BR>
Brisbane, Australia
<?php endSection(); ?>





</BODY>
</HTML>
