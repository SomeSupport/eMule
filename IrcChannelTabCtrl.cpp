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

#include "StdAfx.h"
#define MMNODRV			// mmsystem: Installable driver support
//#define MMNOSOUND		// mmsystem: Sound support
#define MMNOWAVE		// mmsystem: Waveform support
#define MMNOMIDI		// mmsystem: MIDI support
#define MMNOAUX			// mmsystem: Auxiliary audio support
#define MMNOMIXER		// mmsystem: Mixer support
#define MMNOTIMER		// mmsystem: Timer support
#define MMNOJOY			// mmsystem: Joystick support
#define MMNOMCI			// mmsystem: MCI support
#define MMNOMMIO		// mmsystem: Multimedia file I/O support
#define MMNOMMSYSTEM	// mmsystem: General MMSYSTEM functions
#include <Mmsystem.h>
#include "IrcChannelTabCtrl.h"
#include "emule.h"
#include "IrcWnd.h"
#include "IrcMain.h"
#include "otherfunctions.h"
#include "MenuCmds.h"
#include "HTRichEditCtrl.h"
#include "UserMsgs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CIrcChannelTabCtrl, CClosableTabCtrl)

BEGIN_MESSAGE_MAP(CIrcChannelTabCtrl, CClosableTabCtrl)
	ON_MESSAGE(UM_CLOSETAB, OnCloseTab)
	ON_MESSAGE(UM_QUERYTAB, OnQueryTab)
	ON_NOTIFY_REFLECT(TCN_SELCHANGE, OnTcnSelChange)
END_MESSAGE_MAP()

CIrcChannelTabCtrl::CIrcChannelTabCtrl()
{
	m_pCurrentChannel = NULL;
	m_pChanStatus = NULL;
	m_pChanList = NULL;
	m_pParent = NULL;
	m_bCloseable = true;
}

CIrcChannelTabCtrl::~CIrcChannelTabCtrl()
{
	//Remove and delete all our open channels.
	DeleteAllChannels();
}

void CIrcChannelTabCtrl::Init()
{
	//This adds the two static windows, Status and ChanneList
	m_pChanStatus = NewChannel(GetResString(IDS_STATUS), Channel::ctStatus);
	m_pChanList = NewChannel(GetResString(IDS_IRC_CHANNELLIST), Channel::ctChannelList);
	//Initialize the IRC window to be in the ChannelName
	m_pCurrentChannel = m_lstChannels.GetTail();
	SetCurSel(0);
	OnTcnSelChange(NULL, NULL);
	SetAllIcons();
}

void CIrcChannelTabCtrl::OnSysColorChange()
{
	CClosableTabCtrl::OnSysColorChange();
	SetAllIcons();
}

void CIrcChannelTabCtrl::SetAllIcons()
{
	CImageList imlist;
	imlist.Create(16, 16, theApp.m_iDfltImageListColorFlags | ILC_MASK, 0, 1);
	imlist.Add(CTempIconLoader(_T("Log")));
	imlist.Add(CTempIconLoader(_T("IRC")));
	imlist.Add(CTempIconLoader(_T("Message")));
	imlist.Add(CTempIconLoader(_T("MessagePending")));
	SetImageList(&imlist);
	m_imlistIRC.DeleteImageList();
	m_imlistIRC.Attach(imlist.Detach());
	SetPadding(CSize(12, 3));
}

Channel* CIrcChannelTabCtrl::FindChannelByName(const CString& sName)
{
	CString sTempName(sName);
	sTempName.Trim();

	POSITION pos = m_lstChannels.GetHeadPosition();
	while (pos)
	{
		Channel* pChannel = m_lstChannels.GetNext(pos);
		if (pChannel->m_sName.CompareNoCase(sTempName) == 0 && (pChannel->m_eType == Channel::ctNormal || pChannel->m_eType == Channel::ctPrivate))
			return pChannel;
	}
	return NULL;
}

