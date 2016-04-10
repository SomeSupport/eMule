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
#include "kademlia/kademlia/tag.h"
#include "MetaDataDlg.h"
#include "OtherFunctions.h"
#include "Preferences.h"
#include "MenuCmds.h"
#include "Packets.h"
#include "KnownFile.h"
#include "UserMsgs.h"
#include "MediaInfo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//////////////////////////////////////////////////////////////////////////////
// COLUMN_INIT -- List View Columns

enum EMetaDataCols
{
	META_DATA_COL_NAME = 0,
	META_DATA_COL_TYPE,
	META_DATA_COL_VALUE
};

static LCX_COLUMN_INIT s_aColumns[] =
{
	{ META_DATA_COL_NAME,	_T("Name"),		IDS_SW_NAME,	LVCFMT_LEFT,	-1, 0, ASCENDING,  NONE, _T("Temporary file MMMMM") },
	{ META_DATA_COL_TYPE,	_T("Type"),		IDS_TYPE,		LVCFMT_LEFT,	-1, 1, ASCENDING,  NONE, _T("Integer") },
	{ META_DATA_COL_VALUE,	_T("Value"),	IDS_VALUE,		LVCFMT_LEFT,	-1, 2, ASCENDING,  NONE, _T("long long long long long long long long file name.avi") }
};

#define	PREF_INI_SECTION	_T("MetaDataDlg")

// CMetaDataDlg dialog

IMPLEMENT_DYNAMIC(CMetaDataDlg, CResizablePage)

BEGIN_MESSAGE_MAP(CMetaDataDlg, CResizablePage)
	ON_COMMAND(MP_COPYSELECTED, OnCopyTags)
	ON_COMMAND(MP_SELECTALL, OnSelectAllTags)
	ON_MESSAGE(UM_DATA_CHANGED, OnDataChanged)
	ON_NOTIFY(LVN_KEYDOWN, IDC_TAGS, OnLvnKeydownTags)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

CMetaDataDlg::CMetaDataDlg()
	: CResizablePage(CMetaDataDlg::IDD, 0)
{
	m_paFiles = NULL;
	m_bDataChanged = false;
	m_taglist = NULL;
	m_strCaption = GetResString(IDS_META_DATA);
	m_psp.pszTitle = m_strCaption;
	m_psp.dwFlags |= PSP_USETITLE;
	m_pMenuTags = NULL;
	m_tags.m_pParent = this;
	m_tags.SetRegistryKey(PREF_INI_SECTION);
	m_tags.SetRegistryPrefix(_T("MetaDataTags_"));
}

CMetaDataDlg::~CMetaDataDlg()
{
	delete m_pMenuTags;
}

void CMetaDataDlg::SetTagList(Kademlia::TagList* taglist)
{
	m_taglist = taglist;
}

void CMetaDataDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizablePage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAGS, m_tags);
}

BOOL CMetaDataDlg::OnInitDialog()
{
	CResizablePage::OnInitDialog();
	InitWindowStyles(this);

	AddAnchor(IDC_TAGS, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_TOTAL_TAGS, BOTTOM_LEFT, BOTTOM_RIGHT);
	
	GetDlgItem(IDC_TOTAL_TAGS)->SetWindowText(GetResString(IDS_METATAGS));

	m_tags.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_GRIDLINES);
	m_tags.ReadColumnStats(ARRSIZE(s_aColumns), s_aColumns);
	m_tags.CreateColumns(ARRSIZE(s_aColumns), s_aColumns);

	m_pMenuTags = new CMenu();
	if (m_pMenuTags->CreatePopupMenu())
	{
		m_pMenuTags->AppendMenu(MF_ENABLED | MF_STRING, MP_COPYSELECTED, GetResString(IDS_COPY));
		m_pMenuTags->AppendMenu(MF_SEPARATOR);
		m_pMenuTags->AppendMenu(MF_ENABLED | MF_STRING, MP_SELECTALL, GetResString(IDS_SELECTALL));
	}
	m_tags.m_pMenu = m_pMenuTags;
	m_tags.m_pParent = this;

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CMetaDataDlg::OnSetActive()
{
	if (!CResizablePage::OnSetActive())
		return FALSE;
	if (m_bDataChanged)
	{
		RefreshData();
		m_bDataChanged = false;
	}
	return TRUE;
}

