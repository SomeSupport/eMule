#pragma once
#include "TreeOptionsCtrlEx.h"

class CPPgTweaks : public CPropertyPage
{
	DECLARE_DYNAMIC(CPPgTweaks)

public:
	CPPgTweaks();
	virtual ~CPPgTweaks();

// Dialog Data
	enum { IDD = IDD_PPG_TWEAKS };

	void Localize(void);

protected:
	UINT m_iFileBufferSize;
	UINT m_iQueueSize;
	int m_iMaxConnPerFive;
	int m_iMaxHalfOpen;
	bool m_bConditionalTCPAccept;
	bool m_bAutoTakeEd2kLinks;
	bool m_bVerbose;
	bool m_bDebugSourceExchange;
	bool m_bLogBannedClients;
	bool m_bLogRatingDescReceived;
	bool m_bLogSecureIdent;
	bool m_bLogFilteredIPs;
	bool m_bLogFileSaving;
    bool m_bLogA4AF;
	bool m_bLogUlDlEvents;
	bool m_bCreditSystem;
	bool m_bLog2Disk;
	bool m_bDebug2Disk;
	int m_iCommitFiles;
	bool m_bFilterLANIPs;
	bool m_bExtControls;
	UINT m_uServerKeepAliveTimeout;
	bool m_bSparsePartFiles;
	bool m_bFullAlloc;
	bool m_bCheckDiskspace;
	float m_fMinFreeDiskSpaceMB;
	CString m_sYourHostname;
	bool m_bFirewallStartup;
	int m_iLogLevel;
    bool m_bDynUpEnabled;
    int m_iDynUpMinUpload;
    int m_iDynUpPingTolerance;
    int m_iDynUpPingToleranceMilliseconds;
    int m_iDynUpRadioPingTolerance;
    int m_iDynUpGoingUpDivider;
    int m_iDynUpGoingDownDivider;
    int m_iDynUpNumberOfPings;
    bool m_bA4AFSaveCpu;
	bool m_bAutoArchDisable;
	int m_iExtractMetaData;
	bool m_bCloseUPnPOnExit;
	bool m_bSkipWANIPSetup;
	bool m_bSkipWANPPPSetup;
	int m_iShareeMule;
	bool bShowedWarning;
	bool m_bResolveShellLinks;


	CSliderCtrl m_ctlFileBuffSize;
	CSliderCtrl m_ctlQueueSize;
    CTreeOptionsCtrlEx m_ctrlTreeOptions;
	bool m_bInitializedTreeOpts;
	HTREEITEM m_htiTCPGroup;
	HTREEITEM m_htiMaxCon5Sec;
	HTREEITEM m_htiMaxHalfOpen;
	HTREEITEM m_htiConditionalTCPAccept;
	HTREEITEM m_htiAutoTakeEd2kLinks;
	HTREEITEM m_htiVerboseGroup;
	HTREEITEM m_htiVerbose;
	HTREEITEM m_htiDebugSourceExchange;
	HTREEITEM m_htiLogBannedClients;
	HTREEITEM m_htiLogRatingDescReceived;
	HTREEITEM m_htiLogSecureIdent;
	HTREEITEM m_htiLogFilteredIPs;
	HTREEITEM m_htiLogFileSaving;
    HTREEITEM m_htiLogA4AF;
	HTREEITEM m_htiLogUlDlEvents;
	HTREEITEM m_htiCreditSystem;
	HTREEITEM m_htiLog2Disk;
	HTREEITEM m_htiDebug2Disk;
	HTREEITEM m_htiCommit;
	HTREEITEM m_htiCommitNever;
	HTREEITEM m_htiCommitOnShutdown;
	HTREEITEM m_htiCommitAlways;
	HTREEITEM m_htiFilterLANIPs;
	HTREEITEM m_htiExtControls;
	HTREEITEM m_htiServerKeepAliveTimeout;
	HTREEITEM m_htiSparsePartFiles;
	HTREEITEM m_htiFullAlloc;
	HTREEITEM m_htiCheckDiskspace;
	HTREEITEM m_htiMinFreeDiskSpace;
	HTREEITEM m_htiYourHostname;
	HTREEITEM m_htiFirewallStartup;
	HTREEITEM m_htiLogLevel;
    HTREEITEM m_htiDynUp;
	HTREEITEM m_htiDynUpEnabled;
    HTREEITEM m_htiDynUpMinUpload;
    HTREEITEM m_htiDynUpPingTolerance;
    HTREEITEM m_htiDynUpPingToleranceMilliseconds;
    HTREEITEM m_htiDynUpPingToleranceGroup;
    HTREEITEM m_htiDynUpRadioPingTolerance;
    HTREEITEM m_htiDynUpRadioPingToleranceMilliseconds;
    HTREEITEM m_htiDynUpGoingUpDivider;
    HTREEITEM m_htiDynUpGoingDownDivider;
    HTREEITEM m_htiDynUpNumberOfPings;
    HTREEITEM m_htiA4AFSaveCpu;
	HTREEITEM m_htiExtractMetaData;
	HTREEITEM m_htiExtractMetaDataNever;
	HTREEITEM m_htiExtractMetaDataID3Lib;
	HTREEITEM m_htiAutoArch;
	HTREEITEM m_htiUPnP;
	HTREEITEM m_htiCloseUPnPPorts;
	HTREEITEM m_htiSkipWANIPSetup;
	HTREEITEM m_htiSkipWANPPPSetup;
	HTREEITEM m_htiShareeMule;
	HTREEITEM m_htiShareeMuleMultiUser;
	HTREEITEM m_htiShareeMulePublicUser;
	HTREEITEM m_htiShareeMuleOldStyle;
	//HTREEITEM m_htiExtractMetaDataMediaDet;
	HTREEITEM m_htiResolveShellLinks;

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnDestroy();
	afx_msg LRESULT OnTreeOptsCtrlNotify(WPARAM wParam, LPARAM lParam);
	afx_msg void OnHelp();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnBnClickedOpenprefini();
};
