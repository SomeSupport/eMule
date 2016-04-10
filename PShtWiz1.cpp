//this file is part of eMule
//Copyright (C)2002-2008 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#include "stdafx.h"
#include <afxinet.h>
#include "emule.h"
#include "enbitmap.h"
#include "OtherFunctions.h"
#include "Preferences.h"
#include "emuledlg.h"
#include "Statistics.h"
#include "ListenSocket.h"
#include "ClientUDPSocket.h"
#include "UPnPImpl.h"
#include "UPnPImplWrapper.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


///////////////////////////////////////////////////////////////////////////////
// CDlgPageWizard dialog

class CDlgPageWizard : public CPropertyPageEx
{
	DECLARE_DYNCREATE(CDlgPageWizard)

public:
	CDlgPageWizard();

	CDlgPageWizard(UINT nIDTemplate, LPCTSTR pszCaption = NULL, LPCTSTR pszHeaderTitle = NULL, LPCTSTR pszHeaderSubTitle = NULL)
		: CPropertyPageEx(nIDTemplate)
	{
		if (pszCaption)
		{
			m_strCaption = pszCaption; // "convenience storage"
			m_psp.pszTitle = m_strCaption;
			m_psp.dwFlags |= PSP_USETITLE;
		}
		if (pszHeaderTitle && pszHeaderTitle[0] != _T('\0'))
		{
			m_strHeaderTitle = pszHeaderTitle;
			m_psp.dwSize = sizeof(m_psp);
		}
		if (pszHeaderSubTitle && pszHeaderSubTitle[0] != _T('\0'))
		{
			m_strHeaderSubTitle = pszHeaderSubTitle;
			m_psp.dwSize = sizeof(m_psp);
		}
	}

protected:
	CString m_strCaption;

	virtual BOOL OnSetActive();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};

IMPLEMENT_DYNCREATE(CDlgPageWizard, CPropertyPageEx)

BEGIN_MESSAGE_MAP(CDlgPageWizard, CPropertyPageEx)
END_MESSAGE_MAP()

CDlgPageWizard::CDlgPageWizard() 
	: CPropertyPageEx()
{
}

void CDlgPageWizard::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageEx::DoDataExchange(pDX);
}

BOOL CDlgPageWizard::OnSetActive() 
{
	CPropertySheetEx* pSheet = (CPropertySheetEx*)GetParent();
	if (pSheet->IsWizard())
	{
		int iPages = pSheet->GetPageCount();
		int iActPage = pSheet->GetActiveIndex();
		DWORD dwButtons = 0;
		if (iActPage > 0)
			dwButtons |= PSWIZB_BACK;
		if (iActPage < iPages)
			dwButtons |= PSWIZB_NEXT;
		if (iActPage == iPages-1)
		{
			if (pSheet->m_psh.dwFlags & PSH_WIZARDHASFINISH)
				dwButtons &= ~PSWIZB_NEXT;
			dwButtons |= PSWIZB_FINISH;
		}
		pSheet->SetWizardButtons(dwButtons);
	}
	return CPropertyPageEx::OnSetActive();
}


///////////////////////////////////////////////////////////////////////////////
// CPPgWiz1Welcome dialog

class CPPgWiz1Welcome : public CDlgPageWizard
{
	DECLARE_DYNAMIC(CPPgWiz1Welcome)

public:
	CPPgWiz1Welcome();
	CPPgWiz1Welcome(UINT nIDTemplate, LPCTSTR pszCaption = NULL, LPCTSTR pszHeaderTitle = NULL, LPCTSTR pszHeaderSubTitle = NULL)
		: CDlgPageWizard(nIDTemplate, pszCaption, pszHeaderTitle, pszHeaderSubTitle)
	{
	}
	virtual ~CPPgWiz1Welcome();
	virtual BOOL OnInitDialog();

// Dialog Data
	enum { IDD = IDD_WIZ1_WELCOME };

protected:
	CFont m_FontTitle;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};

IMPLEMENT_DYNAMIC(CPPgWiz1Welcome, CDlgPageWizard)

BEGIN_MESSAGE_MAP(CPPgWiz1Welcome, CDlgPageWizard)
END_MESSAGE_MAP()

CPPgWiz1Welcome::CPPgWiz1Welcome()
	: CDlgPageWizard(CPPgWiz1Welcome::IDD)
{
}