Channel* CIrcChannelTabCtrl::NewChannel(const CString& sName, Channel::EType eType)
{
	Channel* pChannel = new Channel;
	pChannel->m_sName = sName;
	pChannel->m_sTitle = sName;
	pChannel->m_eType = eType;
	pChannel->m_iHistoryPos = 0;
	if (eType != Channel::ctChannelList)
	{
		CRect rcChannelPane;
		m_pParent->m_wndChanList.GetWindowRect(&rcChannelPane);
		m_pParent->ScreenToClient(rcChannelPane);

		CRect rcLog(rcChannelPane);
		if (eType == Channel::ctNormal
#ifdef _DEBUG
//#define DEBUG_IRC_TEXT
#endif
#ifdef DEBUG_IRC_TEXT
			|| eType == Channel::ctStatus
#endif
			)
		{
			CRect rcTitle(rcChannelPane.left, rcChannelPane.top, 
						  rcChannelPane.right, rcChannelPane.top + IRC_TITLE_WND_DFLT_HEIGHT);
			pChannel->m_wndTitle.Create(WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_NOHIDESEL, rcTitle, m_pParent, IDC_TITLEWINDOW);
			pChannel->m_wndTitle.ModifyStyleEx(0, WS_EX_STATICEDGE, SWP_FRAMECHANGED);
			pChannel->m_wndTitle.SendMessage(EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(3, 3));
			pChannel->m_wndTitle.SetEventMask(pChannel->m_wndTitle.GetEventMask() | ENM_LINK);
			pChannel->m_wndTitle.SetAutoScroll(false);
			pChannel->m_wndTitle.SetFont(&theApp.m_fontHyperText);
			pChannel->m_wndTitle.SetTitle(sName);
			pChannel->m_wndTitle.SetProfileSkinKey(_T("IRCChannel"));
			// The idea is to show the channel title with black background, thus giving the
			// user a chance to more easier read the colorful strings which are often created
			// to be read against a black or at least dark background. Though, there is one
			// problem. If the user has customized the system default selection color to
			// black, he would not be able to read any highlighted URLs against a black
			// background any longer because the RE control is using that very same color for
			// hyperlinks. So, we select a black background only if the user is already using
			// the default windows background (white), because it is very unlikely that such
			// a user would have costomized also the selection color to white.
			if (GetSysColor(COLOR_WINDOW) == RGB(255,255,255))
			{
				pChannel->m_wndTitle.SetDfltForegroundColor(RGB(255,255,255));
				pChannel->m_wndTitle.SetDfltBackgroundColor(RGB(0,0,0));
			}
			pChannel->m_wndTitle.ApplySkin();
			pChannel->m_wndTitle.EnableSmileys(thePrefs.GetIRCEnableSmileys());

#ifdef DEBUG_IRC_TEXT
			if (eType == Channel::ctStatus) {
				pChannel->m_wndTitle.AddLine(_T("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ^°1234567890!\"§$%&/()=?´`²³{[]}ß\\öäüÖÄÜ+*~#',.-;:_"));
			}
#endif

			CRect rcSplitter(rcChannelPane.left, rcTitle.bottom, 
							 rcChannelPane.right, rcTitle.bottom + IRC_CHANNEL_SPLITTER_HEIGHT);
			pChannel->m_wndSplitter.Create(WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, rcSplitter, m_pParent, IDC_SPLITTER_IRC_CHANNEL);
			pChannel->m_wndSplitter.SetRange(rcTitle.top + IRC_TITLE_WND_MIN_HEIGHT + IRC_CHANNEL_SPLITTER_HEIGHT/2, rcTitle.top + IRC_TITLE_WND_MAX_HEIGHT - IRC_CHANNEL_SPLITTER_HEIGHT/2);
			rcLog.top = rcSplitter.bottom;
			rcLog.bottom = rcChannelPane.bottom;
		}

		pChannel->m_wndLog.Create(WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_NOHIDESEL, rcLog, m_pParent, (UINT)-1);
		pChannel->m_wndLog.ModifyStyleEx(0, WS_EX_STATICEDGE, SWP_FRAMECHANGED);
		pChannel->m_wndLog.SendMessage(EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(3, 3));
		pChannel->m_wndLog.SetEventMask(pChannel->m_wndLog.GetEventMask() | ENM_LINK);
		pChannel->m_wndLog.SetFont(&theApp.m_fontHyperText);
		pChannel->m_wndLog.SetTitle(sName);
		pChannel->m_wndLog.SetProfileSkinKey(_T("IRCChannel"));
		pChannel->m_wndLog.ApplySkin();
		pChannel->m_wndLog.EnableSmileys(thePrefs.GetIRCEnableSmileys());

		if (eType == Channel::ctNormal || eType == Channel::ctPrivate)
		{
			PARAFORMAT pf = {0};
			pf.cbSize = sizeof pf;
			pf.dwMask = PFM_OFFSET;
			pf.dxOffset = 150;
			pChannel->m_wndLog.SetParaFormat(pf);
		}

#ifdef DEBUG_IRC_TEXT
		if (eType == Channel::ctStatus){
			//pChannel->m_wndLog.AddLine(_T(":) debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug debug"));
			m_pParent->AddColorLine(L"normal\002bold\002normal\r\n", pChannel->m_wndLog);
			m_pParent->AddColorLine(L"\0034red\002bold\002red\r\n", pChannel->m_wndLog);
			m_pParent->AddColorLine(L"normal\r\n", pChannel->m_wndLog);
			m_pParent->AddColorLine(L"\0032,4red\002bold\002red\r\n", pChannel->m_wndLog);
			m_pParent->AddColorLine(L"\017normal\r\n", pChannel->m_wndLog);

			LPCWSTR log = L"C:\\Programme\\mIRC 2\\channels\\MindForge_Sorted_X.txt";
			FILE *fp = _wfopen(log, L"rt");
			if (fp)
			{
				int i = 0;
				int iMax = 10000;
				TCHAR szLine[1024];
				while (i++ < iMax && fgetws(szLine, _countof(szLine), fp))
				{
					size_t nLen = wcslen(szLine);
					if (nLen >= 1 && szLine[nLen - 1] == L'\n')
						--nLen;
					szLine[nLen++] = L'\r';
					szLine[nLen++] = L'\n';
					//TRACE(_T("%u: %s\n"), i, szLine);
					CString strLine(szLine, nLen);
					m_pParent->AddColorLine(strLine, pChannel->m_wndLog);
				}
				fclose(fp);
			}
		}
#endif
	}
	m_lstChannels.AddTail(pChannel);

	TCITEM newitem;
	newitem.mask = TCIF_PARAM | TCIF_TEXT | TCIF_IMAGE;
	newitem.lParam = (LPARAM)pChannel;
	CString strTcLabel(sName);
	strTcLabel.Replace(_T("&"), _T("&&"));
	newitem.pszText = const_cast<LPTSTR>((LPCTSTR)strTcLabel);
	if (eType == Channel::ctStatus)
		newitem.iImage = 0;
	else if (eType == Channel::ctChannelList)
		newitem.iImage = 1;
	else
		newitem.iImage = 2;
	int iInsertAt = GetItemCount();
	int iItem = InsertItem(iInsertAt, &newitem);
	if (eType == Channel::ctNormal)
		SelectChannel(iItem);
	return pChannel;
}

