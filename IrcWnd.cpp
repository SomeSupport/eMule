//this file is part of eMule
//Copyright (C)2002-2008 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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
#include "emule.h"
#include "emuleDlg.h"
#include "IrcWnd.h"
#include "IrcMain.h"
#include "otherfunctions.h"
#include "MenuCmds.h"
#include "HTRichEditCtrl.h"
#include "ClosableTabCtrl.h"
#include "HelpIDs.h"
#include "opcodes.h"
#include "InputBox.h"
#include "UserMsgs.h"
#include "ColourPopup.h"
#include "SmileySelector.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Request from IRC-folks: don't use similar colors for Status and Info messages
#define	STATUS_MSG_COLOR		RGB(0,128,0)		// dark green
#define	INFO_MSG_COLOR			RGB(192,0,0)		// mid red
#define	SENT_TARGET_MSG_COLOR	RGB(0,192,0)		// bright green
#define	RECV_SOURCE_MSG_COLOR	RGB(0,128,255)		// bright cyan/blue

#define	TIME_STAMP_FORMAT		_T("[%H:%M] ")

#pragma warning(disable:4125) // decimal digit terminates octal escape sequence
//static TCHAR s_szTimeStampColorPrefix[] = _T("\00302");		// dark blue
static TCHAR s_szTimeStampColorPrefix[] = _T("");	// default foreground color
#pragma warning(default:4125) // decimal digit terminates octal escape sequence

#define	SPLITTER_HORZ_MARGIN	0
#define	SPLITTER_HORZ_WIDTH		4
#define	SPLITTER_HORZ_RANGE_MIN	170
#define	SPLITTER_HORZ_RANGE_MAX	400

IMPLEMENT_DYNAMIC(CIrcWnd, CDialog)

BEGIN_MESSAGE_MAP(CIrcWnd, CResizableDialog)
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_WM_CONTEXTMENU()
	ON_WM_SYSCOLORCHANGE()
	ON_WM_CTLCOLOR()
	ON_WM_HELPINFO()
	ON_MESSAGE(UM_CLOSETAB, OnCloseTab)
	ON_MESSAGE(UM_QUERYTAB, OnQueryTab)
	ON_MESSAGE(UM_CPN_SELENDOK, OnSelEndOK)
	ON_MESSAGE(UM_CPN_SELENDCANCEL, OnSelEndCancel)
	ON_NOTIFY(EN_REQUESTRESIZE, IDC_TITLEWINDOW, OnEnRequestResizeTitle)
END_MESSAGE_MAP()

CIrcWnd::CIrcWnd(CWnd* pParent)
	: CResizableDialog(CIrcWnd::IDD, pParent)
{
	m_pIrcMain = NULL;
	m_bConnected = false;
	m_bLoggedIn = false;
	m_wndNicks.m_pParent = this;
	m_wndChanList.m_pParent = this;
	m_wndChanSel.m_bCloseable = true;
	m_wndChanSel.m_pParent = this;
	m_pwndSmileySel = NULL;
}

CIrcWnd::~CIrcWnd()
{
	if (m_bConnected)
		m_pIrcMain->Disconnect(true);
	delete m_pIrcMain;
	if (m_pwndSmileySel != NULL){
		m_pwndSmileySel->DestroyWindow();
		delete m_pwndSmileySel;
	}
}

void CIrcWnd::OnSysColorChange()
{
	CResizableDialog::OnSysColorChange();
	SetAllIcons();
}

void CIrcWnd::SetAllIcons()
{
	CImageList iml;
	iml.Create(16, 16, theApp.m_iDfltImageListColorFlags | ILC_MASK, 0, 1);
	iml.Add(CTempIconLoader(_T("Smiley_Smile")));
	iml.Add(CTempIconLoader(_T("Bold")));
	iml.Add(CTempIconLoader(_T("Underline")));
	iml.Add(CTempIconLoader(_T("Colour")));
	iml.Add(CTempIconLoader(_T("ResetFormat")));
	CImageList* pImlOld = m_wndFormat.SetImageList(&iml);
	iml.Detach();
	if (pImlOld)
		pImlOld->DeleteImageList();
}

void CIrcWnd::Localize()
{
	if (m_bConnected)
		GetDlgItem(IDC_BN_IRCCONNECT)->SetWindowText(GetResString(IDS_IRC_DISCONNECT));
	else
		GetDlgItem(IDC_BN_IRCCONNECT)->SetWindowText(GetResString(IDS_IRC_CONNECT));
	GetDlgItem(IDC_CHATSEND)->SetWindowText(GetResString(IDS_IRC_SEND));
	GetDlgItem(IDC_CLOSECHAT)->SetWindowText(GetResString(IDS_FD_CLOSE));
	m_wndChanList.Localize();
	m_wndChanSel.Localize();
	m_wndNicks.Localize();

	m_wndFormat.SetBtnText(IDC_SMILEY, _T("Smileys"));
	m_wndFormat.SetBtnText(IDC_BOLD, GetResString(IDS_BOLD));
	m_wndFormat.SetBtnText(IDC_UNDERLINE, GetResString(IDS_UNDERLINE));
	m_wndFormat.SetBtnText(IDC_COLOUR, GetResString(IDS_COLOUR));
	m_wndFormat.SetBtnText(IDC_RESET, GetResString(IDS_RESETFORMAT));
}