CPPgWiz1Welcome::~CPPgWiz1Welcome()
{
}

void CPPgWiz1Welcome::DoDataExchange(CDataExchange* pDX)
{
	CDlgPageWizard::DoDataExchange(pDX);
}

BOOL CPPgWiz1Welcome::OnInitDialog()
{
	CFont fontVerdanaBold;
	CreatePointFont(fontVerdanaBold, 12*10, _T("Verdana Bold"));
	LOGFONT lf;
	fontVerdanaBold.GetLogFont(&lf);
	lf.lfWeight = FW_BOLD;
	m_FontTitle.CreateFontIndirect(&lf);

	CStatic* pStatic = (CStatic*)GetDlgItem(IDC_WIZ1_TITLE);
	pStatic->SetFont(&m_FontTitle);

	CDlgPageWizard::OnInitDialog();
	InitWindowStyles(this);
	GetDlgItem(IDC_WIZ1_TITLE)->SetWindowText(GetResString(IDS_WIZ1_WELCOME_TITLE));
	GetDlgItem(IDC_WIZ1_ACTIONS)->SetWindowText(GetResString(IDS_WIZ1_WELCOME_ACTIONS));
	GetDlgItem(IDC_WIZ1_BTN_HINT)->SetWindowText(GetResString(IDS_WIZ1_WELCOME_BTN_HINT));
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// CPPgWiz1General dialog

class CPPgWiz1General : public CDlgPageWizard
{
	DECLARE_DYNAMIC(CPPgWiz1General)

public:
	CPPgWiz1General();
	CPPgWiz1General(UINT nIDTemplate, LPCTSTR pszCaption = NULL, LPCTSTR pszHeaderTitle = NULL, LPCTSTR pszHeaderSubTitle = NULL)
		: CDlgPageWizard(nIDTemplate, pszCaption, pszHeaderTitle, pszHeaderSubTitle)
	{
		m_iAutoConnectAtStart = 0;
		m_iAutoStart = 0;
	}
	virtual ~CPPgWiz1General();
	virtual BOOL OnInitDialog();

// Dialog Data
	enum { IDD = IDD_WIZ1_GENERAL };

	CString m_strNick;
	int m_iAutoConnectAtStart;
	int m_iAutoStart;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};

IMPLEMENT_DYNAMIC(CPPgWiz1General, CDlgPageWizard)

BEGIN_MESSAGE_MAP(CPPgWiz1General, CDlgPageWizard)
END_MESSAGE_MAP()

CPPgWiz1General::CPPgWiz1General()
	: CDlgPageWizard(CPPgWiz1General::IDD)
{
	m_iAutoConnectAtStart = 0;
	m_iAutoStart = 0;
}

CPPgWiz1General::~CPPgWiz1General()
{
}

void CPPgWiz1General::DoDataExchange(CDataExchange* pDX)
{
	CDlgPageWizard::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_NICK, m_strNick);
	DDX_Check(pDX, IDC_AUTOCONNECT, m_iAutoConnectAtStart);
	DDX_Check(pDX, IDC_AUTOSTART, m_iAutoStart);
}