int CIrcChannelTabCtrl::FindChannel(const Channel *pChannel)
{
	TCITEM item;
	item.mask = TCIF_PARAM;
	item.lParam = -1;
	int iItems = GetItemCount();
	int iIndex;
	for (iIndex = 0; iIndex < iItems; iIndex++)
	{
		GetItem(iIndex, &item);
		if ((const Channel*)item.lParam == pChannel)
			return iIndex;
	}
	return -1;
}

void CIrcChannelTabCtrl::SelectChannel(int iItem)
{
	ASSERT( iItem >= 0 && iItem < GetItemCount() );
	SetCurSel(iItem);
	SetCurFocus(iItem);
	OnTcnSelChange(NULL, NULL);
}

void CIrcChannelTabCtrl::SelectChannel(const Channel *pChannel)
{
	int iItem = FindChannel(pChannel);
	if (iItem < 0)
		return;
	SelectChannel(iItem);
}

void CIrcChannelTabCtrl::RemoveChannel(const CString& sChannel)
{
	Channel* pToDel = FindChannelByName(sChannel);
	if (!pToDel)
		return;

	ASSERT( pToDel != m_pChanStatus );
	ASSERT( pToDel != m_pChanList );

	TCITEM item;
	item.mask = TCIF_PARAM;
	item.lParam = -1;
	int iItems = GetItemCount();
	int iIndex;
	for (iIndex = 0; iIndex < iItems; iIndex++)
	{
		GetItem(iIndex, &item);
		if ((Channel*)item.lParam == pToDel)
			break;
	}
	if ((Channel*)item.lParam != pToDel)
		return;
	DeleteItem(iIndex);

	if (pToDel == m_pCurrentChannel)
	{
		m_pParent->m_wndNicks.DeleteAllItems();
		if (GetItemCount() > 2 && iIndex > 1)
		{
			if (iIndex == 2)
				iIndex++;
			SetCurSel(iIndex - 1);
			SetCurFocus(iIndex - 1);
			OnTcnSelChange(NULL, NULL);
		}
		else
		{
			SetCurSel(0);
			SetCurFocus(0);
			OnTcnSelChange(NULL, NULL);
		}
	}
	m_pParent->m_wndNicks.DeleteAllNick(pToDel->m_sName);
	m_lstChannels.RemoveAt(m_lstChannels.Find(pToDel));
	delete pToDel;
}

