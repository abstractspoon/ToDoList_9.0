// TDCMenuIconMgr.h: interface for the CTDCMenuIconMgr class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TDCMENUICONMGR_H__0FF0228C_515C_4E93_A957_1952AFD0F3A1__INCLUDED_)
#define AFX_TDCMENUICONMGR_H__0FF0228C_515C_4E93_A957_1952AFD0F3A1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//////////////////////////////////////////////////////////////////////

#include "..\shared\menuiconmgr.h"

//////////////////////////////////////////////////////////////////////

class CPreferencesDlg;

//////////////////////////////////////////////////////////////////////

class CTDCMenuIconMgr : public CMenuIconMgr
{
public:
	CTDCMenuIconMgr();
	virtual ~CTDCMenuIconMgr();
	
	void Release();

	void Populate(const CPreferencesDlg& prefs);
	void UpdateSourceControlStatus(BOOL bIsDisabled, BOOL bIsCheckedOut);
	void UpdateNewTaskIcons(const CPreferencesDlg& prefs);

protected:
	UINT m_nNewTaskID, m_nNewSubtaskID;

protected:
	static UINT GetNewTaskCmdID(const CPreferencesDlg& prefs);
	static UINT GetNewSubtaskCmdID(const CPreferencesDlg& prefs);

};

#endif // !defined(AFX_TDCMENUICONMGR_H__0FF0228C_515C_4E93_A957_1952AFD0F3A1__INCLUDED_)