BOOL CPPgWiz1General::OnInitDialog()
{
	CDlgPageWizard::OnInitDialog();
	InitWindowStyles(this);
	((CEdit*)GetDlgItem(IDC_NICK))->SetLimitText(thePrefs.GetMaxUserNickLength());
	GetDlgItem(IDC_NICK_FRM)->SetWindowText(GetResString(IDS_ENTERUSERNAME));
	GetDlgItem(IDC_AUTOCONNECT)->SetWindowText(GetResString(IDS_FIRSTAUTOCON));
	GetDlgItem(IDC_AUTOSTART)->SetWindowText(GetResString(IDS_WIZ_STARTWITHWINDOWS));
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// CPPgWiz1Ports & Connections test dialog

class CPPgWiz1Ports : public CDlgPageWizard
{
	DECLARE_DYNAMIC(CPPgWiz1Ports)

public:
	CPPgWiz1Ports();
	CPPgWiz1Ports(UINT nIDTemplate, LPCTSTR pszCaption = NULL, LPCTSTR pszHeaderTitle = NULL, LPCTSTR pszHeaderSubTitle = NULL)
		: CDlgPageWizard(nIDTemplate, pszCaption, pszHeaderTitle, pszHeaderSubTitle)
	{
	}

	void ValidateShownPorts();

	virtual ~CPPgWiz1Ports();
	virtual BOOL OnInitDialog();
	afx_msg void OnStartConTest();
	afx_msg void OnStartUPnP();
	afx_msg void OnEnChangeUDPDisable();

	afx_msg void OnEnChangeUDP();
	afx_msg void OnEnChangeTCP();
	afx_msg void OnTimer(UINT nIDEvent);
	
	BOOL	OnKillActive();
	void	OnOK();
	void	OnCancel();

	void OnPortChange();

	CString m_sTestURL,m_sUDP,m_sTCP;
	uint16 GetTCPPort();
	uint16 GetUDPPort();

	bool*	m_pbUDPDisabled;

// Dialog Data
	enum { IDD = IDD_WIZ1_PORTS };

protected:
	CString			lastudp;
	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	void			ResetUPnPProgress();

	DECLARE_MESSAGE_MAP()

	int m_nUPnPTicks;
};

IMPLEMENT_DYNAMIC(CPPgWiz1Ports, CDlgPageWizard)

BEGIN_MESSAGE_MAP(CPPgWiz1Ports, CDlgPageWizard)
	ON_BN_CLICKED(IDC_STARTTEST, OnStartConTest)
	ON_BN_CLICKED(IDC_UDPDISABLE, OnEnChangeUDPDisable)
	ON_BN_CLICKED(IDC_UPNPSTART, OnStartUPnP)
	ON_EN_CHANGE(IDC_TCP, OnEnChangeTCP)
	ON_EN_CHANGE(IDC_UDP, OnEnChangeUDP)
	ON_WM_TIMER()
END_MESSAGE_MAP()

CPPgWiz1Ports::CPPgWiz1Ports()
	: CDlgPageWizard(CPPgWiz1Ports::IDD)
{
	m_pbUDPDisabled = NULL;
}

CPPgWiz1Ports::~CPPgWiz1Ports()
{
}

void CPPgWiz1Ports::DoDataExchange(CDataExchange* pDX)
{
	CDlgPageWizard::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_TCP, m_sTCP);
	DDX_Text(pDX, IDC_UDP, m_sUDP);
}

void CPPgWiz1Ports::OnEnChangeTCP() {
	OnPortChange();
}
void CPPgWiz1Ports::OnEnChangeUDP() {
	OnPortChange();
}

uint16 CPPgWiz1Ports::GetTCPPort() {
	CString buffer;

	GetDlgItem(IDC_TCP)->GetWindowText(buffer);
	return (uint16)_tstoi(buffer);
}

uint16 CPPgWiz1Ports::GetUDPPort() {
	uint16 udp = 0;
	if (IsDlgButtonChecked(IDC_UDPDISABLE)==0) {
		CString buffer;
		GetDlgItem(IDC_UDP)->GetWindowText(buffer);
		udp = (uint16)_tstoi(buffer);
	}
	return udp;
}

void CPPgWiz1Ports::OnPortChange() {
	
	bool flag= (theApp.IsPortchangeAllowed() && 
		( 
		(theApp.listensocket->GetConnectedPort()!=GetTCPPort()  || theApp.listensocket->GetConnectedPort()==0)
		||
		(theApp.clientudp->GetConnectedPort()!=GetUDPPort() || theApp.clientudp->GetConnectedPort()==0 )    
		)	
	);
	
	GetDlgItem(IDC_STARTTEST)->EnableWindow(flag);
}

BOOL CPPgWiz1Ports::OnKillActive(){
	ResetUPnPProgress();
	return CDlgPageWizard::OnKillActive();
}

void CPPgWiz1Ports::OnOK(){
	ResetUPnPProgress();
	CDlgPageWizard::OnOK();
}

void CPPgWiz1Ports::OnCancel(){
	ResetUPnPProgress();
	CDlgPageWizard::OnCancel();
}

