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

#pragma once
#include "ResizableLib/ResizableDialog.h"
#include "IrcNickListCtrl.h"
#include "IrcChannelListCtrl.h"
#include "IrcChannelTabCtrl.h"
#include "SplitterControl.h"
#include "ToolBarCtrlX.h"

class CIrcMain;
class CSmileySelector;

class CIrcWnd : public CResizableDialog
{
	DECLARE_DYNAMIC(CIrcWnd)
public:
	CIrcWnd(CWnd* pParent = NULL);
	virtual ~CIrcWnd();

	enum { IDD = IDD_IRC };

	void Localize();
	bool GetLoggedIn();
	void SetLoggedIn(bool bFlag);
	void SetSendFileString(const CString& sInFile);
	CString GetSendFileString();
	bool IsConnected();
	void UpdateFonts(CFont* pFont);
	void ParseChangeMode(const CString& sChannel, const CString& sChanger, CString sCommands, const CString& sParams);
	void AddStatus(CString sLine, bool bShowActivity = true, UINT uStatusCode = 0);
	void AddStatusF(CString sReceived, ...);
	void AddInfoMessage(Channel *pChannel, CString sLine);
	void AddInfoMessage(const CString& sChannelName, CString sReceived, bool bShowChannel = false);
	void AddInfoMessageF(const CString& sChannelName, CString sReceived, ...);
	void AddMessage(const CString& sChannelName, CString sTargetname, CString sLine);
	void AddMessageF(const CString& sChannelName, CString sTargetname, CString sLine, ...);
	void AddColorLine(const CString& line, CHTRichEditCtrl& wnd, COLORREF crForeground = CLR_DEFAULT);
	void SetConnectStatus(bool bConnected);
	void NoticeMessage(const CString& sSource, const CString& sTarget, const CString& sMessage);
	CString StripMessageOfFontCodes(CString sTemp);
	CString StripMessageOfColorCodes(CString sTemp);
	void SetTitle(const CString& sChannel, const CString& sTitle);
	void SendString(const CString& sSend);
	void EnableSmileys(bool bEnable)										{m_wndChanSel.EnableSmileys(bEnable);}
	afx_msg void OnBnClickedCloseChannel(int iItem = -1);

	CEdit m_wndInput;
	CIrcMain* m_pIrcMain;
	CIrcChannelTabCtrl m_wndChanSel;
	CIrcNickListCtrl m_wndNicks;
	CIrcChannelListCtrl m_wndChanList;
	CToolBarCtrlX m_wndFormat;

protected:
	friend class CIrcChannelTabCtrl;

	CString m_sSendString;
	bool m_bLoggedIn;
	bool m_bConnected;
	CSplitterControl m_wndSplitterHorz;
	CSmileySelector *m_pwndSmileySel;

	void OnChatTextChange();
	void AutoComplete();
	void DoResize(int iDelta);
	void UpdateChannelChildWindowsSize();
	void SetAllIcons();

	virtual BOOL OnInitDialog();
	virtual void OnSize(UINT iType, int iCx, int iCy);
	virtual int OnCreate(LPCREATESTRUCT lpCreateStruct);
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnCommand(WPARAM wParam,LPARAM lParam );
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual LRESULT DefWindowProc(UINT uMessage, WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnSysColorChange();
	afx_msg void OnBnClickedIrcConnect();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnBnClickedIrcSend();
	afx_msg LRESULT OnCloseTab(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnQueryTab(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedColour();
	afx_msg void OnBnClickedUnderline();
	afx_msg void OnBnClickedBold();
	afx_msg void OnBnClickedReset();
	afx_msg void OnBnClickedSmiley();
	afx_msg LONG OnSelEndOK(UINT lParam, LONG wParam);
	afx_msg LONG OnSelEndCancel(UINT lParam, LONG wParam);
	afx_msg void OnEnRequestResizeTitle(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};
