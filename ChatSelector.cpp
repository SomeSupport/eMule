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
#include "ChatSelector.h"
#include "packets.h"
#include "HTRichEditCtrl.h"
#include "emuledlg.h"
#include "Statistics.h"
#include "OtherFunctions.h"
#include "UpDownClient.h"
#include "Preferences.h"
#include "TaskbarNotifier.h"
#include "ListenSocket.h"
#include "ChatWnd.h"
#include "SafeFile.h"
#include "Log.h"
#include "MenuCmds.h"
#include "ClientDetailDialog.h"
#include "FriendList.h"
#include "ClientList.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define	STATUS_MSG_COLOR		RGB(0,128,0)		// dark green
#define	SENT_TARGET_MSG_COLOR	RGB(0,192,0)		// bright green
#define	RECV_SOURCE_MSG_COLOR	RGB(0,128,255)		// bright cyan/blue

#define	TIME_STAMP_FORMAT		_T("[%H:%M] ")

///////////////////////////////////////////////////////////////////////////////
// CChatItem

CChatItem::CChatItem()
{
	client = NULL;
	log = NULL;
	notify = false;
	history_pos = 0;
}

CChatItem::~CChatItem()
{
	delete log;
}

///////////////////////////////////////////////////////////////////////////////
// CChatSelector

IMPLEMENT_DYNAMIC(CChatSelector, CClosableTabCtrl)

BEGIN_MESSAGE_MAP(CChatSelector, CClosableTabCtrl)
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_WM_SYSCOLORCHANGE()
	ON_NOTIFY_REFLECT(TCN_SELCHANGE, OnTcnSelChangeChatSel)
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

CChatSelector::CChatSelector()
{
	m_lastemptyicon = false;
	m_blinkstate = false;
	m_Timer = 0;
	m_bCloseable = true;
}

CChatSelector::~CChatSelector()
{
}

void CChatSelector::Init(CChatWnd *pParent)
{
	m_pParent = pParent;

	ModifyStyle(0, WS_CLIPCHILDREN);
	SetAllIcons();

	VERIFY( (m_Timer = SetTimer(20, 1500, 0)) != NULL );
}

void CChatSelector::OnSysColorChange()
{
	CClosableTabCtrl::OnSysColorChange();
	SetAllIcons();
}

void CChatSelector::SetAllIcons()
{
	CImageList iml;
	iml.Create(16, 16, theApp.m_iDfltImageListColorFlags | ILC_MASK, 0, 1);
	iml.Add(CTempIconLoader(_T("Chat")));
	iml.Add(CTempIconLoader(_T("Message")));
	iml.Add(CTempIconLoader(_T("MessagePending")));
	SetImageList(&iml);
	m_imlChat.DeleteImageList();
	m_imlChat.Attach(iml.Detach());
	SetPadding(CSize(12, 3));
}

void CChatSelector::UpdateFonts(CFont* pFont)
{
	TCITEM item;
	item.mask = TCIF_PARAM;
	int i = 0;
	while (GetItem(i++, &item)){
		CChatItem* ci = (CChatItem*)item.lParam;
		ci->log->SetFont(pFont);
	}
}

