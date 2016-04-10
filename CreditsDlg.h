#pragma once
#include "GDIThread.h"
#include "resource.h"
#include "enbitmap.h"

/////////////////////////////////////////////////////////////////////////////
// CCreditsDlg dialog

class CCreditsDlg : public CDialog
{
// Construction
public:
	void KillThread();
	void StartThread();
	CCreditsDlg(CWnd* pParent = NULL);   // standard constructor
	CCreditsDlg::~CCreditsDlg();

	CClientDC*	m_pDC;
	CRect		m_rectScreen;

	CGDIThread* m_pThread;

// Dialog Data
	//{{AFX_DATA(CCreditsDlg)
	enum { IDD = IDD_ABOUTBOX };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCreditsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCreditsDlg)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	virtual BOOL OnInitDialog();
	virtual void OnPaint();
	afx_msg void OnDestroy();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	CBitmap m_imgSplash;
};