//--------------------------------------------------------------------------------------------
//  Name:           CCustomAutoComplete (CCUSTOMAUTOCOMPLETE.H)
//  Type:           Wrapper class
//  Description:    Matches IAutoComplete, IEnumString and the registry (optional) to provide
//					custom auto-complete functionality for EDIT controls - including those in
//					combo boxes - in WTL projects.
//
//  Author:         Klaus H. Probst [kprobst@vbbox.com]
//  URL:            http://www.vbbox.com/
//  Copyright:      This work is copyright © 2002, Klaus H. Probst
//  Usage:          You may use this code as you see fit, provided that you assume all
//                  responsibilities for doing so.
//  Distribution:   Distribute freely as long as you maintain this notice as part of the
//					file header.
//
//
//  Updates:        09-Mai-2003 [bluecow]: 
//						- changed original string list code to deal with a LRU list
//						  and auto cleanup of list entries according 'iMaxItemCount'.
//						- splitted original code into cpp/h file
//						- removed registry stuff
//						- added file stuff
//					15-Jan-2004 [Ornis]:
//						- changed adding strings to replace existing ones on a new position
//
//
//  Notes:			
//
//
//  Dependencies:
//
//					The usual ATL/WTL headers for a normal EXE, plus <atlmisc.h>
//
//--------------------------------------------------------------------------------------------
#include "stdafx.h"
#include <share.h>
#include "CustomAutoComplete.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CCustomAutoComplete::CCustomAutoComplete()
{
	InternalInit();
}

CCustomAutoComplete::CCustomAutoComplete(const CStringArray& p_sItemList)
{
	InternalInit();
	SetList(p_sItemList);
}

CCustomAutoComplete::~CCustomAutoComplete()
{
	if (m_pac)
		m_pac.Release();
}

BOOL CCustomAutoComplete::Bind(HWND p_hWndEdit, DWORD p_dwOptions, LPCTSTR p_lpszFormatString)
{
	ATLASSERT(::IsWindow(p_hWndEdit));
	if ((m_fBound) || (m_pac))
		return FALSE;

	HRESULT hr = m_pac.CoCreateInstance(CLSID_AutoComplete);
	if (SUCCEEDED(hr))
	{
		if (p_dwOptions){
			CComQIPtr<IAutoComplete2> pAC2(m_pac);
			if (pAC2){
				pAC2->SetOptions(p_dwOptions);
				pAC2.Release();
			}
		}

		if (SUCCEEDED(hr = m_pac->Init(p_hWndEdit, this, NULL, p_lpszFormatString)))
		{
			m_fBound = TRUE;
			return TRUE;
		}
	}
	return FALSE;
}

VOID CCustomAutoComplete::Unbind()
{
	if (!m_fBound)
		return;
	if (m_pac){
		m_pac.Release();
		m_fBound = FALSE;
	}
}

BOOL CCustomAutoComplete::SetList(const CStringArray& p_sItemList)
{
	ATLASSERT(p_sItemList.GetSize() != 0);
	Clear();
	m_asList.Append(p_sItemList);
	return TRUE;
}

int CCustomAutoComplete::FindItem(const CString& rstr)
{
	for (int i = 0; i < m_asList.GetCount(); i++)
		if (m_asList[i].Compare(rstr) == 0)
			return i;
	return -1;
}

BOOL CCustomAutoComplete::AddItem(const CString& p_sItem, int iPos)
{
	if (p_sItem.GetLength() != 0)
	{
		int oldpos=FindItem(p_sItem);
		if (oldpos == -1)
		{
			// use a LRU list
			if (iPos == -1)
				m_asList.Add(p_sItem);
			else
				m_asList.InsertAt(iPos, p_sItem);

			while (m_asList.GetSize() > m_iMaxItemCount)
				m_asList.RemoveAt(m_asList.GetSize() - 1);
			return TRUE;
		} else if (iPos!=-1) {
			m_asList.RemoveAt(oldpos);
			if (oldpos<iPos) --iPos;
			m_asList.InsertAt(iPos, p_sItem);

			while (m_asList.GetSize() > m_iMaxItemCount)
				m_asList.RemoveAt(m_asList.GetSize() - 1);
			return TRUE;
		}
	}
	return FALSE;
}

int CCustomAutoComplete::GetItemCount()
{
	return (int)m_asList.GetCount();
}

BOOL CCustomAutoComplete::RemoveItem(const CString& p_sItem)
{
	if (p_sItem.GetLength() != 0)
	{
		int iPos = FindItem(p_sItem);
		if (iPos != -1)
		{
			m_asList.RemoveAt(iPos);
			return TRUE;
		}
	}
	return FALSE;
}

BOOL CCustomAutoComplete::RemoveSelectedItem()
{
	if (m_pac == NULL || !IsBound())
		return FALSE;
	CComQIPtr<IAutoCompleteDropDown> pIAutoCompleteDropDown = m_pac;
	if (!pIAutoCompleteDropDown)
		return FALSE;

	DWORD dwFlags;
	LPWSTR pwszItem;
	if (FAILED(pIAutoCompleteDropDown->GetDropDownStatus(&dwFlags, &pwszItem)))
		return FALSE;
	if (dwFlags != ACDD_VISIBLE)
		return FALSE;
	if (pwszItem == NULL)
		return FALSE;
	CString strItem(pwszItem);
	CoTaskMemFree(pwszItem);

	return RemoveItem(strItem);
}