void CIrcChannelTabCtrl::DeleteAllChannels()
{
	POSITION pos1, pos2;
	for (pos1 = m_lstChannels.GetHeadPosition(); (pos2 = pos1) != NULL; )
	{
		m_lstChannels.GetNext(pos1);
		Channel* pCurChannel = m_lstChannels.GetAt(pos2);
		m_pParent->m_wndNicks.DeleteAllNick(pCurChannel->m_sName);
		m_lstChannels.RemoveAt(pos2);
		delete pCurChannel;
	}
	m_pChanStatus = NULL;
	m_pChanList = NULL;
}

bool CIrcChannelTabCtrl::ChangeChanMode(const CString& sChannel, const CString& sParam, const CString& sDir, const CString& sCommand)
{
	(void)sParam;

	//Must have a Command.
	if (sCommand.IsEmpty())
		return false;

	//Must be a channel!
	if (sChannel.IsEmpty() || sChannel[0] != _T('#'))
		return false;

	//Get Channel.
	//Make sure we have this channel in our list.
	Channel* pUpdate = FindChannelByName(sChannel);
	if (!pUpdate)
		return false;

	//Update modes.
	int iCommandIndex = m_sChannelModeSettingsTypeA.Find(sCommand);
	if (iCommandIndex != -1)
	{
		//Remove the setting. This takes care of "-" and makes sure we don't add the same symbol twice.
		pUpdate->m_sModesA.Replace(sCommand, _T(""));
		if (sDir == _T("+"))
		{
			//Update mode.
			if (pUpdate->m_sModesA.IsEmpty())
				pUpdate->m_sModesA = sCommand;
			else
			{
				//The chan does have other modes.. Lets make sure we put things in order..
				int iChannelModeSettingsTypeALength = m_sChannelModeSettingsTypeA.GetLength();
				//This will pad the mode string..
				for (int iIndex = 0; iIndex < iChannelModeSettingsTypeALength; iIndex++)
				{
					if (pUpdate->m_sModesA.Find(m_sChannelModeSettingsTypeA[iIndex]) == -1)
						pUpdate->m_sModesA.Insert(iIndex, _T(" "));
				}
				//Insert the new mode
				pUpdate->m_sModesA.Insert(iCommandIndex, sCommand);
				//Remove pads
				pUpdate->m_sModesA.Remove(_T(' '));
			}
		}
		return true;
	}

	iCommandIndex = m_sChannelModeSettingsTypeB.Find(sCommand);
	if (iCommandIndex != -1)
	{
		//Remove the setting. This takes care of "-" and makes sure we don't add the same symbol twice.
		pUpdate->m_sModesB.Replace(sCommand, _T(""));
		if (sDir == _T("+"))
		{
			//Update mode.
			if (pUpdate->m_sModesB.IsEmpty())
				pUpdate->m_sModesB = sCommand;
			else
			{
				//The chan does have other modes.. Lets make sure we put things in order..
				int iChannelModeSettingsTypeBLength = m_sChannelModeSettingsTypeB.GetLength();
				//This will pad the mode string..
				for (int iIndex = 0; iIndex < iChannelModeSettingsTypeBLength; iIndex++)
				{
					if (pUpdate->m_sModesB.Find(m_sChannelModeSettingsTypeB[iIndex]) == -1)
						pUpdate->m_sModesB.Insert(iIndex, _T(" "));
				}
				//Insert the new mode
				pUpdate->m_sModesB.Insert(iCommandIndex, sCommand);
				//Remove pads
				pUpdate->m_sModesB.Remove(_T(' '));
			}
		}
		return true;
	}

	iCommandIndex = m_sChannelModeSettingsTypeC.Find(sCommand);
	if (iCommandIndex != -1)
	{
		//Remove the setting. This takes care of "-" and makes sure we don't add the same symbol twice.
		pUpdate->m_sModesC.Replace(sCommand, _T(""));
		if (sDir == _T("+"))
		{
			//Update mode.
			if (pUpdate->m_sModesC.IsEmpty())
				pUpdate->m_sModesC = sCommand;
			else
			{
				//The chan does have other modes.. Lets make sure we put things in order..
				int iChannelModeSettingsTypeCLength = m_sChannelModeSettingsTypeC.GetLength();
				//This will pad the mode string..
				for (int iIndex = 0; iIndex < iChannelModeSettingsTypeCLength; iIndex++)
				{
					if (pUpdate->m_sModesC.Find(m_sChannelModeSettingsTypeC[iIndex]) == -1)
						pUpdate->m_sModesC.Insert(iIndex, _T(" "));
				}
				//Insert the new mode
				pUpdate->m_sModesC.Insert(iCommandIndex, sCommand);
				//Remove pads
				pUpdate->m_sModesC.Remove(_T(' '));
			}
		}
		return true;
	}

	iCommandIndex = m_sChannelModeSettingsTypeD.Find(sCommand);
	if (iCommandIndex != -1)
	{
		//Remove the setting. This takes care of "-" and makes sure we don't add the same symbol twice.
		pUpdate->m_sModesD.Replace(sCommand, _T(""));
		if (sDir == _T("+"))
		{
			//Update mode.
			if (pUpdate->m_sModesD.IsEmpty())
				pUpdate->m_sModesD = sCommand;
			else
			{
				//The chan does have other modes.. Lets make sure we put things in order..
				int iChannelModeSettingsTypeDLength = m_sChannelModeSettingsTypeD.GetLength();
				//This will pad the mode string..
				for (int iIndex = 0; iIndex < iChannelModeSettingsTypeDLength; iIndex++)
				{
					if (pUpdate->m_sModesD.Find(m_sChannelModeSettingsTypeD[iIndex]) == -1)
						pUpdate->m_sModesD.Insert(iIndex, _T(" "));
				}
				//Insert the new mode
				pUpdate->m_sModesD.Insert(iCommandIndex, sCommand);
				//Remove pads
				pUpdate->m_sModesD.Remove(_T(' '));
			}
		}
		return true;
	}

	return false;
}

