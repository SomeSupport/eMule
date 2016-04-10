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
#include "PPGProxy.h"
#include "opcodes.h"
#include "OtherFunctions.h"
#include "Preferences.h"
#include "HelpIDs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNAMIC(CPPgProxy, CPropertyPage)

BEGIN_MESSAGE_MAP(CPPgProxy, CPropertyPage)
	ON_BN_CLICKED(IDC_ENABLEPROXY, OnBnClickedEnableProxy)
	ON_BN_CLICKED(IDC_ENABLEAUTH, OnBnClickedEnableAuthentication)
	ON_CBN_SELCHANGE(IDC_PROXYTYPE, OnCbnSelChangeProxyType)
	ON_EN_CHANGE(IDC_PROXYNAME, OnEnChangeProxyName)
	ON_EN_CHANGE(IDC_PROXYPORT, OnEnChangeProxyPort)
	ON_EN_CHANGE(IDC_USERNAME_A, OnEnChangeUserName)
	ON_EN_CHANGE(IDC_PASSWORD, OnEnChangePassword)
	ON_WM_HELPINFO()
END_MESSAGE_MAP()

CPPgProxy::CPPgProxy()
	: CPropertyPage(CPPgProxy::IDD)
{
}

CPPgProxy::~CPPgProxy()
{
}

void CPPgProxy::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}

BOOL CPPgProxy::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	InitWindowStyles(this);

	proxy = thePrefs.GetProxySettings();
	LoadSettings();
	Localize();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPgProxy::OnApply()
{
	proxy.UseProxy = (IsDlgButtonChecked(IDC_ENABLEPROXY) != 0);
	proxy.EnablePassword = ((CButton*)GetDlgItem(IDC_ENABLEAUTH))->GetCheck() != 0;
	proxy.type = (uint16)((CComboBox*)GetDlgItem(IDC_PROXYTYPE))->GetCurSel();

	if (GetDlgItem(IDC_PROXYNAME)->GetWindowTextLength()) {
		CStringA strProxyA;
		GetWindowTextA(*GetDlgItem(IDC_PROXYNAME), strProxyA.GetBuffer(256), 256);
		strProxyA.ReleaseBuffer();
		strProxyA.FreeExtra();
		int iColon = strProxyA.Find(':');
		if (iColon > -1) {
			SetDlgItemTextA(m_hWnd, IDC_PROXYPORT, strProxyA.Mid(iColon + 1));
			strProxyA = strProxyA.Left(iColon);
		}
		proxy.name = strProxyA;
	}
	else {
		proxy.name.Empty();
        proxy.UseProxy = false;
	}

	if (GetDlgItem(IDC_PROXYPORT)->GetWindowTextLength()) {
		TCHAR buffer[6];
		GetDlgItem(IDC_PROXYPORT)->GetWindowText(buffer, _countof(buffer));
		if ((proxy.port = (uint16)_tstoi(buffer)) == 0)
			proxy.port = 1080;
	}
	else
		proxy.port = 1080;

	if (GetDlgItem(IDC_USERNAME_A)->GetWindowTextLength()) { 
		CString strUser;
		GetDlgItem(IDC_USERNAME_A)->GetWindowText(strUser);
		proxy.user = CStringA(strUser);
	}
	else {
		proxy.user.Empty();
		proxy.EnablePassword = false;
	}

	if (GetDlgItem(IDC_PASSWORD)->GetWindowTextLength()) { 
		CString strPasswd;
		GetDlgItem(IDC_PASSWORD)->GetWindowText(strPasswd);
		proxy.password = CStringA(strPasswd);
	}
	else {
		proxy.password.Empty();
		proxy.EnablePassword = false;
	}

	thePrefs.SetProxySettings(proxy);
	LoadSettings();
	return TRUE;
}

void CPPgProxy::OnBnClickedEnableProxy()
{
	SetModified(TRUE);
	CButton* btn = (CButton*) GetDlgItem(IDC_ENABLEPROXY);
	GetDlgItem(IDC_ENABLEAUTH)->EnableWindow(btn->GetCheck() != 0);
	GetDlgItem(IDC_PROXYTYPE)->EnableWindow(btn->GetCheck() != 0);
	GetDlgItem(IDC_PROXYNAME)->EnableWindow(btn->GetCheck() != 0);
	GetDlgItem(IDC_PROXYPORT)->EnableWindow(btn->GetCheck() != 0);
	GetDlgItem(IDC_USERNAME_A)->EnableWindow(btn->GetCheck() != 0);
	GetDlgItem(IDC_PASSWORD)->EnableWindow(btn->GetCheck() != 0);
	if (btn->GetCheck() != 0) OnBnClickedEnableAuthentication();
	if (btn->GetCheck() != 0) OnCbnSelChangeProxyType();
}

