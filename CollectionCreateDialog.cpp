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
#include "emuledlg.h"
#include "CollectionCreateDialog.h"
#include "OtherFunctions.h"
#include "Collection.h"
#include "Sharedfilelist.h"
#include "CollectionFile.h"
#include "KnownFile.h"
#include "KnownFileList.h"
#include "PartFile.h"
#include "TransferDlg.h"
#include "DownloadListCtrl.h"
#pragma warning(disable:4516) // access-declarations are deprecated; member using-declarations provide a better alternative
#pragma warning(disable:4244) // conversion from 'type1' to 'type2', possible loss of data
#pragma warning(disable:4100) // unreferenced formal parameter
#pragma warning(disable:4702) // unreachable code
#include <crypto51/rsa.h>
#include <crypto51/base64.h>
#include <crypto51/osrng.h>
#include <crypto51/files.h>
#include <crypto51/sha.h>
#pragma warning(default:4702) // unreachable code
#pragma warning(default:4100) // unreferenced formal parameter
#pragma warning(default:4244) // conversion from 'type1' to 'type2', possible loss of data
#pragma warning(default:4516) // access-declarations are deprecated; member using-declarations provide a better alternative
#include "Preferences.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define	PREF_INI_SECTION	_T("CollectionCreateDlg")

enum ECols
{
	colName = 0,
	colSize,
	colHash
};

IMPLEMENT_DYNAMIC(CCollectionCreateDialog, CDialog)

BEGIN_MESSAGE_MAP(CCollectionCreateDialog, CResizableDialog)
	ON_BN_CLICKED(IDC_CCOLL_CANCEL, OnCancel)
	ON_BN_CLICKED(IDC_CCOLL_SAVE, OnBnClickedOk)
	ON_BN_CLICKED(IDC_COLLECTIONADD, OnBnClickedCollectionAdd)
	ON_BN_CLICKED(IDC_COLLECTIONCREATEFORMAT, OnBnClickedCollectionFormat)
	ON_BN_CLICKED(IDC_COLLECTIONREMOVE, OnBnClickedCollectionRemove)
	ON_BN_CLICKED(IDC_COLLECTIONVIEWSHAREBUTTON, OnBnClickedCollectionViewShared)
	ON_EN_KILLFOCUS(IDC_COLLECTIONNAMEEDIT, OnEnKillFocusCollectionName)
	ON_NOTIFY(NM_DBLCLK, IDC_COLLECTIONAVAILLIST, OnNmDblClkCollectionAvailList)
	ON_NOTIFY(NM_DBLCLK, IDC_COLLECTIONLISTCTRL, OnNmDblClkCollectionList)
END_MESSAGE_MAP()

CCollectionCreateDialog::CCollectionCreateDialog(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CCollectionCreateDialog::IDD, pParent)
	, m_pCollection(NULL)
	, m_bSharedFiles(false)
{
	m_icoWnd = NULL;
	m_icoForward = NULL;
	m_icoBack = NULL;
	m_icoColl = NULL;
	m_icoFiles = NULL;
	m_bCreatemode = false;
}

CCollectionCreateDialog::~CCollectionCreateDialog()
{
	if (m_icoWnd)
		VERIFY( DestroyIcon(m_icoWnd) );
	if (m_icoForward)
		VERIFY( DestroyIcon(m_icoForward) );
	if (m_icoBack)
		VERIFY( DestroyIcon(m_icoBack) );
	if (m_icoColl)
		VERIFY( DestroyIcon(m_icoColl) );
	if (m_icoFiles)
		VERIFY( DestroyIcon(m_icoFiles) );
}

void CCollectionCreateDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COLLECTIONLISTCTRL, m_CollectionListCtrl);
	DDX_Control(pDX, IDC_COLLECTIONAVAILLIST, m_CollectionAvailListCtrl);
	DDX_Control(pDX, IDC_COLLECTIONNAMEEDIT, m_CollectionNameEdit);
	DDX_Control(pDX, IDC_COLLECTIONVIEWSHAREBUTTON, m_CollectionViewShareButton);
	DDX_Control(pDX, IDC_COLLECTIONADD, m_AddCollectionButton);
	DDX_Control(pDX, IDC_COLLECTIONREMOVE, m_RemoveCollectionButton);
	DDX_Control(pDX, IDC_COLLECTIONLISTLABEL, m_CollectionListLabel);
	DDX_Control(pDX, IDC_CCOLL_SAVE, m_SaveButton);
	DDX_Control(pDX, IDC_CCOLL_CANCEL, m_CancelButton);
	DDX_Control(pDX, IDC_COLLECTIONLISTICON, m_CollectionListIcon);
	DDX_Control(pDX, IDC_COLLECTIONSOURCELISTICON, m_CollectionSourceListIcon);
	DDX_Control(pDX, IDC_COLLECTIONCREATESIGNCHECK, m_CollectionCreateSignNameKeyCheck);
	DDX_Control(pDX, IDC_COLLECTIONCREATEFORMAT, m_CollectionCreateFormatCheck);
}

