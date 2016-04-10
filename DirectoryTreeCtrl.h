#pragma once
/////////////////////////////////////////////
// written by robert rostek - tecxx@rrs.at //
/////////////////////////////////////////////

#define MP_SHAREDFOLDERS_FIRST	46901

class CDirectoryTreeCtrl : public CTreeCtrl
{
	DECLARE_DYNAMIC(CDirectoryTreeCtrl)

public:
	// initialize control
	void Init(void);
	// get all shared directories
	void GetSharedDirectories(CStringList* list);
	// set shared directories
	void SetSharedDirectories(CStringList* list);

private:
	CImageList m_image; 
	// add a new item
	HTREEITEM AddChildItem(HTREEITEM hRoot, CString strText);
	// add subdirectory items
	void AddSubdirectories(HTREEITEM hRoot, CString strDir);
	// return the full path of an item (like C:\abc\somewhere\inheaven\)
	CString GetFullPath(HTREEITEM hItem);
	// returns true if strDir has at least one subdirectory
	bool HasSubdirectories(CString strDir);
	// check status of an item has changed
	void CheckChanged(HTREEITEM hItem, bool bChecked);
	// returns true if a subdirectory of strDir is shared
	bool HasSharedSubdirectory(CString strDir);
	// when sharing a directory, make all parent directories bold
	void UpdateParentItems(HTREEITEM hChild);
	void ShareSubDirTree(HTREEITEM hItem, BOOL bShare);

	// share list access
	bool IsShared(CString strDir);
	void AddShare(CString strDir);
	void DelShare(CString strDir);
	void MarkChilds(HTREEITEM hChild,bool mark);

	CStringList m_lstShared;
	CString m_strLastRightClicked;
	bool m_bSelectSubDirs;

public:
	// construction / destruction
	CDirectoryTreeCtrl();
	virtual ~CDirectoryTreeCtrl();
	virtual BOOL OnCommand(WPARAM wParam,LPARAM lParam );

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnTvnItemexpanding(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTvnGetdispinfo(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnTvnDeleteItem(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnDestroy();
};
