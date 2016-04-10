//--------------------------------------------------------------------------------------------
//  Author:         Klaus H. Probst [kprobst@vbbox.com]
// 
//--------------------------------------------------------------------------------------------
#pragma once
#include <initguid.h>
#include <shldisp.h>
#include <shlguid.h>

class CCustomAutoComplete : 
	public IEnumString
{
private:
	CStringArray m_asList;
	CComPtr<IAutoComplete> m_pac;

	ULONG m_nCurrentElement;
	ULONG m_nRefCount;
	BOOL m_fBound;
	int m_iMaxItemCount;

	// Constructors/destructors
public:
	CCustomAutoComplete();
	CCustomAutoComplete(const CStringArray& p_sItemList);
	~CCustomAutoComplete();

	// Implementation
public:
	BOOL Bind(HWND p_hWndEdit, DWORD p_dwOptions = 0, LPCTSTR p_lpszFormatString = NULL);
	VOID Unbind();
	BOOL IsBound() const { return m_fBound; }

	BOOL SetList(const CStringArray& p_sItemList);
	const CStringArray& GetList() const;
	int GetItemCount();

	BOOL AddItem(const CString& p_sItem, int iPos);
	BOOL RemoveItem(const CString& p_sItem);
	BOOL RemoveSelectedItem();
	CString GetItem(int pos);
	
	BOOL Clear();
	BOOL Disable();
	BOOL Enable(VOID);
	
	BOOL LoadList(LPCTSTR pszFileName);
	BOOL SaveList(LPCTSTR pszFileName);

public:
	STDMETHOD_(ULONG,AddRef)();
	STDMETHOD_(ULONG,Release)();
	STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject);

public:
	STDMETHOD(Next)(ULONG celt, LPOLESTR* rgelt, ULONG* pceltFetched);
	STDMETHOD(Skip)(ULONG celt);
	STDMETHOD(Reset)(void);
	STDMETHOD(Clone)(IEnumString** ppenum);

	// Internal implementation
private:
	void InternalInit();
	HRESULT EnDisable(BOOL p_fEnable);
	int FindItem(const CString& rstr);
};