CChatItem* CChatSelector::StartSession(CUpDownClient* client, bool show)
{
	if (show)
		m_pParent->m_wndMessage.SetFocus();
	if (GetTabByClient(client) != -1){
		if (show){
			SetCurSel(GetTabByClient(client));
			ShowChat();
		}
		return NULL;
	}

	CChatItem* chatitem = new CChatItem();
	chatitem->client = client;
	chatitem->log = new CHTRichEditCtrl;

	CRect rcChat;
	GetChatSize(rcChat);
	if (GetItemCount() == 0)
		rcChat.top += 19; // add the height of the tab which is not yet there
	// using ES_NOHIDESEL is actually not needed, but it helps to get around a tricky window update problem!
	// If that style is not specified there are troubles with right clicking into the control for the very first time!?
	chatitem->log->Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_NOHIDESEL, rcChat, this, (UINT)-1);
	chatitem->log->ModifyStyleEx(0, WS_EX_STATICEDGE, SWP_FRAMECHANGED);
	chatitem->log->SendMessage(EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(3, 3));
	chatitem->log->SetEventMask(chatitem->log->GetEventMask() | ENM_LINK);
	chatitem->log->SetFont(&theApp.m_fontHyperText);
	chatitem->log->SetProfileSkinKey(_T("Chat"));
	chatitem->log->ApplySkin();
	chatitem->log->EnableSmileys(thePrefs.GetMessageEnableSmileys());

	PARAFORMAT pf = {0};
	pf.cbSize = sizeof pf;
	pf.dwMask = PFM_OFFSET;
	pf.dxOffset = 150;
	chatitem->log->SetParaFormat(pf);

	if (thePrefs.GetIRCAddTimeStamp())
		AddTimeStamp(chatitem);
	chatitem->log->AppendKeyWord(GetResString(IDS_CHAT_START) + client->GetUserName() + _T("\n"), STATUS_MSG_COLOR);
	client->SetChatState(MS_CHATTING);

	CString name;
	if (client->GetUserName() != NULL)
		name = client->GetUserName();
	else
		name.Format(_T("(%s)"), GetResString(IDS_UNKNOWN));
	chatitem->log->SetTitle(name);

	TCITEM newitem;
	newitem.mask = TCIF_PARAM | TCIF_TEXT | TCIF_IMAGE;
	newitem.lParam = (LPARAM)chatitem;
	name.Replace(_T("&"), _T("&&"));
	newitem.pszText = const_cast<LPTSTR>((LPCTSTR)name);
	newitem.iImage = 0;
	int iItemNr = InsertItem(GetItemCount(), &newitem);
	if (show || IsWindowVisible()){
		SetCurSel(iItemNr);
		ShowChat();
	}
	return chatitem;
}

int CChatSelector::GetTabByClient(CUpDownClient* client)
{
	for (int i = 0; i < GetItemCount(); i++){
		TCITEM cur_item;
		cur_item.mask = TCIF_PARAM;
		if (GetItem(i, &cur_item) && ((CChatItem*)cur_item.lParam)->client == client)
			return i;
	}
	return -1;
}

CChatItem* CChatSelector::GetItemByIndex(int index)
{
	TCITEM item;
	item.mask = TCIF_PARAM;
	if (GetItem(index, &item)==FALSE)
		return NULL;

    return (CChatItem*)item.lParam;
}

CChatItem* CChatSelector::GetItemByClient(CUpDownClient* client)
{
	for (int i = 0; i < GetItemCount(); i++){
		TCITEM cur_item;
		cur_item.mask = TCIF_PARAM;
		if (GetItem(i, &cur_item) && ((CChatItem*)cur_item.lParam)->client == client)
			return (CChatItem*)cur_item.lParam;
	}
	return NULL;
}

void CChatSelector::ProcessMessage(CUpDownClient* sender, const CString& message)
{
	sender->IncMessagesReceived();
	CChatItem* ci = GetItemByClient(sender);

	AddLogLine(true, GetResString(IDS_NEWMSG), sender->GetUserName(), ipstr(sender->GetConnectIP()));
	
	bool isNewChatWindow = false;
	if (!ci)
	{
		if ((UINT)GetItemCount() >= thePrefs.GetMsgSessionsMax())
			return;
		ci = StartSession(sender, false);
		isNewChatWindow = true; 
	}
	if (thePrefs.GetIRCAddTimeStamp())
		AddTimeStamp(ci);
	ci->log->AppendKeyWord(sender->GetUserName(), RECV_SOURCE_MSG_COLOR);
	ci->log->AppendText(_T(": "));
	ci->log->AppendText(message + _T("\n"));
	int iTabItem = GetTabByClient(sender);
	if (GetCurSel() == iTabItem && GetParent()->IsWindowVisible())
	{
		// chat window is already visible
		;
	}
	else if (GetCurSel() != iTabItem)
	{
		// chat window is already visible, but tab is not selected
		ci->notify = true;
	}
	else
	{
		ci->notify = true;
        if (isNewChatWindow || thePrefs.GetNotifierOnEveryChatMsg())
			theApp.emuledlg->ShowNotifier(GetResString(IDS_TBN_NEWCHATMSG) + _T(" ") + CString(sender->GetUserName()) + _T(":'") + message + _T("'\n"), TBN_CHAT);
		isNewChatWindow = false;
	}
}

