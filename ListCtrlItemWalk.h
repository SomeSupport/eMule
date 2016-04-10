#pragma once

class CListCtrlItemWalk
{
public:
	CListCtrlItemWalk(CListCtrl* pListCtrl) { m_pListCtrl = pListCtrl; }

	virtual CObject* GetNextSelectableItem();
	virtual CObject* GetPrevSelectableItem();

	CListCtrl* GetListCtrl() const { return m_pListCtrl; }

protected:
	CListCtrl* m_pListCtrl;
};
