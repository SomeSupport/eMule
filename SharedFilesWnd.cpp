//this file is part of eMule
//Copyright (C)2002-2005 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
#include "emuleDlg.h"
#include "SharedFilesWnd.h"
#include "OtherFunctions.h"
#include "SharedFileList.h"
#include "KnownFileList.h"
#include "KnownFile.h"
#include "UserMsgs.h"
#include "HelpIDs.h"
#include "HighColorTab.hpp"
#include "filedetaildlgstatistics.h"
#include "ED2kLinkDlg.h"
#include "FileInfoDialog.h"
#include "ArchivePreviewDlg.h"
#include "MetaDataDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define	SPLITTER_RANGE_MIN		100
#define	SPLITTER_RANGE_MAX		350

#define	SPLITTER_MARGIN			0
#define	SPLITTER_WIDTH			4


// CSharedFilesWnd dialog

IMPLEMENT_DYNAMIC(CSharedFilesWnd, CDialog)

BEGIN_MESSAGE_MAP(CSharedFilesWnd, CResizableDialog)
	ON_BN_CLICKED(IDC_RELOADSHAREDFILES, OnBnClickedReloadSharedFiles)
	ON_MESSAGE(UM_DELAYED_EVALUATE, OnChangeFilter)
	ON_NOTIFY(LVN_ITEMACTIVATE, IDC_SFLIST, OnLvnItemActivateSharedFiles)
	ON_NOTIFY(NM_CLICK, IDC_SFLIST, OnNmClickSharedFiles)
	ON_NOTIFY(TVN_SELCHANGED, IDC_SHAREDDIRSTREE, OnTvnSelChangedSharedDirsTree)
	ON_STN_DBLCLK(IDC_FILES_ICO, OnStnDblClickFilesIco)
	ON_WM_CTLCOLOR()
	ON_WM_HELPINFO()
	ON_WM_SYSCOLORCHANGE()
	ON_BN_CLICKED(IDC_SF_HIDESHOWDETAILS, OnBnClickedSfHideshowdetails)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_SFLIST, OnLvnItemchangedSflist)
	ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()

CSharedFilesWnd::CSharedFilesWnd(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CSharedFilesWnd::IDD, pParent)
{
	icon_files = NULL;
	m_nFilterColumn = 0;
	m_bDetailsVisible = true;
}

CSharedFilesWnd::~CSharedFilesWnd()
{
	m_ctlSharedListHeader.Detach();
	if (icon_files)
		VERIFY( DestroyIcon(icon_files) );
}

void CSharedFilesWnd::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SFLIST, sharedfilesctrl);
	DDX_Control(pDX, IDC_SHAREDDIRSTREE, m_ctlSharedDirTree);
	DDX_Control(pDX, IDC_SHAREDFILES_FILTER, m_ctlFilter);
}

