#pragma once
#include "ResizableLib\ResizableDialog.h"
#include "IconStatic.h"
#include "kademlia/routing/contact.h"

class CKadContactListCtrl;
class CKadContactHistogramCtrl;
class CKadLookupGraph;
class CKadSearchListCtrl;
class CCustomAutoComplete;
class CDropDownButton;
namespace Kademlia
{
	class CLookupHistory;
}

class CKademliaWnd : public CResizableDialog
{
	DECLARE_DYNAMIC(CKademliaWnd)

public:
	CKademliaWnd(CWnd* pParent = NULL);   // standard constructor
	virtual ~CKademliaWnd();

	// Dialog Data
	enum { IDD = IDD_KADEMLIAWND };

	// Contacts
	UINT GetContactCount() const;
	void UpdateKadContactCount();
	void StartUpdateContacts();
	void StopUpdateContacts();
	bool ContactAdd(const Kademlia::CContact* contact);
	void ContactRem(const Kademlia::CContact* contact);
	void ContactRef(const Kademlia::CContact* contact);
	void UpdateNodesDatFromURL(CString strURL);
	void UpdateSearchGraph(Kademlia::CLookupHistory* pLookupHistory);
	void SetSearchGraph(Kademlia::CLookupHistory* pLookupHistory, bool bMakeVisible);
	void ShowLookupGraph(bool bShow);
	void UpdateContactCount();
	void SetBootstrapListMode();

	// Searches
	CKadSearchListCtrl* searchList;

	void Localize();
	void UpdateControlsState();
	BOOL SaveAllSettings();

protected:
	CIconStatic					m_ctrlBootstrap;
	CKadContactListCtrl*		m_contactListCtrl;
	CKadContactHistogramCtrl*	m_contactHistogramCtrl;
	CKadLookupGraph*			m_kadLookupGraph; 
	CCustomAutoComplete*		m_pacONBSIPs;
	HICON						icon_kadsea;
	CDropDownButton*			m_pbtnWnd;

	bool						m_bBootstrapListMode;

	void		SetAllIcons();
	void		UpdateButtonTitle(bool bLookupGraph);

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedBootstrapbutton();
	afx_msg void OnBnConnect();
	afx_msg void OnBnClickedFirewallcheckbutton();
	afx_msg void OnSysColorChange();
	afx_msg void OnEnSetfocusBootstrapip();
	afx_msg void OnEnSetfocusBootstrapNodesdat();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnNMDblclkSearchlist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnListModifiedSearchlist(NMHDR *pNMHDR, LRESULT *pResult);
};
