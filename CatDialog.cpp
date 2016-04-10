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
#include "CustomAutoComplete.h"
#include "Preferences.h"
#include "otherfunctions.h"
#include "SharedFileList.h"
#include "emuledlg.h"
#include "TransferDlg.h"
#include "CatDialog.h"
#include "UserMsgs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define	REGULAREXPRESSIONS_STRINGS_PROFILE	_T("AC_VF_RegExpr.dat")

// CCatDialog dialog

IMPLEMENT_DYNAMIC(CCatDialog, CDialog)

BEGIN_MESSAGE_MAP(CCatDialog, CDialog)
	ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDC_REB, OnDDBnClicked)
	ON_MESSAGE(UM_CPN_SELENDOK, OnSelChange) //UM_CPN_SELCHANGE
END_MESSAGE_MAP()

CCatDialog::CCatDialog(int index)
	: CDialog(CCatDialog::IDD)
{
	m_myCat = thePrefs.GetCategory(index);
	if (m_myCat == NULL)
		return;
	m_pacRegExp=NULL;
	newcolor = (DWORD)-1;
}

CCatDialog::~CCatDialog()
{
	if (m_pacRegExp){
		m_pacRegExp->SaveList(thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + REGULAREXPRESSIONS_STRINGS_PROFILE);
		m_pacRegExp->Unbind();
		m_pacRegExp->Release();
	}
}

BOOL CCatDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	InitWindowStyles(this);
	Localize();
	m_ctlColor.SetDefaultColor(GetSysColor(COLOR_BTNTEXT));
	UpdateData();

	if (!thePrefs.IsExtControlsEnabled()) {
		GetDlgItem(IDC_REGEXPR)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_STATIC_REGEXP)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_REGEXP)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_REB)->ShowWindow(SW_HIDE);
	}

	m_pacRegExp = new CCustomAutoComplete();
	m_pacRegExp->AddRef();
	if (m_pacRegExp->Bind(::GetDlgItem(m_hWnd, IDC_REGEXP), ACO_UPDOWNKEYDROPSLIST | ACO_AUTOSUGGEST)) {
		m_pacRegExp->LoadList(thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + REGULAREXPRESSIONS_STRINGS_PROFILE);
	}
	if (theApp.m_fontSymbol.m_hObject){
		GetDlgItem(IDC_REB)->SetFont(&theApp.m_fontSymbol);
		GetDlgItem(IDC_REB)->SetWindowText(_T("6")); // show a down-arrow
	}

	return TRUE;
}

void CCatDialog::UpdateData()
{
	GetDlgItem(IDC_TITLE)->SetWindowText(m_myCat->strTitle);
	GetDlgItem(IDC_INCOMING)->SetWindowText(m_myCat->strIncomingPath);
	GetDlgItem(IDC_COMMENT)->SetWindowText(m_myCat->strComment);

	if (m_myCat->filter==18)
		SetDlgItemText(IDC_REGEXP,m_myCat->regexp);

	CheckDlgButton(IDC_REGEXPR,m_myCat->ac_regexpeval);

	newcolor = m_myCat->color;
	m_ctlColor.SetColor(m_myCat->color == -1 ? m_ctlColor.GetDefaultColor() : m_myCat->color);
	
	GetDlgItem(IDC_AUTOCATEXT)->SetWindowText(m_myCat->autocat);

	m_prio.SetCurSel(m_myCat->prio);
}

void CCatDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CATCOLOR, m_ctlColor);
	DDX_Control(pDX, IDC_PRIOCOMBO, m_prio);
}

