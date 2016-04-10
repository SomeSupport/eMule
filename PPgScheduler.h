#pragma once

class CPPgScheduler : public CPropertyPage
{
	DECLARE_DYNAMIC(CPPgScheduler)

public:
	CPPgScheduler();
	virtual ~CPPgScheduler();

	// Dialog Data
	enum { IDD = IDD_PPG_SCHEDULER };

	void Localize(void);

// Dialog Data
protected:
	CComboBox m_timesel;
	CDateTimeCtrl m_time;
	CDateTimeCtrl m_timeTo;
	CListCtrl m_list;
	CListCtrl m_actions;

	CString GetActionLabel(int index);
	CString GetDayLabel(int index);
	void LoadSchedule(int index);
	void RecheckSchedules();
	void FillScheduleList();

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnBnClickedAdd();
	afx_msg void OnBnClickedApply();
	afx_msg void OnBnClickedRemove();
	afx_msg void OnDisableTime2();
	afx_msg void OnEnableChange();
	afx_msg void OnHelp();
	afx_msg void OnNmClickList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNmDblClkActionlist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNmRClickActionlist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnSettingsChange() {SetModified();}
};