BOOL CIrcWnd::OnInitDialog()
{
	CResizableDialog::OnInitDialog();

	m_bConnected = false;
	m_bLoggedIn = false;
	m_pIrcMain = new CIrcMain();
	m_pIrcMain->SetIRCWnd(this);

	UpdateFonts(&theApp.m_fontHyperText);
	InitWindowStyles(this);
	SetAllIcons();

	m_wndInput.SetLimitText(MAX_IRC_MSG_LEN);
	if (theApp.m_fontChatEdit.m_hObject)
	{
		m_wndInput.SendMessage(WM_SETFONT, (WPARAM)theApp.m_fontChatEdit.m_hObject, FALSE);
		CRect rcEdit;
		m_wndInput.GetWindowRect(&rcEdit);
		ScreenToClient(&rcEdit);
		rcEdit.top -= 2;
		rcEdit.bottom += 2;
		m_wndInput.MoveWindow(&rcEdit, FALSE);
	}

	CRect rcSpl;
	m_wndNicks.GetWindowRect(rcSpl);
	ScreenToClient(rcSpl);
	rcSpl.left = rcSpl.right + SPLITTER_HORZ_MARGIN;
	rcSpl.right = rcSpl.left + SPLITTER_HORZ_WIDTH;
	m_wndSplitterHorz.Create(WS_CHILD | WS_VISIBLE, rcSpl, this, IDC_SPLITTER_IRC);

	AddAnchor(IDC_BN_IRCCONNECT, BOTTOM_LEFT);
	AddAnchor(IDC_CLOSECHAT, BOTTOM_LEFT);
	AddAnchor(IDC_CHATSEND, BOTTOM_RIGHT);
	AddAnchor(m_wndFormat, BOTTOM_LEFT);
	AddAnchor(m_wndInput, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(m_wndNicks, TOP_LEFT, BOTTOM_LEFT);
	AddAnchor(m_wndChanList, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(m_wndChanSel, TOP_LEFT, TOP_RIGHT);
	AddAnchor(m_wndSplitterHorz, TOP_LEFT, BOTTOM_LEFT);

	// Vista: Remove the TBSTYLE_TRANSPARENT to avoid flickering (can be done only after the toolbar was initially created with TBSTYLE_TRANSPARENT !?)
	m_wndFormat.ModifyStyle((theApp.m_ullComCtrlVer >= MAKEDLLVERULL(6, 16, 0, 0)) ? TBSTYLE_TRANSPARENT : 0, TBSTYLE_TOOLTIPS);
	m_wndFormat.SetExtendedStyle(m_wndFormat.GetExtendedStyle() | TBSTYLE_EX_MIXEDBUTTONS);

	TBBUTTON atb[5] = {0};
	atb[0].iBitmap = 0;
	atb[0].idCommand = IDC_SMILEY;
	atb[0].fsState = TBSTATE_ENABLED;
	atb[0].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
	atb[0].iString = -1;

	atb[1].iBitmap = 1;
	atb[1].idCommand = IDC_BOLD;
	atb[1].fsState = TBSTATE_ENABLED;
	atb[1].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
	atb[1].iString = -1;

	atb[2].iBitmap = 2;
	atb[2].idCommand = IDC_UNDERLINE;
	atb[2].fsState = TBSTATE_ENABLED;
	atb[2].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
	atb[2].iString = -1;

	atb[3].iBitmap = 3;
	atb[3].idCommand = IDC_COLOUR;
	atb[3].fsState = TBSTATE_ENABLED;
	atb[3].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
	atb[3].iString = -1;

	atb[4].iBitmap = 4;
	atb[4].idCommand = IDC_RESET;
	atb[4].fsState = TBSTATE_ENABLED;
	atb[4].fsStyle = BTNS_BUTTON | BTNS_AUTOSIZE;
	atb[4].iString = -1;
	m_wndFormat.AddButtons(_countof(atb), atb);

	CSize size;
	m_wndFormat.GetMaxSize(&size);
	::SetWindowPos(m_wndFormat, NULL, 0, 0, size.cx, size.cy, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

	int iPosStatInit = rcSpl.left;
	int iPosStatNew = thePrefs.GetSplitterbarPositionIRC();
	if (iPosStatNew > SPLITTER_HORZ_RANGE_MAX)
		iPosStatNew = SPLITTER_HORZ_RANGE_MAX;
	else if (iPosStatNew < SPLITTER_HORZ_RANGE_MIN)
		iPosStatNew = SPLITTER_HORZ_RANGE_MIN;
	rcSpl.left = iPosStatNew;
	rcSpl.right = iPosStatNew + SPLITTER_HORZ_WIDTH;
	if (iPosStatNew != iPosStatInit)
	{
		m_wndSplitterHorz.MoveWindow(rcSpl);
		DoResize(iPosStatNew - iPosStatInit);
	}

	Localize();
	m_wndChanList.Init();
	m_wndNicks.Init();
	m_wndNicks.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
	m_wndChanSel.Init();
	OnChatTextChange();

	return true;
}

void CIrcWnd::DoResize(int iDelta)
{
	CSplitterControl::ChangeWidth(&m_wndNicks, iDelta);
	m_wndNicks.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
	CSplitterControl::ChangeWidth(&m_wndChanList, -iDelta, CW_RIGHTALIGN);
	CSplitterControl::ChangeWidth(&m_wndChanSel, -iDelta, CW_RIGHTALIGN);

	if (m_wndChanSel.m_pCurrentChannel && m_wndChanSel.m_pCurrentChannel->m_wndLog.m_hWnd)
	{
		CRect rcChannelPane;
		m_wndChanList.GetWindowRect(&rcChannelPane);
		ScreenToClient(&rcChannelPane);
		Channel *pChannel = m_wndChanSel.m_pCurrentChannel;
		if (pChannel->m_wndTitle.m_hWnd)
			CSplitterControl::ChangeWidth(&pChannel->m_wndTitle, -iDelta, CW_RIGHTALIGN);
		if (pChannel->m_wndSplitter.m_hWnd)
			CSplitterControl::ChangeWidth(&pChannel->m_wndSplitter, -iDelta, CW_RIGHTALIGN);
		if (pChannel->m_wndLog.m_hWnd)
			CSplitterControl::ChangeWidth(&pChannel->m_wndLog, -iDelta, CW_RIGHTALIGN);
	}

	CRect rcSpl;
	m_wndSplitterHorz.GetWindowRect(rcSpl);
	ScreenToClient(rcSpl);
	thePrefs.SetSplitterbarPositionIRC(rcSpl.left);

	RemoveAnchor(IDC_BN_IRCCONNECT);
	AddAnchor(IDC_BN_IRCCONNECT, BOTTOM_LEFT);
	RemoveAnchor(IDC_CLOSECHAT);
	AddAnchor(IDC_CLOSECHAT, BOTTOM_LEFT);
	RemoveAnchor(m_wndFormat);
	AddAnchor(m_wndFormat, BOTTOM_LEFT);
	RemoveAnchor(m_wndInput);
	AddAnchor(m_wndInput, BOTTOM_LEFT, BOTTOM_RIGHT);
	RemoveAnchor(m_wndNicks);
	AddAnchor(m_wndNicks, TOP_LEFT, BOTTOM_LEFT);
	RemoveAnchor(m_wndChanList);
	AddAnchor(m_wndChanList, TOP_LEFT, BOTTOM_RIGHT);
	RemoveAnchor(m_wndChanSel);
	AddAnchor(m_wndChanSel, TOP_LEFT, TOP_RIGHT);
	RemoveAnchor(m_wndSplitterHorz);
	AddAnchor(m_wndSplitterHorz, TOP_LEFT, BOTTOM_LEFT);

	CRect rcWnd;
	GetWindowRect(rcWnd);
	ScreenToClient(rcWnd);
	m_wndSplitterHorz.SetRange(rcWnd.left + SPLITTER_HORZ_RANGE_MIN + SPLITTER_HORZ_WIDTH/2, 
							   rcWnd.left + SPLITTER_HORZ_RANGE_MAX - SPLITTER_HORZ_WIDTH/2);

	Invalidate();
	UpdateWindow();
}

LRESULT CIrcWnd::DefWindowProc(UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	switch (uMessage)
	{
		case WM_PAINT:
			if (m_wndSplitterHorz)
			{
				CRect rcWnd;
				GetWindowRect(rcWnd);
				if (rcWnd.Width() > 0)
				{
					CRect rcSpl;
					m_wndNicks.GetWindowRect(rcSpl);
					ScreenToClient(rcSpl);
					rcSpl.left = rcSpl.right + SPLITTER_HORZ_MARGIN;
					rcSpl.right = rcSpl.left + SPLITTER_HORZ_WIDTH;
					m_wndSplitterHorz.MoveWindow(rcSpl, TRUE);
				}
			}
			break;

		case WM_NOTIFY:
			if (wParam == IDC_SPLITTER_IRC)
			{
				SPC_NMHDR* pHdr = (SPC_NMHDR*)lParam;
				DoResize(pHdr->delta);
			}
			else if (wParam == IDC_SPLITTER_IRC_CHANNEL)
			{
				SPC_NMHDR* pHdr = (SPC_NMHDR*)lParam;
				if (m_wndChanSel.m_pCurrentChannel)
				{
					CSplitterControl::ChangeHeight(&m_wndChanSel.m_pCurrentChannel->m_wndTitle, pHdr->delta);
					m_wndChanSel.m_pCurrentChannel->m_wndTitle.ScrollToFirstLine();
					CSplitterControl::ChangeHeight(&m_wndChanSel.m_pCurrentChannel->m_wndLog, -pHdr->delta, CW_BOTTOMALIGN);
				}
			}
			break;

		case WM_SIZE:
			if (m_wndSplitterHorz)
			{
				CRect rcWnd;
				GetWindowRect(rcWnd);
				ScreenToClient(rcWnd);
				m_wndSplitterHorz.SetRange(rcWnd.left + SPLITTER_HORZ_RANGE_MIN + SPLITTER_HORZ_WIDTH/2, 
										   rcWnd.left + SPLITTER_HORZ_RANGE_MAX - SPLITTER_HORZ_WIDTH/2);
			}
			break;
	}
	return CResizableDialog::DefWindowProc(uMessage, wParam, lParam);
}

void CIrcWnd::UpdateFonts(CFont* pFont)
{
	TCITEM tci;
	tci.mask = TCIF_PARAM;
	int iIndex = 0;
	while (m_wndChanSel.GetItem(iIndex++, &tci))
	{
		Channel* pChannel = (Channel*)tci.lParam;
		if (pChannel->m_wndTitle.m_hWnd != NULL) {
			pChannel->m_wndTitle.SetFont(pFont);
			pChannel->m_wndTitle.ScrollToFirstLine();
		}
		if (pChannel->m_wndLog.m_hWnd != NULL)
			pChannel->m_wndLog.SetFont(pFont);
	}
}

void CIrcWnd::UpdateChannelChildWindowsSize()
{
	if (m_wndChanSel.m_pCurrentChannel)
	{
		Channel *pChannel = m_wndChanSel.m_pCurrentChannel;
		CRect rcChannelPane;
		m_wndChanList.GetWindowRect(&rcChannelPane);
		ScreenToClient(&rcChannelPane);

		if (pChannel->m_wndTitle.m_hWnd)
		{
			CRect rcTitle;
			pChannel->m_wndTitle.GetWindowRect(rcTitle);
			ScreenToClient(rcTitle);
			pChannel->m_wndTitle.SetWindowPos(NULL, rcChannelPane.left, rcTitle.top, rcChannelPane.Width(), rcTitle.Height(), SWP_NOZORDER);
			pChannel->m_wndTitle.ScrollToFirstLine();

			if (pChannel->m_wndSplitter.m_hWnd)
			{
				CRect rcSplitter;
				pChannel->m_wndSplitter.GetWindowRect(rcSplitter);
				ScreenToClient(rcSplitter);
				pChannel->m_wndSplitter.SetWindowPos(NULL, rcChannelPane.left, rcSplitter.top, rcChannelPane.Width(), rcSplitter.Height(), SWP_NOZORDER);
			}
		}

		if (pChannel->m_wndLog.m_hWnd)
		{
			if (pChannel->m_wndTitle.m_hWnd)
			{
				CRect rcLog;
				pChannel->m_wndLog.GetWindowRect(rcLog);
				ScreenToClient(rcLog);
				rcLog.bottom = rcChannelPane.bottom;
				pChannel->m_wndLog.SetWindowPos(NULL, rcChannelPane.left, rcLog.top, rcChannelPane.Width(), rcLog.Height(), SWP_NOZORDER);
			}
			else
				pChannel->m_wndLog.SetWindowPos(NULL, rcChannelPane.left, rcChannelPane.top, rcChannelPane.Width(), rcChannelPane.Height(), SWP_NOZORDER);
		}
	}
}

void CIrcWnd::OnSize(UINT uType, int iCx, int iCy)
{
	CResizableDialog::OnSize(uType, iCx, iCy);
	UpdateChannelChildWindowsSize();
}

int CIrcWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	return CResizableDialog::OnCreate(lpCreateStruct);
}

void CIrcWnd::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_NICKLIST, m_wndNicks);
	DDX_Control(pDX, IDC_INPUTWINDOW, m_wndInput);
	DDX_Control(pDX, IDC_SERVERCHANNELLIST, m_wndChanList);
	DDX_Control(pDX, IDC_TAB2, m_wndChanSel);
	DDX_Control(pDX, IDC_TEXT_FORMAT, m_wndFormat);
}