void CIrcChannelTabCtrl::OnTcnSelChange(NMHDR*, LRESULT* pResult)
{
	//What channel did we select?
	int iCurSel = GetCurSel();
	if (iCurSel == -1) {
		//No channel, abort..
		return;
	}

	TCITEM item;
	item.mask = TCIF_PARAM;
	if (!GetItem(iCurSel, &item)) {
		//We had no valid item here.. Something isn't right..
		//TODO: this should never happen, so maybe we should remove this tab?
		return;
	}
	Channel* pUpdate = (Channel*)item.lParam;
	Channel* pCh2 = NULL;

	//Set our current channel to the new one for quick reference..
	m_pCurrentChannel = pUpdate;

	//We entered the channel, set activity flag off.
	SetActivity(m_pCurrentChannel, false);

	if (m_pCurrentChannel->m_eType == Channel::ctChannelList)
	{
		//Since some channels can have a LOT of nicks, hide the window then remove them to speed it up..
		m_pParent->m_wndNicks.ShowWindow(SW_HIDE);
		m_pParent->m_wndNicks.DeleteAllItems();
		m_pParent->m_wndNicks.UpdateNickCount();
		m_pParent->m_wndNicks.ShowWindow(SW_SHOW);
		//Show our ChanList..
		m_pParent->m_wndChanList.ShowWindow(SW_SHOW);
		TCITEM tci;
		tci.mask = TCIF_PARAM;
		//Go through the channel tabs and hide the channels..
		//Maybe overkill? Maybe just remember our previous channel and hide it?
		int iIndex = 0;
		while (GetItem(iIndex++, &tci))
		{
			pCh2 = (Channel*)tci.lParam;
			if (pCh2 != m_pCurrentChannel && pCh2->m_wndLog.m_hWnd != NULL)
				pCh2->Hide();
		}
		return;
	}

	//Show new current channel..
	m_pCurrentChannel->Show();
	m_pParent->UpdateChannelChildWindowsSize();

	//Hide all channels not in focus..
	//Maybe overkill? Maybe remember previous channel and hide?
	TCITEM tci;
	tci.mask = TCIF_PARAM;
	int iIndex = 0;
	while (GetItem(iIndex++, &tci))
	{
		pCh2 = (Channel*)tci.lParam;
		if (pCh2 != m_pCurrentChannel && pCh2->m_wndLog.m_hWnd != NULL)
			pCh2->Hide();
	}

	//Make sure channelList is hidden.
	m_pParent->m_wndChanList.ShowWindow(SW_HIDE);
	//Update nicklist to the new channel..
	m_pParent->m_wndNicks.RefreshNickList(pUpdate->m_sName);
	//Push focus back to the input box..
	m_pParent->m_wndInput.SetFocus();
	if (pResult)
		*pResult = 0;
}

