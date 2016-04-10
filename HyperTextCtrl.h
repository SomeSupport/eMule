/********************************************************************

		HyperTextCtrl.h - Controls that shows hyperlinks 
		in text

		Copyright (C) 2001-2002 Magomed G. Abdurakhmanov			

********************************************************************/
//edited by (C)2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
//-> converted it to MFC
//-> included colored keywords
//-> fixed GPF bugs
//-> made it flickerfree
//-> some other small changes
// (the whole code still needs some work though)
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#pragma once
#include <list>
#include <vector>

#define HTC_WORDWRAP			1	// word wrap text
#define HTC_AUTO_SCROLL_BARS	2	// auto hide scroll bars
#define HTC_UNDERLINE_LINKS		4	// underline links
#define HTC_UNDERLINE_HOVER		8	// underline hover links
#define HTC_ENABLE_TOOLTIPS		16	// enable hyperlink tool tips

// --------------------------------------------------------------
// CHyperLink

class CHyperLink{
	friend class CPreparedHyperText;
public:
	CHyperLink(); // i_a 
	CHyperLink(int iBegin, uint16 iEnd, const CString& sTitle, const CString& sCommand, const CString& sDirectory);
	CHyperLink(int iBegin, uint16 iEnd, const CString& sTitle, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	CHyperLink(const CHyperLink& Src);

	void Execute();
	bool operator < (const CHyperLink& Arg) const	{return m_iEnd < Arg.m_iEnd;}
	UINT Begin() const								{return m_iBegin;}
	UINT End() const								{return m_iEnd;}
	UINT Len() const								{return m_iEnd - m_iBegin + 1;}
	CString Title() const							{return m_sTitle;}
	void SetBegin( uint16 m_iInBegin )				{m_iBegin = m_iInBegin;}
	void SetEnd( uint16 m_iInEnd )					{m_iEnd = m_iInEnd;}

protected:
	int m_iBegin;
	int m_iEnd;
	CString m_sTitle;

	enum LinkType
	{
		lt_Unknown = 0,  // i_a 
		lt_Shell = 0, /* http:// mailto:*/
		lt_Message = 1 /* WM_COMMAND */
	} m_Type;

	// used for lt_Shell
	CString m_sCommand;
	CString m_sDirectory;
	// used for lt_Message
	HWND m_hWnd; 
	UINT m_uMsg;
	WPARAM m_wParam;
	LPARAM m_lParam;
};

// --------------------------------------------------------------
// CKeyWord

class CKeyWord{
	friend class CPreparedHyperText;
public:
	CKeyWord();
	CKeyWord(int iBegin, uint16 iEnd, COLORREF icolor);

	bool operator< (const CKeyWord& Arg) const		{return m_iEnd < Arg.m_iEnd;}
	UINT Begin() const								{return m_iBegin;}
	UINT End() const								{return m_iEnd;}
	void SetBegin( uint16 m_iInBegin )				{m_iBegin = m_iInBegin;}
	void SetEnd( uint16 m_iInEnd )					{m_iEnd = m_iInEnd;}
	COLORREF Color() const							{return color;}
	UINT Len() const								{return m_iEnd - m_iBegin + 1;}
protected:
	int m_iBegin;
	int m_iEnd;
	COLORREF color;
};

// --------------------------------------------------------------
// CPreparedHyperText

class CPreparedHyperText{
public:
	CPreparedHyperText()						{}
	CPreparedHyperText(const CString& sText);
	CPreparedHyperText(const CPreparedHyperText& src);

	void Clear();
	void SetText(const CString& sText);
	void AppendText(const CString& sText);
	void AppendHyperLink(const CString& sText, const CString& sTitle, const CString& sCommand, const CString& sDirectory);
	void AppendHyperLink(const CString& sText, const CString& sTitle, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void AppendKeyWord(const CString& sText, COLORREF iColor);

	 CString& GetText()					{return m_sText;}
	 std::list<CHyperLink>& GetLinks()	{return m_Links;}
	 std::list<CKeyWord>& GetKeywords()	{return m_KeyWords;}
	//friend class CHyperTextCtrl;

protected:
	CString m_sText;
	std::list<CHyperLink> m_Links;
	std::list<CKeyWord> m_KeyWords;

	void RemoveLastSign(CString& sLink);
	void PrepareText(const CString& sText);
	bool tspace(TCHAR c)						{return _istspace((_TUCHAR)c) || /*c < _T(' ') || */c == _T(';') || c == _T('!');}

};
// --------------------------------------------------------------
// CLinePartInfo
class CLinePartInfo{
public:
	uint16 m_xBegin;
	uint16 m_xEnd;
	CHyperLink* m_pHyperLink;
	CKeyWord* m_pKeyWord;

	CLinePartInfo(int iBegin, uint16 iEnd, CHyperLink* pHyperLink = NULL, CKeyWord* pKeyWord = NULL);
	CLinePartInfo(const CLinePartInfo& Src);
	uint16 Begin()							{return m_xBegin;}
	uint16 End()							{return m_xEnd;}
	uint16 Len()							{return ((m_xEnd - m_xBegin) + 1);}
};

// --------------------------------------------------------------
// CLineInfo
class CLineInfo : public std::vector<CLinePartInfo>{
public:
	int m_iBegin;
	int m_iEnd;

