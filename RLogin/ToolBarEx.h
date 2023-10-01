#pragma once

class CToolBarEx : public CToolBar
{
	DECLARE_DYNAMIC(CToolBarEx)

public:
	CToolBarEx();
	virtual ~CToolBarEx();

public:
	typedef struct _DropItem {
		int		index;
		int     item;
		int     count;
		UINT    *plist;
	} DropItem;

	CArray<DropItem, DropItem &> m_DropItem;
	CWordArray m_ItemImage;

public:
	void RemoveAll();
	void InitDropDown();
	void SetDropItem(int index, int item, int count, UINT *plist);

	int GetItemImage(int item);
	int AddItemImage(int item);
	void CreateItemImage(int width, int height);

public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnDropDown(NMHDR* pNMHDR, LRESULT* pResult);
};