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
#include "PreferencesDlg.h"
#include "Wizard.h"
#include "emuledlg.h"
#include "StatisticsDlg.h"
#include "opcodes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CConnectionWizardDlg dialog

IMPLEMENT_DYNAMIC(CConnectionWizardDlg, CDialog)

BEGIN_MESSAGE_MAP(CConnectionWizardDlg, CDialog)
	ON_BN_CLICKED(IDC_WIZ_APPLY_BUTTON, OnBnClickedApply)
	ON_BN_CLICKED(IDC_WIZ_CANCEL_BUTTON, OnBnClickedCancel)
	ON_BN_CLICKED(IDC_WIZ_XP_RADIO, OnBnClickedWizRadioOsNtxp)
	ON_BN_CLICKED(IDC_WIZ_ME_RADIO, OnBnClickedWizRadioUs98me)
	ON_BN_CLICKED(IDC_WIZ_LOWDOWN_RADIO, OnBnClickedWizLowdownloadRadio)
	ON_BN_CLICKED(IDC_WIZ_MEDIUMDOWN_RADIO, OnBnClickedWizMediumdownloadRadio)
	ON_BN_CLICKED(IDC_WIZ_HIGHDOWN_RADIO, OnBnClickedWizHighdownloadRadio)
	ON_NOTIFY(NM_CLICK, IDC_PROVIDERS, OnNmClickProviders)
END_MESSAGE_MAP()

CConnectionWizardDlg::CConnectionWizardDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CConnectionWizardDlg::IDD, pParent)
{
	m_iOS = 0;
	m_iTotalDownload = 0;
	m_icnWnd = NULL;
}

CConnectionWizardDlg::~CConnectionWizardDlg()
{
	if (m_icnWnd)
		VERIFY( DestroyIcon(m_icnWnd) );
}

void CConnectionWizardDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PROVIDERS, m_provider);
	DDX_Radio(pDX, IDC_WIZ_XP_RADIO, m_iOS);
	DDX_Radio(pDX, IDC_WIZ_LOWDOWN_RADIO, m_iTotalDownload);
}

