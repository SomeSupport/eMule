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
#include "IrcChannelListCtrl.h"
#include "emuleDlg.h"
#include "otherfunctions.h"
#include "MenuCmds.h"
#include "IrcWnd.h"
#include "IrcMain.h"
#include "emule.h"
#include "MemDC.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

struct ChannelName
{
	ChannelName(const CString& sName, UINT uUsers, const CString& sDesc)
		: m_sName(sName), m_uUsers(uUsers), m_sDesc(sDesc)
	{ }
	CString m_sName;
	UINT m_uUsers;
	CString m_sDesc;
};

IMPLEMENT_DYNAMIC(CIrcChannelListCtrl, CMuleListCtrl)

BEGIN_MESSAGE_MAP(CIrcChannelListCtrl, CMuleListCtrl)
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnLvnColumnClick)
	ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnLvnGetDispInfo)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnNmDblClk)
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

CIrcChannelListCtrl::CIrcChannelListCtrl()
{
	m_pParent = NULL;
	SetSkinKey(L"IRCChannelsLv");
}

CIrcChannelListCtrl::~CIrcChannelListCtrl()
{
	ResetServerChannelList(true);
}

void CIrcChannelListCtrl::Init()
{
	SetPrefsKey(_T("IrcChannelListCtrl"));
	SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

	InsertColumn(0, GetResString(IDS_IRC_NAME),		LVCFMT_LEFT,  200);
	InsertColumn(1, GetResString(IDS_UUSERS),		LVCFMT_RIGHT,  50);
	InsertColumn(2, GetResString(IDS_DESCRIPTION),	LVCFMT_LEFT,  350);

	LoadSettings();
	SetSortArrow();
	SortItems(&SortProc, GetSortItem() + (GetSortAscending() ? 0 : 10));
}

void CIrcChannelListCtrl::Localize()
{
	CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();
	HDITEM hdi;
	hdi.mask = HDI_TEXT;
	CString strRes;

	strRes = GetResString(IDS_IRC_NAME);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(0, &hdi);

	strRes = GetResString(IDS_UUSERS);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(1, &hdi);

	strRes = GetResString(IDS_DESCRIPTION);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(2, &hdi);
}

void CIrcChannelListCtrl::GetItemDisplayText(const ChannelName *pChannel, int iSubItem, LPTSTR pszText, int cchTextMax)
{
	if (pszText == NULL || cchTextMax <= 0) {
		ASSERT(0);
		return;
	}
	pszText[0] = _T('\0');
	switch (iSubItem)
	{
		case 0:
			_tcsncpy(pszText, pChannel->m_sName, cchTextMax);
			break;

		case 1:
			_sntprintf(pszText, cchTextMax, _T("%u"), pChannel->m_uUsers);
			break;

		case 2:
			_tcsncpy(pszText, pChannel->m_sDesc, cchTextMax);
			break;
	}
	pszText[cchTextMax - 1] = _T('\0');
}

void CIrcChannelListCtrl::OnLvnGetDispInfo(NMHDR *pNMHDR, LRESULT *pResult)
{
	if (theApp.emuledlg->IsRunning()) {
		// Although we have an owner drawn listview control we store the text for the primary item in the listview, to be
		// capable of quick searching those items via the keyboard. Because our listview items may change their contents,
		// we do this via a text callback function. The listview control will send us the LVN_DISPINFO notification if
		// it needs to know the contents of the primary item.
		//
		// But, the listview control sends this notification all the time, even if we do not search for an item. At least
		// this notification is only sent for the visible items and not for all items in the list. Though, because this
		// function is invoked *very* often, do *NOT* put any time consuming code in here.
		//
		// Vista: That callback is used to get the strings for the label tips for the sub(!) items.
		//
		NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
		if (pDispInfo->item.mask & LVIF_TEXT) {
			const ChannelName *pChannel = (ChannelName *)pDispInfo->item.lParam;
			if (pChannel != NULL)
				GetItemDisplayText(pChannel, pDispInfo->item.iSubItem, pDispInfo->item.pszText, pDispInfo->item.cchTextMax);
		}
	}
	*pResult = 0;
}

