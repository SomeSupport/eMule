//this file is part of eMule
//Copyright (C)2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
#include "stdafx.h"
#include <share.h>
#include "emule.h"
#include "LogEditCtrl.h"
#include "OtherFunctions.h"
#include "Preferences.h"
#include "MenuCmds.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNAMIC(CLogEditCtrl, CEdit)

BEGIN_MESSAGE_MAP(CLogEditCtrl, CEdit)
	ON_WM_CONTEXTMENU()
	ON_WM_KEYDOWN()
	ON_WM_PAINT()
	ON_CONTROL_REFLECT(EN_ERRSPACE, OnEnErrspace)
	ON_CONTROL_REFLECT(EN_MAXTEXT, OnEnMaxtext)
	ON_WM_CTLCOLOR_REFLECT()
	ON_WM_SYSCOLORCHANGE()
END_MESSAGE_MAP()

CLogEditCtrl::CLogEditCtrl(){
	m_bRichEdit = false;
	m_bAutoScroll = true;
	m_bNoPaint = false;
	m_bEnErrSpace = false;
	m_iMaxLogBuff = 0;
	m_crForeground = CLR_DEFAULT;
	m_crBackground = CLR_DEFAULT;
}

CLogEditCtrl::~CLogEditCtrl(){
}

void CLogEditCtrl::Localize(){
}

void CLogEditCtrl::Init(LPCTSTR pszTitle, LPCTSTR pszSkinKey)
{
	CHAR szClassName[MAX_PATH];
	GetClassNameA(*this, szClassName, ARRSIZE(szClassName));
	m_bRichEdit = __ascii_stricmp(szClassName, "EDIT") != 0;
	m_strSkinKey = pszSkinKey;

	SetTitle(pszTitle);

	m_LogMenu.CreatePopupMenu();
	m_LogMenu.AddMenuTitle(GetResString(IDS_LOGENTRY));
	m_LogMenu.AppendMenu(MF_STRING, MP_COPYSELECTED, GetResString(IDS_COPY));
	m_LogMenu.AppendMenu(MF_SEPARATOR);
	m_LogMenu.AppendMenu(MF_STRING, MP_SELECTALL, GetResString(IDS_SELECTALL));
	m_LogMenu.AppendMenu(MF_STRING, MP_REMOVEALL, GetResString(IDS_PW_RESET));
	m_LogMenu.AppendMenu(MF_STRING, MP_SAVELOG, GetResString(IDS_SAVELOG) + _T("..."));
	m_LogMenu.AppendMenu(MF_SEPARATOR);
	m_LogMenu.AppendMenu(MF_STRING, MP_AUTOSCROLL, GetResString(IDS_AUTOSCROLL));

	VERIFY( SendMessage(EM_SETUNDOLIMIT, 0, 0) == 0 );
	int iMaxLogBuff = thePrefs.GetMaxLogBuff();
	if (afxData.bWin95)
		LimitText(m_iMaxLogBuff = (iMaxLogBuff > 0xFFFF ? 0xFFFF : iMaxLogBuff));
	else
		LimitText(m_iMaxLogBuff = (iMaxLogBuff ? iMaxLogBuff : 128*1024));
	FlushBuffer();
}

void CLogEditCtrl::SetTitle(LPCTSTR pszTitle)
{
	m_strTitle = pszTitle;
}

LRESULT CLogEditCtrl::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
		case WM_ERASEBKGND:
			if (m_bNoPaint)
				return TRUE;
		case WM_PAINT:
			if (m_bNoPaint)
				return TRUE;
	}
	return CEdit::WindowProc(message, wParam, lParam);
}

void CLogEditCtrl::FlushBuffer()
{
	if (m_astrBuff.GetSize() > 0){ // flush buffer
		for (int i = 0; i < m_astrBuff.GetSize(); i++)
			AddLine(m_astrBuff[i], m_astrBuff[i].GetLength());
		m_astrBuff.RemoveAll();
	}
}

void CLogEditCtrl::AddEntry(LPCTSTR pszMsg)
{
	CString strLine(pszMsg);
	strLine += _T("\r\n");
	if (m_hWnd == NULL){
		m_astrBuff.Add(strLine);
	}
	else{
		FlushBuffer();
		AddLine(strLine, strLine.GetLength());
	}
}

void CLogEditCtrl::Add(LPCTSTR pszMsg, int iLen)
{
	if (m_hWnd == NULL){
		CString strLine(pszMsg);
		m_astrBuff.Add(strLine);
	}
	else{
		FlushBuffer();
		AddLine(pszMsg, iLen);
	}
}

