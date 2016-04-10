#pragma once

#include "enbitmap.h"

class CSplashScreen : public CDialog
{
	DECLARE_DYNAMIC(CSplashScreen)

public:
	CSplashScreen(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSplashScreen();

// Dialog Data
	enum { IDD = IDD_SPLASH };

protected:
	CBitmap m_imgSplash;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	BOOL OnInitDialog();
	BOOL PreTranslateMessage(MSG* pMsg);

	DECLARE_MESSAGE_MAP()
	void OnPaint(); 
};
