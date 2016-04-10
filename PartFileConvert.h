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
#include "ResizableLib\ResizableDialog.h"
#include "ProgressCtrlX.h"

struct ConvertJob;

class CPartFileConvert
{
public:
	static int	ScanFolderToAdd(CString folder,bool deletesource=false);
	static void ConvertToeMule(CString folder,bool deletesource=false);
	static void CPartFileConvert::StartThread();
	static void ShowGUI();
	static void UpdateGUI(float percent,CString text, bool fullinfo=false);	// current file information
	static void UpdateGUI(ConvertJob* job); // listcontrol update
	static void CloseGUI();
	static void ClosedGUI();
	static void RemoveAllJobs();
	static void RemoveAllSuccJobs();
	static void RemoveJob(ConvertJob* job);
	static CString GetReturncodeText(int ret);
	static void Localize();

private:
	CPartFileConvert(void); // Just use static recover method

	static int performConvertToeMule(CString folder);
	static UINT AFX_CDECL run(LPVOID lpParam);
	static void	ReConvertNewEDPartfile(CString folder);
	static UINT	PFConvertThread(LPVOID param);
};


class CPartFileConvertDlg : public CResizableDialog
{
	DECLARE_DYNAMIC(CPartFileConvertDlg)
public:
	CPartFileConvertDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPartFileConvertDlg();

	enum { IDD = IDD_CONVERTPARTFILES };
	CWnd* m_pParent;

	void AddJob(ConvertJob* job);
	void RemoveJob(ConvertJob* job);
	void UpdateJobInfo(ConvertJob* job);

protected:
	HICON m_icnWnd;
	CProgressCtrlX pb_current;
	CListCtrl	   joblist;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnSysColorChange();
	afx_msg void OnBnClickedOk();
	afx_msg void OnAddFolder();
	afx_msg void OnCancel();
	afx_msg void RetrySel();
	afx_msg void RemoveSel();
	afx_msg void ShowInfo();

	friend class CPartFileConvert;
};

static CWinThread*	convertPfThread;
static CList<ConvertJob*,ConvertJob*> m_jobs;
static ConvertJob*	pfconverting;
static CPartFileConvertDlg*	m_convertgui;
