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
#include "IrcNickListCtrl.h"
#include "otherfunctions.h"
#include "IrcWnd.h"
#include "IrcMain.h"
#include "MenuCmds.h"
#include "HTRichEditCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CIrcNickListCtrl, CMuleListCtrl)

BEGIN_MESSAGE_MAP(CIrcNickListCtrl, CMuleListCtrl)
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnLvnColumnClick)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnNmDblClk)
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

CIrcNickListCtrl::CIrcNickListCtrl()
{
	m_pParent = NULL;
	SetSkinKey(L"IRCNicksLv");
}

void CIrcNickListCtrl::Init()
{
	SetPrefsKey(_T("IrcNickListCtrl"));
	SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

	InsertColumn(0, GetResString(IDS_IRC_NICK), LVCFMT_LEFT, 90);

	LoadSettings();
	SetSortArrow();
	SortItems(&SortProc, GetSortItem() + (GetSortAscending() ? 0 : 10));
}

int CIrcNickListCtrl::SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	const Nick* pItem1 = (Nick*)lParam1;
	const Nick* pItem2 = (Nick*)lParam2;
	int iColumn = lParamSort >= 10 ? lParamSort - 10 : lParamSort;
	int iResult = 0;
	switch (iColumn)
	{
		case 0:
			if (pItem1->m_iLevel == pItem2->m_iLevel)
				iResult = pItem1->m_sNick.CompareNoCase(pItem2->m_sNick);
			else
			{
				if (pItem1->m_iLevel == -1)
					return 1;
				if (pItem2->m_iLevel == -1)
					return -1;
				return pItem1->m_iLevel - pItem2->m_iLevel;
			}
			break;

		default:
			return 0;
	}
	if (lParamSort >= 10)
		iResult = -iResult;
	return iResult;
}

void CIrcNickListCtrl::OnLvnColumnClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	bool bSortAscending = (GetSortItem() != pNMLV->iSubItem) ? true : !GetSortAscending();
	SetSortArrow(pNMLV->iSubItem, bSortAscending);
	SortItems(&SortProc, pNMLV->iSubItem + (bSortAscending ? 0 : 10));

	*pResult = 0;
}

void CIrcNickListCtrl::OnContextMenu(CWnd*, CPoint point)
{
	int iCurSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (iCurSel == -1)
		return;
	const Nick* pNick = (Nick*)GetItemData(iCurSel);
	if (!pNick)
		return;

	CTitleMenu menuNick;
	menuNick.CreatePopupMenu();
	menuNick.AddMenuTitle(GetResString(IDS_IRC_NICK) + _T(" : ") + pNick->m_sNick);
	menuNick.AppendMenu(MF_STRING, Irc_Priv, GetResString(IDS_IRC_PRIVMESSAGE));
	menuNick.AppendMenu(MF_STRING, Irc_AddFriend, GetResString(IDS_IRC_ADDTOFRIENDLIST));
	if (!m_pParent->GetSendFileString().IsEmpty())
		menuNick.AppendMenu(MF_STRING, Irc_SendLink, GetResString(IDS_IRC_SENDLINK) + m_pParent->GetSendFileString());
	else
		menuNick.AppendMenu(MF_STRING, Irc_SendLink, GetResString(IDS_IRC_SENDLINK) + GetResString(IDS_IRC_NOSFS));
	menuNick.AppendMenu(MF_STRING, Irc_Kick, GetResString(IDS_IRC_KICK));
	menuNick.AppendMenu(MF_STRING, Irc_Ban, _T("Ban"));
	//Ban currently uses chanserv to ban which seems to kick also.. May change this later..
	//	menuNick.AppendMenu(MF_STRING, Irc_KB, _T("Kick/Ban"));
	menuNick.AppendMenu(MF_STRING, Irc_Slap, GetResString(IDS_IRC_SLAP));
	menuNick.SetDefaultItem(Irc_Priv);
	GetPopupMenuPos(*this, point);
	menuNick.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
	VERIFY( menuNick.DestroyMenu() );
}

void CIrcNickListCtrl::OpenPrivateChannel(const Nick *pNick)
{
	m_pParent->AddInfoMessage(pNick->m_sNick, GetResString(IDS_IRC_PRIVATECHANSTART), true);
}

void CIrcNickListCtrl::OnNmDblClk(NMHDR*, LRESULT* pResult)
{
	//We double clicked a nick.. Try to open private channel
	int iNickItem = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (iNickItem != -1)
	{
		const Nick* pNick = (Nick*)GetItemData(iNickItem);
		if (pNick)
			OpenPrivateChannel(pNick);
	}
	*pResult = 0;
}

