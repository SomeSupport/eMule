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
#include "SearchDlg.h"
#include "PPgDisplay.h"
#include <dlgs.h>
#include "HTRichEditCtrl.h"
#include "Preferences.h"
#include "OtherFunctions.h"
#include "emuledlg.h"
#include "TransferDlg.h"
#include "ServerWnd.h"
#include "HelpIDs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MAX_TOOLTIP_DELAY_SEC	32


IMPLEMENT_DYNAMIC(CPPgDisplay, CPropertyPage)

BEGIN_MESSAGE_MAP(CPPgDisplay, CPropertyPage)
	ON_BN_CLICKED(IDC_MINTRAY, OnSettingsChange)
	ON_BN_CLICKED(IDC_DBLCLICK, OnSettingsChange)
	ON_EN_CHANGE(IDC_TOOLTIPDELAY, OnSettingsChange)
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_SHOWRATEONTITLE, OnSettingsChange)
	ON_BN_CLICKED(IDC_DISABLEHIST , OnSettingsChange)
	ON_BN_CLICKED(IDC_DISABLEKNOWNLIST, OnSettingsChange)
	ON_BN_CLICKED(IDC_DISABLEQUEUELIST, OnSettingsChange)
	ON_BN_CLICKED(IDC_SHOWCATINFO, OnSettingsChange)
	ON_BN_CLICKED(IDC_SHOWDWLPERCENT, OnSettingsChange)
	ON_BN_CLICKED(IDC_SELECT_HYPERTEXT_FONT, OnBnClickedSelectHypertextFont)
	ON_BN_CLICKED(IDC_CLEARCOMPL,OnSettingsChange)
	ON_BN_CLICKED(IDC_SHOWTRANSTOOLBAR,OnSettingsChange)
	ON_BN_CLICKED(IDC_STORESEARCHES, OnSettingsChange)
	ON_BN_CLICKED(IDC_WIN7TASKBARGOODIES, OnSettingsChange)
	ON_BN_CLICKED(IDC_RESETHIST, OnBtnClickedResetHist)
	ON_WM_HELPINFO()
END_MESSAGE_MAP()

CPPgDisplay::CPPgDisplay()
	: CPropertyPage(CPPgDisplay::IDD)
{
	m_eSelectFont = sfServer;
}

CPPgDisplay::~CPPgDisplay()
{
}

void CPPgDisplay::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PREVIEW, m_3DPreview);
}

void CPPgDisplay::LoadSettings(void)
{
	if(thePrefs.mintotray)
		CheckDlgButton(IDC_MINTRAY,1);
	else
		CheckDlgButton(IDC_MINTRAY,0);

	if(thePrefs.transferDoubleclick)
		CheckDlgButton(IDC_DBLCLICK,1);
	else
		CheckDlgButton(IDC_DBLCLICK,0);

	if(thePrefs.showRatesInTitle)
		CheckDlgButton(IDC_SHOWRATEONTITLE,1);
	else
		CheckDlgButton(IDC_SHOWRATEONTITLE,0);

	if(thePrefs.m_bDisableKnownClientList)
		CheckDlgButton(IDC_DISABLEKNOWNLIST,1);
	else
		CheckDlgButton(IDC_DISABLEKNOWNLIST,0);

	if(thePrefs.m_bDisableQueueList)
		CheckDlgButton(IDC_DISABLEQUEUELIST,1);
	else
		CheckDlgButton(IDC_DISABLEQUEUELIST,0);

	if(thePrefs.IsStoringSearchesEnabled())
		CheckDlgButton(IDC_STORESEARCHES,1);
	else
		CheckDlgButton(IDC_STORESEARCHES,0);

	CheckDlgButton(IDC_SHOWCATINFO,(UINT)thePrefs.ShowCatTabInfos());
	CheckDlgButton(IDC_SHOWDWLPERCENT,(UINT)thePrefs.GetUseDwlPercentage() );
	CheckDlgButton(IDC_CLEARCOMPL, (uint8)thePrefs.GetRemoveFinishedDownloads());
	CheckDlgButton(IDC_SHOWTRANSTOOLBAR, (uint8)thePrefs.IsTransToolbarEnabled());
	CheckDlgButton(IDC_DISABLEHIST, (uint8)thePrefs.GetUseAutocompletion());
	
#ifndef HAVE_WIN7_SDK_H
	GetDlgItem(IDC_WIN7TASKBARGOODIES)->EnableWindow(FALSE);
#else
	if ( thePrefs.GetWindowsVersion() >= _WINVER_7_)
		CheckDlgButton(IDC_WIN7TASKBARGOODIES, (uint8)thePrefs.IsWin7TaskbarGoodiesEnabled());	
	else
		GetDlgItem(IDC_WIN7TASKBARGOODIES)->EnableWindow(FALSE);
#endif

	SetDlgItemInt(IDC_TOOLTIPDELAY, thePrefs.m_iToolDelayTime, FALSE);
}

