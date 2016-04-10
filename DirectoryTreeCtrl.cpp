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
#include "DirectoryTreeCtrl.h"
#include "otherfunctions.h"
#include "Preferences.h"
#include "TitleMenu.h"
#include "UserMsgs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////
// written by robert rostek - tecxx@rrs.at //
/////////////////////////////////////////////

struct STreeItem
{
	CString strPath;
};


// CDirectoryTreeCtrl

IMPLEMENT_DYNAMIC(CDirectoryTreeCtrl, CTreeCtrl)

BEGIN_MESSAGE_MAP(CDirectoryTreeCtrl, CTreeCtrl)
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, OnTvnItemexpanding)
	ON_NOTIFY_REFLECT(TVN_GETDISPINFO, OnTvnGetdispinfo)
	ON_WM_LBUTTONDOWN()
	ON_NOTIFY_REFLECT(TVN_DELETEITEM, OnTvnDeleteItem)
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONDOWN()
	ON_WM_KEYDOWN()
	ON_WM_CHAR()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

CDirectoryTreeCtrl::CDirectoryTreeCtrl()
{
	m_bSelectSubDirs = false;
}

CDirectoryTreeCtrl::~CDirectoryTreeCtrl()
{
	// don't destroy the system's image list
	m_image.Detach();
}

void CDirectoryTreeCtrl::OnDestroy()
{
	// If a treeview control is created with TVS_CHECKBOXES, the application has to 
	// delete the image list which was implicitly created by the control.
	CImageList *piml = GetImageList(TVSIL_STATE);
	if (piml)
		piml->DeleteImageList();

	CTreeCtrl::OnDestroy();
}

void CDirectoryTreeCtrl::OnTvnItemexpanding(NMHDR *pNMHDR, LRESULT *pResult)
{
	CWaitCursor curWait;
	SetRedraw(FALSE);

	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	HTREEITEM hItem = pNMTreeView->itemNew.hItem;
	// remove all subitems
	HTREEITEM hRemove = GetChildItem(hItem);
	while(hRemove)
	{
		DeleteItem(hRemove);
		hRemove = GetChildItem(hItem);
	}

	// get the directory
	CString strDir = GetFullPath(hItem);

	// fetch all subdirectories and add them to the node
	AddSubdirectories(hItem, strDir);

	SetRedraw(TRUE);
	Invalidate();
	*pResult = 0;
}

void CDirectoryTreeCtrl::ShareSubDirTree(HTREEITEM hItem, BOOL bRecurse)
{
	CWaitCursor curWait;
	SetRedraw(FALSE);

	HTREEITEM hItemVisibleItem = GetFirstVisibleItem();
	CheckChanged(hItem, !GetCheck(hItem));
	if (bRecurse)
	{
		Expand(hItem, TVE_TOGGLE);
		HTREEITEM hChild = GetChildItem(hItem);
		while (hChild != NULL)
		{
			MarkChilds(hChild, !GetCheck(hItem));
			hChild = GetNextSiblingItem(hChild);
		}
		Expand(hItem, TVE_TOGGLE);
	}
	if (hItemVisibleItem)
		SelectSetFirstVisible(hItemVisibleItem);

	SetRedraw(TRUE);
	Invalidate();
}

void CDirectoryTreeCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
	//VQB adjustments to provide for sharing or unsharing of subdirectories when control key is Down 
	UINT uHitFlags; 
	HTREEITEM hItem = HitTest(point, &uHitFlags); 
	if (hItem && (uHitFlags & TVHT_ONITEMSTATEICON))
		ShareSubDirTree(hItem, nFlags & MK_CONTROL);
	CTreeCtrl::OnLButtonDown(nFlags, point); 
}

void CDirectoryTreeCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == VK_SPACE)
	{
		HTREEITEM hItem = GetSelectedItem();
		if (hItem)
		{
			ShareSubDirTree(hItem, GetKeyState(VK_CONTROL) & 0x8000);

			// if Ctrl+Space is passed to the tree control, it just beeps and does not check/uncheck the item!
			SetCheck(hItem, !GetCheck(hItem));
			return;
		}
	}

	CTreeCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CDirectoryTreeCtrl::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// If we let any keystrokes which are handled by us -- but not by the tree
	// control -- pass to the control, the user will hear a system event
	// sound (Standard Error!)
	BOOL bCallDefault = TRUE;

	if (GetKeyState(VK_CONTROL) & 0x8000)
	{
		if (nChar == VK_SPACE)
			bCallDefault = FALSE;
	}

	if (bCallDefault)
		CTreeCtrl::OnChar(nChar, nRepCnt, nFlags);
}