void CChatSelector::ShowCaptchaRequest(CUpDownClient* sender, HBITMAP bmpCaptcha)
{
	CChatItem* ci = GetItemByClient(sender);
	if (ci != NULL)
	{
		if (thePrefs.GetIRCAddTimeStamp())
			AddTimeStamp(ci);
		ci->log->AppendKeyWord(_T("*** ") + GetResString(IDS_CAPTCHAREQUEST), STATUS_MSG_COLOR);
		ci->log->AddCaptcha(bmpCaptcha);
		ci->log->AddLine(_T("\n"));
	}
}

void CChatSelector::ShowCaptchaResult(CUpDownClient* sender, CString strResult)
{
	CChatItem* ci = GetItemByClient(sender);
	if (ci != NULL)
	{
		if (thePrefs.GetIRCAddTimeStamp())
			AddTimeStamp(ci);
		ci->log->AppendKeyWord(_T("*** ") + strResult + _T("\n"), STATUS_MSG_COLOR);
	}
}

bool CChatSelector::SendMessage(const CString& rstrMessage)
{
	CChatItem* ci = GetCurrentChatItem();
	if (!ci)
		return false;

	if ((UINT)ci->history.GetCount() == thePrefs.GetMaxChatHistoryLines())
		ci->history.RemoveAt(0);
	ci->history.Add(rstrMessage);
	ci->history_pos = ci->history.GetCount();

	// advance spamfilter stuff
	ci->client->IncMessagesSent();
	ci->client->SetSpammer(false);
	if (ci->client->GetChatState() == MS_CONNECTING)
		return false;
	
	if (ci->client->GetChatCaptchaState() == CA_CAPTCHARECV)
		ci->client->SetChatCaptchaState(CA_SOLUTIONSENT);
	else if (ci->client->GetChatCaptchaState() == CA_SOLUTIONSENT)
		ASSERT( false ); // we responsed to a captcha but didn't heard from the client afterwards - hopefully its just lag and this message will get through
	else
		ci->client->SetChatCaptchaState(CA_ACCEPTING);

	


	// there are three cases on connectiing/sending the message:
	if (ci->client->socket && ci->client->socket->IsConnected())
	{
		// 1.) the client is connected already - this is simple, jsut send it
		ci->client->SendChatMessage(rstrMessage);
		if (thePrefs.GetIRCAddTimeStamp())
			AddTimeStamp(ci);
		ci->log->AppendKeyWord(thePrefs.GetUserNick(), SENT_TARGET_MSG_COLOR);
		ci->log->AppendText(_T(": "));
		ci->log->AppendText(rstrMessage + _T("\n"));
	}
	else if (ci->client->GetFriend() != NULL)
	{
		// We are not connected and this client is a friend - friends have additional ways to connect and additional checks
		// to make sure they are really friends, let the friend class is handling it
		ci->strMessagePending = rstrMessage;
		ci->client->SetChatState(MS_CONNECTING);
		ci->client->GetFriend()->TryToConnect(this);
	}
	else
	{
		// this is a normal client, who is not connected right now. just try to connect to the given IP, without any
		// additional checks or searchings.
		if (thePrefs.GetIRCAddTimeStamp())
			AddTimeStamp(ci);
		ci->log->AppendKeyWord(_T("*** ") + GetResString(IDS_CONNECTING), STATUS_MSG_COLOR);
		ci->strMessagePending = rstrMessage;
		ci->client->SetChatState(MS_CONNECTING);
		ci->client->TryToConnect(true);
	}
	return true;
}