void CPPgProxy::OnBnClickedEnableAuthentication()
{
	SetModified(TRUE);
	CButton* btn = (CButton*)GetDlgItem(IDC_ENABLEAUTH);
	GetDlgItem(IDC_USERNAME_A)->EnableWindow(btn->GetCheck() != 0);
	GetDlgItem(IDC_PASSWORD)->EnableWindow(btn->GetCheck() != 0);
}

void CPPgProxy::OnCbnSelChangeProxyType()
{
	SetModified(TRUE);
	CComboBox* cbbox = (CComboBox*)GetDlgItem(IDC_PROXYTYPE);
	if (!(cbbox->GetCurSel() == PROXYTYPE_SOCKS5 || cbbox->GetCurSel() == PROXYTYPE_HTTP10 || cbbox->GetCurSel() == PROXYTYPE_HTTP11))
	{
		((CButton*)GetDlgItem(IDC_ENABLEAUTH))->SetCheck(0);
		OnBnClickedEnableAuthentication();
		GetDlgItem(IDC_ENABLEAUTH)->EnableWindow(FALSE);
	} 
	else
		GetDlgItem(IDC_ENABLEAUTH)->EnableWindow(TRUE);
}

void CPPgProxy::LoadSettings()
{
	((CButton*)GetDlgItem(IDC_ENABLEPROXY))->SetCheck(proxy.UseProxy);
	((CButton*)GetDlgItem(IDC_ENABLEAUTH))->SetCheck(proxy.EnablePassword);
	((CComboBox*)GetDlgItem(IDC_PROXYTYPE))->SetCurSel(proxy.type);
	SetWindowTextA(*GetDlgItem(IDC_PROXYNAME), proxy.name);
	CString buffer;
	buffer.Format(_T("%u"), proxy.port);
	GetDlgItem(IDC_PROXYPORT)->SetWindowText(buffer);
	SetWindowTextA(*GetDlgItem(IDC_USERNAME_A), proxy.user);
	SetWindowTextA(*GetDlgItem(IDC_PASSWORD), proxy.password);
	OnBnClickedEnableProxy();
}

void CPPgProxy::Localize()
{
	if (m_hWnd)
	{
		SetWindowText(GetResString(IDS_PW_PROXY));
		GetDlgItem(IDC_ENABLEPROXY)->SetWindowText(GetResString(IDS_PROXY_ENABLE));	
		GetDlgItem(IDC_PROXYTYPE_LBL)->SetWindowText(GetResString(IDS_PROXY_TYPE));	
		GetDlgItem(IDC_PROXYNAME_LBL)->SetWindowText(GetResString(IDS_PROXY_HOST));	
		GetDlgItem(IDC_PROXYPORT_LBL)->SetWindowText(GetResString(IDS_PROXY_PORT));	
		GetDlgItem(IDC_ENABLEAUTH)->SetWindowText(GetResString(IDS_PROXY_AUTH));	
		GetDlgItem(IDC_USERNAME_LBL)->SetWindowText(GetResString(IDS_CD_UNAME));	
		GetDlgItem(IDC_PASSWORD_LBL)->SetWindowText(GetResString(IDS_WS_PASS) + _T(":"));	
		GetDlgItem(IDC_AUTH_LBL)->SetWindowText(GetResString(IDS_AUTH));	
		GetDlgItem(IDC_AUTH_LBL2)->SetWindowText(GetResString(IDS_PW_GENERAL));	
	}
}

void CPPgProxy::OnHelp()
{
	theApp.ShowHelp(eMule_FAQ_Preferences_Proxy);
}

BOOL CPPgProxy::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (wParam == ID_HELP)
	{
		OnHelp();
		return TRUE;
	}
	return __super::OnCommand(wParam, lParam);
}

BOOL CPPgProxy::OnHelpInfo(HELPINFO* /*pHelpInfo*/)
{
	OnHelp();
	return TRUE;
}