	CLineInfo(int iBegin, uint16 iEnd);
	CLineInfo(const CLineInfo& Src);
	UINT Begin()						{return m_iBegin;}
	UINT End()							{return m_iEnd;}
	UINT Len()							{return m_iEnd - m_iBegin + 1;}
};

// --------------------------------------------------------------
// CVisPart
class CVisPart : public CLinePartInfo {
public:
	CRect m_rcBounds;
	int m_iRealBegin;
	int m_iRealLen;
	CVisPart* m_pPrev;
	CVisPart* m_pNext;

	CVisPart(const CLinePartInfo& LinePartInfo, const CRect& rcBounds, 
		int iRealBegin, uint16 iRealLen,CVisPart* pPrev,CVisPart* pNext);
	CVisPart(const CVisPart& Src);
};

class CVisLine : public std::vector<CVisPart>
{	};


// --------------------------------------------------------------
// CHyperTextCtrl

class CHyperTextCtrl : public CWnd{
	DECLARE_DYNAMIC(CHyperTextCtrl)
protected:
	CPreparedHyperText* m_Text;
	CPreparedHyperText  standart_Text;
	std::vector<CLineInfo> m_Lines;	
	CFont* m_Font;
	COLORREF m_BkColor;
	COLORREF m_TextColor;
	COLORREF m_LinkColor;
	COLORREF m_HoverColor;
	HCURSOR m_LinkCursor;
	HCURSOR m_DefaultCursor;

	CToolTipCtrl m_tip;

	//temporary variables
	bool vscrollon;
	int m_iMaxWidth;				// The maximum line width
	int m_iLineHeight;				// Height of one line
	int m_iLinesHeight;				// Sum of height of all lines
	bool m_bDontUpdateSizeInfo;		// Used to prevent recursive call of the UpdateSize() method
	int m_iVertPos;					// Vertical position in percents
	int m_iHorzPos;					// Horizontal position in percents
	CFont m_DefaultFont;			// This font is set by default
	CFont m_LinksFont;				// Copied from main font to faster draw (link)
	CFont m_HoverFont;				// Copied from main font to faster draw (hover link)
	std::vector<CVisLine> m_VisLines;	// Currently visible text
	CVisPart* m_pActivePart;		// Active part of link (hovered)
	int m_iWheelDelta;				// Mouse wheel scroll delta
	DECLARE_MESSAGE_MAP()

public:
	CHyperTextCtrl();
	virtual BOOL PreTranslateMessage(MSG* /*pMsg*/)			{return FALSE;}
	void Clear()					{m_Text->Clear();UpdateSize(true);}

	//message handlers
	afx_msg void OnMouseMove(UINT nFlags,CPoint pt);
	afx_msg void OnLButtonDown(UINT nFlags,CPoint pt);
	afx_msg BOOL OnMouseWheel(UINT nFlags,short zDelta,CPoint pt);
	afx_msg LRESULT OnCreate(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDestroy(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnPaint(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSize(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnShowWindow(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSetText(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetText(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSetFont(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGetFont(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnHScroll(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnVScroll(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnCaptureChanged(WPARAM wParam, LPARAM lParam);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSysColorChange();
	// Operations
	 CPreparedHyperText* GetHyperText();
	 void SetHyperText(CPreparedHyperText* Src, bool bInvalidate = true);
	 void AppendText(const CString& sText, bool bInvalidate = true);
	 void AppendHyperLink(const CString& sText, const CString& sTitle, const CString& sCommand, const CString& sDirectory, bool bInvalidate = true);
	 void AppendHyperLink(const CString& sText, const CString& sTitle, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool bInvalidate = true);
	 void AppendKeyWord(const CString& sText, COLORREF icolor);
	 COLORREF GetBkColor()							{return m_BkColor;}
	 void SetBkColor(COLORREF Color)				{m_BkColor = Color;}	
	 COLORREF GetTextColor()						{return m_TextColor;}
	 void SetTextColor(COLORREF Color)				{m_TextColor = Color;}	
	 COLORREF GetLinkColor()						{return m_LinkColor;}
	 void SetLinkColor(COLORREF LinkColor, bool bInvalidate = true);
	 COLORREF GetHoverColor()						{return m_HoverColor;}
	 void SetHoverColor(COLORREF HoverColor)		{m_HoverColor = HoverColor;}	
	 HCURSOR GetLinkCursor()						{return m_LinkCursor;}
	 void SetLinkCursor(HCURSOR LinkCursor)			{m_LinkCursor = LinkCursor;}
	 HCURSOR GetDefaultCursor()						{return m_DefaultCursor;}
	 void SetDefaultCursor(HCURSOR DefaultCursor)	{m_DefaultCursor = DefaultCursor;}
	 void UpdateSize(bool bRepaint);
protected:
	bool check_bits(DWORD Value, DWORD Mask)		{return (Value & Mask) == Mask;}
	void UpdateFonts();
	void UpdateVisLines();
	void HighlightLink(CVisPart* Part, const CPoint& MouseCoords);
	void RestoreLink();
	void SetColors();
	void LoadHandCursor();
};