void CIrcChannelTabCtrl::ScrollHistory(bool bDown)
{
	if ((m_pCurrentChannel->m_iHistoryPos == 0 && !bDown) || (m_pCurrentChannel->m_iHistoryPos == m_pCurrentChannel->m_astrHistory.GetCount() && bDown))
		return;

	if (bDown)
		++m_pCurrentChannel->m_iHistoryPos;
	else
		--m_pCurrentChannel->m_iHistoryPos;

	CString sBuffer = (m_pCurrentChannel->m_iHistoryPos == m_pCurrentChannel->m_astrHistory.GetCount()) ? _T("") : m_pCurrentChannel->m_astrHistory.GetAt(m_pCurrentChannel->m_iHistoryPos);
	m_pParent->m_wndInput.SetWindowText(sBuffer);
	m_pParent->m_wndInput.SetSel(sBuffer.GetLength(), sBuffer.GetLength());
}

void CIrcChannelTabCtrl::SetActivity(Channel *pChannel, bool bFlag)
{
	if (bFlag && pChannel == m_pCurrentChannel)
		return;
	if (!pChannel) {
		pChannel = m_lstChannels.GetHead();
		if (!pChannel)
			return;
	}

	TCITEM item;
	item.mask = TCIF_PARAM;
	item.lParam = -1;
	int iItems = GetItemCount();
	int iIndex;
	for (iIndex = 0; iIndex < iItems; iIndex++)
	{
		GetItem(iIndex, &item);
		if ((Channel*)item.lParam == pChannel)
			break;
	}
	if ((Channel*)item.lParam != pChannel)
		return;

	if (pChannel->m_eType == Channel::ctNormal || pChannel->m_eType == Channel::ctPrivate)
	{
		item.mask = TCIF_IMAGE;
		GetItem(iIndex, &item);
		if (bFlag)
		{
			if (item.iImage != 3) {
				item.iImage = 3; // 'MessagePending'
				SetItem(iIndex, &item);
			}
		}
		else
		{
			if (item.iImage != 2) {
				item.iImage = 2; // 'Message'
				SetItem(iIndex, &item);
			}
		}
	}
	if (bFlag)
		HighlightItem(iIndex, TRUE);
	else
		HighlightItem(iIndex, FALSE);
}