void CCollectionCreateDialog::SetCollection(CCollection* pCollection, bool create)
{
	if (!pCollection) {
		ASSERT(0);
		return;
	}
	m_pCollection = pCollection;
	m_bCreatemode = create;
}

BOOL CCollectionCreateDialog::OnInitDialog(void)
{
	CDialog::OnInitDialog();
	InitWindowStyles(this);

	if (!m_pCollection) {
		ASSERT(0);
		return TRUE;
	}
	SetIcon(m_icoWnd = theApp.LoadIcon(_T("AABCollectionFileType")), FALSE);
	if (m_bCreatemode)
		SetWindowText(GetResString(IDS_CREATECOLLECTION));
	else
		SetWindowText(GetResString(IDS_MODIFYCOLLECTION) + _T(": ") + m_pCollection->m_sCollectionName);

	m_CollectionListCtrl.Init(_T("CollectionCreateR"));
	m_CollectionAvailListCtrl.Init(_T("CollectionCreateL"));

	m_AddCollectionButton.SetIcon(m_icoForward = theApp.LoadIcon(_T("FORWARD")));
	m_RemoveCollectionButton.SetIcon(m_icoBack = theApp.LoadIcon(_T("BACK")));
	m_CollectionListIcon.SetIcon(m_icoColl = theApp.LoadIcon(_T("AABCollectionFileType")));
	m_CollectionSourceListIcon.SetIcon(m_icoFiles = theApp.LoadIcon(_T("SharedFilesList")));

	m_SaveButton.SetWindowText(GetResString(IDS_SAVE));
	m_CancelButton.SetWindowText(GetResString(IDS_CANCEL));
	m_CollectionCreateSignNameKeyCheck.SetWindowText(GetResString(IDS_COLL_SIGN));
	m_CollectionCreateFormatCheck.SetWindowText(GetResString(IDS_COLL_TEXTFORMAT));
	SetDlgItemText(IDC_CCOLL_STATIC_NAME, GetResString(IDS_SW_NAME) + _T(":"));
	SetDlgItemText(IDC_CCOLL_BASICOPTIONS, GetResString(IDS_LD_BASICOPT));
	SetDlgItemText(IDC_CCOLL_ADVANCEDOPTIONS, GetResString(IDS_LD_ADVANCEDOPT));

	AddAnchor(IDC_COLLECTIONAVAILLIST, TOP_LEFT, BOTTOM_CENTER);
	AddAnchor(IDC_COLLECTIONLISTCTRL, TOP_CENTER, BOTTOM_RIGHT);
	AddAnchor(IDC_COLLECTIONLISTLABEL, TOP_CENTER);
	AddAnchor(IDC_COLLECTIONLISTICON, TOP_CENTER);
	AddAnchor(IDC_COLLECTIONADD, MIDDLE_CENTER);
	AddAnchor(IDC_COLLECTIONREMOVE, MIDDLE_CENTER);
	AddAnchor(IDC_CCOLL_SAVE, BOTTOM_RIGHT);
	AddAnchor(IDC_CCOLL_CANCEL, BOTTOM_RIGHT);
	AddAnchor(IDC_CCOLL_BASICOPTIONS, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_CCOLL_ADVANCEDOPTIONS, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_CCOLL_STATIC_NAME, BOTTOM_LEFT);
	AddAnchor(IDC_COLLECTIONNAMEEDIT, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_COLLECTIONCREATEFORMAT, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_COLLECTIONCREATESIGNCHECK, BOTTOM_LEFT, BOTTOM_RIGHT);
	EnableSaveRestore(PREF_INI_SECTION);

	POSITION pos = m_pCollection->m_CollectionFilesMap.GetStartPosition();
	while (pos != NULL)
	{
		CSKey key;
		CCollectionFile* pCollectionFile;
		m_pCollection->m_CollectionFilesMap.GetNextAssoc(pos, key, pCollectionFile);
		m_CollectionListCtrl.AddFileToList(pCollectionFile);
	}
	CString strTitle;
	strTitle.Format(GetResString(IDS_COLLECTIONLIST) + _T(" (%u)"), m_CollectionListCtrl.GetItemCount());
	m_CollectionListLabel.SetWindowText(strTitle);

	OnBnClickedCollectionViewShared();
	
	m_CollectionNameEdit.SetWindowText(::CleanupFilename(m_pCollection->m_sCollectionName));
	m_CollectionCreateFormatCheck.SetCheck(m_pCollection->m_bTextFormat);
	OnBnClickedCollectionFormat();
	GetDlgItem(IDC_CCOLL_SAVE)->EnableWindow(m_CollectionListCtrl.GetItemCount() > 0);

	return TRUE;
}