BOOL CIrcWnd::OnCommand(WPARAM wParam, LPARAM)
{
	switch (wParam)
	{
		case IDC_BN_IRCCONNECT:
			OnBnClickedIrcConnect();
			return TRUE;

		case IDC_CLOSECHAT:
			OnBnClickedCloseChannel();
			return TRUE;

		case IDC_CHATSEND:
			OnBnClickedIrcSend();
			return TRUE;

		case IDC_BOLD:
			OnBnClickedBold();
			return TRUE;

		case IDC_COLOUR:
			OnBnClickedColour();
			return TRUE;

		case IDC_UNDERLINE:
			OnBnClickedUnderline();
			return TRUE;

		case IDC_RESET:
			OnBnClickedReset();
			return TRUE;

		case IDC_SMILEY:
			OnBnClickedSmiley();
			return TRUE;
	}
	return TRUE;
}

BOOL CIrcWnd::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		// Don't handle Ctrl+Tab in this window. It will be handled by main window.
		if (pMsg->wParam == VK_TAB && GetAsyncKeyState(VK_CONTROL) < 0)
			return FALSE;

		if (pMsg->hwnd == m_wndInput)
		{
			if (pMsg->wParam == VK_RETURN)
			{
				//If we press the enter key, treat is as if we pressed the send button.
				OnBnClickedIrcSend();
				return TRUE;
			}

			if (pMsg->wParam == VK_UP || pMsg->wParam == VK_DOWN)
			{
				//If we press page up/down scroll..
				m_wndChanSel.ScrollHistory(pMsg->wParam == VK_DOWN);
				return TRUE;
			}

			if (pMsg->wParam == VK_TAB)
			{
				AutoComplete();
				return TRUE;
			}
		}
	}
	OnChatTextChange();
	return CResizableDialog::PreTranslateMessage(pMsg);
}

void CIrcWnd::AutoComplete()
{
	CString sSend;
	CString sName;
	m_wndInput.GetWindowText(sSend);
	if (sSend.ReverseFind(_T(' ')) == -1)
	{
		if (sSend.IsEmpty())
			return;
		sName = sSend;
		sSend.Empty();
	}
	else
	{
		sName = sSend.Mid(sSend.ReverseFind(_T(' ')) + 1);
		sSend = sSend.Mid(0, sSend.ReverseFind(_T(' ')) + 1);
	}

	POSITION pos = m_wndChanSel.m_pCurrentChannel->m_lstNicks.GetHeadPosition();
	while (pos)
	{
		Nick* pNick = m_wndChanSel.m_pCurrentChannel->m_lstNicks.GetNext(pos);
		if (pNick->m_sNick.Left(sName.GetLength()).CompareNoCase(sName) == 0)
		{
			m_wndInput.SetWindowText(sSend + pNick->m_sNick);
			m_wndInput.SetFocus();
			m_wndInput.SendMessage(WM_KEYDOWN, VK_END);
			break;
		}
	}
}

void CIrcWnd::OnBnClickedIrcConnect()
{
	if (!m_bConnected)
	{
		CString sInput = thePrefs.GetIRCNick();
		sInput.Trim();
		sInput = sInput.SpanExcluding(_T(" !@#$%^&*():;<>,.?{}~`+=-"));
		sInput = sInput.Left(25);
		while (sInput.IsEmpty()
			   || sInput.CompareNoCase(_T("emule")) == 0
			   || stristr(sInput, _T("emuleirc")) != NULL)
		{
			InputBox inputBox;
			inputBox.SetLabels(GetResString(IDS_IRC_NEWNICK), GetResString(IDS_IRC_NEWNICKDESC), sInput);
			if (inputBox.DoModal() == IDOK)
			{
				sInput = inputBox.GetInput();
				sInput.Trim();
				sInput = sInput.SpanExcluding(_T(" !@#$%^&*():;<>,.?{}~`+=-"));
				sInput = sInput.Left(25);
			}
			else
				return;
		}
		thePrefs.SetIRCNick(sInput);
		//if not connected, connect..
		m_pIrcMain->Connect();
	}
	else
	{
		//If connected, disconnect..
		m_pIrcMain->Disconnect();
		m_wndChanList.ResetServerChannelList();
	}
}