BOOL CSharedFilesWnd::OnInitDialog()
{
	CResizableDialog::OnInitDialog();
	InitWindowStyles(this);
	SetAllIcons();
	sharedfilesctrl.Init();
	m_ctlSharedDirTree.Initalize(&sharedfilesctrl);
	if (thePrefs.GetUseSystemFontForMainControls())
		m_ctlSharedDirTree.SendMessage(WM_SETFONT, NULL, FALSE);

	m_ctlSharedListHeader.Attach(sharedfilesctrl.GetHeaderCtrl()->Detach());
	CArray<int, int> aIgnore; // ignored no-text columns for filter edit
	aIgnore.Add(8); // shared parts
	aIgnore.Add(11); // shared ed2k/kad
	m_ctlFilter.OnInit(&m_ctlSharedListHeader, &aIgnore);

	CRect rcSpl;
	m_ctlSharedDirTree.GetWindowRect(rcSpl);
	ScreenToClient(rcSpl);

	CRect rcFiles;
	sharedfilesctrl.GetWindowRect(rcFiles);
	ScreenToClient(rcFiles);
	VERIFY( m_dlgDetails.Create(this, DS_CONTROL | DS_SETFONT | WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, WS_EX_CONTROLPARENT) );
	m_dlgDetails.SetWindowPos(NULL, rcFiles.left - 6, rcFiles.bottom - 2, rcFiles.Width() + 12, rcSpl.bottom + 7 - (rcFiles.bottom - 2), 0);
	AddAnchor(m_dlgDetails, BOTTOM_LEFT, BOTTOM_RIGHT);

	rcSpl.left = rcSpl.right + SPLITTER_MARGIN;
	rcSpl.right = rcSpl.left + SPLITTER_WIDTH;
	m_wndSplitter.Create(WS_CHILD | WS_VISIBLE, rcSpl, this, IDC_SPLITTER_SHAREDFILES);
	
	AddAnchor(IDC_SF_HIDESHOWDETAILS, BOTTOM_RIGHT);
	AddAnchor(m_wndSplitter, TOP_LEFT);
	AddAnchor(sharedfilesctrl, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(m_ctlSharedDirTree, TOP_LEFT, BOTTOM_LEFT);
	AddAnchor(m_ctlFilter, TOP_LEFT);
	AddAnchor(IDC_FILES_ICO, TOP_LEFT);
	AddAnchor(IDC_RELOADSHAREDFILES, TOP_RIGHT);
	AddAnchor(IDC_TRAFFIC_TEXT, TOP_LEFT);
	
	int iPosStatInit = rcSpl.left;
	int iPosStatNew = thePrefs.GetSplitterbarPositionShared();
	if (iPosStatNew > SPLITTER_RANGE_MAX)
		iPosStatNew = SPLITTER_RANGE_MAX;
	else if (iPosStatNew < SPLITTER_RANGE_MIN)
		iPosStatNew = SPLITTER_RANGE_MIN;
	rcSpl.left = iPosStatNew;
	rcSpl.right = iPosStatNew + SPLITTER_WIDTH;
	if (iPosStatNew != iPosStatInit)
	{
		m_wndSplitter.MoveWindow(rcSpl);
		DoResize(iPosStatNew - iPosStatInit);
	}

	GetDlgItem(IDC_SF_HIDESHOWDETAILS)->SetFont(&theApp.m_fontSymbol);
	GetDlgItem(IDC_SF_HIDESHOWDETAILS)->SetWindowText(_T("6"));
	GetDlgItem(IDC_SF_HIDESHOWDETAILS)->BringWindowToTop();
	ShowDetailsPanel(thePrefs.GetShowSharedFilesDetails());
	Localize();
	return TRUE;
}

void CSharedFilesWnd::DoResize(int iDelta)
{
	CSplitterControl::ChangeWidth(&m_ctlSharedDirTree, iDelta);
	CSplitterControl::ChangeWidth(&m_ctlFilter, iDelta);
	CSplitterControl::ChangePos(&sharedfilesctrl, -iDelta, 0);
	CSplitterControl::ChangeWidth(&sharedfilesctrl, -iDelta);
	bool bAntiFlicker = m_dlgDetails.IsWindowVisible() == TRUE;
	if (bAntiFlicker)
		m_dlgDetails.SetRedraw(FALSE);
	CSplitterControl::ChangePos(&m_dlgDetails, -iDelta, 0);
	CSplitterControl::ChangeWidth(&m_dlgDetails, -iDelta);
	if (bAntiFlicker)
		m_dlgDetails.SetRedraw(TRUE);

	CRect rcSpl;
	m_wndSplitter.GetWindowRect(rcSpl);
	ScreenToClient(rcSpl);
	thePrefs.SetSplitterbarPositionShared(rcSpl.left);

	RemoveAnchor(m_wndSplitter);
	AddAnchor(m_wndSplitter, TOP_LEFT);
	RemoveAnchor(sharedfilesctrl);
	RemoveAnchor(m_ctlSharedDirTree);
	RemoveAnchor(m_ctlFilter);
	RemoveAnchor(m_dlgDetails);

	AddAnchor(sharedfilesctrl, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(m_ctlSharedDirTree, TOP_LEFT, BOTTOM_LEFT);
	AddAnchor(m_ctlFilter, TOP_LEFT);
	AddAnchor(m_dlgDetails, BOTTOM_LEFT, BOTTOM_RIGHT);

	CRect rcWnd;
	GetWindowRect(rcWnd);
	ScreenToClient(rcWnd);
	m_wndSplitter.SetRange(rcWnd.left + SPLITTER_RANGE_MIN, rcWnd.left + SPLITTER_RANGE_MAX);

	Invalidate();
	UpdateWindow();
}


void CSharedFilesWnd::Reload(bool bForceTreeReload)
{	
	sharedfilesctrl.SetDirectoryFilter(NULL, false);
	m_ctlSharedDirTree.Reload(bForceTreeReload); // force a reload of the tree to update the 'accessible' state of each directory
	sharedfilesctrl.SetDirectoryFilter(m_ctlSharedDirTree.GetSelectedFilter(), false);
	theApp.sharedfiles->Reload();

	ShowSelectedFilesDetails();
}

void CSharedFilesWnd::OnStnDblClickFilesIco()
{
	theApp.emuledlg->ShowPreferences(IDD_PPG_DIRECTORIES);
}

void CSharedFilesWnd::OnBnClickedReloadSharedFiles()
{
	CWaitCursor curWait;
#ifdef _DEBUG
	if (GetAsyncKeyState(VK_CONTROL) < 0) {
		theApp.sharedfiles->RebuildMetaData();
		sharedfilesctrl.Invalidate();
		sharedfilesctrl.UpdateWindow();
		return;
	}
#endif
	Reload(true);
}

void CSharedFilesWnd::OnLvnItemActivateSharedFiles(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	ShowSelectedFilesDetails();
}

void CSharedFilesWnd::OnNmClickSharedFiles(NMHDR *pNMHDR, LRESULT *pResult)
{
	OnLvnItemActivateSharedFiles(pNMHDR, pResult);
	*pResult = 0;
}

BOOL CSharedFilesWnd::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		// Don't handle Ctrl+Tab in this window. It will be handled by main window.
		if (pMsg->wParam == VK_TAB && GetAsyncKeyState(VK_CONTROL) < 0)
			return FALSE;
		if (pMsg->wParam == VK_ESCAPE)
			return FALSE;
	}
	else if (pMsg->message == WM_KEYUP)
	{
		if (pMsg->hwnd == sharedfilesctrl.m_hWnd)
			OnLvnItemActivateSharedFiles(0, 0);
	}
	else if (!thePrefs.GetStraightWindowStyles() && pMsg->message == WM_MBUTTONUP)
	{
		POINT point;
		::GetCursorPos(&point);
		CPoint p = point; 
		sharedfilesctrl.ScreenToClient(&p); 
		int it = sharedfilesctrl.HitTest(p); 
		if (it == -1)
			return FALSE;

		sharedfilesctrl.SetItemState(-1, 0, LVIS_SELECTED);
		sharedfilesctrl.SetItemState(it, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		sharedfilesctrl.SetSelectionMark(it);   // display selection mark correctly!
		sharedfilesctrl.ShowComments((CKnownFile*)sharedfilesctrl.GetItemData(it));
		return TRUE;
	}

	return CResizableDialog::PreTranslateMessage(pMsg);
}

void CSharedFilesWnd::OnSysColorChange()
{
	CResizableDialog::OnSysColorChange();
	SetAllIcons();
}

void CSharedFilesWnd::SetAllIcons()
{
	if (icon_files)
		VERIFY( DestroyIcon(icon_files) );
	icon_files = theApp.LoadIcon(_T("SharedFilesList"), 16, 16);
	((CStatic*)GetDlgItem(IDC_FILES_ICO))->SetIcon(icon_files);
}

void CSharedFilesWnd::Localize()
{
	sharedfilesctrl.Localize();
	m_ctlSharedDirTree.Localize();
	m_ctlFilter.ShowColumnText(true);
	sharedfilesctrl.SetDirectoryFilter(NULL,true);

	GetDlgItem(IDC_RELOADSHAREDFILES)->SetWindowText(GetResString(IDS_SF_RELOAD));
}

void CSharedFilesWnd::OnTvnSelChangedSharedDirsTree(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	sharedfilesctrl.SetDirectoryFilter(m_ctlSharedDirTree.GetSelectedFilter(), !m_ctlSharedDirTree.IsCreatingTree());
	*pResult = 0;
}

LRESULT CSharedFilesWnd::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	switch (message)
	{
		case WM_PAINT:
			if (m_wndSplitter)
			{
				CRect rcWnd;
				GetWindowRect(rcWnd);
				if (rcWnd.Width() > 0)
				{
					CRect rcSpl;
					m_ctlSharedDirTree.GetWindowRect(rcSpl);
					ScreenToClient(rcSpl);
					rcSpl.left = rcSpl.right + SPLITTER_MARGIN;
					rcSpl.right = rcSpl.left + SPLITTER_WIDTH;

					CRect rcFilter;
					m_ctlFilter.GetWindowRect(rcFilter);
					ScreenToClient(rcFilter);
					rcSpl.top = rcFilter.top;
					m_wndSplitter.MoveWindow(rcSpl, TRUE);
				}
			}
			break;

		case WM_NOTIFY:
			if (wParam == IDC_SPLITTER_SHAREDFILES)
			{ 
				SPC_NMHDR* pHdr = (SPC_NMHDR*)lParam;
				DoResize(pHdr->delta);
			}
			break;

		case WM_SIZE:
			if (m_wndSplitter)
			{
				CRect rcWnd;
				GetWindowRect(rcWnd);
				ScreenToClient(rcWnd);
				m_wndSplitter.SetRange(rcWnd.left + SPLITTER_RANGE_MIN, rcWnd.left + SPLITTER_RANGE_MAX);
			}
			break;
	}
	return CResizableDialog::DefWindowProc(message, wParam, lParam);
}