void CCollectionCreateDialog::AddSelectedFiles(void)
{
	CTypedPtrList<CPtrList, CKnownFile*> knownFileList;
	POSITION pos = m_CollectionAvailListCtrl.GetFirstSelectedItemPosition();
	while (pos != NULL)
	{
		int index = m_CollectionAvailListCtrl.GetNextSelectedItem(pos);
		if (index >= 0)
			knownFileList.AddTail((CKnownFile*)m_CollectionAvailListCtrl.GetItemData(index));
	}

	while (knownFileList.GetCount() > 0)
	{
		CAbstractFile* pAbstractFile = knownFileList.RemoveHead();
		CCollectionFile* pCollectionFile = m_pCollection->AddFileToCollection(pAbstractFile, true);
		if (pCollectionFile)
			m_CollectionListCtrl.AddFileToList(pCollectionFile);
	}

	CString strTitle;
	strTitle.Format(GetResString(IDS_COLLECTIONLIST) + _T(" (%u)"), m_CollectionListCtrl.GetItemCount());
	m_CollectionListLabel.SetWindowText(strTitle);

	GetDlgItem(IDC_CCOLL_SAVE)->EnableWindow(m_CollectionListCtrl.GetItemCount() > 0);
}

void CCollectionCreateDialog::RemoveSelectedFiles(void)
{
	CTypedPtrList<CPtrList, CCollectionFile*> collectionFileList;
	POSITION pos = m_CollectionListCtrl.GetFirstSelectedItemPosition();
	while (pos != NULL)
	{
		int index = m_CollectionListCtrl.GetNextSelectedItem(pos);
		if (index >= 0)
			collectionFileList.AddTail((CCollectionFile*)m_CollectionListCtrl.GetItemData(index));
	}

	while (collectionFileList.GetCount() > 0)
	{
		CCollectionFile* pCollectionFile = collectionFileList.RemoveHead();
		m_CollectionListCtrl.RemoveFileFromList(pCollectionFile);
		m_pCollection->RemoveFileFromCollection(pCollectionFile);
	}

	CString strTitle;
	strTitle.Format(GetResString(IDS_COLLECTIONLIST) + _T(" (%u)"), m_CollectionListCtrl.GetItemCount());
	m_CollectionListLabel.SetWindowText(strTitle);
	GetDlgItem(IDC_CCOLL_SAVE)->EnableWindow(m_CollectionListCtrl.GetItemCount() > 0);
}

void CCollectionCreateDialog::OnBnClickedCollectionRemove()
{
	RemoveSelectedFiles();
}

void CCollectionCreateDialog::OnBnClickedCollectionAdd()
{
	AddSelectedFiles();
}