void CIrcWnd::OnBnClickedCloseChannel(int iItem)
{
	//Remove a channel..
	TCITEM item;
	item.mask = TCIF_PARAM;
	if (iItem == -1)
	{
		//If no item was send, get our current channel..
		iItem = m_wndChanSel.GetCurSel();
	}

	if (iItem == -1) {
		//We have no channel, abort.
		return;
	}

	if (!m_wndChanSel.GetItem(iItem, &item)) {
		//We had no valid item here.. Something isn't right..
		//TODO: this should never happen, so maybe we should remove this tab?
		return;
	}

	Channel* pPartChannel = (Channel*)item.lParam;
	if (pPartChannel->m_eType == Channel::ctNormal && m_bConnected)
	{
		//If this was a channel and we were connected, do not just delete the channel!!
		//Send a part command and the server must respond with a successful part which will remove the channel!
		m_pIrcMain->SendString(_T("PART ") + pPartChannel->m_sName);
		return;
	}
	else if (pPartChannel->m_eType == Channel::ctNormal || pPartChannel->m_eType == Channel::ctPrivate)
	{
		//If this is a private room, we just remove it as the server doesn't track this.
		//If this was a channel, but we are disconnected, remove the channel..
		m_wndChanSel.RemoveChannel(pPartChannel->m_sName);
		return;
	}
}

void CIrcWnd::AddStatus(CString sLine, bool bShowActivity, UINT uStatusCode)
{
	Channel* pStatusChannel = m_wndChanSel.m_lstChannels.GetHead();
	if (!pStatusChannel)
		return;
	sLine += _T("\r\n");

	// This allows for us to add blank lines to the status..
	if (sLine == _T("\r\n"))
		pStatusChannel->m_wndLog.AppendText(sLine);
	else if (sLine.GetLength() >= 1 && sLine[0] == _T('*'))
	{
		if (thePrefs.GetIRCAddTimeStamp())
			AddColorLine(s_szTimeStampColorPrefix + CTime::GetCurrentTime().Format(TIME_STAMP_FORMAT), pStatusChannel->m_wndLog);
		AddColorLine(sLine, pStatusChannel->m_wndLog, STATUS_MSG_COLOR);
	}
	else if (uStatusCode >= 400)
	{
		if (thePrefs.GetIRCAddTimeStamp())
			AddColorLine(s_szTimeStampColorPrefix + CTime::GetCurrentTime().Format(TIME_STAMP_FORMAT), pStatusChannel->m_wndLog);
		if (!(sLine.GetLength() >= 1 && sLine[0] == _T('-')))
			sLine = _T("-Error- ") + sLine;
		AddColorLine(sLine, pStatusChannel->m_wndLog, INFO_MSG_COLOR);
	}
	else
	{
		if (thePrefs.GetIRCAddTimeStamp())
			AddColorLine(s_szTimeStampColorPrefix + CTime::GetCurrentTime().Format(TIME_STAMP_FORMAT), pStatusChannel->m_wndLog);
		AddColorLine(sLine, pStatusChannel->m_wndLog);
	}
	if (m_wndChanSel.m_pCurrentChannel == pStatusChannel)
		return;
	if (bShowActivity)
		m_wndChanSel.SetActivity(pStatusChannel, true);
	if (uStatusCode >= 400)
	{
		if (!(sLine.GetLength() >= 1 && sLine[0] == _T('-')))
			sLine = _T("-Error- ") + sLine;
		AddInfoMessage(m_wndChanSel.m_pCurrentChannel, sLine);
	}
}

void CIrcWnd::AddStatusF(CString sLine, ...)
{
	va_list argptr;
	va_start(argptr, sLine);
	CString sTemp;
	sTemp.FormatV(sLine, argptr);
	va_end(argptr);
	AddStatus(sTemp);
}

void CIrcWnd::AddInfoMessage(Channel *pChannel, CString sLine)
{
	if (sLine.GetLength() >= 1 && sLine[0] == _T('*'))
	{
		if (thePrefs.GetIRCAddTimeStamp())
			AddColorLine(s_szTimeStampColorPrefix + CTime::GetCurrentTime().Format(TIME_STAMP_FORMAT), pChannel->m_wndLog);
		AddColorLine(sLine, pChannel->m_wndLog, STATUS_MSG_COLOR);
	}
	else if (sLine.GetLength() >= 1 && sLine[0] == _T('-') && sLine.Find(_T('-'), 1) != -1)
	{
		if (thePrefs.GetIRCAddTimeStamp())
			AddColorLine(s_szTimeStampColorPrefix + CTime::GetCurrentTime().Format(TIME_STAMP_FORMAT), pChannel->m_wndLog);
		AddColorLine(sLine, pChannel->m_wndLog, INFO_MSG_COLOR);
	}
	else
	{
		if (thePrefs.GetIRCAddTimeStamp())
			AddColorLine(s_szTimeStampColorPrefix + CTime::GetCurrentTime().Format(TIME_STAMP_FORMAT), pChannel->m_wndLog);
		AddColorLine(sLine, pChannel->m_wndLog);
	}
	if (m_wndChanSel.m_pCurrentChannel == pChannel)
		return;
	m_wndChanSel.SetActivity(pChannel, true);
}

void CIrcWnd::AddInfoMessage(const CString& sChannelName, CString sLine, bool bShowChannel)
{
	if (sChannelName.IsEmpty())
		return;
	sLine += _T("\r\n");

	Channel* pUpdateChannel = m_wndChanSel.FindChannelByName(sChannelName);
	if (!pUpdateChannel)
	{
		if (sChannelName.GetLength() >= 1 && sChannelName[0] == _T('#'))
			pUpdateChannel = m_wndChanSel.NewChannel(sChannelName, Channel::ctNormal);
		else if (sChannelName == thePrefs.GetIRCNick())
		{
			// A 'Notice' message for myself - display in current channel window
			pUpdateChannel = m_wndChanSel.m_pCurrentChannel;
			if (pUpdateChannel && pUpdateChannel->m_eType == Channel::ctChannelList)
				pUpdateChannel = NULL;	// channels list window -> open a new channel window
		}
		if (!pUpdateChannel)
			pUpdateChannel = m_wndChanSel.NewChannel(sChannelName, Channel::ctPrivate);
	}
	if (bShowChannel)
		m_wndChanSel.SelectChannel(pUpdateChannel);
	AddInfoMessage(pUpdateChannel, sLine);
}

void CIrcWnd::AddInfoMessageF(const CString& sChannelName, CString sLine, ...)
{
	if (sChannelName.IsEmpty())
		return;
	va_list argptr;
	va_start(argptr, sLine);
	CString sTemp;
	sTemp.FormatV(sLine, argptr);
	va_end(argptr);
	AddInfoMessage(sChannelName, sTemp);
}

void CIrcWnd::AddMessage(const CString& sChannelName, CString sTargetName, CString sLine)
{
	if (sChannelName.IsEmpty() || sTargetName.IsEmpty())
		return;
	sLine += _T("\r\n");

	Channel* pUpdateChannel = m_wndChanSel.FindChannelByName(sChannelName);
	if (!pUpdateChannel)
	{
		if (sChannelName.GetLength() >= 1 && sChannelName[0] == _T('#'))
			pUpdateChannel = m_wndChanSel.NewChannel(sChannelName, Channel::ctNormal);
		else
			pUpdateChannel = m_wndChanSel.NewChannel(sChannelName, Channel::ctPrivate);
	}
	COLORREF color;
	if (m_pIrcMain->GetNick() == sTargetName)
		color = SENT_TARGET_MSG_COLOR;	// color for own nick of a sent message
	else
		color = RECV_SOURCE_MSG_COLOR;	// color for nick of received message
	sTargetName = _T("<") + sTargetName + _T(">");
	if (thePrefs.GetIRCAddTimeStamp())
		AddColorLine(s_szTimeStampColorPrefix + CTime::GetCurrentTime().Format(TIME_STAMP_FORMAT), pUpdateChannel->m_wndLog);
	pUpdateChannel->m_wndLog.AppendKeyWord(sTargetName, color);
	AddColorLine(_T(" ") + sLine, pUpdateChannel->m_wndLog);
	if (m_wndChanSel.m_pCurrentChannel == pUpdateChannel)
		return;
	m_wndChanSel.SetActivity(pUpdateChannel, true);
}

