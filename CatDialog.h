//this file is part of eMule
// added by quekky
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
#pragma once
#include "ColorButton.h"

struct Category_Struct;
class CCustomAutoComplete;

class CCatDialog : public CDialog
{
	DECLARE_DYNAMIC(CCatDialog)
public:
	CCatDialog(int catindex);   // standard constructor
	virtual ~CCatDialog();

	enum { IDD = IDD_CAT };

protected:
	Category_Struct* m_myCat;
	DWORD newcolor;
	CColorButton m_ctlColor;
	CComboBox m_prio;
	CCustomAutoComplete* m_pacRegExp;

	void Localize();
	void UpdateData();

	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	afx_msg LONG OnSelChange(UINT lParam, LONG wParam);
	afx_msg void OnBnClickedBrowse();
	afx_msg void OnBnClickedOk();
	afx_msg void OnDDBnClicked();
};
