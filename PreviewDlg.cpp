//this file is part of eMule
//Copyright (C)2003 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
#include "PreviewDlg.h"
#include "CxImage/xImage.h"
#include "OtherFunctions.h"
#include "SearchFile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNAMIC(PreviewDlg, CDialog)

BEGIN_MESSAGE_MAP(PreviewDlg, CDialog)
	ON_BN_CLICKED(IDC_PV_EXIT, OnBnClickedPvExit)
	ON_BN_CLICKED(IDC_PV_NEXT, OnBnClickedPvNext)
	ON_BN_CLICKED(IDC_PV_PRIOR, OnBnClickedPvPrior)
END_MESSAGE_MAP()

PreviewDlg::PreviewDlg(CWnd* pParent /*=NULL*/)
	: CDialog(PreviewDlg::IDD, pParent)
{
	memset(m_icons, 0, sizeof m_icons);
}

PreviewDlg::~PreviewDlg()
{
	for (int i = 0; i < ARRSIZE(m_icons); i++)
	{
		if (m_icons[i])
			VERIFY( DestroyIcon(m_icons[i]) );
	}
}

void PreviewDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PV_IMAGE, m_ImageStatic);
}

BOOL PreviewDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	if (m_pFile == NULL){
		ASSERT ( false );
		return FALSE;
	}
	InitWindowStyles(this);
	CString title =GetResString(IDS_DL_PREVIEW);
	title.Remove(_T('&'));
	SetWindowText( title + CString(_T(": ")) + m_pFile->GetFileName());

	m_nCurrentImage = 0;
	ShowImage(0);

	((CButton*)GetDlgItem(IDC_PV_EXIT))->SetIcon(m_icons[0] = theApp.LoadIcon(_T("Cancel")));
	((CButton*)GetDlgItem(IDC_PV_NEXT))->SetIcon(m_icons[1] = theApp.LoadIcon(_T("Forward")));
	((CButton*)GetDlgItem(IDC_PV_PRIOR))->SetIcon(m_icons[2] = theApp.LoadIcon(_T("Back")));
	return TRUE;
}

void PreviewDlg::ShowImage(int nNumber)
{
	int nImageCount = m_pFile->GetPreviews().GetSize();
	if (nImageCount == 0)
		return;
	else if (nImageCount <= nNumber)
		nNumber = 0;
	else if (nNumber < 0)
		nNumber = nImageCount-1;

	m_nCurrentImage = nNumber;
	HBITMAP hbitmap = m_ImageStatic.SetBitmap(m_pFile->GetPreviews()[nNumber]->MakeBitmap(m_ImageStatic.GetDC()->m_hDC));
	if (hbitmap)
		DeleteObject(hbitmap);
	CString strInfo;
	strInfo.Format(_T("Image %u of %u"), nNumber+1, nImageCount);
	SetDlgItemText(IDC_PREVIEW_INFO, strInfo);
}

void PreviewDlg::Show(){
	Create(IDD_PREVIEWDIALOG, NULL);
}

// PreviewDlg message handlers

void PreviewDlg::OnBnClickedPvExit()
{
	OnClose();
}

void PreviewDlg::OnBnClickedPvNext()
{
	ShowImage(m_nCurrentImage+1);
}

void PreviewDlg::OnBnClickedPvPrior()
{
	ShowImage(m_nCurrentImage-1);
}

void PreviewDlg::OnClose(){
	HBITMAP hbitmap = m_ImageStatic.SetBitmap(NULL);
	if (hbitmap)
		DeleteObject(hbitmap);
	CDialog::OnClose();
	delete this;
}