void CIrcWnd::AddMessageF(const CString& sChannelName, CString sTargetName, CString sLine, ...)
{
	if (sChannelName.IsEmpty() || sTargetName.IsEmpty())
		return;
	va_list argptr;
	va_start(argptr, sLine);
	CString sTemp;
	sTemp.FormatV(sLine, argptr);
	va_end(argptr);
	AddMessage(sChannelName, sTargetName, sTemp);
}

//To add colour functionality we need to isolate hyperlinks and send them to AppendColoredText! :)
static const struct
{
	LPCTSTR pszScheme;
	int iLen;
}
s_apszSchemes[] =
{
    { _T("ed2k://"),  7 },
    { _T("http://"),  7 },
    { _T("https://"), 8 },
    { _T("ftp://"),   6 },
    { _T("www."),     4 },
    { _T("ftp."),     4 },
    { _T("mailto:"),  7 }
};

static const COLORREF s_aColors[16] =
{
    //RGB(255,255,255), //  0: white
    //RGB(  0,  0,  0), //  1: black
    //RGB(  0,  0,128), //  2: dark blue
    //RGB(  0,128,  0), //  3: dark green
    //RGB(255,  0,  0), //  4: red
    //RGB(128,  0,  0), //  5: dark red
    //RGB(128,  0,128), //  6: purple
    //RGB(255,128,  0), //  7: orange
    //RGB(255,255,  0), //  8: yellow
    //RGB(  0,255,  0), //  9: green
    //RGB(  0,128,128), // 10: dark cyan
    //RGB(  0,255,255), // 11: cyan
    //RGB(  0,  0,255), // 12: blue
    //RGB(255,  0,255), // 13: pink
    //RGB(128,128,128), // 14: dark grey
    //RGB(192,192,192)  // 15: light grey
    RGB(255,255,255), //  0: white
    RGB(  0,  0,  0), //  1: black
    RGB(  0,  0,192), //  2: dark blue
    RGB(  0,192,  0), //  3: dark green
    RGB(255,  0,  0), //  4: red
    RGB(192,  0,  0), //  5: dark red
    RGB(192,  0,192), //  6: purple
    RGB(255,128,  0), //  7: orange
    RGB(255,255,  0), //  8: yellow
    RGB(  0,255,  0), //  9: green
    RGB(  0,128,128), // 10: dark cyan
    RGB(  0,255,255), // 11: cyan
    RGB(  0,  0,255), // 12: blue
    RGB(255,  0,255), // 13: pink
    RGB(128,128,128), // 14: dark grey
    RGB(192,192,192)  // 15: light grey
};

bool IsValidURLTerminationChar(TCHAR ch)
{
	// truncate some special chars from end (and only from end), those
	// are the same chars which are supported (not supported actually)
	// by rich edit control auto url detection.
	return _tcschr(_T("^!\"&()=?´`{}[]@+*~#,.-;:_"), ch) == NULL;
}

void CIrcWnd::AddColorLine(const CString& line, CHTRichEditCtrl &wnd, COLORREF crForeground)
{
	DWORD dwMask = 0;
	int index = 0;
	int linkfoundat = 0;//This variable is to save needless costly string manipulation
	COLORREF foregroundColour = crForeground;
	COLORREF cr = foregroundColour;//set start foreground colour
	COLORREF backgroundColour = CLR_DEFAULT;
	COLORREF bgcr = backgroundColour;//set start background colour COMMENTED left for possible future use
	CString text;
	while (line.GetLength() > index)
	{
		TCHAR aChar = line[index];

		// find any hyperlinks
		if (index == linkfoundat) //only run the link finding code once it a line with no links
		{
			for (int iScheme = 0; iScheme < _countof(s_apszSchemes); /**/)
			{
				CString strLeft = line.Right(line.GetLength() - index);//make a string of what we have left
				int foundat = strLeft.Find(s_apszSchemes[iScheme].pszScheme);//get position of link -1 == not found
				if (foundat == 0) //link starts at this character
				{
					if (!text.IsEmpty()) {
						wnd.AppendColoredText(text, cr, bgcr, dwMask);
						text.Empty();
					}

					// search next space or EOL or control code
					int iLen = strLeft.FindOneOf(_T(" \t\r\n\x02\x03\x0F\x16\x1F"));
					if (iLen == -1)
					{
						// truncate some special chars from end of URL (and only from end)
						iLen = strLeft.GetLength();
						while (iLen > 0) {
							if (IsValidURLTerminationChar(strLeft[iLen - 1]))
								break;
							iLen--;
						}
						wnd.AddLine(strLeft.Left(iLen), iLen, true);
						index += iLen;
						if (index >= line.GetLength())
							return;

						aChar = line[index]; // get a new char
						break;
					}
					else
					{
						// truncate some special chars from end of URL (and only from end)
						while (iLen > 0) {
							if (IsValidURLTerminationChar(strLeft[iLen - 1]))
								break;
							iLen--;
						}
						wnd.AddLine(strLeft.Left(iLen), iLen, true);
						index += iLen;
						if (index >= line.GetLength())
							return;

						iScheme = 0; // search from the new position
						foundat = -1; // do not record this processed location as a future target location
						linkfoundat = index; // reset previous finds as iScheme=0 we re-search
						aChar = line[index]; // get a new char
					}
				}
				else
				{
					iScheme++;//only increment if not found at this position so if we find http at this position we check for further http occurances
					//foundat A Valid Position && (no valid position recorded || a farther position previously recorded)
					if (foundat != -1 && (linkfoundat == index || (index + foundat) < linkfoundat))
						linkfoundat = index + foundat;//set the next closest link to process
				}
			}
		}

		switch ((_TUCHAR)aChar)
		{
			case 0x02: // Bold
				if (!text.IsEmpty()) {
					wnd.AppendColoredText(text, cr, bgcr, dwMask);
					text.Empty();
				}
				index++;
				if (dwMask & CFM_BOLD)
					dwMask ^= CFM_BOLD;
				else
					dwMask |= CFM_BOLD;
				break;

			case 0x03: // foreground & background colour
				if (!text.IsEmpty()) {
					wnd.AppendColoredText(text, cr, bgcr, dwMask);
					text.Empty();
				}
				index++;
				if (line[index] >= _T('0') && line[index] <= _T('9'))
				{
					int iColour = (int)(line[index] - _T('0'));
					if (iColour == 1 && line[index + 1] >= _T('0') && line[index + 1] <= _T('5')) //is there a second digit
					{
						// make a two digit number
						index++;
						iColour = 10 + (int)(line[index] - _T('0'));
					}
					else if (iColour == 0 && line[index + 1] >= _T('0') && line[index + 1] <= _T('9')) //if first digit is zero and there is a second digit eg: 3 in 03
					{
						// make a two digit number
						index++;
						iColour = (int)(line[index] - _T('0'));
					}

					if (iColour >= 0 && iColour < 16)
					{
						// If the first colour is not valid, don't look for a second background colour!
						cr = s_aColors[iColour];//if the number is a valid colour index set new foreground colour
						index++;
						if (line[index] == _T(',') && line[index + 1] >= _T('0') && line[index + 1] <= _T('9'))//is there a background colour
						{
							index++;
							int iColour = (int)(line[index] - _T('0'));
							if (iColour == 1 && line[index + 1] >= _T('0') && line[index + 1] <= _T('5')) // is there a second digit
							{
								// make a two digit number
								index++;
								iColour = 10 + (int)(line[index] - _T('0'));
							}
							else if (iColour == 0 && line[index + 1] >= _T('0') && line[index + 1] <= _T('9')) // if first digit is zero and there is a second digit eg: 3 in 03
							{
								// make a two digit number
								index++;
								iColour = (int)(line[index] - _T('0'));
							}
							index++;
							if (iColour >= 0 && iColour < 16)
								bgcr = s_aColors[iColour];//if the number is a valid colour index set new foreground colour
						}
					}
				}
				else
				{
					// reset
					cr = foregroundColour;
					bgcr = backgroundColour;
				}
				break;

			case 0x0F: // attributes reset
				if (!text.IsEmpty()) {
					wnd.AppendColoredText(text, cr, bgcr, dwMask);
					text.Empty();
				}
				index++;
				dwMask = 0;
				cr = foregroundColour;
				bgcr = backgroundColour;
				break;

			case 0x16: // Reverse (as per Mirc) toggle
				// NOTE:This does not reset the bold/underline,(dwMask = 0), attributes but does reset colours 'As per mIRC 6.16!!'
				if (!text.IsEmpty()) {
					wnd.AppendColoredText(text, cr, bgcr, dwMask);
					text.Empty();
				}
				index++;
				if (cr != backgroundColour || bgcr != foregroundColour) {
					// set inverse
					cr = backgroundColour;
					bgcr = foregroundColour;
				}
				else {
					// reset fg/bk colours
					cr = foregroundColour;
					bgcr = backgroundColour;
				}
				break;

			case 0x1f: // Underlined toggle
				if (!text.IsEmpty()) {
					wnd.AppendColoredText(text, cr, bgcr, dwMask);
					text.Empty();
				}
				index++;
				if (dwMask & CFM_UNDERLINE)
					dwMask ^= CFM_UNDERLINE;
				else
					dwMask |= CFM_UNDERLINE;
				break;

			default:
				text += aChar;
				index++;
		}
	}
	if (!text.IsEmpty())
		wnd.AppendColoredText(text, cr, bgcr, dwMask);
}