void CDirectoryTreeCtrl::MarkChilds(HTREEITEM hChild,bool mark) { 
	CheckChanged(hChild, mark); 
	SetCheck(hChild,mark);  
	Expand(hChild, TVE_TOGGLE); // VQB - make sure tree has entries 
	HTREEITEM hChild2; 
	hChild2 = GetChildItem(hChild); 
	while( hChild2 != NULL) 
	{ 
		MarkChilds(hChild2,mark); 
		hChild2 = GetNextSiblingItem( hChild2 ); 
	} 
	Expand(hChild, TVE_TOGGLE); // VQB - restore tree to initial disposition 
}

void CDirectoryTreeCtrl::OnTvnGetdispinfo(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTVDISPINFO pTVDispInfo = reinterpret_cast<LPNMTVDISPINFO>(pNMHDR);
	pTVDispInfo->item.cChildren = 1;
	*pResult = 0;
}

void CDirectoryTreeCtrl::Init(void)
{
	// Win98: Explicitly set to Unicode to receive Unicode notifications.
	SendMessage(CCM_SETUNICODEFORMAT, TRUE);

	ShowWindow(SW_HIDE);
	DeleteAllItems();

	// START: added by FoRcHa /////////////
	WORD wWinVer = thePrefs.GetWindowsVersion();	// maybe causes problems on 98 & nt4
	if (wWinVer != _WINVER_95_ && wWinVer != _WINVER_NT4_)
	{
		SHFILEINFO shFinfo;
		HIMAGELIST hImgList = NULL;

		// Get the system image list using a "path" which is available on all systems. [patch by bluecow]
		hImgList = (HIMAGELIST)SHGetFileInfo(_T("."), 0, &shFinfo, sizeof(shFinfo),
											 SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
		if(!hImgList)
		{
			TRACE(_T("Cannot retrieve the Handle of SystemImageList!"));
			//return;
		}

		m_image.m_hImageList = hImgList;
		SetImageList(&m_image, TVSIL_NORMAL);
	}
	////////////////////////////////


	TCHAR drivebuffer[500];
	DWORD dwRet = GetLogicalDriveStrings(_countof(drivebuffer) - 1, drivebuffer);
	if (dwRet > 0 && dwRet < _countof(drivebuffer))
	{
		drivebuffer[_countof(drivebuffer) - 1] = _T('\0');

		const TCHAR* pos = drivebuffer;
		while(*pos != _T('\0')){

			// Copy drive name
			TCHAR drive[4];
			_tcsncpy(drive, pos, _countof(drive));
			drive[_countof(drive) - 1] = _T('\0');

			drive[2] = _T('\0');
			AddChildItem(NULL, drive); // e.g. "C:"

			// Point to the next drive
			pos += _tcslen(pos) + 1;
		}
	}
	ShowWindow(SW_SHOW);
}

HTREEITEM CDirectoryTreeCtrl::AddChildItem(HTREEITEM hRoot, CString strText)
{
	CString strPath = GetFullPath(hRoot);
	if (hRoot != NULL && strPath.Right(1) != _T("\\"))
		strPath += _T("\\");
	CString strDir = strPath + strText;
	TVINSERTSTRUCT itInsert = {0};
	
	// START: changed by FoRcHa /////
	WORD wWinVer = thePrefs.GetWindowsVersion();
	if (wWinVer != _WINVER_95_ && wWinVer != _WINVER_NT4_)
	{
		itInsert.item.mask = TVIF_CHILDREN | TVIF_HANDLE | TVIF_TEXT |
							 TVIF_STATE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		itInsert.item.stateMask = TVIS_BOLD | TVIS_STATEIMAGEMASK;
	}
	else
	{
		itInsert.item.mask = TVIF_CHILDREN | TVIF_HANDLE | TVIF_TEXT | TVIF_STATE;
		itInsert.item.stateMask = TVIS_BOLD;
	}
	// END: changed by FoRcHa ///////
	
	if (HasSharedSubdirectory(strDir))
		itInsert.item.state = TVIS_BOLD;
	else
		itInsert.item.state = 0;
	if (HasSubdirectories(strDir))
		itInsert.item.cChildren = I_CHILDRENCALLBACK;		// used to display the + symbol next to each item
	else
		itInsert.item.cChildren = 0;

	itInsert.item.pszText = const_cast<LPTSTR>((LPCTSTR)strText);
	itInsert.hInsertAfter = hRoot ? TVI_SORT : TVI_LAST;
	itInsert.hParent = hRoot;
	
	// START: added by FoRcHa ////////////////
	if (wWinVer != _WINVER_95_ && wWinVer != _WINVER_NT4_)
	{
		CString strTemp = strDir;
		if(strTemp.Right(1) != _T("\\"))
			strTemp += _T("\\");
		
		UINT nType = GetDriveType(strTemp);
		if(DRIVE_REMOVABLE <= nType && nType <= DRIVE_RAMDISK)
			itInsert.item.iImage = nType;
	
		SHFILEINFO shFinfo;
		shFinfo.szDisplayName[0] = _T('\0');
		if(!SHGetFileInfo(strTemp, 0, &shFinfo,	sizeof(shFinfo),
						SHGFI_ICON | SHGFI_SMALLICON | SHGFI_DISPLAYNAME))
		{
			TRACE(_T("Error Gettting SystemFileInfo!"));
			itInsert.itemex.iImage = 0; // :(
		}
		else
		{
			itInsert.itemex.iImage = shFinfo.iIcon;
			DestroyIcon(shFinfo.hIcon);
			if (hRoot == NULL && shFinfo.szDisplayName[0] != _T('\0'))
			{
				STreeItem* pti = new STreeItem;
				pti->strPath = strText;
				strText = shFinfo.szDisplayName;
				itInsert.item.pszText = const_cast<LPTSTR>((LPCTSTR)strText);
				itInsert.item.mask |= TVIF_PARAM;
				itInsert.item.lParam = (LPARAM)pti;
			}
		}

		if(!SHGetFileInfo(strTemp, 0, &shFinfo, sizeof(shFinfo),
							SHGFI_ICON | SHGFI_OPENICON | SHGFI_SMALLICON))
		{
			TRACE(_T("Error Gettting SystemFileInfo!"));
			itInsert.itemex.iImage = 0;
		}
		else
		{
			itInsert.itemex.iSelectedImage = shFinfo.iIcon;
			DestroyIcon(shFinfo.hIcon);
		}
	}
	// END: added by FoRcHa //////////////

	HTREEITEM hItem = InsertItem(&itInsert);
	if (IsShared(strDir))
		SetCheck(hItem);

	return hItem;
}

CString CDirectoryTreeCtrl::GetFullPath(HTREEITEM hItem)
{
	CString strDir;
	HTREEITEM hSearchItem = hItem;
	while(hSearchItem != NULL)
	{
		CString strSearchItemDir;
		STreeItem* pti = (STreeItem*)GetItemData(hSearchItem);
		if (pti)
			strSearchItemDir = pti->strPath;
		else
			strSearchItemDir = GetItemText(hSearchItem);
		strDir = strSearchItemDir + _T("\\") + strDir;
		hSearchItem = GetParentItem(hSearchItem);
	}
	return strDir;
}

void CDirectoryTreeCtrl::AddSubdirectories(HTREEITEM hRoot, CString strDir)
{
	if (strDir.Right(1) != _T("\\"))
		strDir += _T("\\");
	CFileFind finder;
	BOOL bWorking = finder.FindFile(strDir+_T("*.*"));
	while (bWorking)
	{
		bWorking = finder.FindNextFile();
		if (finder.IsDots())
			continue;
		if (finder.IsSystem())
			continue;
		if (!finder.IsDirectory())
			continue;
		
		CString strFilename = finder.GetFileName();
		if (strFilename.ReverseFind(_T('\\')) != -1)
			strFilename = strFilename.Mid(strFilename.ReverseFind(_T('\\')) + 1);
		AddChildItem(hRoot, strFilename);
	}
	finder.Close();
}

bool CDirectoryTreeCtrl::HasSubdirectories(CString strDir)
{
	if (strDir.Right(1) != _T('\\'))
		strDir += _T('\\');
	// Never try to enumerate the files of a drive and thus physically access the drive, just
	// for the information whether the drive has sub directories in the root folder. Depending
	// on the physical drive type (floppy disk, CD-ROM drive, etc.) this creates an annoying
	// physical access to that drive - which is to be avoided in each case. Even Windows
	// Explorer shows all drives by default with a '+' sign (which means that the user has
	// to explicitly open the drive to really get the content) - and that approach will be fine
	// for eMule as well.
	// Since the restriction for drives 'A:' and 'B:' was removed, this gets more important now.
	if (PathIsRoot(strDir))
		return true;
	CFileFind finder;
	BOOL bWorking = finder.FindFile(strDir+_T("*.*"));
	while (bWorking)
	{
		bWorking = finder.FindNextFile();
		if (finder.IsDots())
			continue;
		if (finder.IsSystem())
			continue;
		if (!finder.IsDirectory())
			continue;
		finder.Close();
		return true;
	}
	finder.Close();
	return false;
}

void CDirectoryTreeCtrl::GetSharedDirectories(CStringList* list)
{
	for (POSITION pos = m_lstShared.GetHeadPosition(); pos != NULL; )
		list->AddTail(m_lstShared.GetNext(pos));
}

void CDirectoryTreeCtrl::SetSharedDirectories(CStringList* list)
{
	m_lstShared.RemoveAll();

	for (POSITION pos = list->GetHeadPosition(); pos != NULL; )
	{
		CString str = list->GetNext(pos);
		if (str.Left(2)==_T("\\\\")) continue;
		if (str.Right(1) != _T('\\'))
			str += _T('\\');
		m_lstShared.AddTail(str);
	}
	Init();
}

bool CDirectoryTreeCtrl::HasSharedSubdirectory(CString strDir)
{
	if (strDir.Right(1) != _T('\\'))
		strDir += _T('\\');
	strDir.MakeLower();
	for (POSITION pos = m_lstShared.GetHeadPosition(); pos != NULL; )
	{
		CString str = m_lstShared.GetNext(pos);
		str.MakeLower();
		if (str.Find(strDir) == 0 && strDir != str)//strDir.GetLength() != str.GetLength())
			return true;
	}
	return false;
}

void CDirectoryTreeCtrl::CheckChanged(HTREEITEM hItem, bool bChecked)
{
	CString strDir = GetFullPath(hItem);
	if (bChecked)
		AddShare(strDir);
	else
		DelShare(strDir);

	UpdateParentItems(hItem);
	GetParent()->SendMessage(WM_COMMAND, UM_ITEMSTATECHANGED, (long)m_hWnd);
}

bool CDirectoryTreeCtrl::IsShared(CString strDir)
{
	if (strDir.Right(1) != _T('\\'))
		strDir += _T('\\');
	for (POSITION pos = m_lstShared.GetHeadPosition(); pos != NULL; )
	{
		CString str = m_lstShared.GetNext(pos);
		if (str.Right(1) != _T('\\'))
			str += _T('\\');
		if (str.CompareNoCase(strDir) == 0)
			return true;
	}
	return false;
}

void CDirectoryTreeCtrl::AddShare(CString strDir)
{
	if (strDir.Right(1) != _T('\\'))
		strDir += _T('\\');
	
	if (IsShared(strDir) || !strDir.CompareNoCase(thePrefs.GetMuleDirectory(EMULE_CONFIGDIR)))
		return;

	m_lstShared.AddTail(strDir);
}

void CDirectoryTreeCtrl::DelShare(CString strDir)
{
	if (strDir.Right(1) != _T('\\'))
		strDir += _T('\\');
	for (POSITION pos = m_lstShared.GetHeadPosition(); pos != NULL; )
	{
		POSITION pos2 = pos;
		CString str = m_lstShared.GetNext(pos);
		if (str.CompareNoCase(strDir) == 0)
			m_lstShared.RemoveAt(pos2);
	}
}

void CDirectoryTreeCtrl::UpdateParentItems(HTREEITEM hChild)
{
	HTREEITEM hSearch = GetParentItem(hChild);
	while(hSearch != NULL)
	{
		if (HasSharedSubdirectory(GetFullPath(hSearch)))
			SetItemState(hSearch, TVIS_BOLD, TVIS_BOLD);
		else
			SetItemState(hSearch, 0, TVIS_BOLD);
		hSearch = GetParentItem(hSearch);
	}
}

void CDirectoryTreeCtrl::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	if (point.x != -1 || point.y != -1) {
		CRect rcClient;
		GetClientRect(&rcClient);
		ClientToScreen(&rcClient);
		if (!rcClient.PtInRect(point)) {
			Default();
			return;
		}
	}

	CPoint ptMenu(-1, -1);
	if (point.x != -1 && point.y != -1)
	{
		ptMenu = point;
		ScreenToClient(&point);
	}
	else
	{
		HTREEITEM hSel = GetNextItem(TVI_ROOT, TVGN_CARET);
		if (hSel)
		{
			CRect rcItem;
			if (GetItemRect(hSel, &rcItem, TRUE))
			{
				ptMenu.x = rcItem.left;
				ptMenu.y = rcItem.top;
				ClientToScreen(&ptMenu);
			}
		}
		else
		{
			ptMenu.SetPoint(0, 0);
			ClientToScreen(&ptMenu);
		}
	}

	HTREEITEM hItem = HitTest(point);

	// create the menu
	CTitleMenu SharedMenu;
	SharedMenu.CreatePopupMenu();
	SharedMenu.AddMenuTitle(GetResString(IDS_SHAREDFOLDERS));
	bool bMenuIsEmpty = true;

	// add all shared directories
	int iCnt = 0;
	for (POSITION pos = m_lstShared.GetHeadPosition(); pos != NULL; iCnt++)
	{
		CString strDisplayPath(m_lstShared.GetNext(pos));
		PathRemoveBackslash(strDisplayPath.GetBuffer(strDisplayPath.GetLength()));
		strDisplayPath.ReleaseBuffer();
		SharedMenu.AppendMenu(MF_STRING,MP_SHAREDFOLDERS_FIRST+iCnt, GetResString(IDS_VIEW1) + strDisplayPath);
		bMenuIsEmpty = false;
	}

	// add right clicked folder, if any
	if (hItem)
	{
		m_strLastRightClicked = GetFullPath(hItem);
		if (!IsShared(m_strLastRightClicked))
		{
			CString strDisplayPath(m_strLastRightClicked);
			PathRemoveBackslash(strDisplayPath.GetBuffer(strDisplayPath.GetLength()));
			strDisplayPath.ReleaseBuffer();
			if (!bMenuIsEmpty)
				SharedMenu.AppendMenu(MF_SEPARATOR);
			SharedMenu.AppendMenu(MF_STRING, MP_SHAREDFOLDERS_FIRST-1, GetResString(IDS_VIEW1) + strDisplayPath + GetResString(IDS_VIEW2));
			bMenuIsEmpty = false;
		}
	}

	// display menu
	if (!bMenuIsEmpty)
		SharedMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, ptMenu.x, ptMenu.y, this);
	VERIFY( SharedMenu.DestroyMenu() );
}

void CDirectoryTreeCtrl::OnRButtonDown(UINT /*nFlags*/, CPoint /*point*/)
{
	// catch WM_RBUTTONDOWN and do not route it the default way.. otherwise we won't get a WM_CONTEXTMENU.
	//CTreeCtrl::OnRButtonDown(nFlags, point);
}

BOOL CDirectoryTreeCtrl::OnCommand(WPARAM wParam, LPARAM /*lParam*/)
{
	if (wParam < MP_SHAREDFOLDERS_FIRST)
	{
		ShellExecute(NULL, _T("open"), m_strLastRightClicked, NULL, NULL, SW_SHOW);
	}
	else
	{
		POSITION pos = m_lstShared.FindIndex(wParam - MP_SHAREDFOLDERS_FIRST);
		if (pos)
			ShellExecute(NULL, _T("open"), m_lstShared.GetAt(pos), NULL, NULL, SW_SHOW);
	}

	return TRUE;
}

void CDirectoryTreeCtrl::OnTvnDeleteItem(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	if (pNMTreeView->itemOld.lParam)
		delete (STreeItem*)pNMTreeView->itemOld.lParam;
	*pResult = 0;
}