BOOL CCustomAutoComplete::Clear()
{
	if (m_asList.GetSize() != 0)
	{
		m_asList.RemoveAll();
		return TRUE;
	}
	return FALSE;
}

BOOL CCustomAutoComplete::Disable()
{
	if ((!m_pac) || (!m_fBound))
		return FALSE;
	return SUCCEEDED(EnDisable(FALSE));
}

BOOL CCustomAutoComplete::Enable(VOID)
{
	if ((!m_pac) || (m_fBound))
		return FALSE;
	return SUCCEEDED(EnDisable(TRUE));
}

const CStringArray& CCustomAutoComplete::GetList() const
{
	return m_asList;
}

//
//	IUnknown implementation
//
STDMETHODIMP_(ULONG) CCustomAutoComplete::AddRef()
{
	ULONG nCount = ::InterlockedIncrement(reinterpret_cast<LONG*>(&m_nRefCount));
	return nCount;
}

STDMETHODIMP_(ULONG) CCustomAutoComplete::Release()
{
	ULONG nCount = 0;
	nCount = (ULONG) ::InterlockedDecrement(reinterpret_cast<LONG*>(&m_nRefCount));
	if (nCount == 0)
		delete this;
	return nCount;
}

STDMETHODIMP CCustomAutoComplete::QueryInterface(REFIID riid, void** ppvObject)
{
	HRESULT hr = E_NOINTERFACE;
	if (ppvObject != NULL)
	{
		*ppvObject = NULL;

		if (IID_IUnknown == riid)
			*ppvObject = static_cast<IUnknown*>(this);
		else if (IID_IEnumString == riid)
			*ppvObject = static_cast<IEnumString*>(this);
		if (*ppvObject != NULL)
		{
			hr = S_OK;
			((LPUNKNOWN)*ppvObject)->AddRef();
		}
	}
	else
	{
		hr = E_POINTER;
	}
	return hr;
}

//
//	IEnumString implementation
//
STDMETHODIMP CCustomAutoComplete::Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched)
{
	HRESULT hr = S_FALSE;

	if (!celt)
		celt = 1;
	if (pceltFetched)
		*pceltFetched = 0;
	ULONG i;
	for (i = 0; i < celt; i++)
	{
		if (m_nCurrentElement == (ULONG)m_asList.GetSize())
			break;

		rgelt[i] = (LPWSTR)::CoTaskMemAlloc((ULONG) sizeof(WCHAR) * (m_asList[m_nCurrentElement].GetLength() + 1));
		wcscpy(rgelt[i], m_asList[m_nCurrentElement]);

		if (pceltFetched)
			(*pceltFetched)++;
		
		m_nCurrentElement++;
	}

	if (i == celt)
		hr = S_OK;

	return hr;
}

STDMETHODIMP CCustomAutoComplete::Skip(ULONG celt)
{
	m_nCurrentElement += celt;
	if (m_nCurrentElement > (ULONG)m_asList.GetSize())
		m_nCurrentElement = 0;

	return S_OK;
}

STDMETHODIMP CCustomAutoComplete::Reset(void)
{
	m_nCurrentElement = 0;
	return S_OK;
}

STDMETHODIMP CCustomAutoComplete::Clone(IEnumString** ppenum)
{
	if (!ppenum)
		return E_POINTER;
	
	CCustomAutoComplete* pnew = new CCustomAutoComplete();
	pnew->AddRef();
	*ppenum = pnew;
	return S_OK;
}

void CCustomAutoComplete::InternalInit()
{
	m_nCurrentElement = 0;
	m_nRefCount = 0;
	m_fBound = FALSE;
	m_iMaxItemCount = 30;
}

HRESULT CCustomAutoComplete::EnDisable(BOOL p_fEnable)
{
	ATLASSERT(m_pac);

	HRESULT hr = m_pac->Enable(p_fEnable);
	if (SUCCEEDED(hr))
		m_fBound = p_fEnable;
	return hr;
}

BOOL CCustomAutoComplete::LoadList(LPCTSTR pszFileName)
{
	FILE* fp = _tfsopen(pszFileName, _T("rb"), _SH_DENYWR);
	if (fp == NULL)
		return FALSE;

	// verify Unicode byte-order mark 0xFEFF
	WORD wBOM = fgetwc(fp);
	if (wBOM != 0xFEFF){
		fclose(fp);
		return FALSE;
	}

	TCHAR szItem[256];
	while (_fgetts(szItem, ARRSIZE(szItem), fp) != NULL){
		CString strItem(szItem);
		strItem.Trim(_T(" \r\n"));
		AddItem(strItem, -1);
	}
	fclose(fp);
	return TRUE;
}

BOOL CCustomAutoComplete::SaveList(LPCTSTR pszFileName)
{
	FILE* fp = _tfsopen(pszFileName, _T("wb"), _SH_DENYWR);
	if (fp == NULL)
		return FALSE;

	// write Unicode byte-order mark 0xFEFF
	fputwc(0xFEFF, fp);

	for (int i = 0; i < m_asList.GetCount(); i++)
		_ftprintf(fp, _T("%s\r\n"), m_asList[i]);
	fclose(fp);
	return !ferror(fp);
}

CString CCustomAutoComplete::GetItem(int pos){
	if (pos>=m_asList.GetCount()) return NULL; 
	else return m_asList.GetAt(pos);
}