LRESULT CSharedFilesWnd::OnChangeFilter(WPARAM wParam, LPARAM lParam)
{
	CWaitCursor curWait; // this may take a while

	bool bColumnDiff = (m_nFilterColumn != (uint32)wParam);
	m_nFilterColumn = (uint32)wParam;

	CStringArray astrFilter;
	CString strFullFilterExpr = (LPCTSTR)lParam;
	int iPos = 0;
	CString strFilter(strFullFilterExpr.Tokenize(_T(" "), iPos));
	while (!strFilter.IsEmpty()) {
		if (strFilter != _T("-"))
			astrFilter.Add(strFilter);
		strFilter = strFullFilterExpr.Tokenize(_T(" "), iPos);
	}

	bool bFilterDiff = (astrFilter.GetSize() != m_astrFilter.GetSize());
	if (!bFilterDiff) {
		for (int i = 0; i < astrFilter.GetSize(); i++) {
			if (astrFilter[i] != m_astrFilter[i]) {
				bFilterDiff = true;
				break;
			}
		}
	}

	if (!bColumnDiff && !bFilterDiff)
		return 0;
	m_astrFilter.RemoveAll();
	m_astrFilter.Append(astrFilter);

	sharedfilesctrl.ReloadFileList();
	return 0;
}

BOOL CSharedFilesWnd::OnHelpInfo(HELPINFO* /*pHelpInfo*/)
{
	theApp.ShowHelp(eMule_FAQ_GUI_SharedFiles);
	return TRUE;
}

