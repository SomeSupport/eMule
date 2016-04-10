/*
Module : HTTPDOWNLOADDLG.H
Purpose: Defines the interface for an MFC dialog which performs HTTP downloads
		 similiar to the Internet Explorer download dialog

Copyright (c) 1999 - 2000 by PJ Naughter.
All rights reserved.

*/


////////////////////////////////// Macros ///////////////////////////
#pragma once
#include <afxinet.h>


/////////////////////////// Classes /////////////////////////////////

class CHttpDownloadDlg : public CDialog
{
public:
//Constructors / Destructors
	CHttpDownloadDlg(CWnd* pParent = NULL);

//Public Member variables
	CString m_strTitle;
	CString m_sURLToDownload;
	CString m_sFileToDownloadInto;
	CString m_sUserName;
	CString m_sPassword;

	const CString& GetError() const { return m_sError; }

protected:
	//{{AFX_DATA(CHttpDownloadDlg)
	enum { IDD = IDD_HTTPDOWNLOAD };
	CStatic m_ctrlStatus;
	CStatic m_ctrlTransferRate;
	CStatic m_ctrlTimeLeft;
	CProgressCtrl	m_ctrlProgress;
	CStatic m_ctrlFileStatus;
	CAnimateCtrl m_ctrlAnimate;
	//}}AFX_DATA

	//{{AFX_VIRTUAL(CHttpDownloadDlg)
	virtual void DoDataExchange(CDataExchange* pDX);
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CHttpDownloadDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	virtual void OnCancel();
	afx_msg void OnClose();
	//}}AFX_MSG
	afx_msg LRESULT OnThreadFinished(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
	DECLARE_DYNAMIC(CHttpDownloadDlg);

	static void CALLBACK _OnStatusCallBack(HINTERNET hInternet, DWORD dwContext, DWORD dwInternetStatus,
										   LPVOID lpvStatusInformation, DWORD dwStatusInformationLength);
	void OnStatusCallBack(HINTERNET hInternet, DWORD dwInternetStatus,
						  LPVOID lpvStatusInformation, DWORD dwStatusInformationLength);
	CString GetStatusInfo(LPVOID lpvStatusInformation, DWORD dwStatusInformationLength);
	static UINT _DownloadThread(LPVOID pParam);
	void HandleThreadErrorWithLastError(CString strIDError, DWORD dwLastError = 0);
	void HandleThreadError(CString strIDError);
	void DownloadThread();
	void SetPercentage(int nPercentage);
	void SetTimeLeft(DWORD dwSecondsLeft, DWORD dwBytesRead, DWORD dwFileSize);
	void SetProgressRange(DWORD dwFileSize);
	void SetStatus(const CString& sCaption);
	void SetStatus(CString strFmt, LPCTSTR lpsz1);
	void SetTransferRate(double KbPerSecond);
	void PlayAnimation();
	void SetProgress(DWORD dwBytesRead);
	void UpdateControlsDuringTransfer(DWORD dwStartTicks, DWORD& dwCurrentTicks, DWORD dwTotalBytesRead, DWORD& dwLastTotalBytes,
									  DWORD& dwLastPercentage, BOOL bGotFileSize, DWORD dwFileSize);

	CString		m_sError;
	CString		m_sServer;
	CString		m_sObject;
	CString		m_sFilename;
	INTERNET_PORT m_nPort;
	DWORD		m_dwServiceType;
	HINTERNET	m_hInternetSession;
	HINTERNET	m_hHttpConnection;
	HINTERNET	m_hHttpFile;
	BOOL		m_bAbort;
	BOOL		m_bSafeToClose;
	CFile		m_FileToWrite;
	CWinThread*	m_pThread;
	static ULONGLONG sm_ullWinInetVer;
};
