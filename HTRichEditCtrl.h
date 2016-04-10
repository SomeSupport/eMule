#pragma once

#include "TitleMenu.h"

class CHTRichEditCtrl : public CRichEditCtrl
{
	DECLARE_DYNAMIC(CHTRichEditCtrl)

public:
	CHTRichEditCtrl();
	virtual ~CHTRichEditCtrl();

	void Init(LPCTSTR pszTitle, LPCTSTR pszSkinKey = NULL);
	void SetProfileSkinKey(LPCTSTR pszSkinKey);
	void SetTitle(LPCTSTR pszTitle);
	void Localize();
	void ApplySkin();
	void EnableSmileys(bool bEnable = true);

	void AddEntry(LPCTSTR pszMsg);
	void Add(LPCTSTR pszMsg, int iLen = -1);
	void AddTyped(LPCTSTR pszMsg, int iLen, UINT uFlags);
	void AddLine(LPCTSTR pszMsg, int iLen = -1, bool bLink = false, COLORREF cr = CLR_DEFAULT, COLORREF bk = CLR_DEFAULT, DWORD mask = 0);
	bool AddCaptcha(HBITMAP hbmp);
	void Reset();
	CString GetLastLogEntry();
	CString GetAllLogEntries();
	bool SaveLog(LPCTSTR pszDefName = NULL);

	void AppendText(const CString& sText);
	void AppendHyperLink(const CString& sText, const CString& sTitle, const CString& sCommand, const CString& sDirectory);
	void AppendKeyWord(const CString& sText, COLORREF cr);
	void AppendColoredText(LPCTSTR pszText, COLORREF cr, COLORREF bk = CLR_DEFAULT, DWORD mask = 0);
	COLORREF GetForegroundColor() const { return m_crForeground; }
	COLORREF GetBackgroundColor() const { return m_crBackground; }
	void SetDfltForegroundColor(COLORREF crColor) { m_crDfltForeground = crColor; }
	void SetDfltBackgroundColor(COLORREF crColor) { m_crDfltBackground = crColor; }

	CString GetText() const;
	bool IsAutoScroll() const { return m_bAutoScroll; }
	void SetAutoScroll(bool bAutoScroll) { m_bAutoScroll = bAutoScroll; }
	void ScrollToLastLine(bool bForceLastLineAtBottom = false);
	void ScrollToFirstLine();

	void SetFont(CFont* pFont, BOOL bRedraw = TRUE);
	CFont* GetFont() const;

protected:
	bool m_bRichEdit;
	int m_iLimitText;
	bool m_bAutoScroll;
	CStringArray m_astrBuff;
	bool m_bNoPaint;
	bool m_bEnErrSpace;
	CString m_strTitle;
	CString m_strSkinKey;
	bool m_bRestoreFormat;
	CHARFORMAT m_cfDefault;
	bool m_bForceArrowCursor;
	HCURSOR m_hArrowCursor;
	bool m_bEnableSmileys;
	COLORREF m_crForeground;
	COLORREF m_crBackground;
	bool m_bDfltForeground;
	bool m_bDfltBackground;
	COLORREF m_crDfltForeground;
	COLORREF m_crDfltBackground;
	static int sm_iSmileyClients;
	static CComPtr<IStorage> sm_pIStorageSmileys;
	static CMapStringToPtr sm_aSmileyBitmaps;
	CComPtr<IStorage> m_pIStorageCaptchas;

	void SelectAllItems();
	void CopySelectedItems();
	int GetMaxSize();
	void SafeAddLine(int nPos, LPCTSTR pszLine, int iLen, long& nStartChar, long& nEndChar, bool bLink, COLORREF cr, COLORREF bk, DWORD mask);
	void FlushBuffer();
	void AddString(int nPos, LPCTSTR pszString, bool bLink, COLORREF cr, COLORREF bk, DWORD mask);
	bool InsertSmiley(LPCTSTR pszSmileyID);
	HBITMAP GetSmileyBitmap(LPCTSTR pszSmileyID);
	void AddSmileys(LPCTSTR pszLine);
	void PurgeSmileyCaches();

	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnEnErrspace();
	afx_msg void OnEnMaxtext();
	afx_msg BOOL OnEnLink(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnSysColorChange();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnSize(UINT nType, int cx, int cy);
};