HBRUSH CSharedFilesWnd::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = theApp.emuledlg->GetCtlColor(pDC, pWnd, nCtlColor);
	if (hbr)
		return hbr;
	return __super::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CSharedFilesWnd::SetToolTipsDelay(DWORD dwDelay)
{
	sharedfilesctrl.SetToolTipsDelay(dwDelay);
}

void CSharedFilesWnd::ShowSelectedFilesDetails(bool bForce)
{
	CTypedPtrList<CPtrList, CShareableFile*> selectedList;
	bool m_bChanged = false;
	if (m_bDetailsVisible)
	{
		POSITION pos = sharedfilesctrl.GetFirstSelectedItemPosition();
		int i = 0;
		while (pos != NULL){
			int index = sharedfilesctrl.GetNextSelectedItem(pos);
			if (index >= 0)
			{
				CShareableFile* file = (CShareableFile*)sharedfilesctrl.GetItemData(index);
				if (file != NULL)
				{
					selectedList.AddTail(file);
					m_bChanged |= m_dlgDetails.GetItems().GetSize() <= i || m_dlgDetails.GetItems()[i] != file;
					i++;
				}
			}
		}
	}
	m_bChanged |= m_dlgDetails.GetItems().GetSize() != selectedList.GetCount();

	if (m_bChanged || bForce)
		m_dlgDetails.SetFiles(selectedList);
}
void CSharedFilesWnd::ShowDetailsPanel(bool bShow)
{
	if (bShow == m_bDetailsVisible)
		return;
	m_bDetailsVisible = bShow;
	thePrefs.SetShowSharedFilesDetails(bShow);
	RemoveAnchor(sharedfilesctrl);
	RemoveAnchor(IDC_SF_HIDESHOWDETAILS);
	
	CRect rcFile, rcDetailDlg, rcButton;
	sharedfilesctrl.GetWindowRect(rcFile);
	m_dlgDetails.GetWindowRect(rcDetailDlg);
	GetDlgItem(IDC_SF_HIDESHOWDETAILS)->GetWindowRect(rcButton);
	ScreenToClient(rcButton);
	const int nOffset = 29;
	if (!bShow)
	{
		sharedfilesctrl.SetWindowPos(NULL, 0, 0, rcFile.Width(), rcFile.Height() + (rcDetailDlg.Height() - nOffset), SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOACTIVATE);
		GetDlgItem(IDC_SF_HIDESHOWDETAILS)->SetWindowPos(NULL, rcButton.left, rcButton.top + (rcDetailDlg.Height() - nOffset), 0, 0, SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOACTIVATE);
		m_dlgDetails.ShowWindow(SW_HIDE);
		GetDlgItem(IDC_SF_HIDESHOWDETAILS)->SetWindowText(_T("5"));
	}
	else
	{
		sharedfilesctrl.SetWindowPos(NULL, 0, 0, rcFile.Width(), rcFile.Height() - (rcDetailDlg.Height() - nOffset), SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOACTIVATE);
		GetDlgItem(IDC_SF_HIDESHOWDETAILS)->SetWindowPos(NULL, rcButton.left, rcButton.top - (rcDetailDlg.Height() - nOffset), 0, 0, SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOACTIVATE);
		m_dlgDetails.ShowWindow(SW_SHOW);
		GetDlgItem(IDC_SF_HIDESHOWDETAILS)->SetWindowText(_T("6"));
	}
	sharedfilesctrl.SetFocus();
	AddAnchor(sharedfilesctrl, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_SF_HIDESHOWDETAILS, BOTTOM_RIGHT);
	ShowSelectedFilesDetails();
}

