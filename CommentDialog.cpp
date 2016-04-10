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
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#include "stdafx.h"
#include "emule.h"
#include "CommentDialog.h"
#include "KnownFile.h"
#include "PartFile.h"
#include "OtherFunctions.h"
#include "Opcodes.h"
#include "StringConversion.h"
#include "UpDownClient.h"
#include "kademlia/kademlia/kademlia.h"
#include "kademlia/kademlia/SearchManager.h"
#include "kademlia/kademlia/Search.h"
#include "UserMsgs.h"
#include "searchlist.h"
#include "sharedfilelist.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CommentDialog dialog

IMPLEMENT_DYNAMIC(CCommentDialog, CResizablePage)

BEGIN_MESSAGE_MAP(CCommentDialog, CResizablePage)
	ON_BN_CLICKED(IDC_RESET, OnBnClickedReset)
	ON_MESSAGE(UM_DATA_CHANGED, OnDataChanged)
	ON_EN_CHANGE(IDC_CMT_TEXT, OnEnChangeCmtText)
	ON_CBN_SELENDOK(IDC_RATELIST, OnCbnSelendokRatelist)
	ON_CBN_SELCHANGE(IDC_RATELIST, OnCbnSelchangeRatelist)
   	ON_BN_CLICKED(IDC_SEARCHKAD, OnBnClickedSearchKad) 
	ON_WM_TIMER()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

CCommentDialog::CCommentDialog()
	: CResizablePage(CCommentDialog::IDD, 0)
{
	m_paFiles = NULL;
	m_bDataChanged = false;
	m_strCaption = GetResString(IDS_COMMENT);
	m_psp.pszTitle = m_strCaption;
	m_psp.dwFlags |= PSP_USETITLE;
	m_bMergedComment = false;
	m_bSelf = false;
	m_timer = 0;
	m_bEnabled = true;
}

CCommentDialog::~CCommentDialog()
{
}

void CCommentDialog::DoDataExchange(CDataExchange* pDX)
{
	CResizablePage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RATELIST, m_ratebox);
	DDX_Control(pDX, IDC_LST, m_lstComments);
}

void CCommentDialog::OnTimer(UINT /*nIDEvent*/)
{
	RefreshData(false);
}

BOOL CCommentDialog::OnInitDialog()
{
	CResizablePage::OnInitDialog();
	InitWindowStyles(this);

	AddAnchor(IDC_LST, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_CMT_LQUEST, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_CMT_LAIDE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_CMT_TEXT, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_RATEQUEST, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_RATEHELP, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_USERCOMMENTS, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_RESET, TOP_RIGHT);
	AddAnchor(IDC_SEARCHKAD, BOTTOM_RIGHT);

	m_lstComments.Init();
	Localize();

	// start time for calling 'RefreshData'
	VERIFY( (m_timer = SetTimer(301, 5000, 0)) != NULL );

	return TRUE;
}

BOOL CCommentDialog::OnSetActive()
{
	if (!CResizablePage::OnSetActive())
		return FALSE;
	if (m_bDataChanged)
	{
		bool bContainsSharedKnownFile = false;;
		int iRating = -1;
		m_bMergedComment = false;
		CString strComment;
		for (int i = 0; i < m_paFiles->GetSize(); i++)
		{
			if (!(*m_paFiles)[i]->IsKindOf(RUNTIME_CLASS(CKnownFile)))
				continue;
			CKnownFile* file = STATIC_DOWNCAST(CKnownFile, (*m_paFiles)[i]);
			// we actually could show, add and even search for comments on kad for known but not shared files,
			// but we don't publish coments entered by the user if the file is not shared (which might be changed at some point)
			// so make sure we don't let him think he can comment and disable the dialog for such files
			if (theApp.sharedfiles->GetFileByID(file->GetFileHash()) == NULL)
				continue;
			bContainsSharedKnownFile = true;
			if (i == 0)
			{
				strComment = file->GetFileComment();
				iRating = file->GetFileRating();
			}
			else
			{
				if (!m_bMergedComment && strComment.Compare(file->GetFileComment()) != 0)
				{
					strComment.Empty();
					m_bMergedComment = true;
				}
				if (iRating != -1 && (UINT)iRating != file->GetFileRating())
					iRating = -1;
			}
		}
		m_bSelf = true;
		SetDlgItemText(IDC_CMT_TEXT, strComment);
		((CEdit*)GetDlgItem(IDC_CMT_TEXT))->SetLimitText(MAXFILECOMMENTLEN);
		m_ratebox.SetCurSel(iRating);
		m_bSelf = false;
		EnableDialog(bContainsSharedKnownFile);

		m_bDataChanged = false;

		RefreshData();
	}

	return TRUE;
}

