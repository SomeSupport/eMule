#pragma once
#include "TitleMenu.h"

class CLogEditCtrl : public CEdit
{
	DECLARE_DYNAMIC(CLogEditCtrl)
public:
	CLogEditCtrl();
	virtual ~CLogEditCtrl();

	void Init(LPCTSTR pszTitle, LPCTSTR pszSkinKey = NULL);
	void SetTitle(LPCTSTR pszTitle);
	void Localize();
	void ApplySkin();

	void AddEntry(LPCTSTR pszMsg);
	void Add(LPCTSTR pszMsg, int iLen = -1);
	void Reset();
	CString GetLastLogEntry();
	CString GetAllLogEntries();
	bool SaveLog(LPCTSTR pszDefName = NULL);

protected:
	bool m_bRichEdit;
	CTitleMenu m_LogMenu;
	int m_iMaxLogBuff;
	bool m_bAutoScroll;
	CStringArray m_astrBuff;
	bool m_bNoPaint;
	bool m_bEnErrSpace;
	CString m_strTitle;
	CString m_strSkinKey;
	COLORREF m_crForeground;
	COLORREF m_crBackground;
	CBrush m_brBackground;

	void AddLine(LPCTSTR pszMsg, int iLen);
	void SelectAllItems();
	void CopySelectedItems();
	int GetMaxSize();
	void SafeAddLine(int nPos, int iLineLen, LPCTSTR pszLine, int& nStartChar, int& nEndChar);
	void FlushBuffer();
	void ScrollToLastLine();

	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnEnErrspace();
	afx_msg void OnEnMaxtext();
	afx_msg void OnSysColorChange();
	afx_msg HBRUSH CtlColor(CDC* pDC, UINT nCtlColor);
};