// ** UPnP Button stuff
void CPPgWiz1Ports::OnStartUPnP() {
	CDlgPageWizard::OnApply();
	theApp.emuledlg->StartUPnP(true, GetTCPPort(), GetUDPPort());

	GetDlgItem(IDC_UPNPSTATUS)->SetWindowText(GetResString(IDS_UPNPSETUP));
	GetDlgItem(IDC_UPNPSTART)->EnableWindow(FALSE);
	m_nUPnPTicks = 0;
	((CProgressCtrl*)GetDlgItem(IDC_UPNPPROGRESS))->SetPos(0);
	VERIFY( SetTimer(1, 1000, NULL) );
}

void CPPgWiz1Ports::OnTimer(UINT /*nIDEvent*/){
	m_nUPnPTicks++;
	if (theApp.m_pUPnPFinder && theApp.m_pUPnPFinder->GetImplementation()->ArePortsForwarded() == TRIS_UNKNOWN)
	{
		if (m_nUPnPTicks < 40){
			((CProgressCtrl*)GetDlgItem(IDC_UPNPPROGRESS))->SetPos(m_nUPnPTicks);
			return;
		}
	}
	if (theApp.m_pUPnPFinder && theApp.m_pUPnPFinder->GetImplementation()->ArePortsForwarded() == TRIS_TRUE){
		((CProgressCtrl*)GetDlgItem(IDC_UPNPPROGRESS))->SetPos(40);
		CString strMessage;
		strMessage.Format(GetResString(IDS_UPNPSUCCESS), GetTCPPort(), GetUDPPort());
		GetDlgItem(IDC_UPNPSTATUS)->SetWindowText(strMessage);
		// enable UPnP in the preferences after the successful try
		thePrefs.m_bEnableUPnP = true;
	}
	else{
		((CProgressCtrl*)GetDlgItem(IDC_UPNPPROGRESS))->SetPos(0);
		GetDlgItem(IDC_UPNPSTATUS)->SetWindowText(GetResString(IDS_UPNPFAILED));
	}
	GetDlgItem(IDC_UPNPSTART)->EnableWindow(TRUE);
	VERIFY( KillTimer(1));
}

void CPPgWiz1Ports::ResetUPnPProgress(){
	KillTimer(1);
	((CProgressCtrl*)GetDlgItem(IDC_UPNPPROGRESS))->SetPos(0);
	GetDlgItem(IDC_UPNPSTART)->EnableWindow(TRUE);
}

// **

void CPPgWiz1Ports::OnStartConTest() {

	uint16 tcp=GetTCPPort();
	uint16 udp=GetUDPPort();

	if (tcp==0)
		return;

	if ( (tcp!=theApp.listensocket->GetConnectedPort() || udp!=theApp.clientudp->GetConnectedPort() ) ) {

		if (!theApp.IsPortchangeAllowed()) {
			AfxMessageBox(GetResString(IDS_NOPORTCHANGEPOSSIBLE));
			return;
		}

		// set new ports
		thePrefs.port=tcp;
		thePrefs.udpport=udp;

		theApp.listensocket->Rebind() ;
		theApp.clientudp->Rebind();
	}

	TriggerPortTest(tcp,udp);
}

BOOL CPPgWiz1Ports::OnInitDialog()
{
	CDlgPageWizard::OnInitDialog();
	CheckDlgButton(IDC_UDPDISABLE, m_sUDP.IsEmpty() || m_sUDP == _T("0"));
	GetDlgItem(IDC_UDP)->EnableWindow(IsDlgButtonChecked(IDC_UDPDISABLE) == 0);
	((CProgressCtrl*)GetDlgItem(IDC_UPNPPROGRESS))->SetRange(0, 40);
	InitWindowStyles(this);
	
	lastudp = m_sUDP;

	// disable changing ports to prevent harm
	SetDlgItemText(IDC_PORTINFO , GetResString(IDS_PORTINFO) );
	SetDlgItemText(IDC_TESTFRAME , GetResString(IDS_CONNECTIONTEST) );
	SetDlgItemText(IDC_TESTINFO , GetResString(IDS_TESTINFO) );
	SetDlgItemText(IDC_STARTTEST, GetResString(IDS_STARTTEST) );
	SetDlgItemText(IDC_UDPDISABLE, GetResString(IDS_UDPDISABLED));
	SetDlgItemText(IDC_UPNPSTART, GetResString(IDS_UPNPSTART));
	SetDlgItemText(IDC_UPNPSTATUS, _T(""));

	return TRUE;
}