void CConnectionWizardDlg::OnBnClickedApply()
{
	if (m_provider.GetSelectionMark() == 0){
		// change the upload/download to unlimited and dont touch other stuff, keep the default values
		thePrefs.maxGraphUploadRate = UNLIMITED;
		thePrefs.maxGraphDownloadRate = 96;
		thePrefs.maxupload = UNLIMITED;
		thePrefs.maxdownload = UNLIMITED;
		theApp.emuledlg->statisticswnd->SetARange(false, thePrefs.GetMaxGraphUploadRate(true));
		theApp.emuledlg->statisticswnd->SetARange(true, thePrefs.maxGraphDownloadRate);
		theApp.emuledlg->preferenceswnd->m_wndConnection.LoadSettings();
		CDialog::OnOK();
		return;
	}

	TCHAR buffer[510];
	int upload, download;
	if (GetDlgItem(IDC_WIZ_TRUEDOWNLOAD_BOX)->GetWindowTextLength())
	{ 
		GetDlgItem(IDC_WIZ_TRUEDOWNLOAD_BOX)->GetWindowText(buffer, 20);
		download = _tstoi(buffer);
	}
	else
	{
		download = 0;
 	}

	if (GetDlgItem(IDC_WIZ_TRUEUPLOAD_BOX)->GetWindowTextLength())
	{ 
		GetDlgItem(IDC_WIZ_TRUEUPLOAD_BOX)->GetWindowText(buffer, 20);
		upload = _tstoi(buffer);
	}
	else
	{
		upload = 0;
	}

	if (IsDlgButtonChecked(IDC_KBITS) == 1)
	{
		upload = (((upload / 8) * 1000) + 512) / 1024;
		download = (((download / 8) * 1000) + 512) / 1024;
	}
	else
	{
		upload = ((upload * 1000) + 512) / 1024;
		download = ((download * 1000) + 512) / 1024;
	}

	thePrefs.maxGraphDownloadRate = download;
	thePrefs.maxGraphUploadRate = upload;

	if (upload > 0 && download > 0)
	{
		thePrefs.maxupload = (uint16)((upload * 4L) / 5);
		if (upload < 4 && download > upload*3) {
			thePrefs.maxdownload = thePrefs.maxupload * 3;
			download = upload * 3;
		}

		if (upload < 10 && download > upload*4) {
			thePrefs.maxdownload = thePrefs.maxupload * 4;
			download = upload * 4;
		}
		else
			thePrefs.maxdownload = (uint16)((download * 9L) / 10);

		theApp.emuledlg->statisticswnd->SetARange(false, thePrefs.maxGraphUploadRate);
		theApp.emuledlg->statisticswnd->SetARange(true, thePrefs.maxGraphDownloadRate);

		if (m_iOS == 1)
			thePrefs.maxconnections = 50;
		else{
			if (upload <= 7)
				thePrefs.maxconnections = 80;
			else if (upload < 12)
				thePrefs.maxconnections = 200;
			else if (upload < 25)
				thePrefs.maxconnections = 400;
			else if (upload < 37)
				thePrefs.maxconnections = 600;
			else
				thePrefs.maxconnections = 800;	
		}
		
		if (m_iOS == 1)
			download = download/2;

		if (download <= 7)
		{
			switch (m_iTotalDownload)
			{
				case 0:
					thePrefs.maxsourceperfile = 100;
					break;
				case 1:
					thePrefs.maxsourceperfile = 60;
					break;
				case 2:
					thePrefs.maxsourceperfile = 40;
					break;
			}
		}
		else if (download < 62)
		{
			switch (m_iTotalDownload)
			{
				case 0:
					thePrefs.maxsourceperfile = 300;
					break;
				case 1:
					thePrefs.maxsourceperfile = 200;
					break;
				case 2:
					thePrefs.maxsourceperfile = 100;
					break;
			}
		}
		else if (download < 187)
		{
			switch (m_iTotalDownload)
			{
				case 0:
					thePrefs.maxsourceperfile = 500;
					break;
				case 1:
					thePrefs.maxsourceperfile = 400;
					break;
				case 2:
					thePrefs.maxsourceperfile = 350;
					break;
			}
		}
		else if (download <= 312)
		{
			switch (m_iTotalDownload)
			{
				case 0:
					thePrefs.maxsourceperfile = 800;
					break;
				case 1:
					thePrefs.maxsourceperfile = 600;
					break;
				case 2:
					thePrefs.maxsourceperfile = 400;
					break;
			}
		}
		else
		{
			switch (m_iTotalDownload)
			{
				case 0:
					thePrefs.maxsourceperfile = 1000;
					break;
				case 1:
					thePrefs.maxsourceperfile = 750;
					break;
				case 2:
					thePrefs.maxsourceperfile = 500;
					break;
			}
		}
	}
	theApp.emuledlg->preferenceswnd->m_wndConnection.LoadSettings();
	CDialog::OnOK();
}

void CConnectionWizardDlg::OnBnClickedCancel()
{
	CDialog::OnCancel();
}

void CConnectionWizardDlg::OnBnClickedWizRadioOsNtxp()
{
	m_iOS = 0;
}

void CConnectionWizardDlg::OnBnClickedWizRadioUs98me()
{
	m_iOS = 1;
}

void CConnectionWizardDlg::OnBnClickedWizLowdownloadRadio()
{
	m_iTotalDownload = 0;
}

void CConnectionWizardDlg::OnBnClickedWizMediumdownloadRadio()
{
	m_iTotalDownload = 1;
}

void CConnectionWizardDlg::OnBnClickedWizHighdownloadRadio()
{
	m_iTotalDownload = 2;
}

void CConnectionWizardDlg::OnBnClickedWizResetButton()
{
	SetDlgItemInt(IDC_WIZ_TRUEDOWNLOAD_BOX, 0, FALSE);
	SetDlgItemInt(IDC_WIZ_TRUEUPLOAD_BOX, 0, FALSE);
}