void CIrcWnd::SetConnectStatus(bool bFlag)
{
	if (bFlag)
	{
		GetDlgItem(IDC_BN_IRCCONNECT)->SetWindowText(GetResString(IDS_IRC_DISCONNECT));
		AddStatus(GetResString(IDS_CONNECTED));
		m_bConnected = true;
	}
	else
	{
		GetDlgItem(IDC_BN_IRCCONNECT)->SetWindowText(GetResString(IDS_IRC_CONNECT));
		AddStatus(GetResString(IDS_DISCONNECTED));
		m_bConnected = false;
		m_bLoggedIn = false;
		while (m_wndChanSel.m_lstChannels.GetCount() > 2)
		{
			Channel* pToDel = m_wndChanSel.m_lstChannels.GetTail();
			m_wndChanSel.RemoveChannel(pToDel->m_sName);
		}
	}
}

void CIrcWnd::NoticeMessage(const CString& sSource, const CString& sTarget, const CString& sMessage)
{
	if (sTarget == thePrefs.GetIRCNick()) {
		AddInfoMessageF(sTarget, _T("-%s:%s- %s"), sSource, sTarget, sMessage);
		return;
	}

	bool bFlag = false;
	if (m_wndChanSel.FindChannelByName(sTarget))
	{
		AddInfoMessageF(sTarget, _T("-%s:%s- %s"), sSource, sTarget, sMessage);
		bFlag = true;
	}
	else
	{
		POSITION pos = m_wndChanSel.m_lstChannels.GetHeadPosition();
		while (pos)
		{
			const Channel *pChannel = m_wndChanSel.m_lstChannels.GetNext(pos);
			if (pChannel)
			{
				const Nick* pNick = m_wndNicks.FindNickByName(pChannel->m_sName, sSource);
				if (pNick)
				{
					AddInfoMessageF(pChannel->m_sName, _T("-%s:%s- %s"), sSource, sTarget, sMessage);
					bFlag = true;
				}
			}
		}
	}
	if (!bFlag)
	{
		AddStatusF(_T("-%s:%s- %s"), sSource, sTarget, sMessage);
	}
}

CString CIrcWnd::StripMessageOfFontCodes(CString sTemp)
{
	sTemp = StripMessageOfColorCodes(sTemp);
	sTemp.Remove(_T('\002')); // 0x02 - BOLD
	//sTemp.Remove(_T('\003')); // 0x03 - COLOUR
	sTemp.Remove(_T('\017')); // 0x0f - RESET
	sTemp.Remove(_T('\026')); // 0x16 - REVERSE/INVERSE was once italic?
	sTemp.Remove(_T('\037')); // 0x1f - UNDERLINE
	return sTemp;
}

CString CIrcWnd::StripMessageOfColorCodes(CString sTemp)
{
	if (!sTemp.IsEmpty())
	{
		CString sTemp1, sTemp2;
		int iTest = sTemp.Find(_T('\003'));
		if (iTest != -1)
		{
			int iTestLength = sTemp.GetLength() - iTest;
			if (iTestLength < 2)
				return sTemp;
			sTemp1 = sTemp.Left(iTest);
			sTemp2 = sTemp.Mid(iTest + 2);
			if (iTestLength < 4)
				return sTemp1 + sTemp2;
			if (sTemp2[0] == _T(',') && sTemp2.GetLength() > 2)
			{
				sTemp2 = sTemp2.Mid(2);
				for (TCHAR iIndex = _T('0'); iIndex <= _T('9'); iIndex++)
				{
					if (sTemp2[0] == iIndex)
						sTemp2 = sTemp2.Mid(1);
				}
			}
			else
			{
				for (TCHAR iIndex = _T('0'); iIndex <= _T('9'); iIndex++)
				{
					if (sTemp2[0] == iIndex)
					{
						sTemp2 = sTemp2.Mid(1);
						if (sTemp2[0] == _T(',') && sTemp2.GetLength() > 2)
						{
							sTemp2 = sTemp2.Mid(2);
							for (TCHAR iIndex = _T('0'); iIndex <= _T('9'); iIndex++)
							{
								if (sTemp2[0] == iIndex)
									sTemp2 = sTemp2.Mid(1);
							}
						}
					}
				}
			}
			sTemp = sTemp1 + sTemp2;
			sTemp = StripMessageOfColorCodes(sTemp);
		}
	}
	return sTemp;
}