void CChatSelector::ConnectingResult(CUpDownClient* sender, bool success)
{
	CChatItem* ci = GetItemByClient(sender);
	if (!ci)
		return;

	ci->client->SetChatState(MS_CHATTING);
	if (!success){
		if (!ci->strMessagePending.IsEmpty()){
			ci->log->AppendKeyWord(_T(" ...") + GetResString(IDS_FAILED) + _T("\n"), STATUS_MSG_COLOR);
			ci->strMessagePending.Empty();
		}
		else{
			if (thePrefs.GetIRCAddTimeStamp())
				AddTimeStamp(ci);
			ci->log->AppendKeyWord(GetResString(IDS_CHATDISCONNECTED) + _T("\n"), STATUS_MSG_COLOR);
		}
	}
	else if (!ci->strMessagePending.IsEmpty()){
		ci->log->AppendKeyWord(_T(" ...") + GetResString(IDS_TREEOPTIONS_OK) + _T("\n"), STATUS_MSG_COLOR);
		ci->client->SendChatMessage(ci->strMessagePending);

		if (thePrefs.GetIRCAddTimeStamp())
			AddTimeStamp(ci);
		ci->log->AppendKeyWord(thePrefs.GetUserNick(), SENT_TARGET_MSG_COLOR);
		ci->log->AppendText(_T(": "));
		ci->log->AppendText(ci->strMessagePending + _T("\n"));
		
		ci->strMessagePending.Empty();
	}
	else{
		if (thePrefs.GetIRCAddTimeStamp())
			AddTimeStamp(ci);
		ci->log->AppendKeyWord(_T("*** Connected\n"), STATUS_MSG_COLOR);
	}
}

void CChatSelector::DeleteAllItems()
{
	for (int i = 0; i < GetItemCount(); i++){
		TCITEM cur_item;
		cur_item.mask = TCIF_PARAM;
		if (GetItem(i, &cur_item))
			delete (CChatItem*)cur_item.lParam;
	}
}

void CChatSelector::OnTimer(UINT_PTR /*nIDEvent*/)
{
	m_blinkstate = !m_blinkstate;
	bool globalnotify = false;
	for (int i = 0; i < GetItemCount();i++)
	{
		TCITEM cur_item;
		cur_item.mask = TCIF_PARAM | TCIF_IMAGE;
		if (!GetItem(i, &cur_item))
			break;

		cur_item.mask = TCIF_IMAGE;
		if (((CChatItem*)cur_item.lParam)->notify){
			cur_item.iImage = (m_blinkstate) ? 1 : 2;
			SetItem(i, &cur_item);
			HighlightItem(i, TRUE);
			globalnotify = true;
		}
		else if (cur_item.iImage != 0){
			cur_item.iImage = 0;
			SetItem(i, &cur_item);
			HighlightItem(i, FALSE);
		}
	}

	if (globalnotify) {
		theApp.emuledlg->ShowMessageState(m_blinkstate ? 1 : 2);
		m_lastemptyicon = false;
	}
	else if (!m_lastemptyicon) {
		theApp.emuledlg->ShowMessageState(0);
		m_lastemptyicon = true;
	}
}

CChatItem* CChatSelector::GetCurrentChatItem()
{
	int iCurSel = GetCurSel();
	if (iCurSel == -1)
		return NULL;

	TCITEM cur_item;
	cur_item.mask = TCIF_PARAM;
	if (!GetItem(iCurSel, &cur_item))
		return NULL;

	return (CChatItem*)cur_item.lParam;
}