LRESULT CMetaDataDlg::OnDataChanged(WPARAM, LPARAM)
{
	m_bDataChanged = true;
	return 1;
}

CString GetTagNameByID(UINT id)
{
	switch (id)
	{
		case FT_FILENAME:			return GetResString(IDS_SW_NAME);
		case FT_FILESIZE:			return GetResString(IDS_DL_SIZE);
		case FT_FILETYPE:			return GetResString(IDS_TYPE);
		case FT_FILEFORMAT:			return GetResString(IDS_FORMAT);
		case FT_LASTSEENCOMPLETE:	return GetResString(IDS_LASTSEENCOMPL);	// 01-Nov-2004: Sent in server<->client protocol as 'Last Seen Complete'
		case 0x06:					return GetResString(IDS_META_PARTPATH);
		case 0x07:					return GetResString(IDS_META_PARTHASH);
		case FT_TRANSFERRED:		return GetResString(IDS_DL_TRANSF);
		case FT_GAPSTART:			return GetResString(IDS_META_GAPSTART);
		case FT_GAPEND:				return GetResString(IDS_META_GAPEND);
		case 0x0B:					return GetResString(IDS_DESCRIPTION);
		case 0x0C:					return GetResString(IDS_PING);
		case 0x0D:					return GetResString(IDS_FAIL);
		case 0x0E:					return GetResString(IDS_META_PREFERENCES);
		case 0x0F:					return GetResString(IDS_PORT);
		case 0x10:					return GetResString(IDS_IP);
		case 0x11:					return GetResString(IDS_META_VERSION);
		case FT_PARTFILENAME:		return GetResString(IDS_META_TEMPFILE);
		case 0x13:					return GetResString(IDS_PRIORITY);
		case FT_STATUS:				return GetResString(IDS_STATUS);
		case FT_SOURCES:			return GetResString(IDS_SEARCHAVAIL);
		case 0x16:					return GetResString(IDS_PERMISSION);
		case 0x17:					return GetResString(IDS_FD_PARTS);
		case FT_COMPLETE_SOURCES:	return GetResString(IDS_COMPLSOURCES);
		case FT_MEDIA_ARTIST:		return GetResString(IDS_ARTIST);
		case FT_MEDIA_ALBUM:		return GetResString(IDS_ALBUM);
		case FT_MEDIA_TITLE:		return GetResString(IDS_TITLE);
		case FT_MEDIA_LENGTH:		return GetResString(IDS_LENGTH);
		case FT_MEDIA_BITRATE:		return GetResString(IDS_BITRATE);
		case FT_MEDIA_CODEC:		return GetResString(IDS_CODEC);
		case FT_FILECOMMENT:		return GetResString(IDS_COMMENT);
		case FT_FILERATING:			return GetResString(IDS_QL_RATING);
		case FT_FILEHASH:			return GetResString(IDS_FILEHASH);
		case 0xFA:					return GetResString(IDS_META_SERVERPORT);
		case 0xFB:					return GetResString(IDS_META_SERVERIP);
		case 0xFC:					return GetResString(IDS_META_SRCUDPPORT);
		case 0xFD:					return GetResString(IDS_META_SRCTCPPORT);
		case 0xFE:					return GetResString(IDS_META_SRCIP);
		case 0xFF:					return GetResString(IDS_META_SRCTYPE);
	}

	CString buffer;
	buffer.Format(_T("Tag0x%02X"), id);
	return buffer;
}

CString GetMetaTagName(UINT uTagID)
{
	CString strName = GetTagNameByID(uTagID);
	StripTrailingCollon(strName);
	return strName;
}

CString GetName(const CTag* pTag)
{
	CString strName;
	if (pTag->GetNameID())
		strName = GetTagNameByID(pTag->GetNameID());
	else
		strName = pTag->GetName();
	StripTrailingCollon(strName);
	return strName;
}

CString GetName(const Kademlia::CKadTag* pTag)
{
	CString strName;
	if (pTag->m_name.GetLength() == 1)
		strName = GetTagNameByID((BYTE)pTag->m_name[0]);
	else
		strName = (LPCSTR)pTag->m_name;
	StripTrailingCollon(strName);
	return strName;
}