void CSharedFilesWnd::OnBnClickedSfHideshowdetails()
{
	ShowDetailsPanel(!m_bDetailsVisible);
}

void CSharedFilesWnd::OnLvnItemchangedSflist(NMHDR* /*pNMHDR*/, LRESULT *pResult)
{
	ShowSelectedFilesDetails();
	*pResult = 0;
}

void CSharedFilesWnd::OnShowWindow(BOOL bShow, UINT /*nStatus*/)
{
	if (bShow)
		ShowSelectedFilesDetails(true);
}


/////////////////////////////////////////////////////////////////////////////////////////////
// CSharedFileDetailsModelessSheet
IMPLEMENT_DYNAMIC(CSharedFileDetailsModelessSheet, CListViewPropertySheet)

BEGIN_MESSAGE_MAP(CSharedFileDetailsModelessSheet, CListViewPropertySheet)
	ON_MESSAGE(UM_DATA_CHANGED, OnDataChanged)
	ON_WM_CREATE()
END_MESSAGE_MAP()

int CSharedFileDetailsModelessSheet::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	// skip CResizableSheet::OnCreate because we don't the styles and stuff which are set there
	CreateSizeGrip(FALSE); // create grip but dont show it
	return CPropertySheet::OnCreate(lpCreateStruct);
}

bool NeedArchiveInfoPage(const CSimpleArray<CObject*>* paItems);
void UpdateFileDetailsPages(CListViewPropertySheet *pSheet,
							CResizablePage *pArchiveInfo, CResizablePage *pMediaInfo);

