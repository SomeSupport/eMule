#pragma once 
#include "ResizableLib/ResizablePage.h"
#include "CommentListCtrl.h"

class CPartFile;


///////////////////////////////////////////////////////////////////////////////
// CCommentDialogLst

class CCommentDialogLst : public CResizablePage
{ 
	DECLARE_DYNAMIC(CCommentDialogLst) 

public: 
	CCommentDialogLst(); 
	virtual ~CCommentDialogLst(); 

	void SetFiles(const CSimpleArray<CObject*>* paFiles) { m_paFiles = paFiles; m_bDataChanged = true; }

// Dialog Data 
	enum { IDD = IDD_COMMENTLST }; 

protected: 
	CString m_strCaption;
	CCommentListCtrl m_lstComments;
	const CSimpleArray<CObject*>* m_paFiles;
	bool m_bDataChanged;
	uint32 m_timer;

	void Localize();
	void RefreshData(bool deleteOld = true);

	virtual BOOL OnInitDialog(); 
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support 
	virtual BOOL OnSetActive();

	DECLARE_MESSAGE_MAP() 
	afx_msg void OnBnClickedApply(); 
	afx_msg void OnBnClickedSearchKad(); 
	afx_msg void OnBnClickedFilter();
	afx_msg LRESULT OnDataChanged(WPARAM, LPARAM);
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT nIDEvent);
};