void CPPgWiz1Ports::OnEnChangeUDPDisable()
{
	bool disabled = IsDlgButtonChecked(IDC_UDPDISABLE)!=0;
	GetDlgItem(IDC_UDP)->EnableWindow(!disabled);
	
	if (disabled) {
		GetDlgItemText(IDC_UDP, lastudp);
		GetDlgItem(IDC_UDP)->SetWindowText(_T("0"));
	}
	else
		GetDlgItem(IDC_UDP)->SetWindowText(lastudp);
	
	if (m_pbUDPDisabled != NULL)
		*m_pbUDPDisabled = disabled;
	
	OnPortChange();
}


///////////////////////////////////////////////////////////////////////////////
// CPPgWiz1UlPrio dialog

class CPPgWiz1UlPrio : public CDlgPageWizard
{
	DECLARE_DYNAMIC(CPPgWiz1UlPrio)

public:
	CPPgWiz1UlPrio();
	CPPgWiz1UlPrio(UINT nIDTemplate, LPCTSTR pszCaption = NULL, LPCTSTR pszHeaderTitle = NULL, LPCTSTR pszHeaderSubTitle = NULL)
		: CDlgPageWizard(nIDTemplate, pszCaption, pszHeaderTitle, pszHeaderSubTitle)
	{
		m_iUAP = 1;
		m_iDAP = 1;
	}
	virtual ~CPPgWiz1UlPrio();
	virtual BOOL OnInitDialog();

// Dialog Data
	enum { IDD = IDD_WIZ1_ULDL_PRIO };

	int m_iUAP;
	int m_iDAP;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};

IMPLEMENT_DYNAMIC(CPPgWiz1UlPrio, CDlgPageWizard)

BEGIN_MESSAGE_MAP(CPPgWiz1UlPrio, CDlgPageWizard)
END_MESSAGE_MAP()

CPPgWiz1UlPrio::CPPgWiz1UlPrio()
	: CDlgPageWizard(CPPgWiz1UlPrio::IDD)
{
	m_iUAP = 1;
	m_iDAP = 1;
}

CPPgWiz1UlPrio::~CPPgWiz1UlPrio()
{
}

void CPPgWiz1UlPrio::DoDataExchange(CDataExchange* pDX)
{
	CDlgPageWizard::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_UAP, m_iUAP);
	DDX_Check(pDX, IDC_DAP, m_iDAP);
}

BOOL CPPgWiz1UlPrio::OnInitDialog()
{
	CDlgPageWizard::OnInitDialog();
	InitWindowStyles(this);
	GetDlgItem(IDC_UAP)->SetWindowText(GetResString(IDS_FIRSTAUTOUP));
	GetDlgItem(IDC_DAP)->SetWindowText(GetResString(IDS_FIRSTAUTODOWN));

	return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// CPPgWiz1Upload dialog

class CPPgWiz1Upload : public CDlgPageWizard
{
	DECLARE_DYNAMIC(CPPgWiz1Upload)

public:
	CPPgWiz1Upload();
	CPPgWiz1Upload(UINT nIDTemplate, LPCTSTR pszCaption = NULL, LPCTSTR pszHeaderTitle = NULL, LPCTSTR pszHeaderSubTitle = NULL)
		: CDlgPageWizard(nIDTemplate, pszCaption, pszHeaderTitle, pszHeaderSubTitle)
	{
		m_iObfuscation = 0;
	}
	virtual ~CPPgWiz1Upload();
	virtual BOOL OnInitDialog();

// Dialog Data
	enum { IDD = IDD_WIZ1_UPLOAD };

	int m_iObfuscation;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};

IMPLEMENT_DYNAMIC(CPPgWiz1Upload, CDlgPageWizard)

BEGIN_MESSAGE_MAP(CPPgWiz1Upload, CDlgPageWizard)
END_MESSAGE_MAP()

CPPgWiz1Upload::CPPgWiz1Upload()
	: CDlgPageWizard(CPPgWiz1Upload::IDD)
{
	m_iObfuscation = 0;
}

CPPgWiz1Upload::~CPPgWiz1Upload()
{
}

void CPPgWiz1Upload::DoDataExchange(CDataExchange* pDX)
{
	CDlgPageWizard::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_WIZZARDOBFUSCATION, m_iObfuscation);
}

