#pragma once
#include "ResizableLib/ResizablePage.h"
#include "ListCtrlX.h"
#include <list>

class CAbstractFile;
namespace Kademlia 
{
	class CKadTag;
	typedef std::list<CKadTag*> TagList;
};

class CMetaDataDlg : public CResizablePage
{
	DECLARE_DYNAMIC(CMetaDataDlg)

public:
	CMetaDataDlg();
	virtual ~CMetaDataDlg();

	void SetFiles(const CSimpleArray<CObject*>* paFiles) { m_paFiles = paFiles; m_bDataChanged = true; }
	void SetTagList(Kademlia::TagList* taglist);
	CString GetTagNameByID(UINT id);

// Dialog Data
	enum { IDD = IDD_META_DATA };

protected:
	const CSimpleArray<CObject*>* m_paFiles;
	bool m_bDataChanged;
	Kademlia::TagList* m_taglist;
	CString m_strCaption;
	CMenu* m_pMenuTags;
	CListCtrlX m_tags;

	void RefreshData();

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnSetActive();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnLvnKeydownTags(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnCopyTags();
	afx_msg void OnSelectAllTags();
	afx_msg void OnDestroy();
	afx_msg LRESULT OnDataChanged(WPARAM, LPARAM);
};
