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
#include "KademliaWnd.h"
#include "KadSearchListCtrl.h"
#include "KadContactListCtrl.h"
#include "Ini2.h"
#include "OtherFunctions.h"
#include "emuledlg.h"
#include "DownloadQueue.h"
#include "PartFile.h"
#include "kademlia/kademlia/search.h"
#include "kademlia/utils/LookupHistory.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CKadSearchListCtrl

enum ECols
{
	colNum = 0,
	colKey,
	colType,
	colName,
	colStop,
	colLoad,
	colPacketsSent,
	colResponses
};

IMPLEMENT_DYNAMIC(CKadSearchListCtrl, CMuleListCtrl)

BEGIN_MESSAGE_MAP(CKadSearchListCtrl, CMuleListCtrl)
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnLvnColumnClick)
	ON_WM_SYSCOLORCHANGE()
END_MESSAGE_MAP()

CKadSearchListCtrl::CKadSearchListCtrl()
{
	SetGeneralPurposeFind(true);
	SetSkinKey(L"KadActionsLv");
}

CKadSearchListCtrl::~CKadSearchListCtrl()
{
}

void CKadSearchListCtrl::Init()
{
	SetPrefsKey(_T("KadSearchListCtrl"));
	SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

	InsertColumn(colNum,			GetResString(IDS_NUMBER),		LVCFMT_LEFT,  60);
	InsertColumn(colKey,			GetResString(IDS_KEY),			LVCFMT_LEFT, DFLT_HASH_COL_WIDTH);
	InsertColumn(colType,			GetResString(IDS_TYPE),			LVCFMT_LEFT, 100);
	InsertColumn(colName,			GetResString(IDS_SW_NAME),		LVCFMT_LEFT, DFLT_FILENAME_COL_WIDTH);
	InsertColumn(colStop,			GetResString(IDS_STATUS),		LVCFMT_LEFT, 100);
	InsertColumn(colLoad,			GetResString(IDS_THELOAD),		LVCFMT_LEFT, 100);
	InsertColumn(colPacketsSent,	GetResString(IDS_PACKSENT),		LVCFMT_LEFT, 100);
	InsertColumn(colResponses,		GetResString(IDS_RESPONSES),	LVCFMT_LEFT, 100);
	
	SetAllIcons();
	Localize();

	LoadSettings();
	SetSortArrow();
	SortItems(SortProc, MAKELONG(GetSortItem(), (GetSortAscending() ? 0 : 0x0001)));
}

void CKadSearchListCtrl::UpdateKadSearchCount()
{
	CString id;
	id.Format(_T("%s (%i)"), GetResString(IDS_KADSEARCHLAB), GetItemCount());
	theApp.emuledlg->kademliawnd->GetDlgItem(IDC_KADSEARCHLAB)->SetWindowText(id);
}

void CKadSearchListCtrl::OnSysColorChange()
{
	CMuleListCtrl::OnSysColorChange();
	SetAllIcons();
}

void CKadSearchListCtrl::SetAllIcons()
{
	CImageList iml;
	iml.Create(16, 16, theApp.m_iDfltImageListColorFlags | ILC_MASK, 0, 1);
	iml.Add(CTempIconLoader(_T("KadFileSearch")));
	iml.Add(CTempIconLoader(_T("KadWordSearch")));
	iml.Add(CTempIconLoader(_T("KadNodeSearch")));
	iml.Add(CTempIconLoader(_T("KadStoreFile")));
	iml.Add(CTempIconLoader(_T("KadStoreWord")));
	ASSERT( (GetStyle() & LVS_SHAREIMAGELISTS) == 0 );
	HIMAGELIST himl = ApplyImageList(iml.Detach());
	if (himl)
		ImageList_Destroy(himl);
}

void CKadSearchListCtrl::Localize()
{
	CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();
	HDITEM hdi;
	hdi.mask = HDI_TEXT;
	CString strRes;

	for (int icol = 0; icol < pHeaderCtrl->GetItemCount(); icol++)
	{
		switch (icol)
		{
			case colNum: strRes = GetResString(IDS_NUMBER); break;
			case colKey: strRes = GetResString(IDS_KEY); break;
			case colType: strRes = GetResString(IDS_TYPE); break;
			case colName: strRes = GetResString(IDS_SW_NAME); break;
			case colStop: strRes = GetResString(IDS_STATUS); break;
			case colResponses: strRes = GetResString(IDS_RESPONSES); break;
			case colLoad: strRes = GetResString(IDS_THELOAD); break;
			case colPacketsSent: strRes = GetResString(IDS_PACKSENT); break;
			default: strRes.Empty(); break;
		}
		hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
		pHeaderCtrl->SetItem(icol, &hdi);
	}

	int iItems = GetItemCount();
	for (int i = 0; i < iItems; i++)
		SearchRef((Kademlia::CSearch *)GetItemData(i));

	UpdateKadSearchCount();
}