CString GetValue(const CTag* pTag)
{
	CString strValue;
	if (pTag->IsStr())
	{
		strValue = pTag->GetStr();
		if (pTag->GetNameID() == FT_MEDIA_CODEC)
			strValue = GetCodecDisplayName(strValue);
	}
	else if (pTag->IsInt())
	{
		if (pTag->GetNameID() == FT_MEDIA_LENGTH || pTag->GetNameID() == FT_LASTSEENCOMPLETE)
			SecToTimeLength(pTag->GetInt(), strValue);
		else if (pTag->GetNameID() == FT_FILERATING)
			strValue = GetRateString(pTag->GetInt());
		else if (pTag->GetNameID() == 0x10 || pTag->GetNameID() >= 0xFA)
			strValue.Format(_T("%u"), pTag->GetInt());
		else
			strValue = GetFormatedUInt(pTag->GetInt());
	}
	else if (pTag->IsFloat())
		strValue.Format(_T("%f"), pTag->GetFloat());
	else if (pTag->IsHash())
		strValue = md4str(pTag->GetHash());
	else if (pTag->IsInt64(false))
		strValue = GetFormatedUInt64(pTag->GetInt64());
	else
		strValue.Format(_T("<Unknown value of type 0x%02X>"), pTag->GetType());
	return strValue;
}

CString GetValue(const Kademlia::CKadTag* pTag) // FIXME LARGE FILES
{
	CString strValue;
	if (pTag->IsStr())
	{
		strValue = pTag->GetStr();
		if (pTag->m_name.Compare(TAG_MEDIA_CODEC) == 0)
			strValue = GetCodecDisplayName(strValue);
	}
	else if (pTag->IsInt())
	{
		if (pTag->m_name.Compare(TAG_MEDIA_LENGTH) == 0)
			SecToTimeLength((unsigned long)pTag->GetInt(), strValue);
		else if (pTag->m_name.Compare(TAG_FILERATING) == 0)
			strValue = GetRateString((UINT)pTag->GetInt());
		else if ((BYTE)pTag->m_name[0] == 0x10 || (BYTE)pTag->m_name[0] >= 0xFA)
			strValue.Format(_T("%u"), pTag->GetInt());
		else
			strValue = GetFormatedUInt((UINT)pTag->GetInt());
	}
	else if (pTag->m_type == 4)
		strValue.Format(_T("%f"), pTag->GetFloat());
	else if (pTag->m_type == 5)
		strValue.Format(_T("%u"), pTag->GetInt());
	else
		strValue.Format(_T("<Unknown value of type 0x%02X>"), pTag->m_type);
	return strValue;
}

CString GetType(UINT uType)
{
	CString strValue;
	if (uType == 1)
		strValue = _T("Hash");
	else if (uType == 2)
		strValue = _T("String");
	else if (uType == 3)
		strValue = _T("Int32");
	else if (uType == 4)
		strValue = _T("Float");
	else if (uType == 5)
		strValue = _T("Bool");
	else if (uType == 8)
		strValue = _T("Int16");
	else if (uType == 9)
		strValue = _T("Int8");
	else if (uType == 11)
		strValue = _T("Int64");
	else
		strValue.Format(_T("<Unknown type 0x%02X>"), uType);
	return strValue;
}

