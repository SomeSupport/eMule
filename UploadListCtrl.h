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
#pragma once
#include "MuleListCtrl.h"
#include "ListCtrlItemWalk.h"

class CUpDownClient;
class CToolTipCtrlX;

class CUploadListCtrl : public CMuleListCtrl, public CListCtrlItemWalk
{
	DECLARE_DYNAMIC(CUploadListCtrl)

public:
	CUploadListCtrl();
	virtual ~CUploadListCtrl();

	void	Init();
	void	AddClient(const CUpDownClient *client);
	void	RemoveClient(const CUpDownClient *client);
	void	RefreshClient(const CUpDownClient *client);
	void	Hide() { ShowWindow(SW_HIDE); }
	void	Show() { ShowWindow(SW_SHOW); }
	void	Localize();
	void	ShowSelectedUserDetails();

protected:
	CImageList m_ImageList;
	CToolTipCtrlX *m_tooltip;

	void SetAllIcons();
	void GetItemDisplayText(const CUpDownClient *client, int iSubItem, LPTSTR pszText, int cchTextMax);
	static int CALLBACK SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnLvnColumnClick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnGetDispInfo(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnGetInfoTip(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNmDblClk(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnSysColorChange();
};
