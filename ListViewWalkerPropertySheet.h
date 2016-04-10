#pragma once
#include "ResizableLib/ResizableSheet.h"
#include "ListCtrlItemWalk.h"

class CListViewPropertySheet : public CResizableSheet
{
	DECLARE_DYNAMIC(CListViewPropertySheet)

public:
	CListViewPropertySheet() {}
	CListViewPropertySheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CListViewPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	virtual ~CListViewPropertySheet();

	CPtrArray& GetPages() { return m_pages; }
	const	CSimpleArray<CObject*> &GetItems() const { return m_aItems; }
	void	InsertPage(int iIndex, CPropertyPage* pPage);

protected:
	CSimpleArray<CObject*> m_aItems;
	void ChangedData();
	DECLARE_MESSAGE_MAP()
};

// CListViewWalkerPropertySheet

class CListViewWalkerPropertySheet : public CListViewPropertySheet
{
	DECLARE_DYNAMIC(CListViewWalkerPropertySheet)

public:
	CListViewWalkerPropertySheet(CListCtrlItemWalk* pListCtrl)
	{
		m_pListCtrl = pListCtrl;
	}
	CListViewWalkerPropertySheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CListViewWalkerPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	virtual ~CListViewWalkerPropertySheet();


protected:
	CListCtrlItemWalk* m_pListCtrl;
	CButton m_ctlPrev;
	CButton m_ctlNext;


	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnNext();
	afx_msg void OnPrev();
};