void CKadSearchListCtrl::UpdateSearch(int iItem, const Kademlia::CSearch *search)
{
	CString id;
	id.Format(_T("%i"), search->GetSearchID());
	SetItemText(iItem, colNum, id);

	switch (search->GetSearchTypes())
	{
		case Kademlia::CSearch::FILE:
			id = GetResString(IDS_KAD_SEARCHSRC);
			SetItem(iItem, 0, LVIF_IMAGE, 0, 0, 0, 0, 0, 0);
			break;

		case Kademlia::CSearch::KEYWORD:
			id = GetResString(IDS_KAD_SEARCHKW);
			SetItem(iItem, 0, LVIF_IMAGE, 0, 1, 0, 0, 0, 0);
			break;

		case Kademlia::CSearch::NODE:
		case Kademlia::CSearch::NODECOMPLETE:
		case Kademlia::CSearch::NODESPECIAL:
		case Kademlia::CSearch::NODEFWCHECKUDP:
			id = GetResString(IDS_KAD_NODE);
			SetItem(iItem, 0, LVIF_IMAGE, 0, 2, 0, 0, 0, 0);
			break;

		case Kademlia::CSearch::STOREFILE:
			id = GetResString(IDS_KAD_STOREFILE);
			SetItem(iItem, 0, LVIF_IMAGE, 0, 3, 0, 0, 0, 0);
			break;

		case Kademlia::CSearch::STOREKEYWORD:
			id = GetResString(IDS_KAD_STOREKW);
			SetItem(iItem, 0, LVIF_IMAGE, 0, 4, 0, 0, 0, 0);
			break;

		//JOHNTODO: -
		//I also need to understand skinning so the icons are done correctly.
		case Kademlia::CSearch::FINDBUDDY:
			id = GetResString(IDS_FINDBUDDY);
			break;
		
		case Kademlia::CSearch::STORENOTES:
			id = GetResString(IDS_STORENOTES);
			break;
		
		case Kademlia::CSearch::NOTES:
			id = GetResString(IDS_NOTES);
			break;
		
		default:
			id = GetResString(IDS_KAD_UNKNOWN);
			break;
	}
#ifdef _DEBUG
	CString strTypeNr;
	strTypeNr.Format(_T(" (%u)"), search->GetSearchTypes());
	id += strTypeNr;
#endif
	SetItemText(iItem, colType, id);

	SetItemText(iItem, colName, search->GetGUIName());

	if (search->GetTarget() != NULL)
	{
		search->GetTarget().ToHexString(&id);
		SetItemText(iItem, colKey, id);
	}

	if (search->Stoping())
		SetItemText(iItem, colStop, GetResString(IDS_KADSTATUS_STOPPING));
	else
		SetItemText(iItem, colStop, GetResString(IDS_KADSTATUS_ACTIVE));

	id.Format(_T("%u (%u|%u)"), search->GetNodeLoad(), search->GetNodeLoadResonse(), search->GetNodeLoadTotal());
	SetItemText(iItem, colLoad, id);

	id.Format(_T("%u"), search->GetAnswers());
	SetItemText(iItem, colResponses, id);

	id.Format(_T("%u|%u"), search->GetKadPacketSent(), search->GetRequestAnswer());
	SetItemText(iItem, colPacketsSent, id);
}

void CKadSearchListCtrl::SearchAdd(const Kademlia::CSearch *search)
{
	try
	{
		ASSERT( search != NULL );
		int iItem = InsertItem(LVIF_TEXT | LVIF_PARAM, GetItemCount(), NULL, 0, 0, 0, (LPARAM)search);
		if (iItem >= 0)
		{
			UpdateSearch(iItem, search);
			UpdateKadSearchCount();
		}
	}
	catch(...){ASSERT(0);}
}

void CKadSearchListCtrl::SearchRem(const Kademlia::CSearch *search)
{
	try
	{
		ASSERT( search != NULL );
		LVFINDINFO find;
		find.flags = LVFI_PARAM;
		find.lParam = (LPARAM)search;
		int iItem = FindItem(&find);
		if (iItem != -1)
		{
			DeleteItem(iItem);
			UpdateKadSearchCount();
		}
	}
	catch(...){ASSERT(0);}
}

void CKadSearchListCtrl::SearchRef(const Kademlia::CSearch *search)
{
	try
	{
		ASSERT( search != NULL );
		LVFINDINFO find;
		find.flags = LVFI_PARAM;
		find.lParam = (LPARAM)search;
		int iItem = FindItem(&find);
		if (iItem != -1)
			UpdateSearch(iItem, search);
	}
	catch(...){ASSERT(0);}
}