void CMetaDataDlg::RefreshData()
{
	CWaitCursor curWait;
	m_tags.DeleteAllItems();
	m_tags.SetRedraw(FALSE);

	int iMetaTags = 0;
	if (m_paFiles->GetSize() >= 1)
	{
		CString strBuff;
		if (!STATIC_DOWNCAST(CAbstractFile, (*m_paFiles)[0])->HasNullHash())
		{
			LVITEM lvi;
			lvi.mask = LVIF_TEXT;
			lvi.iItem = INT_MAX;
			lvi.iSubItem = META_DATA_COL_NAME;
			strBuff = GetResString(IDS_FD_HASH);
			StripTrailingCollon(strBuff);
			lvi.pszText = const_cast<LPTSTR>((LPCTSTR)strBuff);
			int iItem = m_tags.InsertItem(&lvi);
			if (iItem >= 0)
			{
				lvi.mask = LVIF_TEXT;
				lvi.iItem = iItem;

				strBuff.Empty(); // intentionally left blank as it's not a real meta tag
				lvi.pszText = const_cast<LPTSTR>((LPCTSTR)strBuff);
				lvi.iSubItem = META_DATA_COL_TYPE;
				m_tags.SetItem(&lvi);


				strBuff = md4str(STATIC_DOWNCAST(CAbstractFile, (*m_paFiles)[0])->GetFileHash());
				lvi.pszText = const_cast<LPTSTR>((LPCTSTR)strBuff);
				lvi.iSubItem = META_DATA_COL_VALUE;
				m_tags.SetItem(&lvi);
			}
		}

		const CArray<CTag*,CTag*>& aTags = STATIC_DOWNCAST(CAbstractFile, (*m_paFiles)[0])->GetTags();
		int iTags = aTags.GetCount();
		for (int i = 0; i < iTags; i++)
		{
			const CTag* pTag = aTags.GetAt(i);
			LVITEM lvi;
			lvi.mask = LVIF_TEXT;
			lvi.iItem = INT_MAX;
			lvi.iSubItem = META_DATA_COL_NAME;
			strBuff = GetName(pTag);
			lvi.pszText = const_cast<LPTSTR>((LPCTSTR)strBuff);
			int iItem = m_tags.InsertItem(&lvi);
			if (iItem >= 0)
			{
				lvi.mask = LVIF_TEXT;
				lvi.iItem = iItem;

				strBuff = GetType(pTag->GetType());
				lvi.pszText = const_cast<LPTSTR>((LPCTSTR)strBuff);
				lvi.iSubItem = META_DATA_COL_TYPE;
				m_tags.SetItem(&lvi);

				strBuff = GetValue(pTag);
				lvi.pszText = const_cast<LPTSTR>((LPCTSTR)strBuff);
				lvi.iSubItem = META_DATA_COL_VALUE;
				m_tags.SetItem(&lvi);

				iMetaTags++;
			}
		}
	}
	else if (m_taglist != NULL)
	{
		const Kademlia::CKadTag* pTag;
		Kademlia::TagList::const_iterator it;
		for (it = m_taglist->begin(); it != m_taglist->end(); it++)
		{
			pTag = *it;
			CString strBuff;
			LVITEM lvi;
			lvi.mask = LVIF_TEXT;
			lvi.iItem = INT_MAX;
			lvi.iSubItem = META_DATA_COL_NAME;
			strBuff = GetName(pTag);
			lvi.pszText = const_cast<LPTSTR>((LPCTSTR)strBuff);
			int iItem = m_tags.InsertItem(&lvi);
			if (iItem >= 0)
			{
				lvi.mask = LVIF_TEXT;
				lvi.iItem = iItem;

				strBuff = GetType(pTag->m_type);
				lvi.pszText = const_cast<LPTSTR>((LPCTSTR)strBuff);
				lvi.iSubItem = META_DATA_COL_TYPE;
				m_tags.SetItem(&lvi);

				strBuff = GetValue(pTag);
				lvi.pszText = const_cast<LPTSTR>((LPCTSTR)strBuff);
				lvi.iSubItem = META_DATA_COL_VALUE;
				m_tags.SetItem(&lvi);

				iMetaTags++;
			}
		}
	}
	CString strTmp;
	strTmp.Format(_T("%s %u"), GetResString(IDS_METATAGS), iMetaTags);
	SetDlgItemText(IDC_TOTAL_TAGS, strTmp);
	m_tags.SetRedraw();
}

void CMetaDataDlg::OnCopyTags()
{
	CWaitCursor curWait;
	int iSelected = 0;
	CString strData;
	POSITION pos = m_tags.GetFirstSelectedItemPosition();
	while (pos)
	{
		int iItem = m_tags.GetNextSelectedItem(pos);
		CString strValue = m_tags.GetItemText(iItem, META_DATA_COL_VALUE);

		if (!strValue.IsEmpty())
		{
			if (!strData.IsEmpty())
				strData += _T("\r\n");
			strData += strValue;
			iSelected++;
		}
	}

	if (!strData.IsEmpty()){
		if (iSelected > 1)
			strData += _T("\r\n");
		theApp.CopyTextToClipboard(strData);
	}
}

void CMetaDataDlg::OnSelectAllTags()
{
	m_tags.SelectAllItems();
}

void CMetaDataDlg::OnLvnKeydownTags(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVKEYDOWN pLVKeyDown = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);

	if (pLVKeyDown->wVKey == 'C' && (GetKeyState(VK_CONTROL) & 0x8000))
		OnCopyTags();
	*pResult = 0;
}

void CMetaDataDlg::OnDestroy()
{
	m_tags.WriteColumnStats(ARRSIZE(s_aColumns), s_aColumns);
	CResizablePage::OnDestroy();
}
