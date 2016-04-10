VERSION 5.00
Begin VB.Form frmID3Test 
   Caption         =   "ID3 Test"
   ClientHeight    =   5595
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   7845
   LinkTopic       =   "Form1"
   ScaleHeight     =   5595
   ScaleWidth      =   7845
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton cmdVersion 
      Caption         =   "Show ID3COM Version"
      Height          =   375
      Left            =   5640
      TabIndex        =   8
      Top             =   2160
      Width           =   2175
   End
   Begin VB.CommandButton cmdRemoveV2 
      Caption         =   "Remove ID3v2"
      Height          =   375
      Left            =   5640
      TabIndex        =   7
      Top             =   1200
      Width           =   2175
   End
   Begin VB.CommandButton cmdAddTestTag 
      Caption         =   "Add Test Tag"
      Height          =   375
      Left            =   5640
      TabIndex        =   6
      Top             =   1680
      Width           =   2175
   End
   Begin VB.CommandButton cmdCopyV1toV2 
      Caption         =   "Copy ID3 v1 to v2"
      Enabled         =   0   'False
      Height          =   375
      Left            =   5640
      TabIndex        =   5
      Top             =   240
      Width           =   2175
   End
   Begin VB.CommandButton cmdRemoveV1 
      Caption         =   "Remove ID3v1"
      Enabled         =   0   'False
      Height          =   375
      Left            =   5640
      TabIndex        =   4
      Top             =   720
      Width           =   2175
   End
   Begin VB.ListBox List1 
      Height          =   2595
      Left            =   120
      TabIndex        =   3
      Top             =   2880
      Width           =   5415
   End
   Begin VB.FileListBox File1 
      Height          =   2625
      Left            =   2400
      Pattern         =   "*.mp3;*.tag"
      TabIndex        =   2
      Top             =   120
      Width           =   3135
   End
   Begin VB.DirListBox Dir1 
      Height          =   2115
      Left            =   120
      TabIndex        =   1
      Top             =   600
      Width           =   2055
   End
   Begin VB.DriveListBox Drive1 
      Height          =   315
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   2055
   End
End
Attribute VB_Name = "frmID3Test"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
Private oTag As ID3ComTag

Private Sub cmdAddTestTag_Click()
    Dim oFrame As ID3ComFrame
    If Not oTag Is Nothing Then
        Set oFrame = oTag.FindFrameString(ID3_USERTEXT, ID3_FIELD_DESCRIPTION, "ID3ComTest")
        oFrame.Field(ID3_FIELD_TEXT).Text(1) = "This is a test done at " & Time
        oTag.SaveV2Tag
        File1_Click
    End If
End Sub

Private Sub cmdCopyV1toV2_Click()
    If Not oTag Is Nothing Then
        If Not oTag.HasV2Tag Then
            oTag.SaveV2Tag
            File1_Click
        Else
            MsgBox "File already Has V2 Tag"
        End If
    End If
End Sub

Private Sub cmdRemoveV1_Click()
    If Not oTag Is Nothing Then
        oTag.StripV1Tag
        File1_Click
    End If
End Sub

Private Sub cmdRemoveV2_Click()
    If Not oTag Is Nothing Then
        oTag.StripV2Tag
        File1_Click
    End If
End Sub

Private Sub cmdVersion_Click()
    If Not oTag Is Nothing Then
        MsgBox oTag.VersionString
    End If
End Sub

Private Sub Dir1_Change()
    On Error Resume Next
    File1.Path = Dir1.Path
End Sub

Private Sub Drive1_Change()
    On Error Resume Next
    Dir1.Path = Drive1.Drive
End Sub

Private Sub File1_Click()
    Dim oFrame As ID3ComFrame
    Dim oField As ID3ComField
    Dim FrameLine As String
    Dim n As Long
    
    List1.Clear
    
    Set oTag = New ID3ComTag

    oTag.Link File1.Path & "\" & File1.FileName

    cmdCopyV1toV2.Enabled = oTag.HasV1Tag And Not oTag.HasV2Tag
    cmdRemoveV1.Enabled = oTag.HasV1Tag
    cmdRemoveV2.Enabled = oTag.HasV2Tag
    
    For Each oFrame In oTag
        FrameLine = oFrame.FrameName
        Set oField = oFrame.Field(ID3_FIELD_DESCRIPTION)
        If Not oField Is Nothing Then
            FrameLine = FrameLine & " - " & oField.Text(1)
        End If
        Set oField = oFrame.Field(ID3_FIELD_EMAIL)
        If Not oField Is Nothing Then
            FrameLine = FrameLine & " - " & oField.Text(1)
        End If
        Set oField = oFrame.Field(ID3_FIELD_TEXT)
        If Not oField Is Nothing Then
            For n = 1 To oField.NumTextItems
                FrameLine = FrameLine & " - " & oField.Text(n)
            Next n
        End If
        ' could go on
        List1.AddItem FrameLine
    Next
End Sub