BOOL CPPgWiz1Upload::OnInitDialog()
{
	CDlgPageWizard::OnInitDialog();
	InitWindowStyles(this);
	GetDlgItem(IDC_WIZZARDOBFUSCATION)->SetWindowText(GetResString(IDS_WIZZARDOBFUSCATION));
	return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// CPPgWiz1Server dialog

class CPPgWiz1Server : public CDlgPageWizard
{
	DECLARE_DYNAMIC(CPPgWiz1Server)

public:
	CPPgWiz1Server();
	CPPgWiz1Server(UINT nIDTemplate, LPCTSTR pszCaption = NULL, LPCTSTR pszHeaderTitle = NULL, LPCTSTR pszHeaderSubTitle = NULL)
		: CDlgPageWizard(nIDTemplate, pszCaption, pszHeaderTitle, pszHeaderSubTitle)
	{
		m_iSafeServerConnect = 0;
		m_iKademlia = 1;
		m_iED2K = 1;
	}
	virtual ~CPPgWiz1Server();
	virtual BOOL OnInitDialog();

// Dialog Data
	enum { IDD = IDD_WIZ1_SERVER };

	int m_iSafeServerConnect;
	int m_iKademlia;
	int m_iED2K;

	bool* m_pbUDPDisabled;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnSetActive();

	DECLARE_MESSAGE_MAP()
};

IMPLEMENT_DYNAMIC(CPPgWiz1Server, CDlgPageWizard)

BEGIN_MESSAGE_MAP(CPPgWiz1Server, CDlgPageWizard)
END_MESSAGE_MAP()

CPPgWiz1Server::CPPgWiz1Server()
	: CDlgPageWizard(CPPgWiz1Server::IDD)
{
	m_iSafeServerConnect = 0;
	m_iKademlia = 1;
	m_iED2K = 1;
	m_pbUDPDisabled = NULL;
}

CPPgWiz1Server::~CPPgWiz1Server()
{
}

void CPPgWiz1Server::DoDataExchange(CDataExchange* pDX)
{
	CDlgPageWizard::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_SAFESERVERCONNECT, m_iSafeServerConnect);
	DDX_Check(pDX, IDC_WIZARD_NETWORK_KADEMLIA, m_iKademlia);
	DDX_Check(pDX, IDC_WIZARD_NETWORK_ED2K, m_iED2K);
}

BOOL CPPgWiz1Server::OnInitDialog()
{
	CDlgPageWizard::OnInitDialog();
	InitWindowStyles(this);
	GetDlgItem(IDC_SAFESERVERCONNECT)->SetWindowText(GetResString(IDS_FIRSTSAFECON));
	GetDlgItem(IDC_WIZARD_NETWORK)->SetWindowText(GetResString(IDS_WIZARD_NETWORK));
	GetDlgItem(IDC_WIZARD_ED2K)->SetWindowText(GetResString(IDS_WIZARD_ED2K));
	return TRUE;
}

BOOL CPPgWiz1Server::OnSetActive(){
	if (m_pbUDPDisabled != NULL){
		m_iKademlia = *m_pbUDPDisabled ? 0 : m_iKademlia;
		if (*m_pbUDPDisabled){
			CheckDlgButton(IDC_SHOWOVERHEAD, 0);
			GetDlgItem(IDC_WIZARD_NETWORK_KADEMLIA)->EnableWindow(FALSE);
		}
		else{
			CheckDlgButton(IDC_SHOWOVERHEAD, m_iKademlia);
			GetDlgItem(IDC_WIZARD_NETWORK_KADEMLIA)->EnableWindow(TRUE);
		}

	}
	return CDlgPageWizard::OnSetActive();
}


///////////////////////////////////////////////////////////////////////////////
// CPPgWiz1End dialog

class CPPgWiz1End : public CDlgPageWizard
{
	DECLARE_DYNAMIC(CPPgWiz1End)

public:
	CPPgWiz1End();
	CPPgWiz1End(UINT nIDTemplate, LPCTSTR pszCaption = NULL, LPCTSTR pszHeaderTitle = NULL, LPCTSTR pszHeaderSubTitle = NULL)
		: CDlgPageWizard(nIDTemplate, pszCaption, pszHeaderTitle, pszHeaderSubTitle)
	{
	}
	virtual ~CPPgWiz1End();
	virtual BOOL OnInitDialog();

// Dialog Data
	enum { IDD = IDD_WIZ1_END };

protected:
	CFont m_FontTitle;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};

