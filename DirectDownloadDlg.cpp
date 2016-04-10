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
#include "DirectDownloadDlg.h"
#include "OtherFunctions.h"
#include "emuleDlg.h"
#include "DownloadQueue.h"
#include "ED2KLink.h"
#include "Preferences.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define	PREF_INI_SECTION	_T("DirectDownloadDlg")

IMPLEMENT_DYNAMIC(CDirectDownloadDlg, CDialog)

BEGIN_MESSAGE_MAP(CDirectDownloadDlg, CResizableDialog)
	ON_EN_KILLFOCUS(IDC_ELINK, OnEnKillfocusElink)
	ON_EN_UPDATE(IDC_ELINK, OnEnUpdateElink)
END_MESSAGE_MAP()

CDirectDownloadDlg::CDirectDownloadDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CDirectDownloadDlg::IDD, pParent)
{
	m_icnWnd = NULL;
}

CDirectDownloadDlg::~CDirectDownloadDlg()
{
	if (m_icnWnd)
		VERIFY( DestroyIcon(m_icnWnd) );
}

void CDirectDownloadDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_DDOWN_FRM, m_ctrlDirectDlFrm);
	DDX_Control(pDX, IDC_CATS, m_cattabs);
}

void CDirectDownloadDlg::UpdateControls()
{
	GetDlgItem(IDOK)->EnableWindow(GetDlgItem(IDC_ELINK)->GetWindowTextLength() > 0);
}

void CDirectDownloadDlg::OnEnUpdateElink()
{
	UpdateControls();
}

void CDirectDownloadDlg::OnEnKillfocusElink()
{
	CString strLinks;
	GetDlgItem(IDC_ELINK)->GetWindowText(strLinks);
	if (strLinks.IsEmpty() || strLinks.Find(_T('\n')) == -1)
		return;
	strLinks.Replace(_T("\n"), _T("\r\n"));
	strLinks.Replace(_T("\r\r"), _T("\r"));
	GetDlgItem(IDC_ELINK)->SetWindowText(strLinks);
}

void CDirectDownloadDlg::OnOK()
{
	CString strLinks;
	GetDlgItem(IDC_ELINK)->GetWindowText(strLinks);

	int curPos = 0;
	CString strTok = strLinks.Tokenize(_T(" \t\r\n"), curPos); // tokenize by whitespaces
	while (!strTok.IsEmpty())
	{
		if (strTok.Right(1) != _T("/"))
			strTok += _T("/");
		try
		{
			CED2KLink* pLink = CED2KLink::CreateLinkFromUrl(strTok);
			if (pLink)
			{
				if (pLink->GetKind() == CED2KLink::kFile)
				{
					theApp.downloadqueue->AddFileLinkToDownload(pLink->GetFileLink(), (thePrefs.GetCatCount() == 0) ? 0 : m_cattabs.GetCurSel());
				}
				else
				{
					delete pLink;
					throw CString(_T("bad link"));
				}
				delete pLink;
			}
		}
		catch(CString error)
		{
			TCHAR szBuffer[200];
			_sntprintf(szBuffer, _countof(szBuffer), GetResString(IDS_ERR_INVALIDLINK), error);
			szBuffer[_countof(szBuffer) - 1] = _T('\0');
			CString strError;
			strError.Format(GetResString(IDS_ERR_LINKERROR), szBuffer);
			AfxMessageBox(strError);
			return;
		}
		strTok = strLinks.Tokenize(_T(" \t\r\n"), curPos); // tokenize by whitespaces
	}

	CResizableDialog::OnOK();
}

BOOL CDirectDownloadDlg::OnInitDialog()
{
	CResizableDialog::OnInitDialog();
	InitWindowStyles(this);
	SetIcon(m_icnWnd = theApp.LoadIcon(_T("PasteLink")), FALSE);

	if (theApp.IsVistaThemeActive())
		m_cattabs.ModifyStyle(0, TCS_HOTTRACK);

	AddAnchor(IDC_DDOWN_FRM, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_ELINK, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDC_CATLABEL, BOTTOM_LEFT);
	AddAnchor(IDC_CATS, BOTTOM_LEFT,BOTTOM_RIGHT);

	EnableSaveRestore(PREF_INI_SECTION);

	SetWindowText(GetResString(IDS_SW_DIRECTDOWNLOAD));
	m_ctrlDirectDlFrm.SetIcon(_T("Download"));
	m_ctrlDirectDlFrm.SetWindowText(GetResString(IDS_SW_DIRECTDOWNLOAD));
    GetDlgItem(IDOK)->SetWindowText(GetResString(IDS_DOWNLOAD));
	GetDlgItem(IDC_FSTATIC2)->SetWindowText(GetResString(IDS_SW_LINK));
	GetDlgItem(IDC_CATLABEL)->SetWindowText(GetResString(IDS_CAT)+_T(":"));

	GetDlgItem(IDOK)->SetWindowText(GetResString(IDS_DOWNLOAD));
	GetDlgItem(IDCANCEL)->SetWindowText(GetResString(IDS_CANCEL));
	

	if (thePrefs.GetCatCount()==0) {
		GetDlgItem(IDC_CATLABEL)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_CATS)->ShowWindow(SW_HIDE);
	}
	else {
		UpdateCatTabs();
		if (theApp.m_fontSymbol.m_hObject){
			GetDlgItem(IDC_CATLABEL)->SetFont(&theApp.m_fontSymbol);
			GetDlgItem(IDC_CATLABEL)->SetWindowText(GetExStyle() & WS_EX_LAYOUTRTL ? _T("3") : _T("4")); // show a right-arrow
		}

	}

	UpdateControls();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

void CDirectDownloadDlg::UpdateCatTabs() {
	int oldsel=m_cattabs.GetCurSel();
	m_cattabs.DeleteAllItems();
	for (int ix=0;ix<thePrefs.GetCatCount();ix++) {
		CString label=(ix==0)?GetResString(IDS_ALL):thePrefs.GetCategory(ix)->strTitle;
		label.Replace(_T("&"),_T("&&"));
		m_cattabs.InsertItem(ix,label);
	}
	if (oldsel>=m_cattabs.GetItemCount() || oldsel==-1)
		oldsel=0;

	m_cattabs.SetCurSel(oldsel);
}