BOOL CPPgDisplay::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	InitWindowStyles(this);

	// Barry - Controls depth of 3d colour shading
	CSliderCtrl *slider3D = (CSliderCtrl*)GetDlgItem(IDC_3DDEPTH);
	slider3D->SetRange(0, 5, true);
	slider3D->SetPos(thePrefs.Get3DDepth());
	slider3D->SetTicFreq(1);
	DrawPreview();

	CSpinButtonCtrl *pSpinCtrl = (CSpinButtonCtrl *)GetDlgItem(IDC_TOOLTIPDELAY_SPIN);
	if (pSpinCtrl)
		pSpinCtrl->SetRange(0, MAX_TOOLTIP_DELAY_SEC);

	LoadSettings();
	Localize();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPgDisplay::OnApply()
{
	TCHAR buffer[510];
	
	bool mintotray_old = thePrefs.mintotray;
	thePrefs.mintotray = IsDlgButtonChecked(IDC_MINTRAY)!=0;
	thePrefs.transferDoubleclick = IsDlgButtonChecked(IDC_DBLCLICK)!=0;
	thePrefs.depth3D = ((CSliderCtrl*)GetDlgItem(IDC_3DDEPTH))->GetPos();
	thePrefs.m_bShowDwlPercentage = IsDlgButtonChecked(IDC_SHOWDWLPERCENT)!=0;
	thePrefs.m_bRemoveFinishedDownloads = IsDlgButtonChecked(IDC_CLEARCOMPL)!=0;
	thePrefs.m_bUseAutocompl = IsDlgButtonChecked(IDC_DISABLEHIST)!=0;
	thePrefs.m_bStoreSearches = IsDlgButtonChecked(IDC_STORESEARCHES) != 0;

#ifdef HAVE_WIN7_SDK_H
	thePrefs.m_bShowWin7TaskbarGoodies = IsDlgButtonChecked(IDC_WIN7TASKBARGOODIES) != 0;
	theApp.emuledlg->EnableTaskbarGoodies(thePrefs.m_bShowWin7TaskbarGoodies);
#endif

	if (IsDlgButtonChecked(IDC_SHOWRATEONTITLE))
		thePrefs.showRatesInTitle = true;
	else
		thePrefs.showRatesInTitle = false;

	thePrefs.ShowCatTabInfos(IsDlgButtonChecked(IDC_SHOWCATINFO) != 0);
	if (!thePrefs.ShowCatTabInfos())
		theApp.emuledlg->transferwnd->UpdateCatTabTitles();

	bool bListDisabled = false;
	bool bResetToolbar = false;
	if (thePrefs.m_bDisableKnownClientList != (IsDlgButtonChecked(IDC_DISABLEKNOWNLIST) != 0)) {
		thePrefs.m_bDisableKnownClientList = (IsDlgButtonChecked(IDC_DISABLEKNOWNLIST) != 0);
		if (thePrefs.m_bDisableKnownClientList)
			bListDisabled = true;
		else
			theApp.emuledlg->transferwnd->GetClientList()->ShowKnownClients();
		bResetToolbar = true;
	}

	if (thePrefs.m_bDisableQueueList != (IsDlgButtonChecked(IDC_DISABLEQUEUELIST) != 0)) {
		thePrefs.m_bDisableQueueList = (IsDlgButtonChecked(IDC_DISABLEQUEUELIST) != 0);
		if (thePrefs.m_bDisableQueueList)
			bListDisabled = true;
		else
			theApp.emuledlg->transferwnd->GetQueueList()->ShowQueueClients();
		bResetToolbar = true;
	}

	GetDlgItem(IDC_TOOLTIPDELAY)->GetWindowText(buffer,20);
	if (_tstoi(buffer) > MAX_TOOLTIP_DELAY_SEC)
		thePrefs.m_iToolDelayTime = MAX_TOOLTIP_DELAY_SEC;
	else
		thePrefs.m_iToolDelayTime = _tstoi(buffer);
	theApp.emuledlg->SetToolTipsDelay(thePrefs.GetToolTipDelay()*1000);

	theApp.emuledlg->transferwnd->GetDownloadList()->SetStyle();

	if (bListDisabled)
		theApp.emuledlg->transferwnd->OnDisableList();
	if ((IsDlgButtonChecked(IDC_SHOWTRANSTOOLBAR) != 0) != thePrefs.IsTransToolbarEnabled()) {
		thePrefs.m_bWinaTransToolbar = !thePrefs.m_bWinaTransToolbar;
		theApp.emuledlg->transferwnd->ResetTransToolbar(thePrefs.m_bWinaTransToolbar);
	}
	else if ((IsDlgButtonChecked(IDC_SHOWTRANSTOOLBAR) != 0) && bResetToolbar) {
		theApp.emuledlg->transferwnd->ResetTransToolbar(thePrefs.m_bWinaTransToolbar);
	}

	LoadSettings();

	if (mintotray_old != thePrefs.mintotray)
		theApp.emuledlg->TrayMinimizeToTrayChange();
	if (!thePrefs.ShowRatesOnTitle())
		theApp.emuledlg->SetWindowText(_T("eMule v") + theApp.m_strCurVersionLong);

	SetModified(FALSE);
	return CPropertyPage::OnApply();
}