Nick* CIrcNickListCtrl::FindNickByName(const CString& sChannel, const CString& sName)
{
	const Channel* pChannel = m_pParent->m_wndChanSel.FindChannelByName(sChannel);
	if (!pChannel)
		return NULL;
	return FindNickByName(pChannel, sName);
}

Nick* CIrcNickListCtrl::FindNickByName(const Channel* pChannel, const CString& sName)
{
	POSITION pos = pChannel->m_lstNicks.GetHeadPosition();
	while (pos)
	{
		Nick* pNick = pChannel->m_lstNicks.GetNext(pos);
		if (pNick->m_sNick == sName)
			return pNick;
	}
	return NULL;
}

Nick* CIrcNickListCtrl::NewNick(const CString& sChannel, const CString& sNick)
{
	Channel* pChannel = m_pParent->m_wndChanSel.FindChannelByName(sChannel);
	if (!pChannel)
		return NULL;
	//This is a little clumsy and makes you think the previous check wasn't needed,
	//But we need the channel object and FindNickByName doesn't do it..
	if (FindNickByName(pChannel, sNick))
		return NULL;

	Nick* pNick = new Nick;

	//Remove all modes from the front of this nick
	pNick->m_sNick = sNick;
	while (pNick->m_sNick.GetLength() >= 1 && m_sUserModeSymbols.Find(pNick->m_sNick[0]) != -1)
	{
		pNick->m_sModes += pNick->m_sNick[0];
		pNick->m_sNick = pNick->m_sNick.Mid(1);
	}

	//Set user level
	if (!pNick->m_sModes.IsEmpty())
		pNick->m_iLevel = m_sUserModeSymbols.Find(pNick->m_sModes[0]);
	else
		pNick->m_iLevel = -1;

	//Add new nick to channel.
	pChannel->m_lstNicks.AddTail(pNick);
	if (pChannel == m_pParent->m_wndChanSel.m_pCurrentChannel)
	{
		//This is our current channel, add it to our nicklist..
		int iItem = InsertItem(LVIF_TEXT | LVIF_PARAM, GetItemCount(), pNick->m_sModes + pNick->m_sNick, 0, 0, 0, (LPARAM)pNick);
		if (iItem >= 0)
			UpdateNickCount();
	}
	return pNick;
}

void CIrcNickListCtrl::RefreshNickList(const CString& sChannel)
{
	//Hide nickList to speed things up..
	ShowWindow(SW_HIDE);
	DeleteAllItems();
	Channel* pChannel = m_pParent->m_wndChanSel.FindChannelByName(sChannel);
	if (!pChannel) {
		//This is not a channel??? shouldn't happen..
		UpdateNickCount();
		ShowWindow(SW_SHOW);
		return;
	}

	POSITION pos = pChannel->m_lstNicks.GetHeadPosition();
	while (pos)
	{
		const Nick* pNick = pChannel->m_lstNicks.GetNext(pos);
		int iItem = InsertItem(LVIF_TEXT | LVIF_PARAM, GetItemCount(), pNick->m_sModes + pNick->m_sNick, 0, 0, 0, (LPARAM)pNick);
		if (iItem < 0)
			break;
	}
	UpdateNickCount();
	ShowWindow(SW_SHOW);
}

bool CIrcNickListCtrl::RemoveNick(const CString& sChannel, const CString& sNick)
{
	Channel* pChannel = m_pParent->m_wndChanSel.FindChannelByName(sChannel);
	if (!pChannel)
		return false;

	POSITION pos = pChannel->m_lstNicks.GetHeadPosition();
	while (pos)
	{
		POSITION posPrev = pos;
		Nick *pNick = pChannel->m_lstNicks.GetNext(pos);
		if (pNick->m_sNick == sNick)
		{
			if (pChannel == m_pParent->m_wndChanSel.m_pCurrentChannel)
			{
				//If it's our current channel, delete the nick from nickList
				LVFINDINFO find;
				find.flags = LVFI_PARAM;
				find.lParam = (LPARAM)pNick;
				int iItem = FindItem(&find);
				if (iItem >= 0)
				{
					DeleteItem(iItem);
					UpdateNickCount();
				}
			}
			pChannel->m_lstNicks.RemoveAt(posPrev);
			delete pNick;
			return true;
		}
	}
	return false;
}

void CIrcNickListCtrl::DeleteAllNick(const CString& sChannel)
{
	Channel* pChannel = m_pParent->m_wndChanSel.FindChannelByName(sChannel);
	if (!pChannel)
		return;

	POSITION pos = pChannel->m_lstNicks.GetHeadPosition();
	while (pos)
		delete pChannel->m_lstNicks.GetNext(pos);
	pChannel->m_lstNicks.RemoveAll();
}