void CCollectionCreateDialog::OnBnClickedOk()
{
	//Some users have noted that the collection can at times
	//save a collection with a invalid name...
	OnEnKillFocusCollectionName();

	CString sFileName;
	m_CollectionNameEdit.GetWindowText(sFileName);
	if (!sFileName.IsEmpty())
	{
		m_pCollection->m_sCollectionAuthorName.Empty();
		m_pCollection->SetCollectionAuthorKey(NULL, 0);
		m_pCollection->m_sCollectionName = sFileName;
		m_pCollection->m_bTextFormat = (m_CollectionCreateFormatCheck.GetCheck() == BST_CHECKED);

		CString sFilePath;
		sFilePath.Format(_T("%s\\%s.emulecollection"), thePrefs.GetMuleDirectory(EMULE_INCOMINGDIR), m_pCollection->m_sCollectionName);

		using namespace CryptoPP;
		RSASSA_PKCS1v15_SHA_Signer* pSignkey = NULL;
		if (m_CollectionCreateSignNameKeyCheck.GetCheck())
		{
			bool bCreateNewKey = false;
			HANDLE hKeyFile = ::CreateFile(thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + _T("collectioncryptkey.dat"), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hKeyFile != INVALID_HANDLE_VALUE)
			{
				if (::GetFileSize(hKeyFile, NULL) == 0)
					bCreateNewKey = true;
				::CloseHandle(hKeyFile);
			}
			else
				bCreateNewKey = true;

			if (bCreateNewKey)
			{
				try
				{
					AutoSeededRandomPool rng;
					InvertibleRSAFunction privkey;
					privkey.Initialize(rng, 1024);
					Base64Encoder privkeysink(new FileSink(CStringA(thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + _T("collectioncryptkey.dat"))));
					privkey.DEREncode(privkeysink);
					privkeysink.MessageEnd();
				}
				catch(...)
				{
					ASSERT(0);
				}
			}

			try
			{
				FileSource filesource(CStringA(thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + _T("collectioncryptkey.dat")), true,new Base64Decoder);
				pSignkey = new RSASSA_PKCS1v15_SHA_Signer(filesource);
				RSASSA_PKCS1v15_SHA_Verifier pubkey(*pSignkey);
				byte abyMyPublicKey[1000];
				ArraySink asink(abyMyPublicKey, 1000);
				pubkey.DEREncode(asink);
				int nLen = asink.TotalPutLength();
				asink.MessageEnd();
				m_pCollection->SetCollectionAuthorKey(abyMyPublicKey, nLen);
			}
			catch(...)
			{
				ASSERT(0);
			}

			m_pCollection->m_sCollectionAuthorName = thePrefs.GetUserNick();
		}

		if (!PathFileExists(sFilePath))
		{
			m_pCollection->WriteToFileAddShared(pSignkey);
		}
		else
		{
			if (AfxMessageBox(GetResString(IDS_COLL_REPLACEEXISTING), MB_ICONWARNING | MB_ICONQUESTION | MB_DEFBUTTON2 | MB_YESNO) == IDNO)
				return;

			bool bDeleteSuccessful = ShellDeleteFile(sFilePath);
			if (bDeleteSuccessful)
			{
				CKnownFile* pKnownFile = theApp.knownfiles->FindKnownFileByPath(sFilePath);
				if (pKnownFile)
				{
					theApp.sharedfiles->RemoveFile(pKnownFile, true);
					if (pKnownFile->IsKindOf(RUNTIME_CLASS(CPartFile)))
						theApp.emuledlg->transferwnd->GetDownloadList()->ClearCompleted(static_cast<CPartFile*>(pKnownFile));
				}
				m_pCollection->WriteToFileAddShared(pSignkey);
			}
			else
			{
				AfxMessageBox(GetResString(IDS_COLL_ERR_DELETING),MB_ICONWARNING | MB_ICONQUESTION | MB_DEFBUTTON2 | MB_YESNO);
			}
		}
		
		delete pSignkey;
		pSignkey = NULL;

		OnOK();
	}
}

void CCollectionCreateDialog::UpdateAvailFiles(void)
{
	m_CollectionAvailListCtrl.DeleteAllItems();

	CMap<CCKey,const CCKey&,CKnownFile*,CKnownFile*> Files_Map;
	if (m_bSharedFiles)
		theApp.sharedfiles->CopySharedFileMap(Files_Map);
	else
		theApp.knownfiles->CopyKnownFileMap(Files_Map);

	POSITION pos = Files_Map.GetStartPosition();
	while (pos != NULL)
	{
		CCKey key;
		CKnownFile* pKnownFile;
		Files_Map.GetNextAssoc(pos, key, pKnownFile);
		m_CollectionAvailListCtrl.AddFileToList(pKnownFile);
	}
}

void CCollectionCreateDialog::OnBnClickedCollectionViewShared()
{
	m_bSharedFiles = !m_bSharedFiles;
	UpdateAvailFiles();
	CString strTitle;
	strTitle.Format(_T("   ") + (m_bSharedFiles ? GetResString(IDS_SHARED) : GetResString(IDS_KNOWN)) + _T(" (%u)"), m_CollectionAvailListCtrl.GetItemCount());
	m_CollectionViewShareButton.SetWindowText(strTitle);
}

void CCollectionCreateDialog::OnNmDblClkCollectionAvailList(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	AddSelectedFiles();
	*pResult = 0;
}

void CCollectionCreateDialog::OnNmDblClkCollectionList(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	RemoveSelectedFiles();
	*pResult = 0;
}

void CCollectionCreateDialog::OnEnKillFocusCollectionName()
{
	CString sFileName;
	CString sNewFileName;
	m_CollectionNameEdit.GetWindowText(sFileName);
	sNewFileName = ValidFilename(sFileName);
	if (sNewFileName.Compare(sFileName))
		m_CollectionNameEdit.SetWindowText(sNewFileName);
}

void CCollectionCreateDialog::OnBnClickedCollectionFormat()
{
	if (m_CollectionCreateFormatCheck.GetCheck()) {
		m_CollectionCreateSignNameKeyCheck.SetCheck(BST_UNCHECKED);
		m_CollectionCreateSignNameKeyCheck.EnableWindow(FALSE);
	}
	else
		m_CollectionCreateSignNameKeyCheck.EnableWindow(TRUE);
}