void CPPgDisplay::Localize(void)
{
	if(m_hWnd)
	{
		SetWindowText(GetResString(IDS_PW_DISPLAY));
		GetDlgItem(IDC_MINTRAY)->SetWindowText(GetResString(IDS_PW_TRAY));
		GetDlgItem(IDC_DBLCLICK)->SetWindowText(GetResString(IDS_PW_DBLCLICK));
		GetDlgItem(IDC_TOOLTIPDELAY_LBL)->SetWindowText(GetResString(IDS_PW_TOOL));
		GetDlgItem(IDC_3DDEP)->SetWindowText(GetResString(IDS_3DDEP));
		GetDlgItem(IDC_FLAT)->SetWindowText(GetResString(IDS_FLAT));
		GetDlgItem(IDC_ROUND)->SetWindowText(GetResString(IDS_ROUND));
		GetDlgItem(IDC_SHOWRATEONTITLE)->SetWindowText(GetResString(IDS_SHOWRATEONTITLE));
		GetDlgItem(IDC_DISABLEKNOWNLIST)->SetWindowText(GetResString(IDS_DISABLEKNOWNLIST));
		GetDlgItem(IDC_DISABLEQUEUELIST)->SetWindowText(GetResString(IDS_DISABLEQUEUELIST));
		GetDlgItem(IDC_STATIC_CPUMEM)->SetWindowText(GetResString(IDS_STATIC_CPUMEM));
		GetDlgItem(IDC_SHOWCATINFO)->SetWindowText(GetResString(IDS_SHOWCATINFO));
		SetDlgItemText(IDC_HYPERTEXT_FONT_HINT, GetResString(IDS_HYPERTEXT_FONT_HINT));
		SetDlgItemText(IDC_SELECT_HYPERTEXT_FONT, GetResString(IDS_SELECT_FONT) + _T("..."));
		SetDlgItemText(IDC_SHOWDWLPERCENT, GetResString(IDS_SHOWDWLPERCENTAGE));
		GetDlgItem(IDC_CLEARCOMPL)->SetWindowText(GetResString(IDS_AUTOREMOVEFD));
		GetDlgItem(IDC_STORESEARCHES)->SetWindowText(GetResString(IDS_STORESEARCHES));

		GetDlgItem(IDC_RESETLABEL)->SetWindowText(GetResString(IDS_RESETLABEL));
		GetDlgItem(IDC_RESETHIST)->SetWindowText(GetResString(IDS_PW_RESET));
		GetDlgItem(IDC_DISABLEHIST)->SetWindowText(GetResString(IDS_ENABLED));

		GetDlgItem(IDC_SHOWTRANSTOOLBAR)->SetWindowText(GetResString(IDS_PW_SHOWTRANSTOOLBAR));
		GetDlgItem(IDC_WIN7TASKBARGOODIES)->SetWindowText(GetResString(IDS_SHOWWIN7TASKBARGOODIES));
	}
}

