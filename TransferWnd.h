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
#pragma once
#include "ResizableLib\ResizableFormView.h"
#include "SplitterControl.h"
#include "TabCtrl.hpp"
#include "UploadListCtrl.h"
#include "DownloadListCtrl.h"
#include "QueueListCtrl.h"
#include "ClientListCtrl.h"
#include "DownloadClientsCtrl.h"

class CDropDownButton;
class CToolTipCtrlX;

class CTransferWnd : public CResizableFormView
{
	DECLARE_DYNCREATE(CTransferWnd)

public:
	CTransferWnd(CWnd* pParent = NULL);   // standard constructor
	virtual ~CTransferWnd();

	enum EWnd1Icon {
		w1iSplitWindow = 0,
		w1iDownloadFiles,
		w1iUploading,
		w1iDownloading,
		w1iOnQueue,
		w1iClientsKnown
	};

	enum EWnd2Icon {
		w2iUploading = 0,
		w2iDownloading,
		w2iOnQueue,
		w2iClientsKnown
	};

	enum EWnd2 {
		wnd2Downloading = 0,
		wnd2Uploading = 1,
		wnd2OnQueue = 2,
		wnd2Clients = 3
	};

	void ShowQueueCount(uint32 number);
	void UpdateListCount(EWnd2 listindex, int iCount = -1);
	void UpdateFilesCount(int iCount);
	void Localize();
	void UpdateCatTabTitles(bool force = true);
	void VerifyCatTabSize();
	int	 AddCategory(CString newtitle,CString newincoming,CString newcomment,CString newautocat,bool addTab=true);
	int	 AddCategoryInteractive();
	void SwitchUploadList();
	void ResetTransToolbar(bool bShowToolbar, bool bResetLists = true);
	void SetToolTipsDelay(DWORD dwDelay);
	void OnDisableList();

	// Dialog Data
	enum { IDD = IDD_TRANSFER };
	CUploadListCtrl			uploadlistctrl;
	CDownloadListCtrl		downloadlistctrl;
	CQueueListCtrl			queuelistctrl;
	CClientListCtrl			clientlistctrl;
	CDownloadClientsCtrl	downloadclientsctrl;

protected:
	CSplitterControl m_wndSplitter;
	EWnd2		m_uWnd2;
	bool		downloadlistactive;
	CDropDownButton* m_btnWnd1;
	CDropDownButton* m_btnWnd2;
	TabControl	m_dlTab;
	int			rightclickindex;
	int			m_nDragIndex;
	int			m_nDropIndex;
	int			m_nLastCatTT;
	int			m_isetcatmenu;
	bool		m_bIsDragging;
	CImageList* m_pDragImage;
	POINT		m_pLastMousePoint;
	uint32		m_dwShowListIDC;
	CToolTipCtrlX* m_tooltipCats;
	bool		m_bLayoutInited;

	void	ShowWnd2(EWnd2 uList);
	void	SetWnd2(EWnd2 uWnd2);
	void	DoResize(int delta);
	void	UpdateSplitterRange();
	void	DoSplitResize(int delta);
	void	SetAllIcons();
	void	SetWnd1Icons();
	void	SetWnd2Icons();
	void	UpdateTabToolTips() {UpdateTabToolTips(-1);}
	void	UpdateTabToolTips(int tab);
	CString	GetTabStatistic(int tab);
	int		GetTabUnderMouse(CPoint* point);
	int		GetItemUnderMouse(CListCtrl* ctrl);
	CString	GetCatTitle(int catid);
	void	EditCatTabLabel(int index,CString newlabel);
	void	EditCatTabLabel(int index);
	void	ShowList(uint32 dwListIDC);
	void	SetWnd1Icon(EWnd1Icon iIcon);
	void	SetWnd2Icon(EWnd2Icon iIcon);
	void	ShowSplitWindow(bool bReDraw = false);
	void	LocalizeToolbars();

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnInitialUpdate();
	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	
	DECLARE_MESSAGE_MAP()
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnBnClickedChangeView();
	afx_msg void OnBnClickedQueueRefreshButton();
	afx_msg void OnDblClickDltab();
	afx_msg void OnHoverDownloadList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnHoverUploadList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLvnBeginDragDownloadList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnKeydownDownloadList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnNmRClickDltab(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg void OnSplitterMoved(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnSysColorChange();
	afx_msg void OnTabMovement(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTcnSelchangeDltab(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnWnd1BtnDropDown(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnWnd2BtnDropDown(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnPaint();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
};
