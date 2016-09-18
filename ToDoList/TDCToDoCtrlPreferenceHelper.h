#if !defined(AFX_TDCTODOCTRLPREFERENCEHELPER_H__F9A9753D_D022_4FC7_B781_DB11A4B4A6E4__INCLUDED_)
#define AFX_TDCTODOCTRLPREFERENCEHELPER_H__F9A9753D_D022_4FC7_B781_DB11A4B4A6E4__INCLUDED_

#pragma once

class CToDoCtrl;
class CPreferencesDlg;

class CTDCToDoCtrlPreferenceHelper
{
public:
	static void UpdateToDoCtrl(const CPreferencesDlg& prefs, const TODOITEM& tdiDefault, 
								BOOL bShowProjectName, BOOL bShowTreeListBar,
								CFont& fontTree, CFont& fontComments, CToDoCtrl& tdc);
	static void UpdateToDoCtrl(const CPreferencesDlg& prefs, CToDoCtrl& tdc,
								BOOL bShowProjectName = FALSE, BOOL bShowTreeListBar = FALSE);

protected:
	static void UpdateToDoCtrlPrefs(const CPreferencesDlg& prefs, BOOL bShowProjectName, BOOL bShowTreeListBar, CToDoCtrl& tdc);

};

#endif // AFX_TDCTODOCTRLPREFERENCEHELPER_H__F9A9753D_D022_4FC7_B781_DB11A4B4A6E4__INCLUDED_