BOOL CConnectionWizardDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	InitWindowStyles(this);

	SetIcon(m_icnWnd = theApp.LoadIcon(_T("Wizard")), FALSE);

	if (thePrefs.GetWindowsVersion() ==_WINVER_95_ || thePrefs.GetWindowsVersion() ==_WINVER_98_ || thePrefs.GetWindowsVersion() ==_WINVER_ME_){
		CheckRadioButton(IDC_WIZ_XP_RADIO, IDC_WIZ_ME_RADIO, IDC_WIZ_ME_RADIO);
		m_iOS = 1;
	}
	else{
		CheckRadioButton(IDC_WIZ_XP_RADIO, IDC_WIZ_ME_RADIO, IDC_WIZ_XP_RADIO);
		m_iOS = 0;
	}
	CheckRadioButton(IDC_WIZ_LOWDOWN_RADIO, IDC_WIZ_HIGHDOWN_RADIO, IDC_WIZ_LOWDOWN_RADIO);
	CheckRadioButton(IDC_KBITS, IDC_KBYTES, IDC_KBITS);

	SetDlgItemInt(IDC_WIZ_TRUEDOWNLOAD_BOX, 0, FALSE);
	SetDlgItemInt(IDC_WIZ_TRUEUPLOAD_BOX, 0, FALSE);

	m_provider.InsertColumn(0, GetResString(IDS_PW_CONNECTION), LVCFMT_LEFT, 150);
	m_provider.InsertColumn(1, GetResString(IDS_WIZ_DOWN),		LVCFMT_LEFT,  85);
	m_provider.InsertColumn(2, GetResString(IDS_WIZ_UP),		LVCFMT_LEFT,  85);
	m_provider.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

	m_provider.InsertItem(0, GetResString(IDS_UNKNOWN));m_provider.SetItemText(0,1,_T(""));m_provider.SetItemText(0,2,_T(""));
	m_provider.InsertItem(1, GetResString(IDS_WIZARD_CUSTOM));m_provider.SetItemText(1,1,GetResString(IDS_WIZARD_ENTERBELOW));m_provider.SetItemText(1,2,GetResString(IDS_WIZARD_ENTERBELOW));
	m_provider.InsertItem(2,_T("56-k Modem"));m_provider.SetItemText(2,1,_T("56"));m_provider.SetItemText(2,2,_T("56"));
	m_provider.InsertItem(3,_T("ISDN"));m_provider.SetItemText(3,1,_T("64"));m_provider.SetItemText(3,2,_T("64"));
	m_provider.InsertItem(4,_T("ISDN 2x"));m_provider.SetItemText(4,1,_T("128"));m_provider.SetItemText(4,2,_T("128"));

	m_provider.InsertItem(5,_T("T DSL 1000 (T,Arcor,Freenet,1&1)"));m_provider.SetItemText(5,1,_T("1024"));m_provider.SetItemText(5,2,_T("128"));
	m_provider.InsertItem(6,_T("T DSL 1500 (T)"));m_provider.SetItemText(6,1,_T("1536"));m_provider.SetItemText(6,2,_T("192"));
	m_provider.InsertItem(7,_T("T DSL 2000 (T,Arcor,Freenet,Tiscali,Alice)"));m_provider.SetItemText(7,1,_T("2048"));m_provider.SetItemText(7,2,_T("192"));
	m_provider.InsertItem(8,_T("Versatel DSL 2000"));m_provider.SetItemText(8,1,_T("2048"));m_provider.SetItemText(8,2,_T("384"));

	m_provider.InsertItem(9,_T("T-DSL 3000 (T,Arcor)"));m_provider.SetItemText(9,1,_T("3072"));m_provider.SetItemText(9,2,_T("384"));
	m_provider.InsertItem(10,_T("T DSL 6000 (T,Arcor)"));m_provider.SetItemText(10,1,_T("6016"));m_provider.SetItemText(10,2,_T("576"));
	m_provider.InsertItem(11,_T("  DSL 6000 (Tiscali,Freenet,1&1)"));m_provider.SetItemText(11,1,_T("6016"));m_provider.SetItemText(11,2,_T("572"));
	m_provider.InsertItem(12,_T("  DSL 6000 (Lycos,Alice)"));m_provider.SetItemText(12,1,_T("6016"));m_provider.SetItemText(12,2,_T("512"));
	m_provider.InsertItem(13,_T("Versatel DSL 6000"));m_provider.SetItemText(13,1,_T("6144"));m_provider.SetItemText(13,2,_T("512"));

	m_provider.InsertItem(14,_T("Cable"));m_provider.SetItemText(14,1,_T("187"));m_provider.SetItemText(14,2,_T("32"));
	m_provider.InsertItem(15,_T("Cable"));m_provider.SetItemText(15,1,_T("187"));m_provider.SetItemText(15,2,_T("64"));
	m_provider.InsertItem(16,_T("T1"));m_provider.SetItemText(16,1,_T("1500"));m_provider.SetItemText(16,2,_T("1500"));
	m_provider.InsertItem(17,_T("T3+"));m_provider.SetItemText(17,1,_T("44 Mbps"));m_provider.SetItemText(17,2,_T("44 Mbps"));

	m_provider.SetSelectionMark(0);
	m_provider.SetItemState(0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	SetCustomItemsActivation();

	Localize();

	return TRUE;
}

