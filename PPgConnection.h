#pragma once

class CPPgConnection : public CPropertyPage
{
	DECLARE_DYNAMIC(CPPgConnection)

public:
	CPPgConnection();
	virtual ~CPPgConnection();

// Dialog Data
	enum { IDD = IDD_PPG_CONNECTION };

	void Localize(void);
	void LoadSettings(void);

protected:
	bool guardian;
	CSliderCtrl m_ctlMaxDown;
	CSliderCtrl m_ctlMaxUp;

	void ShowLimitValues();
	void SetRateSliderTicks(CSliderCtrl& rRate);

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnSettingsChange()					{ SetModified(); }
	afx_msg void OnEnChangeUDPDisable();
	afx_msg void OnLimiterChange();
	afx_msg void OnBnClickedWizard();
	afx_msg void OnBnClickedNetworkKademlia();
	afx_msg void OnHelp();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnBnClickedOpenports();
	afx_msg void OnStartPortTest();
	afx_msg void OnEnChangeTCP();
	afx_msg void OnEnChangeUDP();
	afx_msg void OnEnChangePorts(uint8 istcpport);
};
