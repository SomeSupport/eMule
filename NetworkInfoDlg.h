#pragma once

#include "ResizableLib/ResizableDialog.h"
#include "RichEditCtrlX.h"

// CNetworkInfoDlg dialog

class CNetworkInfoDlg : public CResizableDialog
{
	DECLARE_DYNAMIC(CNetworkInfoDlg)

public:
	CNetworkInfoDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CNetworkInfoDlg();

// Dialog Data
	enum { IDD = IDD_NETWORK_INFO };

protected:
	CRichEditCtrlX m_info;

	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};

void CreateNetworkInfo(CRichEditCtrlX& rCtrl, CHARFORMAT& rcfDef, CHARFORMAT& rcfBold, bool bFullInfo = false);