void CConnectionWizardDlg::OnNmClickProviders(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	SetCustomItemsActivation();

	UINT up, down;
	switch (m_provider.GetSelectionMark())
	{
		case  0: down=   0;up=   0; break;
		case  1: down= ((thePrefs.maxGraphDownloadRate * 1024) + 500) / 1000 * 8; up= ((thePrefs.GetMaxGraphUploadRate(true) * 1024) + 500) / 1000 * 8; break;
		case  2: down=   56;	up=   33; break;
		case  3: down=   64;	up=   64; break;
		case  4: down=  128;	up=  128; break;
		case  5: down= 1024;	up=  128; break;
		case  6: down= 1536;	up=  192; break;
		case  7: down= 2048;	up=  192; break;
		case  8: down= 2048;	up=  384; break;
		case  9: down= 3072;	up=  384; break;
		case 10: down= 6016;	up=  576; break;
		case 11: down= 6016;	up=  572; break;
		case 12: down= 6016;	up=  512; break;
		case 13: down= 6144;	up=  512; break;
		case 14: down=  187;	up=   32; break;
		case 15: down=  187;	up=   64; break;
		case 16: down= 1500;	up= 1500; break;
		case 17: down=44000;	up=44000; break;
		default: return;
	}
	
	SetDlgItemInt(IDC_WIZ_TRUEDOWNLOAD_BOX, down, FALSE);
	SetDlgItemInt(IDC_WIZ_TRUEUPLOAD_BOX, up, FALSE);
	CheckRadioButton(IDC_KBITS, IDC_KBYTES, IDC_KBITS);

	*pResult = 0;
}

void CConnectionWizardDlg::Localize()
{
	SetWindowText(GetResString(IDS_WIZARD));
	GetDlgItem(IDC_WIZ_OS_FRAME)->SetWindowText(GetResString(IDS_WIZ_OS_FRAME));
	GetDlgItem(IDC_WIZ_CONCURENTDOWN_FRAME)->SetWindowText(GetResString(IDS_CONCURDWL));
	GetDlgItem(IDC_WIZ_HOTBUTTON_FRAME)->SetWindowText(GetResString(IDS_WIZ_CTFRAME));
	GetDlgItem(IDC_WIZ_TRUEUPLOAD_TEXT)->SetWindowText(GetResString(IDS_WIZ_TRUEUPLOAD_TEXT));
	GetDlgItem(IDC_WIZ_TRUEDOWNLOAD_TEXT)->SetWindowText(GetResString(IDS_WIZ_TRUEDOWNLOAD_TEXT));
	GetDlgItem(IDC_KBITS)->SetWindowText(GetResString(IDS_KBITSSEC));
	GetDlgItem(IDC_KBYTES)->SetWindowText(GetResString(IDS_KBYTESSEC));
	GetDlgItem(IDC_WIZ_APPLY_BUTTON)->SetWindowText(GetResString(IDS_PW_APPLY));
	GetDlgItem(IDC_WIZ_CANCEL_BUTTON)->SetWindowText(GetResString(IDS_CANCEL));
}

void CConnectionWizardDlg::SetCustomItemsActivation()
{
	BOOL bActive = m_provider.GetSelectionMark() == 1;
	GetDlgItem(IDC_WIZ_TRUEUPLOAD_BOX)->EnableWindow(bActive);
	GetDlgItem(IDC_WIZ_TRUEDOWNLOAD_BOX)->EnableWindow(bActive);
	GetDlgItem(IDC_KBITS)->EnableWindow(bActive);
	GetDlgItem(IDC_KBYTES)->EnableWindow(bActive);
}

