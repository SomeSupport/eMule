#pragma once

// GDIThread.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CGDIThread thread

class CGDIThread : public CWinThread
{
public:
	DECLARE_DYNAMIC(CGDIThread)
	CGDIThread(CWnd* pWnd, HDC hDC);

// Attributes
public:
	HDC m_hDC;
	CDC m_dc;
	HANDLE m_hEventKill;
	HANDLE m_hEventDead;
	//static HANDLE m_hAnotherDead;

	// options
	int m_nDelay;
	int m_nScrollInc;
	BOOL m_bWaitVRT;

	enum {SCROLL_UP = 1, SCROLL_PAUSE = 0, SCROLL_DOWN = -1};

	static CCriticalSection m_csGDILock;

// Operations
public:
	void KillThread();
	virtual void SingleStep() = 0;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGDIThread)
	//}}AFX_VIRTUAL

// Implementation
public:
	BOOL SetWaitVRT(BOOL bWait = TRUE);
	int SetScrollDirection(int nDirection);
	int SetDelay(int nDelay);
	virtual ~CGDIThread();
	virtual void Delete();

protected:
	virtual BOOL InitInstance();

	// Generated message map functions
	//{{AFX_MSG(CGDIThread)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