void CChatSelector::ShowChat()
{
	CChatItem* ci = GetCurrentChatItem();
	if (!ci)
		return;

	// show current chat window
	ci->log->ShowWindow(SW_SHOW);
	m_pParent->m_wndMessage.SetFocus();

	TCITEM item;
	item.mask = TCIF_IMAGE;
	item.iImage = 0;
	SetItem(GetCurSel(), &item);
	HighlightItem(GetCurSel(), FALSE);

	// hide all other chat windows
	item.mask = TCIF_PARAM;
	int i = 0;
	while (GetItem(i++, &item)){
		CChatItem* ci2 = (CChatItem*)item.lParam;
		if (ci2 != ci)
			ci2->log->ShowWindow(SW_HIDE);
	}

	ci->notify = false;
}

void CChatSelector::OnTcnSelChangeChatSel(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	ShowChat();
	*pResult = 0;
}

int CChatSelector::InsertItem(int nItem, TCITEM* pTabCtrlItem)
{
	int iResult = CClosableTabCtrl::InsertItem(nItem, pTabCtrlItem);
	RedrawWindow();
	return iResult;
}

BOOL CChatSelector::DeleteItem(int nItem)
{
	CClosableTabCtrl::DeleteItem(nItem);
	RedrawWindow();
	return TRUE;
}

void CChatSelector::EndSession(CUpDownClient* client)
{
	int iCurSel;
	if (client)
		iCurSel = GetTabByClient(client);
	else
		iCurSel = GetCurSel();
	if (iCurSel == -1)
		return;
	
	TCITEM item;
	item.mask = TCIF_PARAM;
	if (!GetItem(iCurSel, &item) || item.lParam == 0)
		return;
	CChatItem* ci = (CChatItem*)item.lParam;
	ci->client->SetChatState(MS_NONE);
	ci->client->SetChatCaptchaState(CA_NONE);

	DeleteItem(iCurSel);
	delete ci;

	int iTabItems = GetItemCount();
	if (iTabItems > 0){
		// select next tab
		if (iCurSel == CB_ERR)
			iCurSel = 0;
		else if (iCurSel >= iTabItems)
			iCurSel = iTabItems - 1;
		(void)SetCurSel(iCurSel);				// returns CB_ERR if error or no prev. selection(!)
		iCurSel = GetCurSel();					// get the real current selection
		if (iCurSel == CB_ERR)					// if still error
			iCurSel = SetCurSel(0);
		ShowChat();
	}
}

void CChatSelector::GetChatSize(CRect& rcChat)
{
	CRect rcClient;
	GetClientRect(&rcClient);
	AdjustRect(FALSE, rcClient);
	rcChat.left = rcClient.left + 4;
	rcChat.top = rcClient.top + 4;
	rcChat.right = rcClient.right - 4;
	rcChat.bottom = rcClient.bottom - 4;
}

void CChatSelector::OnSize(UINT nType, int cx, int cy)
{
	CClosableTabCtrl::OnSize(nType, cx, cy);

	CRect rcChat;
	GetChatSize(rcChat);

	TCITEM item;
	item.mask = TCIF_PARAM;
	int i = 0;
	while (GetItem(i++, &item)){
		CChatItem* ci = (CChatItem*)item.lParam;
		ci->log->SetWindowPos(NULL, rcChat.left, rcChat.top, rcChat.Width(), rcChat.Height(), SWP_NOZORDER);
	}
}

void CChatSelector::AddTimeStamp(CChatItem* ci)
{
	ci->log->AppendText(CTime::GetCurrentTime().Format(TIME_STAMP_FORMAT));
}

void CChatSelector::OnDestroy()
{
	if (m_Timer){
		KillTimer(m_Timer);
		m_Timer = NULL;
	}
	CClosableTabCtrl::OnDestroy();
}

