#pragma once

class CButtonsTabCtrl : public CTabCtrl
{
	DECLARE_DYNAMIC(CButtonsTabCtrl)

public:
	CButtonsTabCtrl();
	virtual ~CButtonsTabCtrl();

protected:
	void InternalInit();

	virtual void PreSubclassWindow();
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg LRESULT _OnThemeChanged();
};