//////////////////////////////////////////////////////////////////////////////
// This function is based on Daniel Lohmann's article "CEditLog - fast logging
// into an edit control with cout" at http://www.codeproject.com
void CLogEditCtrl::AddLine(LPCTSTR pszMsg, int iLen)
{
	int iMsgLen = (iLen == -1) ? _tcslen(pszMsg) : iLen;
	if (iMsgLen == 0)
		return;
#ifdef _DEBUG
	if (pszMsg[iMsgLen - 1] == _T('\n'))
		ASSERT( iMsgLen >= 2 && pszMsg[iMsgLen - 2] == _T('\r') );
#endif

	// Get Edit contents dimensions and cursor position
	int iStartChar, iEndChar;
	GetSel(iStartChar, iEndChar);
	int iWndTxtLen = GetWindowTextLength();

	if (iStartChar == iWndTxtLen && iWndTxtLen == iEndChar)
	{
		// The cursor resides at the end of text
		SCROLLINFO si;
		si.cbSize = sizeof si;
		si.fMask = SIF_ALL;
		if (m_bAutoScroll && GetScrollInfo(SB_VERT, &si) && si.nPos >= (int)(si.nMax - si.nPage + 1))
		{
			// Not scrolled away
			SafeAddLine(iWndTxtLen, iMsgLen, pszMsg, iStartChar, iEndChar);
			if (m_bAutoScroll && !IsWindowVisible())
				ScrollToLastLine();
		}
		else
		{
			// Reduce flicker by ignoring WM_PAINT
			m_bNoPaint = true;
			BOOL bIsVisible = IsWindowVisible();
			if (bIsVisible)
				SetRedraw(FALSE);

			// Remember where we are
			int nFirstLine = !m_bAutoScroll ? GetFirstVisibleLine() : 0;
		
			// Select at the end of text and replace the selection
			// This is a very fast way to add text to an edit control
			SafeAddLine(iWndTxtLen, iMsgLen, pszMsg, iStartChar, iEndChar);
			SetSel(iStartChar, iEndChar, TRUE); // Restore our previous selection

			if (!m_bAutoScroll)
				LineScroll(nFirstLine - GetFirstVisibleLine());
			else
				ScrollToLastLine();

			m_bNoPaint = false;
			if (bIsVisible){
				SetRedraw();
				if (m_bRichEdit)
					Invalidate();
			}
		}
	}
	else
	{
		// We should add the text anyway...

		// Reduce flicker by ignoring WM_PAINT
		m_bNoPaint = true;
		BOOL bIsVisible = IsWindowVisible();
		if (bIsVisible)
			SetRedraw(FALSE);

		// Remember where we are
		int nFirstLine = !m_bAutoScroll ? GetFirstVisibleLine() : 0;
	
		if (iStartChar != iEndChar)
		{
			// If we are currently selecting some text, we have to find out
			// if the caret is near the beginning of this block or near the end.
			// Note that this does not always work. Because of the EM_CHARFROMPOS
			// message returning only 16 bits this will fail if the user has selected 
			// a block with a length dividable by 64k.

			// NOTE: This may cause a lot of terrible CRASHES within the RichEdit control when used for a RichEdit control!?
			// To reproduce the crash: click in the RE control while it's drawing a line an start a selection!
			if (!m_bRichEdit){
			    CPoint pt;
			    ::GetCaretPos(&pt);
			    int nCaretPos = CharFromPos(pt);
			    if (abs((iStartChar % 0xffff - nCaretPos)) < abs((iEndChar % 0xffff - nCaretPos)))
			    {
				    nCaretPos = iStartChar;
				    iStartChar = iEndChar;
				    iEndChar = nCaretPos;
			    }
		    }
		}

		// Note: This will flicker, if someone has a good idea how to prevent this - let me know
		
		// Select at the end of text and replace the selection
		// This is a very fast way to add text to an edit control
		SafeAddLine(iWndTxtLen, iMsgLen, pszMsg, iStartChar, iEndChar);
		SetSel(iStartChar, iEndChar, TRUE); // Restore our previous selection

		if (!m_bAutoScroll)
			LineScroll(nFirstLine - GetFirstVisibleLine());
		else
			ScrollToLastLine();

		m_bNoPaint = false;
		if (bIsVisible){
			SetRedraw();
			if (m_bRichEdit)
				Invalidate();
		}
	}
}

void CLogEditCtrl::OnEnErrspace()
{
	m_bEnErrSpace = true;
}

void CLogEditCtrl::OnEnMaxtext()
{
	m_bEnErrSpace = true;
}

void CLogEditCtrl::ScrollToLastLine()
{
	// WM_VSCROLL does not work correctly under Win98 (or older version of comctl.dll)
	//SendMessage(WM_VSCROLL, SB_BOTTOM, 0);
	LineScroll(GetLineCount());
}