LRESULT CCommentDialog::OnDataChanged(WPARAM, LPARAM)
{
	m_bDataChanged = true;
	return 1;
}

void CCommentDialog::OnBnClickedReset()
{
	SetDlgItemText(IDC_CMT_TEXT, _T(""));
	m_bMergedComment = false;
	m_ratebox.SetCurSel(0);
}

BOOL CCommentDialog::OnApply()
{
	if (m_bEnabled && !m_bDataChanged)
	{
	    CString strComment;
	    GetDlgItem(IDC_CMT_TEXT)->GetWindowText(strComment);
	    int iRating = m_ratebox.GetCurSel();
	    for (int i = 0; i < m_paFiles->GetSize(); i++)
	    {
			if (!(*m_paFiles)[i]->IsKindOf(RUNTIME_CLASS(CKnownFile)))
				continue;
		    CKnownFile* file = STATIC_DOWNCAST(CKnownFile, (*m_paFiles)[i]);
			if (theApp.sharedfiles->GetFileByID(file->GetFileHash()) == NULL)
				continue;
		    if (!strComment.IsEmpty() || !m_bMergedComment)
			    file->SetFileComment(strComment);
		    if (iRating != -1)
			    file->SetFileRating(iRating);
	    }
	}
	return CResizablePage::OnApply();
}

void CCommentDialog::Localize(void)
{
	GetDlgItem(IDC_RESET)->SetWindowText(GetResString(IDS_PW_RESET));

	GetDlgItem(IDC_CMT_LQUEST)->SetWindowText(GetResString(IDS_CMT_QUEST));
	GetDlgItem(IDC_CMT_LAIDE)->SetWindowText(GetResString(IDS_CMT_AIDE));

	GetDlgItem(IDC_RATEQUEST)->SetWindowText(GetResString(IDS_CMT_RATEQUEST));
	GetDlgItem(IDC_RATEHELP)->SetWindowText(GetResString(IDS_CMT_RATEHELP));

	GetDlgItem(IDC_USERCOMMENTS)->SetWindowText(GetResString(IDS_COMMENT));
	GetDlgItem(IDC_SEARCHKAD)->SetWindowText(GetResString(IDS_SEARCHKAD));

	CImageList iml;
	iml.Create(16, 16, theApp.m_iDfltImageListColorFlags | ILC_MASK, 0, 1);
	iml.Add(CTempIconLoader(_T("Rating_NotRated")));
	iml.Add(CTempIconLoader(_T("Rating_Fake")));
	iml.Add(CTempIconLoader(_T("Rating_Poor")));
	iml.Add(CTempIconLoader(_T("Rating_Fair")));
	iml.Add(CTempIconLoader(_T("Rating_Good")));
	iml.Add(CTempIconLoader(_T("Rating_Excellent")));
	m_ratebox.SetImageList(&iml);
	m_imlRating.DeleteImageList();
	m_imlRating.Attach(iml.Detach());
	
	m_ratebox.ResetContent();
	m_ratebox.AddItem(GetResString(IDS_CMT_NOTRATED), 0);
	m_ratebox.AddItem(GetResString(IDS_CMT_FAKE), 1);
	m_ratebox.AddItem(GetResString(IDS_CMT_POOR), 2);
	m_ratebox.AddItem(GetResString(IDS_CMT_FAIR), 3);
	m_ratebox.AddItem(GetResString(IDS_CMT_GOOD), 4);
	m_ratebox.AddItem(GetResString(IDS_CMT_EXCELLENT), 5);
	UpdateHorzExtent(m_ratebox, 16); // adjust dropped width to ensure all strings are fully visible

	RefreshData();
}

void CCommentDialog::OnDestroy()
{
	m_imlRating.DeleteImageList();
	CResizablePage::OnDestroy();
	if (m_timer){
		KillTimer(m_timer);
		m_timer = 0;
	}
}

void CCommentDialog::OnEnChangeCmtText()
{
	if (!m_bSelf)
		SetModified();
}

void CCommentDialog::OnCbnSelendokRatelist()
{
	if (!m_bSelf)
		SetModified();
}