BOOL CChatSelector::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam) {
		case MP_DETAIL:{
			const CChatItem* ci = GetItemByIndex(m_iContextIndex);
			if (ci) {
				CClientDetailDialog dialog(ci->client);
				dialog.DoModal();
			}
			return TRUE;
		}
		case MP_ADDFRIEND:{
			const CChatItem* ci = GetItemByIndex(m_iContextIndex);
			if (ci) {
				CFriend* fr = theApp.friendlist->SearchFriend(ci->client->GetUserHash(), 0, 0);
				if (!fr)
					theApp.friendlist->AddFriend(ci->client);
			}
			return TRUE;
		}
		case MP_REMOVEFRIEND:{
			const CChatItem* ci = GetItemByIndex(m_iContextIndex);
			if (ci) {
				CFriend* fr = theApp.friendlist->SearchFriend(ci->client->GetUserHash(), 0, 0);
				if (fr)
					theApp.friendlist->RemoveFriend(fr);
			}
			return TRUE;
		}
		case MP_REMOVE:{
			const CChatItem* ci = GetItemByIndex(m_iContextIndex);
			if (ci)
				EndSession(ci->client);
			return TRUE;
		}
	}
	return CClosableTabCtrl::OnCommand(wParam, lParam);
}

void CChatSelector::OnContextMenu(CWnd*, CPoint point)
{
	TCHITTESTINFO hti = {0};
	::GetCursorPos(&hti.pt);
	ScreenToClient(&hti.pt);


	m_iContextIndex=this->HitTest(&hti);
	if (m_iContextIndex==-1)
		return;

	TCITEM item;
	item.mask = TCIF_PARAM;
	GetItem(m_iContextIndex, &item);

	const CChatItem* ci = (CChatItem*)item.lParam;
	if (ci == NULL)
		return;

	CFriend* pFriend = theApp.friendlist->SearchFriend(ci->client->GetUserHash(), 0, 0);

	CTitleMenu menu;
	menu.CreatePopupMenu();
	menu.AddMenuTitle(GetResString(IDS_CLIENT), true);

	menu.AppendMenu(MF_STRING, MP_DETAIL, GetResString(IDS_SHOWDETAILS), _T("CLIENTDETAILS"));

	GetCurrentChatItem();
	if (pFriend == NULL)
		menu.AppendMenu(MF_STRING, MP_ADDFRIEND, GetResString(IDS_IRC_ADDTOFRIENDLIST), _T("ADDFRIEND"));
	else
		menu.AppendMenu(MF_STRING, MP_REMOVEFRIEND, GetResString(IDS_REMOVEFRIEND), _T("DELETEFRIEND"));
	
	menu.AppendMenu(MF_STRING, MP_REMOVE, GetResString(IDS_FD_CLOSE));

	m_ptCtxMenu = point;
	ScreenToClient(&m_ptCtxMenu);
	menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
}

void CChatSelector::EnableSmileys(bool bEnable)
{
	for (int i = 0; i < GetItemCount(); i++){
		TCITEM cur_item;
		cur_item.mask = TCIF_PARAM;
		if (GetItem(i, &cur_item) && ((CChatItem*)cur_item.lParam)->log)
			((CChatItem*)cur_item.lParam)->log->EnableSmileys(bEnable);
	}
}

void CChatSelector::ReportConnectionProgress(CUpDownClient* pClient, CString strProgressDesc, bool bNoTimeStamp)
{
	CChatItem* ci = GetItemByClient(pClient);
	if (!ci)
		return;
	if (thePrefs.GetIRCAddTimeStamp() && !bNoTimeStamp)
		AddTimeStamp(ci);
	ci->log->AppendKeyWord(strProgressDesc, STATUS_MSG_COLOR);
}

void CChatSelector::ClientObjectChanged(CUpDownClient* pOldClient, CUpDownClient* pNewClient){
	// the friend has deceided to change the clients objects (because the old doesnt seems to be our friend) during a connectiontry
	// in order to not close and reopen a new session and loose the prior chat, switch the objects on a existing tab
	// nothing else changes since the tab is supposed to be still connected to the same friend
	CChatItem* ci = GetItemByClient(pOldClient);
	if (!ci)
		return;
	ci->client = pNewClient;
}