void CIrcChannelTabCtrl::ChatSend(CString sSend)
{
	if (sSend.IsEmpty())
		return;
	if (!m_pParent->IsConnected())
		return;

	if ((UINT)m_pCurrentChannel->m_astrHistory.GetCount() == thePrefs.GetMaxChatHistoryLines())
		m_pCurrentChannel->m_astrHistory.RemoveAt(0);
	m_pCurrentChannel->m_astrHistory.Add(sSend);
	m_pCurrentChannel->m_iHistoryPos = m_pCurrentChannel->m_astrHistory.GetCount();

	if (sSend.Left(4) == _T("/hop"))
	{
		if (m_pCurrentChannel->m_sName.Left(1) == _T("#"))
		{
			m_pParent->m_pIrcMain->SendString(_T("PART ") + m_pCurrentChannel->m_sName);
			m_pParent->m_pIrcMain->SendString(_T("JOIN ") + m_pCurrentChannel->m_sName);
		}
		return;
	}

	if (   sSend.Left(1) == _T("/")
		&& sSend.Left(3).MakeLower() != _T("/me")
		&& sSend.Left(6).MakeLower() != _T("/sound"))
	{
		if (sSend.Left(4) == _T("/msg"))
		{
			if (m_pCurrentChannel->m_eType == Channel::ctNormal || m_pCurrentChannel->m_eType == Channel::ctPrivate)
				m_pParent->AddInfoMessageF(m_pCurrentChannel->m_sName, _T("* >> %s"), sSend.Mid(5));
			else
				m_pParent->AddStatusF(_T("* >> %s"), sSend.Mid(5));
			sSend = _T("/PRIVMSG") + sSend.Mid(4);
		}
		else if (sSend.Left(7).CompareNoCase(_T("/notice")) == 0)
		{
			if (m_pCurrentChannel->m_eType == Channel::ctNormal || m_pCurrentChannel->m_eType == Channel::ctPrivate)
				m_pParent->AddInfoMessageF(m_pCurrentChannel->m_sName, _T("* >> %s"), sSend.Mid(8));
			else
				m_pParent->AddStatusF(_T("* >> %s"), sSend.Mid(8));
		}

		if (sSend.Left(17).CompareNoCase(_T("/PRIVMSG nickserv")) == 0)
		{
			sSend = _T("/ns") + sSend.Mid(17);
		}
		else if (sSend.Left(17).CompareNoCase(_T("/PRIVMSG chanserv")) == 0)
		{
			sSend = _T("/cs") + sSend.Mid(17);
		}
		else if (sSend.Left(8).CompareNoCase(_T("/PRIVMSG")) == 0)
		{
			int iIndex = sSend.Find(_T(' '), sSend.Find(_T(' ')) + 1);
			sSend.Insert(iIndex + 1, _T(":"));
		}
		else if (sSend.Left(6).CompareNoCase(_T("/TOPIC")) == 0)
		{
			int iIndex = sSend.Find(_T(' '), sSend.Find(_T(' ')) + 1);
			sSend.Insert(iIndex + 1, _T(":"));
		}
		m_pParent->m_pIrcMain->SendString(sSend.Mid(1));
		return;
	}

	if (m_pCurrentChannel->m_eType < Channel::ctNormal)
	{
		m_pParent->m_pIrcMain->SendString(sSend);
		return;
	}

	CString sBuild;
	if (sSend.Left(3) == _T("/me"))
	{
		sBuild.Format(_T("PRIVMSG %s :\001ACTION %s\001"), m_pCurrentChannel->m_sName, sSend.Mid(4));
		m_pParent->AddInfoMessageF(m_pCurrentChannel->m_sName, _T("* %s %s"), m_pParent->m_pIrcMain->GetNick(), sSend.Mid(4));
		m_pParent->m_pIrcMain->SendString(sBuild);
		return;
	}
	
	if (sSend.Left(6) == _T("/sound"))
	{
		CString sound;
		sBuild.Format(_T("PRIVMSG %s :\001SOUND %s\001"), m_pCurrentChannel->m_sName, sSend.Mid(7));
		m_pParent->m_pIrcMain->SendString(sBuild);
		sSend = sSend.Mid(7);
		int soundlen = sSend.Find(_T(' '));
		if (soundlen != -1)
		{
			sBuild = sSend.Left(soundlen);
			sBuild.Remove(_T('\\'));
			sSend = sSend.Left(soundlen);
		}
		else
		{
			sBuild = sSend;
			sSend = _T("[SOUND]");
		}
		sound.Format(_T("%sSounds\\IRC\\%s"), thePrefs.GetMuleDirectory(EMULE_EXECUTEABLEDIR), sBuild);
		m_pParent->AddInfoMessageF(m_pCurrentChannel->m_sName, _T("* %s %s"), m_pParent->m_pIrcMain->GetNick(), sSend);
		PlaySound(sound, NULL, SND_FILENAME | SND_NOSTOP | SND_NOWAIT | SND_ASYNC);
		return;
	}

	sBuild = _T("PRIVMSG ") + m_pCurrentChannel->m_sName + _T(" :") + sSend;
	m_pParent->m_pIrcMain->SendString(sBuild);
	m_pParent->AddMessageF(m_pCurrentChannel->m_sName, m_pParent->m_pIrcMain->GetNick(), _T("%s"), sSend);
}

