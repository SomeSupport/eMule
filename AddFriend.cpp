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
#include "AddFriend.h"
#include "Friend.h"
#include "otherfunctions.h"
#include "FriendList.h"
#include "Preferences.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CAddFriend dialog

IMPLEMENT_DYNAMIC(CAddFriend, CDialog)

BEGIN_MESSAGE_MAP(CAddFriend, CDialog)
	ON_BN_CLICKED(IDC_ADD, OnAddBtn)
END_MESSAGE_MAP()

CAddFriend::CAddFriend()
	: CDialog(CAddFriend::IDD)
{
	m_pShowFriend = NULL;
	m_icnWnd = NULL;
}

CAddFriend::~CAddFriend()
{
	if (m_icnWnd)
		VERIFY( DestroyIcon(m_icnWnd) );
}

void CAddFriend::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BOOL CAddFriend::OnInitDialog()
{
	CDialog::OnInitDialog();
	InitWindowStyles(this);
	Localize();
	if (m_pShowFriend)
	{
		SetIcon(m_icnWnd = theApp.LoadIcon(_T("ClientDetails")), FALSE);
		SendDlgItemMessage(IDC_IP, EM_SETREADONLY, TRUE);
		SendDlgItemMessage(IDC_PORT, EM_SETREADONLY, TRUE);
		SendDlgItemMessage(IDC_USERNAME, EM_SETREADONLY, TRUE);
	
		SetDlgItemInt(IDC_IP, m_pShowFriend->m_dwLastUsedIP, FALSE);
		SetDlgItemInt(IDC_PORT, m_pShowFriend->m_nLastUsedPort, FALSE);
		SetDlgItemText(IDC_USERNAME, m_pShowFriend->m_strName);
		if (m_pShowFriend->HasUserhash())
			SetDlgItemText(IDC_USERHASH, md4str(m_pShowFriend->m_abyUserhash));
		else
			SetDlgItemText(IDC_USERHASH, _T(""));

		if (m_pShowFriend->m_dwLastSeen){
			CTime t((time_t)m_pShowFriend->m_dwLastSeen);
			SetDlgItemText(IDC_EDIT2, t.Format(thePrefs.GetDateTimeFormat()));
		}
		SetDlgItemText(IDC_AFKADID, m_pShowFriend->HasKadID() ? GetResString(IDS_KNOWN) : GetResString(IDS_UNKNOWN));
		/*if (m_pShowFriend->m_dwLastChatted){
			CTime t((time_t)m_pShowFriend->m_dwLastChatted);
			SetDlgItemText(IDC_LAST_CHATTED, t.Format(thePrefs.GetDateTimeFormat()));
		}*/

		GetDlgItem(IDC_ADD)->ShowWindow(SW_HIDE);
	}
	else
	{
		SetIcon(m_icnWnd = theApp.LoadIcon(_T("AddFriend")), FALSE);
		((CEdit*)GetDlgItem(IDC_USERNAME))->SetLimitText(thePrefs.GetMaxUserNickLength());
		SetDlgItemText(IDC_USERHASH, _T(""));
	}
	return TRUE;
}

void CAddFriend::Localize()
{
	SetWindowText(m_pShowFriend ? GetResString(IDS_DETAILS) : GetResString(IDS_ADDAFRIEND));
	GetDlgItem(IDC_INFO1)->SetWindowText(GetResString(IDS_PAF_REQINFO));
	GetDlgItem(IDC_INFO2)->SetWindowText(GetResString(IDS_PAF_MOREINFO));

	GetDlgItem(IDC_ADD)->SetWindowText(GetResString(IDS_ADD));
	GetDlgItem(IDCANCEL)->SetWindowText(m_pShowFriend ? GetResString(IDS_FD_CLOSE) : GetResString(IDS_CANCEL));

	GetDlgItem(IDC_STATIC31)->SetWindowText(GetResString(IDS_CD_UNAME));
	GetDlgItem(IDC_STATIC32)->SetWindowText(GetResString(IDS_CD_UHASH));
	GetDlgItem(IDC_STATIC34)->SetWindowText(m_pShowFriend ? GetResString(IDS_USERID)+_T(":") : GetResString(IDS_CD_UIP));
	GetDlgItem(IDC_STATIC35)->SetWindowText(GetResString(IDS_PORT)+_T(":"));
	SetDlgItemText(IDC_LAST_SEEN_LABEL, GetResString(IDS_LASTSEEN)+_T(":"));
	SetDlgItemText(IDC_AFKADIDLABEL, GetResString(IDS_KADID)+_T(":"));
	//SetDlgItemText(IDC_LAST_CHATTED_LABEL, GetResString(IDS_LASTCHATTED)+_T(":"));
}

void CAddFriend::OnAddBtn()
{
	if (!m_pShowFriend)
	{
		CString strBuff;
		uint32 ip;
		GetDlgItemText(IDC_IP, strBuff);
		UINT u1, u2, u3, u4, uPort = 0;
		if (_stscanf(strBuff, _T("%u.%u.%u.%u:%u"), &u1, &u2, &u3, &u4, &uPort) != 5 || u1 > 255 || u2 > 255 || u3 > 255 || u4 > 255 || uPort > 65535){
			uPort = 0;
			if (_stscanf(strBuff, _T("%u.%u.%u.%u"), &u1, &u2, &u3, &u4) != 4 || u1 > 255 || u2 > 255 || u3 > 255 || u4 > 255){
				AfxMessageBox(GetResString(IDS_ERR_NOVALIDFRIENDINFO));
				GetDlgItem(IDC_IP)->SetFocus();
				return;
			}
		}
		in_addr iaFriend;
		iaFriend.S_un.S_un_b.s_b1 = (BYTE)u1;
		iaFriend.S_un.S_un_b.s_b2 = (BYTE)u2;
		iaFriend.S_un.S_un_b.s_b3 = (BYTE)u3;
		iaFriend.S_un.S_un_b.s_b4 = (BYTE)u4;
		ip = iaFriend.S_un.S_addr;

		if (uPort == 0)
		{
			GetDlgItemText(IDC_PORT, strBuff);
			if (_stscanf(strBuff, _T("%u"), &uPort) != 1){
				AfxMessageBox(GetResString(IDS_ERR_NOVALIDFRIENDINFO));
				GetDlgItem(IDC_PORT)->SetFocus();
				return;
			}
		}
		
		CString strUserName;
		GetDlgItemText(IDC_USERNAME, strUserName);
		strUserName.Trim();
		strUserName = strUserName.Left(thePrefs.GetMaxUserNickLength());

		// why did we offer an edit control for entering the userhash but did not store it?
		;

		if (!theApp.friendlist->AddFriend(NULL, 0, ip, (uint16)uPort, 0, strUserName, 0)){
			AfxMessageBox(GetResString(IDS_WRN_FRIENDDUPLIPPORT));
			GetDlgItem(IDC_IP)->SetFocus();
			return;
		}
	}
	else{
		// No "update" friend's data for now -- too much work to synchronize/update all
		// possible available related data in the client list...
	}
	
	OnCancel();
}