void CIrcChannelListCtrl::OnLvnColumnClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	bool bSortAscending = (GetSortItem() != pNMLV->iSubItem) ? true : !GetSortAscending();
	SetSortArrow(pNMLV->iSubItem, bSortAscending);
	SortItems(&SortProc, pNMLV->iSubItem + (bSortAscending ? 0 : 10));

	*pResult = 0;
}

int CIrcChannelListCtrl::SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	const ChannelName* pItem1 = (ChannelName*)lParam1;
	const ChannelName* pItem2 = (ChannelName*)lParam2;
	int iColumn = lParamSort >= 10 ? lParamSort - 10 : lParamSort;
	int iResult = 0;
	switch (iColumn)
	{
		case 0:
			iResult = pItem1->m_sName.CompareNoCase(pItem2->m_sName);
			break;
		
		case 1:
			iResult = CompareUnsigned(pItem1->m_uUsers, pItem2->m_uUsers);
			break;
		
		case 2:
			iResult = pItem1->m_sDesc.CompareNoCase(pItem2->m_sDesc);
			break;

		default:
			return 0;
	}
	if (lParamSort >= 10)
		iResult = -iResult;
	return iResult;
}

void CIrcChannelListCtrl::OnNmDblClk(NMHDR*, LRESULT* pResult)
{
	JoinChannels();
	*pResult = 0;
}

void CIrcChannelListCtrl::OnContextMenu(CWnd*, CPoint point)
{
	int iCurSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	CTitleMenu menuChannel;
	menuChannel.CreatePopupMenu();
	menuChannel.AddMenuTitle(GetResString(IDS_IRC_CHANNEL));
	menuChannel.AppendMenu(MF_STRING, Irc_Join, GetResString(IDS_IRC_JOIN));
	if (iCurSel == -1)
		menuChannel.EnableMenuItem(Irc_Join, MF_GRAYED);
	GetPopupMenuPos(*this, point);
	menuChannel.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
	VERIFY( menuChannel.DestroyMenu() );
}

BOOL CIrcChannelListCtrl::OnCommand(WPARAM wParam, LPARAM)
{
	switch (wParam)
	{
		case Irc_Join:
			//Pressed the join button.
			JoinChannels();
			return TRUE;
	}
	return TRUE;
}

bool CIrcChannelListCtrl::AddChannelToList(const CString& sName, const CString& sUsers, const CString& sDesc)
{
	UINT uUsers = _tstoi(sUsers);
	if (thePrefs.GetIRCUseChannelFilter())
	{
		if (uUsers < thePrefs.GetIRCChannelUserFilter())
			return false;
		// was already filtered with "/LIST" command
		//if (!thePrefs.GetIRCChannelFilter().IsEmpty())
		//{
		//	if (stristr(sName, thePrefs.GetIRCChannelFilter()) == NULL)
		//		return false;
		//}
	}

	ChannelName* pChannel = new ChannelName(sName, uUsers, m_pParent->StripMessageOfFontCodes(sDesc));
	m_lstChannelNames.AddTail(pChannel);
	int iItem = InsertItem(LVIF_TEXT | LVIF_PARAM, GetItemCount(), LPSTR_TEXTCALLBACK, 0, 0, 0, (LPARAM)pChannel);
	if (iItem < 0)
		return false;
	SetItemText(iItem, 1, LPSTR_TEXTCALLBACK);
	SetItemText(iItem, 2, LPSTR_TEXTCALLBACK);
	return true;
}

void CIrcChannelListCtrl::ResetServerChannelList(bool bShutDown)
{
	POSITION pos = m_lstChannelNames.GetHeadPosition();
	while (pos)
		delete m_lstChannelNames.GetNext(pos);
	m_lstChannelNames.RemoveAll();
	if (!bShutDown)
		DeleteAllItems();
}

void CIrcChannelListCtrl::JoinChannels()
{
	if (!m_pParent->IsConnected())
		return;
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos != NULL)
	{
		int iIndex = GetNextSelectedItem(pos);
		if (iIndex >= 0)
			m_pParent->m_pIrcMain->SendString(_T("JOIN ") + GetItemText(iIndex, 0));
	}
}
