/*
		You may NOT modify this copyright message. You may add your name, if you
		changed or improved this code, but you mot not delete any part of this message or
		make it invisible etc.
*/
#include "stdafx.h"
#include "emule.h"
#include "CreditsDlg.h"
#include "CreditsThread.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// drawable area of the dialog
#define SCREEN_LEFT		6
#define SCREEN_TOP		175
#define SCREEN_RIGHT	345
#define SCREEN_BOTTOM	296

// button to dismiss dialog
#define BUTTON_TOP_Y	0
#define BUTTON_BOTTOM_Y	300
#define BUTTON_LEFT_X	0
#define BUTTON_RIGHT_X	350

/////////////////////////////////////////////////////////////////////////////
// CCreditsDlg dialog


CCreditsDlg::CCreditsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCreditsDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCreditsDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_pDC = NULL;
}
CCreditsDlg::~CCreditsDlg(){
	m_imgSplash.DeleteObject();
}

void CCreditsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCreditsDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCreditsDlg, CDialog)
	ON_WM_LBUTTONDOWN()
	ON_WM_DESTROY()
	ON_WM_CREATE()
	ON_WM_PAINT()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCreditsDlg message handlers

void CCreditsDlg::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CDialog::OnLButtonDown(nFlags, point);

	// see if they clicked on our button to dismiss the dialog
	if((point.x >= BUTTON_LEFT_X) && (point.x <= BUTTON_RIGHT_X))
	{
		if((point.y >= BUTTON_TOP_Y) && (point.y <= BUTTON_BOTTOM_Y))
		{
			CDialog::OnOK();
			return;
		}
	}

	PostMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x, point.y));
}

BOOL CCreditsDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	VERIFY( m_imgSplash.Attach(theApp.LoadImage(_T("ABOUT"), _T("JPG"))) );
	m_rectScreen.SetRect(SCREEN_LEFT, SCREEN_TOP, SCREEN_RIGHT, SCREEN_BOTTOM);
	StartThread();

	return TRUE;
}

void CCreditsDlg::OnDestroy() 
{
	KillThread();

	delete m_pDC;
	m_pDC = NULL;

	CDialog::OnDestroy();
}

void CCreditsDlg::StartThread()
{
	m_pThread = new CCreditsThread(this, m_pDC->GetSafeHdc(), m_rectScreen);

	if (m_pThread == NULL)
		return;

	ASSERT_VALID(m_pThread);
	m_pThread->m_pThreadParams = NULL;

	// Create Thread in a suspended state so we can set the Priority 
	// before it starts getting away from us
	if (!m_pThread->CreateThread(CREATE_SUSPENDED))
	{
		delete m_pThread;
		m_pThread = NULL;
		return;
	}

	// thread priority has been set at idle priority to keep from bogging
	// down other apps that may also be running.
	VERIFY(m_pThread->SetThreadPriority(THREAD_PRIORITY_IDLE));
	// Now the thread can run wild
	m_pThread->ResumeThread();
}

void CCreditsDlg::KillThread()
{
	// tell thread to shutdown
	VERIFY(SetEvent(m_pThread->m_hEventKill));

	// wait for thread to finish shutdown
	VERIFY(WaitForSingleObject(m_pThread->m_hThread, INFINITE) == WAIT_OBJECT_0);

	delete m_pThread;
	m_pThread = NULL;
}

int CCreditsDlg::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	// m_pDC must be initialized here instead of the constructor
	// because the HWND isn't created until Create is called.
	m_pDC = new CClientDC(this);
	
	return 0;
}

void CCreditsDlg::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

	if (m_imgSplash.GetSafeHandle())
	{
		CDC dcMem;

		if (dcMem.CreateCompatibleDC(&dc))
		{
			CBitmap* pOldBM = dcMem.SelectObject(&m_imgSplash);
			BITMAP BM;
			m_imgSplash.GetBitmap(&BM);

			WINDOWPLACEMENT wp;
			this->GetWindowPlacement(&wp);
			wp.rcNormalPosition.right= wp.rcNormalPosition.left+BM.bmWidth;
			wp.rcNormalPosition.bottom= wp.rcNormalPosition.top+BM.bmHeight;
			this->SetWindowPlacement(&wp);
			
			dc.BitBlt(0, 0, BM.bmWidth, BM.bmHeight, &dcMem, 0, 0, SRCCOPY);
			dcMem.SelectObject(pOldBM);
		}
	}
}