void CCommentDialog::OnCbnSelchangeRatelist()
{
	if (!m_bSelf)
		SetModified();
}

void CCommentDialog::RefreshData(bool deleteOld)
{ 
	if (deleteOld)
		m_lstComments.DeleteAllItems();
	
	if (!m_bEnabled)
		return;
	
	bool kadsearchable = true;
    for (int i = 0; i < m_paFiles->GetSize(); i++)
    {
		CAbstractFile* file = STATIC_DOWNCAST(CAbstractFile, (*m_paFiles)[i]);
		if (file->IsPartFile())
		{
			for (POSITION pos = ((CPartFile*)file)->srclist.GetHeadPosition(); pos != NULL; )
			{ 
				CUpDownClient* cur_src = ((CPartFile*)file)->srclist.GetNext(pos);
				if (cur_src->HasFileRating() || !cur_src->GetFileComment().IsEmpty())
					m_lstComments.AddItem(cur_src);
			}
		}
		else if (!file->IsKindOf(RUNTIME_CLASS(CKnownFile)))
			continue;
		else if (theApp.sharedfiles->GetFileByID(file->GetFileHash()) == NULL)
			continue;
	
		const CTypedPtrList<CPtrList, Kademlia::CEntry*>& list = file->getNotes();
		for (POSITION pos = list.GetHeadPosition(); pos != NULL; )
		{
			Kademlia::CEntry* entry = list.GetNext(pos);
			m_lstComments.AddItem(entry);
		}

		// check if note searches are running for this file(s)
		if (Kademlia::CSearchManager::AlreadySearchingFor(Kademlia::CUInt128(file->GetFileHash())))
			kadsearchable = false;
	}

	CWnd* pWndFocus = GetFocus();
	if (Kademlia::CKademlia::IsConnected()) {
		SetDlgItemText(IDC_SEARCHKAD, kadsearchable ? GetResString(IDS_SEARCHKAD) : GetResString(IDS_KADSEARCHACTIVE));
		GetDlgItem(IDC_SEARCHKAD)->EnableWindow(kadsearchable);
	}
	else {
		SetDlgItemText(IDC_SEARCHKAD, GetResString(IDS_SEARCHKAD));
		GetDlgItem(IDC_SEARCHKAD)->EnableWindow(FALSE);
	}
	if (pWndFocus && pWndFocus->m_hWnd == GetDlgItem(IDC_SEARCHKAD)->m_hWnd)
		m_lstComments.SetFocus();
}

void CCommentDialog::OnBnClickedSearchKad()
{
	if (m_bEnabled && Kademlia::CKademlia::IsConnected())
	{
		bool bSkipped = false;
		int iMaxSearches = min(m_paFiles->GetSize(), KADEMLIATOTALFILE);
	    for (int i = 0; i < iMaxSearches; i++)
	    {
			CAbstractFile* file = STATIC_DOWNCAST(CAbstractFile, (*m_paFiles)[i]);
 			if (file && file->IsKindOf(RUNTIME_CLASS(CKnownFile)) && theApp.sharedfiles->GetFileByID(file->GetFileHash()) != NULL)
			{
				if (!Kademlia::CSearchManager::PrepareLookup(Kademlia::CSearch::NOTES, true, Kademlia::CUInt128(file->GetFileHash())))
					bSkipped = true;
				else{
					theApp.searchlist->SetNotesSearchStatus(file->GetFileHash(), true);
					file->SetKadCommentSearchRunning(true);
				}
			}
		}
		if (bSkipped)
			AfxMessageBox(GetResString(IDS_KADSEARCHALREADY), MB_OK | MB_ICONINFORMATION);
	}
	RefreshData();
}

void CCommentDialog::EnableDialog(bool bEnabled)
{
	if (m_bEnabled == bEnabled)
		return;
	m_bEnabled = bEnabled;
	GetDlgItem(IDC_LST)->EnableWindow(m_bEnabled ? TRUE : FALSE);
	GetDlgItem(IDC_CMT_TEXT)->EnableWindow(m_bEnabled ? TRUE : FALSE);
	GetDlgItem(IDC_RATELIST)->EnableWindow(m_bEnabled ? TRUE : FALSE);
	GetDlgItem(IDC_RESET)->EnableWindow(m_bEnabled ? TRUE : FALSE);
	GetDlgItem(IDC_SEARCHKAD)->EnableWindow(m_bEnabled ? TRUE : FALSE);
}