void CLogEditCtrl::SafeAddLine(int nPos, int iLineLen, LPCTSTR pszLine, int& iStartChar, int& iEndChar)
{
	bool bOldNoPaint = false;
	BOOL bIsVisible = false;
	bool bRestorePaintFlag = false;

	// try to determine if the current log line will exceed the limit of the edit control and free
	// up enough space to add it. if it would be done afterwards (because of EN_ERRSPACE) it may cost
	// noticeable more CPU cycles.
	int iTextLen = nPos; // 'nPos' already holds the current window text length
	if (iTextLen + iLineLen > m_iMaxLogBuff)
	{
		// delete the 1st 10 lines; freeing up only 1 or 2 lines is still not enough (peformance problem)
		int iLine0Len = 0;
		for (int i = 0; i < 10; i++){
			int iLineLen = LineLength(iLine0Len);
			if (iLineLen == 0)
				break;
			iLine0Len += iLineLen + 2;
		}

		bOldNoPaint = m_bNoPaint;
		m_bNoPaint = true;
		bRestorePaintFlag = true;
		bIsVisible = IsWindowVisible();
		if (bIsVisible)
			SetRedraw(FALSE);

		SetSel(0, iLine0Len, TRUE);
		ReplaceSel(_T(""));

		// update any possible available selection
		iStartChar -= iLine0Len;
		if (iStartChar < 0)
			iStartChar = 0;
		iEndChar -= iLine0Len;
		if (iEndChar < 0)
			iEndChar = 0;

		nPos -= iLine0Len;
		if (nPos < 0)
			nPos = 0;
	}

	m_bEnErrSpace = false;
	SetSel(nPos, nPos, TRUE);
	ReplaceSel(pszLine);

	if (bRestorePaintFlag)
	{
		m_bNoPaint = bOldNoPaint;
		if (bIsVisible && !m_bNoPaint){
			SetRedraw();
			if (m_bRichEdit)
				Invalidate();
		}
	}

	if (m_bEnErrSpace)
	{
		// following code works properly, but there is a performance problem. if the control 
		// starts to rotate the text and if there is much to log, CPU usage hits the roof.
		// to get around this, we try to free the needed space for the current line (and more)
		// before adding the line (see code above). actually, the following code should not be
		// executed any longer, but is kept for fail safe handling.

		bool bOldNoPaint = m_bNoPaint;
		m_bNoPaint = true;
		BOOL bIsVisible = IsWindowVisible();
		if (bIsVisible)
			SetRedraw(FALSE);

		// remove the first line as long as we are capable of adding the new line
		int iSafetyCounter = 0;
		while (m_bEnErrSpace && iSafetyCounter < 10)
		{
			// delete the previous partially added line
			SetSel(nPos, -1, TRUE);
			ReplaceSel(_T(""));

			// delete 1st line
			int iLine0Len = LineLength(0) + 2; // add NL character
			SetSel(0, iLine0Len, TRUE);
			ReplaceSel(_T(""));

			// update any possible available selection
			iStartChar -= iLine0Len;
			if (iStartChar < 0)
				iStartChar = 0;
			iEndChar -= iLine0Len;
			if (iEndChar < 0)
				iEndChar = 0;

			// add the new line again
			nPos = GetWindowTextLength();
			SetSel(nPos, nPos, TRUE);
			m_bEnErrSpace = false;
			ReplaceSel(pszLine);

			if (m_bEnErrSpace && nPos == 0){
				// should never happen: if we tried to add the line another time in the 1st line, there 
				// will be no chance to add the line at all -> avoid endless loop!
				break;
			}
			iSafetyCounter++; // never ever create an endless loop!
		}
		m_bNoPaint = bOldNoPaint;
		if (bIsVisible && !m_bNoPaint){
			SetRedraw();
			if (m_bRichEdit)
				Invalidate();
		}
	}
}

void CLogEditCtrl::Reset(){
	m_astrBuff.RemoveAll();
	SetRedraw(FALSE);
	SetWindowText(_T(""));
	SetRedraw();
	if (m_bRichEdit)
		Invalidate();
}

void CLogEditCtrl::OnContextMenu(CWnd* /*pWnd*/, CPoint point){
	int iSelStart, iSelEnd;
	GetSel(iSelStart, iSelEnd);
	int iTextLen = GetWindowTextLength();

	m_LogMenu.EnableMenuItem(MP_COPYSELECTED, iSelEnd > iSelStart ? MF_ENABLED : MF_GRAYED);
	m_LogMenu.EnableMenuItem(MP_REMOVEALL, iTextLen > 0 ? MF_ENABLED : MF_GRAYED);
	m_LogMenu.EnableMenuItem(MP_SELECTALL, iTextLen > 0 ? MF_ENABLED : MF_GRAYED);
	m_LogMenu.EnableMenuItem(MP_SAVELOG, iTextLen > 0 ? MF_ENABLED : MF_GRAYED);
	m_LogMenu.CheckMenuItem(MP_AUTOSCROLL, m_bAutoScroll ? MF_CHECKED : MF_UNCHECKED);
	if (point.x == -1 && point.y == -1){
		point.x = 16;
		point.y = 32;
		ClientToScreen(&point);
	}
	m_LogMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this);
}