void CCatDialog::Localize()
{
	GetDlgItem(IDC_STATIC_TITLE)->SetWindowText(GetResString(IDS_TITLE));
	GetDlgItem(IDC_STATIC_INCOMING)->SetWindowText(GetResString(IDS_PW_INCOMING) + _T("  ") + GetResString(IDS_SHAREWARNING) );
	GetDlgItem(IDC_STATIC_COMMENT)->SetWindowText(GetResString(IDS_COMMENT));
	GetDlgItem(IDCANCEL)->SetWindowText(GetResString(IDS_CANCEL));
	GetDlgItem(IDC_STATIC_COLOR)->SetWindowText(GetResString(IDS_COLOR));
	GetDlgItem(IDC_STATIC_PRIO)->SetWindowText(GetResString(IDS_STARTPRIO));
	GetDlgItem(IDC_STATIC_AUTOCAT)->SetWindowText(GetResString(IDS_AUTOCAT_LABEL));
	GetDlgItem(IDC_REGEXPR)->SetWindowText(GetResString(IDS_ASREGEXPR));
	GetDlgItem(IDOK)->SetWindowText(GetResString(IDS_TREEOPTIONS_OK));

	m_ctlColor.CustomText = GetResString(IDS_COL_MORECOLORS);
	m_ctlColor.DefaultText = GetResString(IDS_DEFAULT);

	SetWindowText(GetResString(IDS_EDITCAT));

	SetDlgItemText(IDC_STATIC_REGEXP,GetResString(IDS_STATIC_REGEXP));	

	m_prio.ResetContent();
	m_prio.AddString(GetResString(IDS_PRIOLOW));
	m_prio.AddString(GetResString(IDS_PRIONORMAL));
	m_prio.AddString(GetResString(IDS_PRIOHIGH));
	m_prio.SetCurSel(m_myCat->prio);
}

void CCatDialog::OnBnClickedBrowse()
{	
	TCHAR buffer[MAX_PATH] = {0};
	GetDlgItemText(IDC_INCOMING, buffer, _countof(buffer));
	if (SelectDir(GetSafeHwnd(), buffer,GetResString(IDS_SELECT_INCOMINGDIR)))
		GetDlgItem(IDC_INCOMING)->SetWindowText(buffer);
}

void CCatDialog::OnBnClickedOk()
{
	CString oldpath = m_myCat->strIncomingPath;
	if (GetDlgItem(IDC_TITLE)->GetWindowTextLength()>0)
		GetDlgItem(IDC_TITLE)->GetWindowText(m_myCat->strTitle);
	
	if (GetDlgItem(IDC_INCOMING)->GetWindowTextLength()>2)
		GetDlgItem(IDC_INCOMING)->GetWindowText(m_myCat->strIncomingPath);
	
	GetDlgItem(IDC_COMMENT)->GetWindowText(m_myCat->strComment);

	m_myCat->ac_regexpeval= IsDlgButtonChecked(IDC_REGEXPR)>0;

	MakeFoldername(m_myCat->strIncomingPath);
	if (!thePrefs.IsShareableDirectory(m_myCat->strIncomingPath)){
		m_myCat->strIncomingPath = thePrefs.GetMuleDirectory(EMULE_INCOMINGDIR);
		MakeFoldername(m_myCat->strIncomingPath);
	}

	if (!PathFileExists(m_myCat->strIncomingPath)){
		if (!::CreateDirectory(m_myCat->strIncomingPath, 0)){
			AfxMessageBox(GetResString(IDS_ERR_BADFOLDER));
			m_myCat->strIncomingPath = oldpath;
			return;
		}
	}

	if (m_myCat->strIncomingPath.CompareNoCase(oldpath)!=0)
		theApp.sharedfiles->Reload();

	m_myCat->color=newcolor;
    m_myCat->prio=m_prio.GetCurSel();
	GetDlgItem(IDC_AUTOCATEXT)->GetWindowText(m_myCat->autocat);

	GetDlgItemText(IDC_REGEXP,m_myCat->regexp);
	if (m_myCat->regexp.GetLength()>0) {
		if (m_pacRegExp && m_pacRegExp->IsBound()){
			m_pacRegExp->AddItem(m_myCat->regexp,0);
			m_myCat->filter=18;
		}
	} else if (m_myCat->filter==18) {
		// deactivate regexp
		m_myCat->filter=0;
	}

	theApp.emuledlg->transferwnd->GetDownloadList()->Invalidate();

	OnOK();
}

LONG CCatDialog::OnSelChange(UINT lParam, LONG /*wParam*/)
{
	if (lParam == CLR_DEFAULT)
		newcolor = (DWORD)-1;
	else
		newcolor = m_ctlColor.GetColor();
	return TRUE;
}

void CCatDialog::OnDDBnClicked()
{
	CWnd* box = GetDlgItem(IDC_REGEXP);
	box->SetFocus();
	box->SetWindowText(_T(""));
	box->SendMessage(WM_KEYDOWN, VK_DOWN, 0x00510001);
}