CSharedFileDetailsModelessSheet::CSharedFileDetailsModelessSheet()
{
	m_psh.dwFlags &= ~PSH_HASHELP;
	m_psh.dwFlags |= PSH_MODELESS;
	m_wndStatistics = new CFileDetailDlgStatistics();
	m_wndFileLink = new CED2kLinkDlg();
	m_wndArchiveInfo = new CArchivePreviewDlg();
	m_wndMediaInfo = new CFileInfoDialog();
	m_wndMetaData = NULL;

	m_wndStatistics->m_psp.dwFlags &= ~PSP_HASHELP;
	m_wndStatistics->m_psp.dwFlags |= PSP_USEICONID;
	m_wndStatistics->m_psp.pszIcon = _T("StatsDetail");
	m_wndStatistics->SetFiles(&m_aItems);
	AddPage(m_wndStatistics);

	m_wndArchiveInfo->m_psp.dwFlags &= ~PSP_HASHELP;
	m_wndArchiveInfo->m_psp.dwFlags |= PSP_USEICONID;
	m_wndArchiveInfo->m_psp.pszIcon = _T("ARCHIVE_PREVIEW");
	m_wndArchiveInfo->SetReducedDialog();
	m_wndArchiveInfo->SetFiles(&m_aItems);
	m_wndMediaInfo->m_psp.dwFlags &= ~PSP_HASHELP;
	m_wndMediaInfo->m_psp.dwFlags |= PSP_USEICONID;
	m_wndMediaInfo->m_psp.pszIcon = _T("MEDIAINFO");
	m_wndMediaInfo->SetReducedDialog();
	m_wndMediaInfo->SetFiles(&m_aItems);
	if (NeedArchiveInfoPage(&m_aItems))
		AddPage(m_wndArchiveInfo);
	else
		AddPage(m_wndMediaInfo);

	m_wndFileLink->m_psp.dwFlags &= ~PSP_HASHELP;
	m_wndFileLink->m_psp.dwFlags |= PSP_USEICONID;
	m_wndFileLink->m_psp.pszIcon = _T("ED2KLINK");
	m_wndFileLink->SetReducedDialog();
	m_wndFileLink->SetFiles(&m_aItems);
	AddPage(m_wndFileLink);

	if (thePrefs.IsExtControlsEnabled())
	{
		m_wndMetaData = new CMetaDataDlg();
		m_wndMetaData->m_psp.dwFlags &= ~PSP_HASHELP;
		m_wndMetaData->m_psp.dwFlags |= PSP_USEICONID;
		m_wndMetaData->m_psp.pszIcon = _T("METADATA");
		m_wndMetaData->SetFiles(&m_aItems);
		AddPage(m_wndMetaData);
	}

	/*LPCTSTR pPshStartPage = m_pPshStartPage;
	if (m_uPshInvokePage != 0)
		pPshStartPage = MAKEINTRESOURCE(m_uPshInvokePage);
	for (int i = 0; i < m_pages.GetSize(); i++)
	{
		CPropertyPage* pPage = GetPage(i);
		if (pPage->m_psp.pszTemplate == pPshStartPage)
		{
			m_psh.nStartPage = i;
			break;
		}
	}*/
}

CSharedFileDetailsModelessSheet::~CSharedFileDetailsModelessSheet()
{
	delete m_wndStatistics;
	delete m_wndFileLink;
	delete m_wndArchiveInfo;
	delete m_wndMediaInfo;
	delete m_wndMetaData;
}

BOOL CSharedFileDetailsModelessSheet::OnInitDialog()
{
	EnableStackedTabs(FALSE);
	BOOL bResult = CListViewPropertySheet::OnInitDialog();
	HighColorTab::UpdateImageList(*this);
	InitWindowStyles(this);
	return bResult;
}

void  CSharedFileDetailsModelessSheet::SetFiles(CTypedPtrList<CPtrList, CShareableFile*>& aFiles)
{
	m_aItems.RemoveAll();
	POSITION pos = aFiles.GetHeadPosition();
	while (pos)
		m_aItems.Add(aFiles.GetNext(pos));
	ChangedData();
}

LRESULT CSharedFileDetailsModelessSheet::OnDataChanged(WPARAM, LPARAM)
{
	UpdateFileDetailsPages(this, m_wndArchiveInfo, m_wndMediaInfo);
	return 1;
}