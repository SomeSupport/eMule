#pragma once

class CComboBoxEx2 : public CComboBoxEx
{
	DECLARE_DYNAMIC(CComboBoxEx2)
public:
	CComboBoxEx2();
	virtual ~CComboBoxEx2();

	int AddItem(LPCTSTR pszText, int iImage);
	BOOL SelectString(LPCTSTR pszText);
	BOOL SelectItemDataStringA(LPCSTR pszText);

	virtual BOOL PreTranslateMessage(MSG* pMsg);

protected:
	DECLARE_MESSAGE_MAP()
};

void UpdateHorzExtent(CComboBox &rctlComboBox, int iIconWidth);