void CIrcNickListCtrl::DeleteNickInAll(const CString& sNick, const CString& sMessage)
{
	POSITION pos = m_pParent->m_wndChanSel.m_lstChannels.GetHeadPosition();
	while (pos)
	{
		Channel *pChannel = m_pParent->m_wndChanSel.m_lstChannels.GetNext(pos);
		if (RemoveNick(pChannel->m_sName, sNick))
		{
			if (!thePrefs.GetIRCIgnoreQuitMessages())
				m_pParent->AddInfoMessageF(pChannel->m_sName, GetResString(IDS_IRC_HASQUIT), sNick, sMessage);
		}
	}
}

bool CIrcNickListCtrl::ChangeNick(const CString& sChannel, const CString& sOldNick, const CString& sNewNick)
{
	Channel *pChannel = m_pParent->m_wndChanSel.FindChannelByName(sChannel);
	if (!pChannel)
		return false;

	POSITION pos = pChannel->m_lstNicks.GetHeadPosition();
	while (pos)
	{
		Nick *pNick = pChannel->m_lstNicks.GetNext(pos);
		if (pNick->m_sNick == sOldNick)
		{
			if (pChannel == m_pParent->m_wndChanSel.m_pCurrentChannel)
			{
				//This channle is in focus, update nick in nickList
				LVFINDINFO find;
				find.flags = LVFI_PARAM;
				find.lParam = (LPARAM)pNick;
				int iItem = FindItem(&find);
				if (iItem != -1)
					SetItemText(iItem, 0, pNick->m_sModes + sNewNick);
			}
			pNick->m_sNick = sNewNick;
			return true;
		}
	}
	return false;
}

bool CIrcNickListCtrl::ChangeNickMode(const CString& sChannel, const CString& sNick, const CString& sMode)
{
	if (sChannel.GetLength() >= 1 && sChannel[0] != _T('#'))
		return true;
	if (sNick.IsEmpty())
		return true;
	Channel *pChannel = m_pParent->m_wndChanSel.FindChannelByName(sChannel);
	if (!pChannel)
		return false;

	POSITION pos = pChannel->m_lstNicks.GetHeadPosition();
	while (pos)
	{
		Nick *pNick = pChannel->m_lstNicks.GetNext(pos);
		if (pNick->m_sNick == sNick)
		{
			int iModeLevel = m_sUserModeSettings.Find(sMode[1]);
			if (iModeLevel != -1)
			{
				CString sModeSymbol = m_sUserModeSymbols.Mid(iModeLevel, 1);
				//Remove the symbol. This takes care of "-" and makes sure we don't add the same symbol twice.
				pNick->m_sModes.Remove(sModeSymbol[0]);
				if (sMode.GetLength() >= 1 && sMode[0] == _T('+'))
				{
					//The nick doesn't have any other modes.. Just set it..
					if (pNick->m_sModes.IsEmpty())
						pNick->m_sModes = sModeSymbol;
					else
					{
						//The nick does have other modes.. Lets make sure we put things in order..
						int iIndex = 0;
						//This will pad the mode string..
						while (iIndex < m_sUserModeSymbols.GetLength())
						{
							if (pNick->m_sModes.Find(m_sUserModeSymbols[iIndex]) == -1)
								pNick->m_sModes.Insert(iIndex, _T(" "));
							iIndex++;
						}
						//Insert the new mode
						pNick->m_sModes.Insert(iModeLevel, sModeSymbol[0]);
						//Remove pads
						pNick->m_sModes.Remove(_T(' '));
					}
				}
			}
			else
			{
				//This should never happen
				pNick->m_sModes.Empty();
				pNick->m_iLevel = -1;
				ASSERT(0);
			}

			//Update user level
			if (pNick->m_sModes.GetLength() > 0)
				pNick->m_iLevel = m_sUserModeSymbols.Find(pNick->m_sModes[0]);
			else
				pNick->m_iLevel = -1;

			if (pChannel == m_pParent->m_wndChanSel.m_pCurrentChannel)
			{
				//Channel was in focus, update the nickList.
				LVFINDINFO find;
				find.flags = LVFI_PARAM;
				find.lParam = (LPARAM)pNick;
				int iItem = FindItem(&find);
				if (iItem != -1)
					SetItemText(iItem, 0, pNick->m_sModes + pNick->m_sNick);
			}
			return true;
		}
	}

	//Nick was not found in list??
	return false;
}