void CIrcChannelTabCtrl::Localize()
{
	int iItems = GetItemCount();
	for (int iIndex = 0; iIndex < iItems; iIndex++)
	{
		TCITEM item;
		item.mask = TCIF_PARAM;
		item.lParam = -1;
		GetItem(iIndex, &item);
		Channel* pCurChan = (Channel*)item.lParam;
		if (pCurChan != NULL)
		{
			if (pCurChan->m_eType == Channel::ctStatus)
			{
				pCurChan->m_sTitle = GetResString(IDS_STATUS);
				item.mask = TCIF_TEXT;
				CString strTcLabel(pCurChan->m_sTitle);
				strTcLabel.Replace(_T("&"), _T("&&"));
				item.pszText = const_cast<LPTSTR>((LPCTSTR)strTcLabel);
				SetItem(iIndex, &item);
			}
			else if (pCurChan->m_eType == Channel::ctChannelList)
			{
				pCurChan->m_sTitle = GetResString(IDS_IRC_CHANNELLIST);
				item.mask = TCIF_TEXT;
				CString strTcLabel(pCurChan->m_sTitle);
				strTcLabel.Replace(_T("&"), _T("&&"));
				item.pszText = const_cast<LPTSTR>((LPCTSTR)strTcLabel);
				SetItem(iIndex, &item);
			}
		}
	}
	SetAllIcons();
}

LRESULT CIrcChannelTabCtrl::OnCloseTab(WPARAM wParam, LPARAM /*lParam*/)
{
	m_pParent->OnBnClickedCloseChannel((int)wParam);
	return TRUE;
}

LRESULT CIrcChannelTabCtrl::OnQueryTab(WPARAM wParam, LPARAM /*lParam*/)
{
	int iItem = (int)wParam;

	TCITEM item;
	item.mask = TCIF_PARAM;
	GetItem(iItem, &item);
	Channel* pPartChannel = (Channel*)item.lParam;
	if (pPartChannel && (pPartChannel->m_eType == Channel::ctNormal || pPartChannel->m_eType == Channel::ctPrivate))
		return 0;
	return 1;
}

void CIrcChannelTabCtrl::EnableSmileys(bool bEnable)
{
	POSITION pos = m_lstChannels.GetHeadPosition();
	while (pos){
		Channel* pChannel = m_lstChannels.GetNext(pos);
		if (pChannel->m_eType != Channel::ctChannelList){
			pChannel->m_wndLog.EnableSmileys(bEnable);			
			if (pChannel->m_eType == Channel::ctNormal)
				pChannel->m_wndTitle.EnableSmileys(bEnable);
		}
	}
}

void Channel::Show()
{
	if (m_wndTitle.m_hWnd) {
		m_wndTitle.EnableWindow(TRUE);
		m_wndTitle.ShowWindow(SW_SHOW);
	}
	
	if (m_wndSplitter.m_hWnd) {
		m_wndSplitter.EnableWindow(TRUE);
		m_wndSplitter.ShowWindow(SW_SHOW);
	}

	if (m_wndLog.m_hWnd) {
		m_wndLog.EnableWindow(TRUE);
		m_wndLog.ShowWindow(SW_SHOW);
	}
}

void Channel::Hide()
{
	if (m_wndTitle.m_hWnd) {
		m_wndTitle.ShowWindow(SW_HIDE);
		m_wndTitle.EnableWindow(FALSE);
	}
	
	if (m_wndSplitter.m_hWnd) {
		m_wndSplitter.ShowWindow(SW_HIDE);
		m_wndSplitter.EnableWindow(FALSE);
	}

	if (m_wndLog.m_hWnd) {
		m_wndLog.ShowWindow(SW_HIDE);
		m_wndLog.EnableWindow(FALSE);
	}
}