BOOL CKadSearchListCtrl::OnCommand(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	// ???
	return TRUE;
}

void CKadSearchListCtrl::OnLvnColumnClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLISTVIEW *pNMListView = (NMLISTVIEW *)pNMHDR;

	// Determine ascending based on whether already sorted on this column
	int iSortItem = GetSortItem();
	bool bOldSortAscending = GetSortAscending();
	bool bSortAscending = (iSortItem != pNMListView->iSubItem) ? true : !bOldSortAscending;

	// Item is column clicked
	iSortItem = pNMListView->iSubItem;

	// Sort table
	UpdateSortHistory(MAKELONG(iSortItem, (bSortAscending ? 0 : 0x0001)));
	SetSortArrow(iSortItem, bSortAscending);
	SortItems(SortProc, MAKELONG(iSortItem, (bSortAscending ? 0 : 0x0001)));

	*pResult = 0;
}

int CKadSearchListCtrl::SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	const Kademlia::CSearch *item1 = (Kademlia::CSearch *)lParam1; 
	const Kademlia::CSearch *item2 = (Kademlia::CSearch *)lParam2; 
	if (item1 == NULL || item2 == NULL)
		return 0;

	int iResult;
	switch (LOWORD(lParamSort))
	{
		case colNum:
			iResult = CompareUnsigned(item1->GetSearchID(), item2->GetSearchID());
			break;

		case colKey:
			if (item1->GetTarget() == NULL && item2->GetTarget() == NULL)
				iResult = 0;
			else if (item1->GetTarget() != NULL && item2->GetTarget() == NULL)
				iResult = -1;
			else if (item1->GetTarget() == NULL && item2->GetTarget() != NULL)
				iResult = 1;
			else
				iResult = item1->GetTarget().CompareTo(item2->GetTarget());
			break;
		
		case colType:
			iResult = item1->GetSearchTypes() - item2->GetSearchTypes();
			break;
		
		case colName:
			iResult = CompareLocaleStringNoCase(item1->GetGUIName(), item2->GetGUIName());
			break;

		case colStop:
			iResult = (int)item1->Stoping() - (int)item2->Stoping();
			break;

		case colLoad:
			iResult = CompareUnsigned(item1->GetNodeLoad(), item2->GetNodeLoad());
			break;

		case colPacketsSent:
			iResult = CompareUnsigned(item1->GetKadPacketSent(), item2->GetKadPacketSent());
			break;

		case colResponses:
			iResult = CompareUnsigned(item1->GetAnswers(), item2->GetAnswers());
			break;

		default:
			return 0;
	}
	if (HIWORD(lParamSort))
		iResult = -iResult;
	return iResult;
}

Kademlia::CLookupHistory* CKadSearchListCtrl::FetchAndSelectActiveSearch(bool bMark)
{
	int iIntrestingItem = (-1);
	int iItem = (-1);
	
	for (int i = 1; i <= GetItemCount(); i++)
	{
		const Kademlia::CSearch* pSearch = (Kademlia::CSearch*)GetItemData(GetItemCount() - i);
		if (pSearch != NULL && !pSearch->GetLookupHistory()->IsSearchStopped() && !pSearch->GetLookupHistory()->IsSearchDeleted())
		{
			// prefer interesting search rather than node searches
			switch (pSearch->GetSearchTypes())
			{
				case Kademlia::CSearch::FILE:
				case Kademlia::CSearch::KEYWORD:
				case Kademlia::CSearch::STORENOTES:
				case Kademlia::CSearch::NOTES:
				case Kademlia::CSearch::STOREFILE:
				case Kademlia::CSearch::STOREKEYWORD:
					iIntrestingItem = GetItemCount() - i;
					break;
				case Kademlia::CSearch::NODE:
				case Kademlia::CSearch::NODECOMPLETE:
				case Kademlia::CSearch::NODESPECIAL:
				case Kademlia::CSearch::NODEFWCHECKUDP:
				case Kademlia::CSearch::FINDBUDDY:
				default:
					if (iItem == (-1))
						iItem = GetItemCount() - i;
					break;
			}
		}
		if (iIntrestingItem >= 0)
			break;
	}
	if (iIntrestingItem >= 0)
	{
		if (bMark)
			SetItemState(iIntrestingItem, LVIS_SELECTED, LVIS_SELECTED);
		return ((Kademlia::CSearch*)GetItemData(iIntrestingItem))->GetLookupHistory();
	}
	if (iItem >= 0)
	{
		if (bMark)
			SetItemState(iItem, LVIS_SELECTED, LVIS_SELECTED);
		return ((Kademlia::CSearch*)GetItemData(iItem))->GetLookupHistory();
	}
	return NULL;
}