IMPLEMENT_DYNAMIC(CPPgWiz1End, CDlgPageWizard)

BEGIN_MESSAGE_MAP(CPPgWiz1End, CDlgPageWizard)
END_MESSAGE_MAP()

CPPgWiz1End::CPPgWiz1End()
	: CDlgPageWizard(CPPgWiz1End::IDD)
{
}

CPPgWiz1End::~CPPgWiz1End()
{
}

void CPPgWiz1End::DoDataExchange(CDataExchange* pDX)
{
	CDlgPageWizard::DoDataExchange(pDX);
}

BOOL CPPgWiz1End::OnInitDialog()
{
	CFont fontVerdanaBold;
	CreatePointFont(fontVerdanaBold, 12*10, _T("Verdana Bold"));
	LOGFONT lf;
	fontVerdanaBold.GetLogFont(&lf);
	lf.lfWeight = FW_BOLD;
	m_FontTitle.CreateFontIndirect(&lf);

	CStatic* pStatic = (CStatic*)GetDlgItem(IDC_WIZ1_TITLE);
	pStatic->SetFont(&m_FontTitle);

	CDlgPageWizard::OnInitDialog();
	InitWindowStyles(this);
	GetDlgItem(IDC_WIZ1_TITLE)->SetWindowText(GetResString(IDS_WIZ1_END_TITLE));
	GetDlgItem(IDC_WIZ1_ACTIONS)->SetWindowText(GetResString(IDS_FIRSTCOMPLETE));
	GetDlgItem(IDC_WIZ1_BTN_HINT)->SetWindowText(GetResString(IDS_WIZ1_END_BTN_HINT));

	return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// CPShtWiz1

class CPShtWiz1 : public CPropertySheetEx
{
	DECLARE_DYNAMIC(CPShtWiz1)

public:
	CPShtWiz1(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	virtual ~CPShtWiz1();

protected:
	DECLARE_MESSAGE_MAP()
};

IMPLEMENT_DYNAMIC(CPShtWiz1, CPropertySheetEx)

BEGIN_MESSAGE_MAP(CPShtWiz1, CPropertySheetEx)
END_MESSAGE_MAP()

CPShtWiz1::CPShtWiz1(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheetEx(nIDCaption, pParentWnd, iSelectPage)
{
}

CPShtWiz1::~CPShtWiz1()
{
}

BOOL FirstTimeWizard()
{
	CEnBitmap bmWatermark;
	VERIFY( bmWatermark.LoadImage(IDR_WIZ1_WATERMARK, _T("GIF"), NULL, GetSysColor(COLOR_WINDOW)) );
	CEnBitmap bmHeader;
	VERIFY( bmHeader.LoadImage(IDR_WIZ1_HEADER, _T("GIF"), NULL, GetSysColor(COLOR_WINDOW)) );
	CPropertySheetEx sheet(GetResString(IDS_WIZ1), NULL, 0, bmWatermark, NULL, bmHeader);
	sheet.m_psh.dwFlags |= PSH_WIZARD;
#ifdef _DEBUG
	sheet.m_psh.dwFlags |= PSH_WIZARDHASFINISH;
#endif
	sheet.m_psh.dwFlags |= PSH_WIZARD97;

	CPPgWiz1Welcome	page1(IDD_WIZ1_WELCOME, GetResString(IDS_WIZ1));
	page1.m_psp.dwFlags |= PSP_HIDEHEADER;
	sheet.AddPage(&page1);

	CPPgWiz1General page2(IDD_WIZ1_GENERAL, GetResString(IDS_WIZ1), GetResString(IDS_PW_GENERAL), GetResString(IDS_QL_USERNAME));
	sheet.AddPage(&page2);

	CPPgWiz1Ports page3(IDD_WIZ1_PORTS, GetResString(IDS_WIZ1), GetResString(IDS_PORTSCON), GetResString(IDS_PW_CONNECTION));
	sheet.AddPage(&page3);
	
	CPPgWiz1UlPrio page4(IDD_WIZ1_ULDL_PRIO, GetResString(IDS_WIZ1), GetResString(IDS_PW_CON_DOWNLBL) + _T(" / ") + GetResString(IDS_PW_CON_UPLBL), GetResString(IDS_PRIORITY));
	sheet.AddPage(&page4);
	
	CPPgWiz1Upload page5(IDD_WIZ1_UPLOAD, GetResString(IDS_WIZ1), GetResString(IDS_SECURITY), GetResString(IDS_OBFUSCATION));
	sheet.AddPage(&page5);
	
	CPPgWiz1Server page6(IDD_WIZ1_SERVER, GetResString(IDS_WIZ1), GetResString(IDS_PW_SERVER), GetResString(IDS_NETWORK));
	sheet.AddPage(&page6);
	
	CPPgWiz1End page7(IDD_WIZ1_END, GetResString(IDS_WIZ1));
	page7.m_psp.dwFlags |= PSP_HIDEHEADER;
	sheet.AddPage(&page7);

	page2.m_strNick = thePrefs.GetUserNick();
	if (page2.m_strNick.IsEmpty())
		page2.m_strNick = DEFAULT_NICK;
	page2.m_iAutoConnectAtStart = 0;
	page3.m_sTCP.Format(_T("%u"), thePrefs.GetPort());
	page3.m_sUDP.Format(_T("%u"), thePrefs.GetUDPPort());
	page4.m_iDAP = 1;
	page4.m_iUAP = 1;
	page5.m_iObfuscation = thePrefs.IsClientCryptLayerRequested() ? 1 : 0;
	page6.m_iSafeServerConnect = 0;
	page6.m_iKademlia = 1;
	page6.m_iED2K = 1;

	bool bUDPDisabled = thePrefs.GetUDPPort() == 0;
	page3.m_pbUDPDisabled = &bUDPDisabled;
	page6.m_pbUDPDisabled = &bUDPDisabled;

	uint16 oldtcpport=thePrefs.GetPort();
	uint16 oldudpport=thePrefs.GetUDPPort();

	int iResult = sheet.DoModal();
	if (iResult == IDCANCEL) {

		// restore port settings?
		thePrefs.port=oldtcpport;
		thePrefs.udpport=oldudpport;
		theApp.listensocket->Rebind() ;
		theApp.clientudp->Rebind();

		return FALSE;
	}

	page2.m_strNick.Trim();
	if (page2.m_strNick.IsEmpty())
		page2.m_strNick = DEFAULT_NICK;

	thePrefs.SetUserNick(page2.m_strNick);
	thePrefs.SetAutoConnect(page2.m_iAutoConnectAtStart!=0);
	thePrefs.SetAutoStart(page2.m_iAutoStart!=0);
	if( thePrefs.GetAutoStart() )
		AddAutoStart();
	else
		RemAutoStart();
	thePrefs.SetNewAutoDown(page4.m_iDAP!=0);
	thePrefs.SetNewAutoUp(page4.m_iUAP!=0);
	thePrefs.m_bCryptLayerRequested = page5.m_iObfuscation != 0;
	if (page5.m_iObfuscation != 0)
		thePrefs.m_bCryptLayerSupported = true;
	thePrefs.SetSafeServerConnectEnabled(page6.m_iSafeServerConnect!=0);
	thePrefs.SetNetworkKademlia(page6.m_iKademlia!=0);
	thePrefs.SetNetworkED2K(page6.m_iED2K!=0);

	// set ports
	thePrefs.port=(uint16)_tstoi(page3.m_sTCP);
	thePrefs.udpport=(uint16)_tstoi(page3.m_sUDP);
	ASSERT( thePrefs.port!=0 && thePrefs.udpport!=0+10 );
	if (thePrefs.port == 0)
		thePrefs.port = thePrefs.GetRandomTCPPort();
	if (thePrefs.udpport == 0+10)
		thePrefs.udpport = thePrefs.GetRandomUDPPort();
	if ( (thePrefs.port!=theApp.listensocket->GetConnectedPort()) || (thePrefs.udpport!=theApp.clientudp->GetConnectedPort()) )
		if (!theApp.IsPortchangeAllowed())
			AfxMessageBox(GetResString(IDS_NOPORTCHANGEPOSSIBLE));
		else {
			theApp.listensocket->Rebind() ;
			theApp.clientudp->Rebind();
		}
	
	return TRUE;
}