void CIrcWnd::SetTitle(const CString& sChannel, const CString& sTitle)
{
	Channel* pChannel = m_wndChanSel.FindChannelByName(sChannel);
	if (!pChannel)
		return;
	pChannel->m_sTitle = sTitle;
	if (pChannel == m_wndChanSel.m_pCurrentChannel)
	{
		pChannel->m_wndTitle.SetWindowText(_T(""));
		pChannel->m_wndTitle.SetEventMask(pChannel->m_wndTitle.GetEventMask() | ENM_REQUESTRESIZE);
		AddColorLine(sTitle, pChannel->m_wndTitle);
		pChannel->m_wndTitle.SetEventMask(pChannel->m_wndTitle.GetEventMask() & ~ENM_REQUESTRESIZE);
		pChannel->m_wndTitle.ScrollToFirstLine();

		CRect rcTitle;
		pChannel->m_wndTitle.GetWindowRect(rcTitle);
		ScreenToClient(rcTitle);

		pChannel->m_wndSplitter.SetWindowPos(NULL, rcTitle.left, rcTitle.bottom, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		
		CRect rcSplitter;
		pChannel->m_wndSplitter.GetWindowRect(rcSplitter);
		ScreenToClient(rcSplitter);

		CRect rcLog;
		pChannel->m_wndLog.GetWindowRect(rcLog);
		ScreenToClient(rcLog);
		rcLog.top = rcSplitter.bottom;
		pChannel->m_wndLog.SetWindowPos(NULL, rcLog.left, rcLog.top, rcLog.Width(), rcLog.Height(), SWP_NOZORDER);
	}
}

void CIrcWnd::OnBnClickedIrcSend()
{
	CString sSend;
	m_wndInput.GetWindowText(sSend);
	m_wndInput.SetWindowText(_T(""));
	m_wndInput.SetFocus();
	m_wndChanSel.ChatSend(sSend);
}

void CIrcWnd::SendString(const CString& sSend)
{
	if (m_bConnected)
		m_pIrcMain->SendString(sSend);
}

BOOL CIrcWnd::OnHelpInfo(HELPINFO*)
{
	theApp.ShowHelp(eMule_FAQ_GUI_IRC);
	return TRUE;
}

void CIrcWnd::OnChatTextChange()
{
	GetDlgItem(IDC_CHATSEND)->EnableWindow(m_wndInput.GetWindowTextLength() > 0);
}

void CIrcWnd::ParseChangeMode(const CString& sChannel, const CString& sChanger, CString sCommands, const CString& sParams)
{
	CString sCommandsOrig = sCommands;
	CString sParamsOrig = sParams;
	try
	{
		if( sCommands.GetLength() >= 2 )
		{
			CString sDir;
			int iParamIndex = 0;
			while( !sCommands.IsEmpty() )
			{
				if (sCommands[0] == _T('+') || sCommands[0] == _T('-'))
				{
					sDir = sCommands.Left(1);
					sCommands = sCommands.Right(sCommands.GetLength()-1);
				}
				if( !sCommands.IsEmpty() && !sDir.IsEmpty() )
				{
					CString sCommand = sCommands.Left(1);
					sCommands = sCommands.Right(sCommands.GetLength()-1);

					if(m_wndNicks.m_sUserModeSettings.Find(sCommand) != -1 )
					{
						//This is a user mode change and must have a param!
						CString sParam = sParams.Tokenize(_T(" "), iParamIndex);
						m_wndNicks.ChangeNickMode( sChannel, sParam, sDir + sCommand);
					}
					if(m_wndChanSel.m_sChannelModeSettingsTypeA.Find(sCommand) != -1)
					{
						//We do not use these messages yet.. But we can display them for the user to see
						//These modes always have a param and will add or remove a user from some type of list.
						CString sParam = sParams.Tokenize(_T(" "), iParamIndex);
						m_wndChanSel.ChangeChanMode( sChannel, sParam, sDir, sCommand);
					}
					if(m_wndChanSel.m_sChannelModeSettingsTypeB.Find(sCommand) != -1)
					{
						//We do not use these messages yet.. But we can display them for the user to see
						//These modes will always have a param..
						CString sParam = sParams.Tokenize(_T(" "), iParamIndex);
						m_wndChanSel.ChangeChanMode( sChannel, sParam, sDir, sCommand);
					}
					if(m_wndChanSel.m_sChannelModeSettingsTypeC.Find(sCommand) != -1 )
					{
						//We do not use these messages yet.. But we can display them for the user to see
						//These modes will only have a param if your setting it!
						CString sParam;
						if (sDir == _T("+"))
							sParam = sParams.Tokenize(_T(" "), iParamIndex);
						m_wndChanSel.ChangeChanMode( sChannel, sParam, sDir, sCommand);
					}
					if(m_wndChanSel.m_sChannelModeSettingsTypeD.Find(sCommand) != -1 )
					{
						//We do not use these messages yet.. But we can display them for the user to see
						//These modes will never have a param for it!
						CString sParam;
						m_wndChanSel.ChangeChanMode( sChannel, sParam, sDir, sCommand);
					}
				}
			}
			if (!thePrefs.GetIRCIgnoreMiscMessages())
				AddInfoMessageF(sChannel, GetResString(IDS_IRC_SETSMODE), sChanger, sCommandsOrig, sParamsOrig);
		}
	}
	catch(...)
	{
		AddInfoMessage(sChannel, GetResString(IDS_IRC_NOTSUPPORTED));
		ASSERT(0);
	}
}

LRESULT CIrcWnd::OnCloseTab(WPARAM wparam, LPARAM)
{
	OnBnClickedCloseChannel((int)wparam);
	return TRUE;
}

LRESULT CIrcWnd::OnQueryTab(WPARAM wParam, LPARAM)
{
	int iItem = (int)wParam;

	TCITEM item;
	item.mask = TCIF_PARAM;
	m_wndChanSel.GetItem(iItem, &item);
	Channel* pPartChannel = (Channel*)item.lParam;
	if (pPartChannel)
	{
		if (pPartChannel->m_eType == Channel::ctNormal && m_bConnected)
		{
			return 0;
		}
		else if (pPartChannel->m_eType == Channel::ctNormal || pPartChannel->m_eType == Channel::ctPrivate)
		{
			return 0;
		}
	}
	return 1;
}

bool CIrcWnd::GetLoggedIn()
{
	return m_bLoggedIn;
}

void CIrcWnd::SetLoggedIn(bool bFlag)
{
	m_bLoggedIn = bFlag;
}

void CIrcWnd::SetSendFileString(const CString& sInFile)
{
	m_sSendString = sInFile;
}

CString CIrcWnd::GetSendFileString()
{
	return m_sSendString;
}

bool CIrcWnd::IsConnected()
{
	return m_bConnected;
}

void CIrcWnd::OnBnClickedColour()
{
	CRect rDraw;
	int iColor = 0;
	m_wndFormat.GetWindowRect(rDraw);
	new CColourPopup(CPoint(rDraw.left+1, rDraw.bottom-92),	// Point to display popup
	                 iColor,	 				            // Selected colour
	                 this,									// parent
	                 GetResString(IDS_DEFAULT),				// "Default" text area
	                 NULL,                                  // Custom Text
	                 (COLORREF *)s_aColors,                 // Pointer to a COLORREF array
	                 16);                                   // Size of the array

	CWnd *pParent = GetParent();
	if (pParent)
		pParent->SendMessage(UM_CPN_DROPDOWN, (LPARAM)iColor, (WPARAM) GetDlgCtrlID());
}

LONG CIrcWnd::OnSelEndOK(UINT lParam, LONG /*wParam*/)
{
	if (lParam != CLR_DEFAULT)
	{
		int iColour = 0;
		while(iColour<16 && (COLORREF)lParam!=s_aColors[iColour])
			iColour++;
		if(iColour>=0 && iColour<16)//iColour in valid range
		{
			CString sAddAttribute;
			int	iSelStart;
			int	iSelEnd;
			TCHAR iSelEnd3Char;
			TCHAR iSelEnd6Char;

			m_wndInput.GetSel(iSelStart, iSelEnd);//get selection area
			m_wndInput.GetWindowText(sAddAttribute);//get the whole line
			if(iSelEnd > iSelStart)
			{
				sAddAttribute.Insert(iSelEnd, _T('1'));//if a selection add default black colour tag
				sAddAttribute.Insert(iSelEnd, _T('0'));//add first half of colour tag
				sAddAttribute.Insert(iSelEnd, _T('\003'));//if a selection add 'end' tag
			}
			iColour += 0x30;
			//a number greater than 9
			if(iColour>0x39)
			{
				sAddAttribute.Insert(iSelStart, (TCHAR)(iColour-10));//add second half of colour tag 1 for range 10 to 15
				sAddAttribute.Insert(iSelStart,  _T('1'));          //add first half of colour tag
			}
			else
			{
				sAddAttribute.Insert(iSelStart, (TCHAR)(iColour));//add second half of colour tag 1 for range 0 to 9
				sAddAttribute.Insert(iSelStart,  _T('0'));       //add first half of colour tag
			}
			//if this is the start of the line not a selection in the line and a colour has already just been set allow background to be set
			if(iSelEnd>2)
				iSelEnd3Char = sAddAttribute[iSelEnd-3];
			else
				iSelEnd3Char = _T(' ');
			if(iSelEnd>5)
				iSelEnd6Char = sAddAttribute[iSelEnd-6];
			else
				iSelEnd6Char = _T(' ');
			if(iSelEnd == iSelStart &&  iSelEnd3Char == _T('\003') && iSelEnd6Char!= _T('\003'))
				sAddAttribute.Insert(iSelStart, _T(','));//separator for background colour
			else
				sAddAttribute.Insert(iSelStart, _T('\003'));//add start tag
			iSelStart+=3;//add 3 to start position
			iSelEnd+=3;//add 3 to end position
			m_wndInput.SetWindowText(sAddAttribute);//write new line to edit control
			m_wndInput.SetSel(iSelStart, iSelEnd);//update selection info
			m_wndInput.SetFocus();//set focus (from button) to edit control
		}
	}
	else
	{//Default button clicked set black
		CString sAddAttribute;
		int iSelStart;
		int iSelEnd;
		TCHAR iSelEnd3Char;
		TCHAR iSelEnd6Char;

		m_wndInput.GetSel(iSelStart, iSelEnd);//get selection area
		m_wndInput.GetWindowText(sAddAttribute);//get the whole line
		//if this is the start of the line not a selection in the line and a colour has already just been set allow background to be set
		if(iSelEnd>2)
			iSelEnd3Char = sAddAttribute[iSelEnd-3];
		else
			iSelEnd3Char = _T(' ');
		if(iSelEnd>5)
			iSelEnd6Char = sAddAttribute[iSelEnd-6];
		else
			iSelEnd6Char = _T(' ');
		if(iSelEnd == iSelStart &&  iSelEnd3Char == _T('\003') && iSelEnd6Char!= _T('\003'))
		{//Set DEFAULT white background
			sAddAttribute.Insert(iSelStart, _T('0'));//add second half of colour tag 0 for range 0 to 9
			sAddAttribute.Insert(iSelStart, _T('0'));//add first half of colour tag
			sAddAttribute.Insert(iSelStart, _T(','));//separator for background colour
		}
		else
		{//Set DEFAULT black foreground
			sAddAttribute.Insert(iSelStart, _T('1'));//add second half of colour tag 1 for range 0 to 9
			sAddAttribute.Insert(iSelStart, _T('0'));//add first half of colour tag
			sAddAttribute.Insert(iSelStart, _T('\003'));//add start tag
		}
		iSelStart+=3;//add 2 to start position
		iSelEnd+=3;
		m_wndInput.SetWindowText(sAddAttribute);//write new line to edit control
		m_wndInput.SetSel(iSelStart, iSelEnd);//update selection info
		m_wndInput.SetFocus();//set focus (from button) to edit control
	}

	CWnd *pParent = GetParent();
	if(pParent)
	{
		pParent->SendMessage(UM_CPN_CLOSEUP, lParam, (WPARAM) GetDlgCtrlID());
		pParent->SendMessage(UM_CPN_SELENDOK, lParam, (WPARAM) GetDlgCtrlID());
	}

	return TRUE;
}

LONG CIrcWnd::OnSelEndCancel(UINT lParam, LONG /*wParam*/)
{
	CWnd *pParent = GetParent();
	if(pParent)
	{
		pParent->SendMessage(UM_CPN_CLOSEUP, lParam, (WPARAM) GetDlgCtrlID());
		pParent->SendMessage(UM_CPN_SELENDCANCEL, lParam, (WPARAM) GetDlgCtrlID());
	}
	return TRUE;
}

void CIrcWnd::OnBnClickedUnderline()
{
	CString sAddAttribute;
	int	iSelStart;
	int	iSelEnd;

	m_wndInput.GetSel(iSelStart, iSelEnd);//get selection area
	m_wndInput.GetWindowText(sAddAttribute);//get the whole line
	if(iSelEnd > iSelStart)
		sAddAttribute.Insert(iSelEnd, _T('\x1f'));//if a selection add end tag
	sAddAttribute.Insert(iSelStart, _T('\x1f'));//add start tag
	iSelStart++;//increment start position
	iSelEnd++;//increment end position
	m_wndInput.SetWindowText(sAddAttribute);//write new line to edit control
	m_wndInput.SetSel(iSelStart, iSelEnd);//update selection info
	m_wndInput.SetFocus();//set focus (from button) to edit control
}

void CIrcWnd::OnBnClickedBold()
{
	CString sAddAttribute;
	int	iSelStart;
	int	iSelEnd;

	m_wndInput.GetSel(iSelStart, iSelEnd);//get selection area
	m_wndInput.GetWindowText(sAddAttribute);//get the whole line
	if(iSelEnd > iSelStart)
		sAddAttribute.Insert(iSelEnd, _T('\x02'));//if a selection add end tag
	sAddAttribute.Insert(iSelStart, _T('\x02'));//add start tag
	iSelStart++;//increment start position
	iSelEnd++;//increment end position
	m_wndInput.SetWindowText(sAddAttribute);//write new line to edit control
	m_wndInput.SetSel(iSelStart, iSelEnd);//update selection info
	m_wndInput.SetFocus();//set focus (from button) to edit control
}

void CIrcWnd::OnBnClickedReset()
{
	CString sAddAttribute;
	int iSelStart;
	int	iSelEnd;

	m_wndInput.GetSel(iSelStart, iSelEnd);//get selection area
	if(!iSelStart)
		return;//reset is not a first character
	m_wndInput.GetWindowText(sAddAttribute);//get the whole line
	//Note the 'else' below! this tag resets all atttribute so only one tag needed at current position or end of selection
	if(iSelEnd > iSelStart)
		sAddAttribute.Insert(iSelEnd, _T('\x0f'));//if a selection add end tag
	else
		sAddAttribute.Insert(iSelStart, _T('\x0f'));//add start tag
	iSelStart++;//increment start position
	iSelEnd++;//increment end position
	m_wndInput.SetWindowText(sAddAttribute);//write new line to edit control
	m_wndInput.SetSel(iSelStart, iSelEnd);//update selection info
	m_wndInput.SetFocus();//set focus (from button) to edit control
}

void CIrcWnd::OnBnClickedSmiley()
{
	if (m_pwndSmileySel) {
		m_pwndSmileySel->DestroyWindow();
		delete m_pwndSmileySel;
		m_pwndSmileySel = NULL;
	}
	m_pwndSmileySel = new CSmileySelector;

	CRect rcBtn;
	m_wndFormat.GetWindowRect(&rcBtn);
	rcBtn.top -= 2;

	if (!m_pwndSmileySel->Create(this, &rcBtn, &m_wndInput))
	{
		delete m_pwndSmileySel;
		m_pwndSmileySel = NULL;
	}
}

void CIrcWnd::OnEnRequestResizeTitle(NMHDR *pNMHDR, LRESULT *pResult)
{
	REQRESIZE *pReqResize = reinterpret_cast<REQRESIZE *>(pNMHDR);
	ASSERT( pReqResize->nmhdr.hwndFrom );

	CRect rcTitle;
	::GetWindowRect(pReqResize->nmhdr.hwndFrom, &rcTitle);
	ScreenToClient(&rcTitle);

	CRect rcResizeAdjusted(pReqResize->rc);
	AdjustWindowRectEx(&rcResizeAdjusted, (DWORD)GetWindowLong(pReqResize->nmhdr.hwndFrom, GWL_STYLE), FALSE, (DWORD)::GetWindowLong(pReqResize->nmhdr.hwndFrom, GWL_EXSTYLE));
	rcTitle.bottom = rcTitle.top + rcResizeAdjusted.Height() + 1/*!?!*/;

	// Don't allow too large title windows
	if (rcTitle.Height() <= IRC_TITLE_WND_MAX_HEIGHT)
		::SetWindowPos(pReqResize->nmhdr.hwndFrom, NULL, 0, 0, rcTitle.Width(), rcTitle.Height(), SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);

	*pResult = 0;
}

HBRUSH CIrcWnd::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = theApp.emuledlg->GetCtlColor(pDC, pWnd, nCtlColor);
	if (hbr)
		return hbr;
	return __super::OnCtlColor(pDC, pWnd, nCtlColor);
}