void CPPgDisplay::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	SetModified(TRUE);

	UpdateData(false); 
	CPropertyPage::OnHScroll(nSBCode, nPos, pScrollBar);

	DrawPreview();
}

// NOTE: Can't use 'lCustData' for a structure which would hold that static members,
// because 's_pfnChooseFontHook' will be needed *before* WM_INITDIALOG (which would
// give as the 'lCustData').
static LPCFHOOKPROC s_pfnChooseFontHook = NULL;
static CPPgDisplay* s_pThis = NULL;

UINT CALLBACK CPPgDisplay::ChooseFontHook(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	UINT uResult;

	// Call MFC's common dialog Hook function
	if (s_pfnChooseFontHook != NULL)
		uResult = (*s_pfnChooseFontHook)(hdlg, uiMsg, wParam, lParam);
	else
		uResult = 0;

	// Do our own Hook processing
	switch (uiMsg)
	{
	case WM_COMMAND:
		if (LOWORD(wParam) == psh3/*Apply*/ && HIWORD(wParam) == BN_CLICKED)
		{
			LOGFONT lf;
			CFontDialog *pDlg = (CFontDialog *)CWnd::FromHandle(hdlg);
			ASSERT( pDlg != NULL );
			if (pDlg != NULL)
			{
				pDlg->GetCurrentFont(&lf);
				if (s_pThis->m_eSelectFont == sfLog)
					theApp.emuledlg->ApplyLogFont(&lf);
				else
					theApp.emuledlg->ApplyHyperTextFont(&lf);
			}
		}
		break;
	}

	// If the hook procedure returns zero, the default dialog box procedure processes the message.
	return uResult;
}

void CPPgDisplay::OnBnClickedSelectHypertextFont()
{
	if (GetAsyncKeyState(VK_CONTROL) < 0)
		m_eSelectFont = sfLog;
	else
		m_eSelectFont = sfServer;

	// get current font description
	CFont* pFont;
	if (m_eSelectFont == sfLog)
		pFont = &theApp.m_fontLog;
	else
		pFont = &theApp.m_fontHyperText;
	LOGFONT lf;
	if (pFont != NULL)
	   pFont->GetObject(sizeof(LOGFONT), &lf);
	else
		AfxGetMainWnd()->GetFont()->GetLogFont(&lf);

	// Initialize 'CFontDialog'
	CFontDialog dlg(&lf, CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT);
	dlg.m_cf.Flags |= CF_APPLY | CF_ENABLEHOOK;

	// Set 'lpfnHook' to our own Hook function. But save MFC's hook!
	s_pfnChooseFontHook = dlg.m_cf.lpfnHook;
	dlg.m_cf.lpfnHook = ChooseFontHook;
	s_pThis = this;

	if (dlg.DoModal() == IDOK)
	{
		if (m_eSelectFont == sfLog)
			theApp.emuledlg->ApplyLogFont(&lf);
		else
			theApp.emuledlg->ApplyHyperTextFont(&lf);
	}

	s_pfnChooseFontHook = NULL;
	s_pThis = NULL;
}

void CPPgDisplay::OnBtnClickedResetHist()
{
	theApp.emuledlg->searchwnd->ResetHistory();
	theApp.emuledlg->serverwnd->ResetHistory();
}

void CPPgDisplay::OnHelp()
{
	theApp.ShowHelp(eMule_FAQ_Preferences_Display);
}

BOOL CPPgDisplay::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (wParam == ID_HELP)
	{
		OnHelp();
		return TRUE;
	}
	return __super::OnCommand(wParam, lParam);
}

BOOL CPPgDisplay::OnHelpInfo(HELPINFO* /*pHelpInfo*/)
{
	OnHelp();
	return TRUE;
}

void CPPgDisplay::DrawPreview()
{
	int dep=((CSliderCtrl*)GetDlgItem(IDC_3DDEPTH))->GetPos();
	m_3DPreview.SetSliderPos( dep);
}