BOOL CLogEditCtrl::OnCommand(WPARAM wParam, LPARAM /*lParam*/){
	switch (wParam) {
	case MP_COPYSELECTED:
		CopySelectedItems();
		break;
	case MP_SELECTALL:
		SelectAllItems();
		break;
	case MP_REMOVEALL:
		Reset();
		break;
	case MP_SAVELOG:
		SaveLog();
		break;
	case MP_AUTOSCROLL:
		m_bAutoScroll = !m_bAutoScroll;
		break;
	}
	return TRUE;
}

bool CLogEditCtrl::SaveLog(LPCTSTR pszDefName)
{
	bool bResult = false;
	CFileDialog dlg(FALSE, _T("log"), pszDefName ? pszDefName : (LPCTSTR)m_strTitle, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Log Files (*.log)|*.log||"), this, 0);
	if (dlg.DoModal() == IDOK)
	{
		FILE* fp = _tfsopen(dlg.GetPathName(), _T("wb"), _SH_DENYWR);
		if (fp)
		{
#ifdef _UNICODE
			// write Unicode byte-order mark 0xFEFF
			fputwc(0xFEFF, fp);
#endif
			CString strText;
			GetWindowText(strText);
			fwrite(strText, sizeof(TCHAR), strText.GetLength(), fp);
			if (ferror(fp)){
				CString strError;
				strError.Format(_T("Failed to write log file \"%s\" - %s"), dlg.GetPathName(), strerror(errno));
				AfxMessageBox(strError, MB_ICONERROR);
			}
			else
				bResult = true;
			fclose(fp);
		}
		else{
			CString strError;
			strError.Format(_T("Failed to create log file \"%s\" - %s"), dlg.GetPathName(), strerror(errno));
			AfxMessageBox(strError, MB_ICONERROR);
		}
	}
	return bResult;
}

CString CLogEditCtrl::GetLastLogEntry(){
	CString strLog;
	int iLastLine = GetLineCount() - 2;
	if (iLastLine >= 0){
		GetLine(iLastLine, strLog.GetBuffer(1024), 1024);
		strLog.ReleaseBuffer();
	}
	return strLog;
}

CString CLogEditCtrl::GetAllLogEntries(){
	CString strLog;
	GetWindowText(strLog);
	return strLog;
}

void CLogEditCtrl::SelectAllItems()
{
	SetSel(0, -1, TRUE);
}

void CLogEditCtrl::CopySelectedItems()
{
	Copy();
}

void CLogEditCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == 'A' && (GetKeyState(VK_CONTROL) & 0x8000))
	{
		//////////////////////////////////////////////////////////////////
		// Ctrl+A: Select all items
		SelectAllItems();
	}
	else if (nChar == 'C' && (GetKeyState(VK_CONTROL) & 0x8000))
	{
		//////////////////////////////////////////////////////////////////
		// Ctrl+C: Copy listview items to clipboard
		CopySelectedItems();
	}

	CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}

HBRUSH CLogEditCtrl::CtlColor(CDC* pDC, UINT /*nCtlColor*/)
{
	if (m_crForeground != CLR_DEFAULT)
		pDC->SetTextColor(m_crForeground);
	else
	{
		// explicitly set the (default) text color -- needed for some contrast window color schemes
		pDC->SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
	}

	if (m_crBackground != CLR_DEFAULT && m_brBackground.m_hObject != NULL)
	{
		pDC->SetBkColor(m_crBackground);
		return m_brBackground;
	}
	else
	{
		pDC->SetBkColor(GetSysColor(COLOR_WINDOW));
		return GetSysColorBrush(COLOR_WINDOW);
	}
}

void CLogEditCtrl::OnSysColorChange()
{
	CEdit::OnSysColorChange();
	ApplySkin();
}

void CLogEditCtrl::ApplySkin()
{
	if (!m_strSkinKey.IsEmpty())
	{
		COLORREF cr;
		if (theApp.LoadSkinColor(m_strSkinKey + _T("Fg"), cr))
			m_crForeground = cr;
		else
			m_crForeground = CLR_DEFAULT;
		if (theApp.LoadSkinColor(m_strSkinKey + _T("Bk"), cr))
		{
			m_crBackground = cr;
			m_brBackground.DeleteObject();
			m_brBackground.CreateSolidBrush(m_crBackground);
		}
		else
		{
			m_crBackground = CLR_DEFAULT;
			m_brBackground.DeleteObject();
		}
	}
}