bool CIrcNickListCtrl::ChangeAllNick(const CString& sOldNick, const CString& sNewNick)
{
	//Change a nick in ALL the channels..
	Channel* pChannel = m_pParent->m_wndChanSel.FindChannelByName(sOldNick);
	if (pChannel)
	{
		//We had a private room open with this nick.. Update the title of the channel!
		pChannel->m_sName = sNewNick;

		TCITEM item;
		item.mask = TCIF_PARAM;
		item.lParam = -1;
		for (int iItem = 0; iItem < m_pParent->m_wndChanSel.GetItemCount(); iItem++)
		{
			m_pParent->m_wndChanSel.GetItem(iItem, &item);
			if ((Channel*)item.lParam == pChannel)
			{
				item.mask = TCIF_TEXT;
				CString strTcLabel(sNewNick);
				strTcLabel.Replace(_T("&"), _T("&&"));
				item.pszText = const_cast<LPTSTR>((LPCTSTR)strTcLabel);
				m_pParent->m_wndChanSel.SetItem(iItem, &item);
				return true;
			}
		}
	}

	// Go through all other channel nicklists..
	bool bChanged = false;
	POSITION pos = m_pParent->m_wndChanSel.m_lstChannels.GetHeadPosition();
	while (pos)
	{
		pChannel = m_pParent->m_wndChanSel.m_lstChannels.GetNext(pos);
		if (ChangeNick(pChannel->m_sName, sOldNick, sNewNick))
		{
			if (!thePrefs.GetIRCIgnoreMiscMessages())
				m_pParent->AddInfoMessageF(pChannel->m_sName, GetResString(IDS_IRC_NOWKNOWNAS), sOldNick, sNewNick);
			bChanged = true;
		}
	}
	return bChanged;
}

void CIrcNickListCtrl::UpdateNickCount()
{
	CString sResource;
	int iItemCount = GetItemCount();
	if (iItemCount)
		sResource.Format(_T("%s (%u)"), GetResString(IDS_IRC_NICK), iItemCount);
	else
		sResource.Format(_T("%s"), GetResString(IDS_IRC_NICK));
	HDITEM hdi;
	hdi.mask = HDI_TEXT;
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)sResource);
	GetHeaderCtrl()->SetItem(0, &hdi);
}

void CIrcNickListCtrl::Localize()
{
	CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();
	CString sResource;
	sResource = GetResString(IDS_STATUS);
	HDITEM hdi;
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)sResource);
	hdi.mask = HDI_TEXT;
	pHeaderCtrl->SetItem(1, &hdi);
	UpdateNickCount();
}

BOOL CIrcNickListCtrl::OnCommand(WPARAM wParam, LPARAM)
{
	int iNickItem = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	int iChanItem = m_pParent->m_wndChanSel.GetCurSel();
	Nick* pNick = (Nick*)GetItemData(iNickItem);
	TCITEM item;
	item.mask = TCIF_PARAM;
	m_pParent->m_wndChanSel.GetItem(iChanItem,&item);
	Channel* pChannel = (Channel*)item.lParam;

	switch (wParam)
	{
		case Irc_Priv:
			if (pNick)
				OpenPrivateChannel(pNick);
			return TRUE;

		case Irc_Kick:
			if (pNick && pChannel)
			{
				CString sSend;
				sSend.Format(_T("KICK %s %s"), pChannel->m_sName, pNick->m_sNick);
				m_pParent->m_pIrcMain->SendString(sSend);
			}
			return TRUE;

		case Irc_Ban:
			if (pNick && pChannel)
			{
				CString sSend;
				sSend.Format(_T("cs ban %s %s"), pChannel->m_sName, pNick->m_sNick);
				m_pParent->m_pIrcMain->SendString(sSend);
			}
			return TRUE;

		case Irc_Slap:
			if (pNick && pChannel)
			{
				CString sSend;
				sSend.Format(GetResString(IDS_IRC_SLAPMSGSEND), pChannel->m_sName, pNick->m_sNick);
				m_pParent->AddInfoMessageF(pChannel->m_sName, GetResString(IDS_IRC_SLAPMSG), m_pParent->m_pIrcMain->GetNick(), pNick->m_sNick);
				m_pParent->m_pIrcMain->SendString(sSend);
			}
			return TRUE;

		case Irc_AddFriend:
			if (pNick && pChannel)
			{
				//SetVerify() sets a new challenge which is required by the other end to respond with for some protection.
				CString sSend;
				sSend.Format(_T("PRIVMSG %s :\001RQSFRIEND|%u|\001"), pNick->m_sNick, m_pParent->m_pIrcMain->SetVerify());
				m_pParent->m_pIrcMain->SendString(sSend);
			}
			return TRUE;

		case Irc_SendLink:
			if (!m_pParent->GetSendFileString())
				return TRUE;
			if (pNick && pChannel)
			{
				//We send our nick and ClientID to allow the other end to only accept links from friends..
				CString sSend;
				sSend.Format(_T("PRIVMSG %s :\001SENDLINK|%s|%s\001"), pNick->m_sNick, md4str(thePrefs.GetUserHash()), m_pParent->GetSendFileString());
				m_pParent->m_pIrcMain->SendString(sSend);
			}
			return TRUE;
	}
	return TRUE;
}
