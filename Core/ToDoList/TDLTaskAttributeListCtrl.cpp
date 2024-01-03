// TDLTaskAttributeListCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "TDLTaskAttributeListCtrl.h"
#include "TDLTaskCtrlBase.h"
#include "TDCImageList.h"
#include "tdcstatic.h"
#include "tdcstruct.h"

#include "..\shared\GraphicsMisc.h"
#include "..\shared\FileMisc.h"
#include "..\shared\HoldRedraw.h"
#include "..\shared\Localizer.h"
#include "..\shared\encolordialog.h"
#include "..\shared\FileIcons.h"

#include "..\3rdParty\ColorDef.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////

enum 
{
	ATTRIB_COL,
	VALUE_COL
};

/////////////////////////////////////////////////////////////////////////////

enum 
{
	IDC_SINGLESEL_COMBO = 1000,
	IDC_MULTISEL_COMBO,
	IDC_DATE_PICKER,
	IDC_TIME_PICKER,
	IDC_PRIORITY_COMBO,
	IDC_RISK_COMBO,
	IDC_DEPENDS_EDIT,
	IDC_PERCENT_SPIN,
	IDC_TIMEPERIOD_EDIT,
	IDC_FILELINK_COMBO,
};

/////////////////////////////////////////////////////////////////////////////

enum 
{
	ID_BTN_TIMETRACK = 10,
	ID_BTN_ADDLOGGEDTIME,
	ID_BTN_SELECTDEPENDS
};

/////////////////////////////////////////////////////////////////////////////

const UINT IDS_PRIORITYRISK_SCALE[] = 
{ 
	IDS_TDC_SCALE0,
	IDS_TDC_SCALE1,
	IDS_TDC_SCALE2,
	IDS_TDC_SCALE3,
	IDS_TDC_SCALE4,
	IDS_TDC_SCALE5,
	IDS_TDC_SCALE6,
	IDS_TDC_SCALE7,
	IDS_TDC_SCALE8,
	IDS_TDC_SCALE9,
	IDS_TDC_SCALE10
};

/////////////////////////////////////////////////////////////////////////////

const int CUSTOMTIMEATTRIBOFFSET = (TDCA_LAST_ATTRIBUTE + 1);
const int ICON_SIZE = GraphicsMisc::ScaleByDPIFactor(16);

/////////////////////////////////////////////////////////////////////////////

static CContentMgr s_mgrContent;

/////////////////////////////////////////////////////////////////////////////
// CTDLTaskAttributeListCtrl

CTDLTaskAttributeListCtrl::CTDLTaskAttributeListCtrl(const CTDLTaskCtrlBase& taskCtrl,
													 const CToDoCtrlData& data,
													 const CTDCImageList& ilIcons,
													 const TDCCOLEDITVISIBILITY& defaultVis)
	:
	m_taskCtrl(taskCtrl),
	m_data(data),
	m_ilIcons(ilIcons),
	m_formatter(data, s_mgrContent),
	m_vis(defaultVis),
	m_cbSingleSelection(ACBS_ALLOWDELETE | ACBS_AUTOCOMPLETE),
	m_cbMultiSelection(ACBS_ALLOWDELETE | ACBS_AUTOCOMPLETE),
	m_cbTimeOfDay(TCB_HALFHOURS | TCB_NOTIME | TCB_HOURSINDAY),
	m_cbPriority(FALSE),
	m_cbRisk(FALSE)
{
	// Icon for 'Time Spent'
	m_iconTrackTime.Load(IDI_TIMETRACK, 16, FALSE);
	m_iconAddTime.Load(IDI_ADD_LOGGED_TIME, 16, FALSE);

	// add buttons to dependency
	m_iconLink.Load(IDI_DEPENDS_LINK, 16, FALSE);
	m_eDepends.AddButton(ID_BTN_SELECTDEPENDS, m_iconLink, CEnString(IDS_TDC_DEPENDSLINK_TIP));
}

CTDLTaskAttributeListCtrl::~CTDLTaskAttributeListCtrl()
{
}


BEGIN_MESSAGE_MAP(CTDLTaskAttributeListCtrl, CInputListCtrl)
	//{{AFX_MSG_MAP(CTDLTaskAttributeListCtrl)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_DROPFILES()
	ON_WM_ERASEBKGND()
	ON_WM_SETCURSOR()

	ON_NOTIFY(DTN_CLOSEUP, IDC_DATE_PICKER, OnDateCloseUp)
	ON_NOTIFY(DTN_DATETIMECHANGE, IDC_DATE_PICKER, OnDateChange)

	ON_EN_CHANGE(IDC_DEPENDS_EDIT, OnDependsChange)
	ON_EN_KILLFOCUS(IDC_TIMEPERIOD_EDIT, OnTimePeriodChange)

	ON_CONTROL_RANGE(CBN_CLOSEUP, 0, 0xffff, OnComboCloseUp)
	ON_CONTROL_RANGE(CBN_SELENDCANCEL, 0, 0xffff, OnComboEditCancel)
	ON_CONTROL_RANGE(CBN_SELCHANGE, 0, 0xffff, OnComboEditChange)

	ON_NOTIFY_REFLECT(LVN_ENDLABELEDIT, OnTextEditOK)

	ON_REGISTERED_MESSAGE(WM_ACBN_ITEMADDED, OnAutoComboAddDelete)
	ON_REGISTERED_MESSAGE(WM_ACBN_ITEMDELETED, OnAutoComboAddDelete)

	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTDLTaskAttributeListCtrl message handlers

int CTDLTaskAttributeListCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CInputListCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

	SetView(LVS_REPORT);
	EnableSorting(TRUE);
	ShowGrid(TRUE, TRUE);
	
	// Add columns
	AddCol(_T("Attribute"));
	AddCol(_T("Value"));

	SetColumnFormat(VALUE_COL, ES_NONE);

	// Add attributes
	Populate();

	// Create our edit fields
	CreateControl(m_cbSingleSelection, IDC_SINGLESEL_COMBO, (CBS_DROPDOWN | CBS_SORT));
	CreateControl(m_cbMultiSelection, IDC_MULTISEL_COMBO, (CBS_DROPDOWN | CBS_SORT));
	CreateControl(m_datePicker, IDC_DATE_PICKER);
	CreateControl(m_cbTimeOfDay, IDC_TIME_PICKER, CBS_DROPDOWN);
	CreateControl(m_cbPriority, IDC_PRIORITY_COMBO, CBS_DROPDOWNLIST);
	CreateControl(m_cbRisk, IDC_RISK_COMBO, CBS_DROPDOWNLIST);
	CreateControl(m_eDepends, IDC_DEPENDS_EDIT, ES_AUTOHSCROLL);
	CreateControl(m_eTimePeriod, IDC_TIMEPERIOD_EDIT, ES_AUTOHSCROLL);
	CreateControl(m_cbFileLinks, IDC_FILELINK_COMBO, CBS_DROPDOWN);

	VERIFY(m_spinPercent.Create(WS_CHILD | UDS_SETBUDDYINT | UDS_ARROWKEYS| UDS_ALIGNRIGHT, CRect(0, 0, 0, 0), this, IDC_PERCENT_SPIN));
	m_spinPercent.SetRange(0, 100);

	CLocalizer::EnableTranslation(m_cbSingleSelection, FALSE);
	CLocalizer::EnableTranslation(m_cbMultiSelection, FALSE);
	CLocalizer::EnableTranslation(m_cbPriority, FALSE);
	CLocalizer::EnableTranslation(m_cbRisk, FALSE);

	return 0;
}

void CTDLTaskAttributeListCtrl::RedrawValue(TDC_ATTRIBUTE nAttribID)
{
	int nRow = FindItemFromData(nAttribID);

	if (nRow != -1)
		RedrawCell(nRow, VALUE_COL, FALSE);
}

void CTDLTaskAttributeListCtrl::RefreshCompletionStatus()
{
	CString sStatus = m_taskCtrl.GetCompletionStatus();

	if (!sStatus.IsEmpty())
		Misc::AddUniqueItem(sStatus, m_tldDefault.aStatus);
	else
		Misc::RemoveItem(sStatus, m_tldDefault.aStatus);
}

void CTDLTaskAttributeListCtrl::RefreshDateTimeFormatting()
{
	int nRow = GetItemCount();

	while (nRow--)
	{
		switch (GetAttributeID(nRow))
		{
		case TDCA_DONEDATE:
		case TDCA_DUEDATE:
		case TDCA_STARTDATE:
		case TDCA_DONETIME:
		case TDCA_DUETIME:
		case TDCA_STARTTIME:
			RefreshSelectedTaskValue(nRow);
			break;
		}
	}
}

void CTDLTaskAttributeListCtrl::SetPercentDoneIncrement(int nAmount)
{
	ASSERT(m_spinPercent.GetSafeHwnd());
	
	UDACCEL uda = { 0, (UINT)nAmount };
	m_spinPercent.SetAccel(1, &uda);
}

void CTDLTaskAttributeListCtrl::SetCustomAttributeDefinitions(const CTDCCustomAttribDefinitionArray& aAttribDefs)
{
	if (Misc::MatchAllT(m_aCustomAttribDefs, aAttribDefs, FALSE))
		return;

	m_aCustomAttribDefs.Copy(aAttribDefs);
	Populate();
}

void CTDLTaskAttributeListCtrl::SetAttributeVisibility(const TDCCOLEDITVISIBILITY& vis)
{
	BOOL bColChange = FALSE, bEditChange = FALSE;
	BOOL bChange = m_vis.HasDifferences(vis, bColChange, bEditChange);

	if (!bChange)
		return;

	m_vis = vis;

	if (bEditChange || (bColChange && (vis.GetShowFields() == TDLSA_ASCOLUMN)))
		Populate();
}

void CTDLTaskAttributeListCtrl::CheckAddAttribute(TDC_ATTRIBUTE nAttribID, UINT nAttribResID)
{
	BOOL bAdd = (m_vis.IsEditFieldVisible(nAttribID) && (nAttribID != TDCA_PROJECTNAME));

	if (!bAdd)
	{
		bAdd = ((m_vis.GetShowFields() == TDLSA_ASCOLUMN) &&
				m_vis.IsColumnVisible(TDC::MapAttributeToColumn(nAttribID)));

		if (!bAdd)
		{
			bAdd = ((nAttribID == TDCA_ICON) && 
					m_data.HasStyle(TDCS_ALLOWTREEITEMCHECKBOX));
		}
	}

	if (bAdd)
	{
		int nItem = AddRow(CEnString(nAttribResID));
		SetItemData(nItem, nAttribID);
	}
}

void CTDLTaskAttributeListCtrl::Populate()
{
	CHoldRedraw hr(*this);

	// Preserve current selection
	int nSelRow, nSelCol;
	TDC_ATTRIBUTE nSelAttribID = TDCA_NONE;
	
	if (GetCurSel(nSelRow, nSelCol))
		nSelAttribID = GetAttributeID(nSelRow);

	DeleteAllItems();

	for (int nAttrib = 1; nAttrib < ATTRIB_COUNT; nAttrib++)
		CheckAddAttribute(ATTRIBUTES[nAttrib].nAttribID, ATTRIBUTES[nAttrib].nAttribResID);

	// Dependent time fields
	CheckAddAttribute(TDCA_STARTTIME, IDS_TDLBC_STARTTIME);
	CheckAddAttribute(TDCA_DUETIME, IDS_TDLBC_DUETIME);
	CheckAddAttribute(TDCA_DONETIME, IDS_TDLBC_DONETIME);

	// Custom attributes
	for (int nCust = 0; nCust < m_aCustomAttribDefs.GetSize(); nCust++)
	{
		const TDCCUSTOMATTRIBUTEDEFINITION& attribDef = m_aCustomAttribDefs[nCust];

		if (attribDef.bEnabled)
		{
			int nItem = AddRow(CEnString(IDS_CUSTOMCOLUMN, attribDef.sLabel));
			SetItemData(nItem, attribDef.GetAttributeID());

			if (attribDef.IsDataType(TDCCA_DATE) && attribDef.HasFeature(TDCCAF_SHOWTIME))
			{
				int nItem = AddRow(CEnString(IDS_CUSTOMCOLUMN, attribDef.sLabel));
				SetItemData(nItem, attribDef.GetAttributeID() + CUSTOMTIMEATTRIBOFFSET); // FUDGE
			}
		}
	}

	RefreshSelectedTaskValues();
	Sort();

	// Restore previous selection
	if (nSelAttribID != TDCA_NONE)
		SetCurSel(FindItemFromData(nSelAttribID), nSelCol);
}

void CTDLTaskAttributeListCtrl::OnSize(UINT nType, int cx, int cy) 
{
	CInputListCtrl::OnSize(nType, cx, cy);
	
	if (GetColumnCount())
	{
		SetColumnWidth(ATTRIB_COL, (cx / 2));
		SetColumnWidth(VALUE_COL, (cx / 2) - 1);
	}
}

void CTDLTaskAttributeListCtrl::OnDropFiles(HDROP hDropInfo) 
{
	// TODO: Add your message handler code here and/or call default
	
	CInputListCtrl::OnDropFiles(hDropInfo);
}

BOOL CTDLTaskAttributeListCtrl::OnEraseBkgnd(CDC* pDC) 
{
	// TODO: Add your message handler code here and/or call default
	
	return CInputListCtrl::OnEraseBkgnd(pDC);
}

BOOL CTDLTaskAttributeListCtrl::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message) 
{
	// TODO: Add your message handler code here and/or call default
	
	return CInputListCtrl::OnSetCursor(pWnd, nHitTest, message);
}


TDC_ATTRIBUTE CTDLTaskAttributeListCtrl::GetAttributeID(int nRow, BOOL bResolveCustomTimeFields) const
{ 
	if (nRow == -1)
		return TDCA_NONE;

	TDC_ATTRIBUTE nAttribID = (TDC_ATTRIBUTE)GetItemData(nRow); 

	if (bResolveCustomTimeFields && (nAttribID > CUSTOMTIMEATTRIBOFFSET))
		nAttribID = (TDC_ATTRIBUTE)(nAttribID - CUSTOMTIMEATTRIBOFFSET);

	return nAttribID;
}

IL_COLUMNTYPE CTDLTaskAttributeListCtrl::GetCellType(int nRow, int nCol) const
{
	if (nCol == ATTRIB_COL)
		return ILCT_TEXT;

	// else
	TDC_ATTRIBUTE nAttribID = GetAttributeID(nRow);

	switch (nAttribID)
	{
	case TDCA_TASKNAME:
	case TDCA_COST:
	case TDCA_EXTERNALID:
	case TDCA_PERCENT:
		return ILCT_TEXT;

	case TDCA_TIMEESTIMATE:
	case TDCA_TIMESPENT:
		return ILCT_POPUPMENU;

	case TDCA_DONEDATE:
	case TDCA_DUEDATE:
	case TDCA_STARTDATE:
		return ILCT_DATE;

	case TDCA_PRIORITY:
	case TDCA_ALLOCTO:
	case TDCA_ALLOCBY:
	case TDCA_STATUS:
	case TDCA_CATEGORY:
	case TDCA_TAGS:
	case TDCA_FILELINK:
	case TDCA_RISK:
	case TDCA_VERSION:
	case TDCA_DONETIME:
	case TDCA_DUETIME:
	case TDCA_STARTTIME:
		return ILCT_DROPLIST;

	case TDCA_ICON:
	case TDCA_RECURRENCE:
	case TDCA_DEPENDENCY:
	case TDCA_COLOR:
		return ILCT_BROWSE;

	case TDCA_FLAG:
	case TDCA_LOCK:
		return ILCT_CHECK;

	case TDCA_CREATEDBY:
	case TDCA_PATH:
	case TDCA_POSITION:
	case TDCA_CREATIONDATE:
	case TDCA_LASTMODDATE:
	case TDCA_COMMENTSSIZE:
	case TDCA_COMMENTSFORMAT:
	case TDCA_SUBTASKDONE:
	case TDCA_LASTMODBY:
	case TDCA_ID:
	case TDCA_PARENTID:
		return ILCT_TEXT;

	default:
		if (TDCCUSTOMATTRIBUTEDEFINITION::IsCustomAttribute(nAttribID))
		{
			int nCust = m_aCustomAttribDefs.Find(nAttribID);
			ASSERT(nCust != -1);

			if (nCust != -1)
			{
				if (m_aCustomAttribDefs[nCust].IsList())
					return ILCT_DROPLIST;

				// else
				switch (m_aCustomAttribDefs[nCust].GetDataType())
				{
				case TDCCA_STRING:
				case TDCCA_FRACTION:
				case TDCCA_INTEGER:
				case TDCCA_DOUBLE:
				case TDCCA_CALCULATION:
				case TDCCA_TIMEPERIOD:
					return ILCT_TEXT;

				case TDCCA_DATE:
					return ILCT_DATE;

				case TDCCA_BOOL:
					return ILCT_CHECK;

				case TDCCA_ICON:
				case TDCCA_FILELINK:
					return ILCT_BROWSE;
				}
			}
		}
		else if (nAttribID > CUSTOMTIMEATTRIBOFFSET)
		{
			return ILCT_DROPLIST;
		}
		break;
	}

	// All else
	ASSERT(0);
	return ILCT_TEXT;
}

BOOL CTDLTaskAttributeListCtrl::CanEditCell(int nRow, int nCol) const
{
	if (!IsWindowEnabled() || (nCol == ATTRIB_COL) || m_data.HasStyle(TDCS_READONLY))
		return FALSE;

	if (m_taskCtrl.SelectionHasLocked())
		return FALSE;

	// else
	TDC_ATTRIBUTE nAttribID = GetAttributeID(nRow);

	switch (nAttribID)
	{
		case TDCA_CREATEDBY:
		case TDCA_PATH:
		case TDCA_POSITION:
		case TDCA_CREATIONDATE:
		case TDCA_LASTMODDATE:
		case TDCA_COMMENTSSIZE:
		case TDCA_COMMENTSFORMAT:
		case TDCA_SUBTASKDONE:
		case TDCA_LASTMODBY:
		case TDCA_ID:
		case TDCA_PARENTID:
			// Permanently read-only fields
			return FALSE;

		case TDCA_PERCENT:
			return !m_data.HasStyle(TDCS_AUTOCALCPERCENTDONE);

		case TDCA_LOCK:
			return TRUE;

		case TDCA_STARTTIME:
			return m_taskCtrl.SelectedTaskHasDate(TDCD_STARTDATE);

		case TDCA_DUETIME:
			return m_taskCtrl.SelectedTaskHasDate(TDCD_DUEDATE);

		case TDCA_DONETIME:
			return m_taskCtrl.SelectedTaskHasDate(TDCD_DONEDATE);

		case TDCA_TIMEESTIMATE:
		case TDCA_TIMESPENT:
			return (m_data.HasStyle(TDCS_ALLOWPARENTTIMETRACKING) || !m_taskCtrl.SelectionHasParents());
	}

	if (TDCCUSTOMATTRIBUTEDEFINITION::IsCustomAttribute(nAttribID))
		return (m_aCustomAttribDefs.GetAttributeDataType(nAttribID) != TDCCA_CALCULATION);

	return TRUE;
}

COLORREF CTDLTaskAttributeListCtrl::GetItemBackColor(int nItem, int nCol, BOOL bSelected, BOOL bDropHighlighted, BOOL bWndFocus) const
{
	if (nCol == VALUE_COL)
	{
		if (!CanEditCell(nItem, nCol))
			return GetSysColor(COLOR_3DFACE);
	}

	// All else
	return CInputListCtrl::GetItemBackColor(nItem, nCol, bSelected, bDropHighlighted, bWndFocus);
}

COLORREF CTDLTaskAttributeListCtrl::GetItemTextColor(int nItem, int nCol, BOOL bSelected, BOOL bDropHighlighted, BOOL bWndFocus) const
{
	if (nCol == VALUE_COL)
	{
		if (!CanEditCell(nItem, nCol))
			return GetSysColor(COLOR_GRAYTEXT);

		switch (GetAttributeID(nItem))
		{
		case TDCA_DEPENDENCY:
			if (m_taskCtrl.SelectionHasCircularDependencies())
				return colorRed;
			break;
		}
	}

	// All else
	return CInputListCtrl::GetItemTextColor(nItem, nCol, bSelected, bDropHighlighted, bWndFocus);
}

void CTDLTaskAttributeListCtrl::SetDefaultAutoListData(const TDCAUTOLISTDATA& tldDefault) 
{ 
	m_tldAll.RemoveItems(m_tldDefault, TDCA_ALL);
	m_tldAll.AppendUnique(tldDefault, TDCA_ALL);

	m_tldDefault.Copy(tldDefault, TDCA_ALL);
}

void CTDLTaskAttributeListCtrl::SetAutoListData(const TDCAUTOLISTDATA& tld, TDC_ATTRIBUTE nAttribID)
{
	m_tldAll.Copy(tld, nAttribID);
}

void CTDLTaskAttributeListCtrl::GetAutoListData(TDCAUTOLISTDATA& tld, TDC_ATTRIBUTE nAttribID) const
{
	tld.Copy(m_tldAll, nAttribID);
}

void CTDLTaskAttributeListCtrl::RefreshSelectedTaskValues(BOOL bForceClear)
{
	CHoldRedraw hr(*this);

	int nRow = GetItemCount();

	while (nRow--)
	{
		if (bForceClear)
			SetItemText(nRow, VALUE_COL, _T(""));
		else
			RefreshSelectedTaskValue(nRow);
	}
}

void CTDLTaskAttributeListCtrl::RefreshSelectedTaskValue(TDC_ATTRIBUTE nAttribID)
{
	int nRow = FindItemFromData(nAttribID);
	ASSERT(nRow);

	RefreshSelectedTaskValue(nRow);
}

void CTDLTaskAttributeListCtrl::RefreshSelectedTaskValue(int nRow)
{
	CString sValue;
	CStringArray aMatched, aMixed;

	DWORD dwSingleSelTaskID = ((m_taskCtrl.GetSelectedCount() == 1) ? m_taskCtrl.GetSelectedTaskID() : 0);
	TDC_ATTRIBUTE nAttribID = GetAttributeID(nRow);

	switch (nAttribID)
	{
	case TDCA_EXTERNALID:	sValue = m_taskCtrl.GetSelectedTaskExtID();		break;
	case TDCA_ALLOCBY:		sValue = m_taskCtrl.GetSelectedTaskAllocBy();	break;
	case TDCA_STATUS:		sValue = m_taskCtrl.GetSelectedTaskStatus();	break;
	case TDCA_VERSION:		sValue = m_taskCtrl.GetSelectedTaskVersion();	break;
	case TDCA_ICON:			sValue = m_taskCtrl.GetSelectedTaskIcon();		break;

	case TDCA_FLAG:			sValue = m_taskCtrl.IsSelectedTaskFlagged() ? _T("+") : _T(""); break;
	case TDCA_LOCK:			sValue = m_taskCtrl.IsSelectedTaskLocked() ? _T("+") : _T("");	break;

	case TDCA_ALLOCTO:		m_taskCtrl.GetSelectedTaskAllocTo(aMatched, aMixed);	break;
	case TDCA_CATEGORY:		m_taskCtrl.GetSelectedTaskCategories(aMatched, aMixed); break;
	case TDCA_TAGS:			m_taskCtrl.GetSelectedTaskTags(aMatched, aMixed);		break;
	case TDCA_FILELINK:		m_taskCtrl.GetSelectedTaskFileLinks(aMatched, TRUE);	break;

	case TDCA_COST:
		{
			TDCCOST cost;

			if (m_taskCtrl.GetSelectedTaskCost(cost))
				sValue = cost.Format(2);
		}
		break;

	case TDCA_PERCENT:
		if (m_taskCtrl.IsSelectedTaskDone())
		{
			sValue = _T("100");
		}
		else if (dwSingleSelTaskID && m_data.HasStyle(TDCS_AUTOCALCPERCENTDONE))
		{
			sValue = m_formatter.GetTaskPercentDone(dwSingleSelTaskID);
		}
		else
		{
			int nValue = m_taskCtrl.GetSelectedTaskPercent();

			if (nValue != -1)
				sValue = Misc::Format(nValue);
		}
		break;

	case TDCA_PRIORITY:
		sValue = Misc::Format(m_taskCtrl.GetSelectedTaskPriority());
		break;

	case TDCA_RISK:
		sValue = Misc::Format(m_taskCtrl.GetSelectedTaskRisk());
		break;

	case TDCA_TIMEESTIMATE:
		if (m_data.HasStyle(TDCS_ALLOWPARENTTIMETRACKING) || !m_taskCtrl.SelectionHasParents())
		{
			TDCTIMEPERIOD tp;

			if (m_taskCtrl.GetSelectedTaskTimeEstimate(tp))
				sValue = tp.Format(2);
		}
		else if (dwSingleSelTaskID)
		{
			sValue = m_formatter.GetTaskTimeEstimate(dwSingleSelTaskID);
		}
		break;

	case TDCA_TIMESPENT:
		if (m_data.HasStyle(TDCS_ALLOWPARENTTIMETRACKING) || !m_taskCtrl.SelectionHasParents())
		{
			TDCTIMEPERIOD tp;

			if (m_taskCtrl.GetSelectedTaskTimeSpent(tp))
				sValue = tp.Format(2);
		}
		else if (dwSingleSelTaskID)
		{
			sValue = m_formatter.GetTaskTimeEstimate(dwSingleSelTaskID);
		}
		break;

	case TDCA_DONEDATE:
	case TDCA_DUEDATE:
	case TDCA_STARTDATE:
		{
			COleDateTime date = m_taskCtrl.GetSelectedTaskDate(TDC::MapAttributeToDate(nAttribID));
			sValue = CDateHelper::FormatDate(date, (m_data.HasStyle(TDCS_SHOWDATESINISO) ? DHFD_ISO : 0));
		}
		break;

	case TDCA_DONETIME:
	case TDCA_DUETIME:
	case TDCA_STARTTIME:
		{
			COleDateTime date = m_taskCtrl.GetSelectedTaskDate(TDC::MapAttributeToDate(nAttribID));

			if (CDateHelper::DateHasTime(date))
				sValue = CTimeHelper::FormatClockTime(date, FALSE, m_data.HasStyle(TDCS_SHOWDATESINISO));
		}
		break;

	case TDCA_LASTMODDATE:
	case TDCA_CREATIONDATE:
		{
			COleDateTime date = m_taskCtrl.GetSelectedTaskDate(TDC::MapAttributeToDate(nAttribID));
			sValue = CDateHelper::FormatDate(date, DHFD_TIME);
		}
		break;

	case TDCA_COLOR:
		sValue = Misc::Format(m_taskCtrl.GetSelectedTaskColor());
		break;

	case TDCA_RECURRENCE:
		{
			TDCRECURRENCE recurs;

			if (m_taskCtrl.GetSelectedTaskRecurrence(recurs))
				sValue = recurs.GetRegularityText();
		}
		break;

	case TDCA_DEPENDENCY:
		{
			CTDCDependencyArray aDepends;

			if (m_taskCtrl.GetSelectedTaskDependencies(aDepends))
				sValue = aDepends.Format();
		}
		break;

	case TDCA_POSITION:
		if (dwSingleSelTaskID)
			sValue = m_formatter.GetTaskPosition(dwSingleSelTaskID);
		break;


	case TDCA_ID:
		if (dwSingleSelTaskID)
			sValue = Misc::Format(dwSingleSelTaskID);
		break;

	case TDCA_PARENTID:
		{
			DWORD dwID = m_taskCtrl.GetSelectedTaskParentID();

			if (dwID)
				sValue = Misc::Format(dwID);
		}
		break;

	case TDCA_PATH:
		if (dwSingleSelTaskID)
		{
			sValue = m_formatter.GetTaskPath(dwSingleSelTaskID);
		}
		else
		{
			DWORD dwID = m_taskCtrl.GetSelectedTaskParentID();

			if (dwID)
				sValue = m_formatter.GetTaskPath(m_taskCtrl.GetSelectedTaskID());
		}
		break;

	case TDCA_TASKNAME:			break; // TODO
	case TDCA_CREATEDBY:		break; // TODO
	case TDCA_COMMENTSSIZE:		break; // TODO
	case TDCA_COMMENTSFORMAT:	break; // TODO
	case TDCA_SUBTASKDONE:		break; // TODO
	case TDCA_LASTMODBY:		break; // TODO

	default:
		if (TDCCUSTOMATTRIBUTEDEFINITION::IsCustomAttribute(nAttribID))
		{
			int nCust = m_aCustomAttribDefs.Find(nAttribID);
			ASSERT(nCust != -1);

			if (nCust != -1)
			{
				CString sAttribID = m_aCustomAttribDefs[nCust].sUniqueID;
				TDCCADATA data;

				if (m_taskCtrl.GetSelectedTaskCustomAttributeData(sAttribID, data))
				{
					if (m_aCustomAttribDefs[nCust].IsList())
					{
						sValue = data.FormatAsArray();
					}
					else
					{
						switch (m_aCustomAttribDefs[nCust].GetDataType())
						{
						case TDCCA_STRING:
						case TDCCA_FRACTION:
						case TDCCA_INTEGER:
						case TDCCA_DOUBLE:
						case TDCCA_ICON:
						case TDCCA_FILELINK:
							sValue = data.AsString();
							break;

						case TDCCA_CALCULATION:
							// TODO
							break;

						case TDCCA_TIMEPERIOD:
							sValue = data.FormatAsTimePeriod();
							break;

						case TDCCA_DATE:
							sValue = data.FormatAsDate(FALSE, FALSE);
							break;

						case TDCCA_BOOL:
							sValue = (data.AsBool() ? _T("+") : _T(""));
							break;
						}
					}
				}
			}
		}
		else if (nAttribID > CUSTOMTIMEATTRIBOFFSET)
		{
			nAttribID = (TDC_ATTRIBUTE)(nAttribID - CUSTOMTIMEATTRIBOFFSET);

			CString sAttribID = m_aCustomAttribDefs.GetAttributeTypeID(nAttribID);
			TDCCADATA data;

			if (m_taskCtrl.GetSelectedTaskCustomAttributeData(sAttribID, data))
			{
				ASSERT(m_aCustomAttribDefs.GetAttributeDataType(sAttribID) == TDCCA_DATE);

				sValue = CTimeHelper::FormatClockTime(data.AsDate());
			}
		}
		break;
	}

	if (aMatched.GetSize() || aMixed.GetSize())
		sValue = FormatMultiSelItems(aMatched, aMixed);

	SetItemText(nRow, VALUE_COL, sValue);
}

void CTDLTaskAttributeListCtrl::DrawCellText(CDC* pDC, int nRow, int nCol, const CRect& rText, const CString& sText, COLORREF crText, UINT nDrawTextFlags)
{
	// Only draw what we really need to
	if (nCol == VALUE_COL)
	{
		TDC_ATTRIBUTE nAttribID = GetAttributeID(nRow);

		switch (nAttribID)
		{
		case TDCA_PRIORITY:
			{
				int nPriority = _ttoi(sText);
				
				if (nPriority >= 0)
				{
					// Draw box
					CRect rBox(rText);
					rBox.DeflateRect(0, 3);
					rBox.right = rBox.left + rBox.Height();

					COLORREF crFill = m_taskCtrl.GetPriorityColor(nPriority);
					COLORREF crBorder = GraphicsMisc::Darker(crFill, 0.5);

					GraphicsMisc::DrawRect(pDC, rBox, crFill, crBorder);

					// Draw text
					CRect rLeft(rText);
					rLeft.left += rText.Height();

					CString sPriority = sText + Misc::Format(_T(" (%s)"), CEnString(IDS_PRIORITYRISK_SCALE[nPriority]));
					CInputListCtrl::DrawCellText(pDC, nRow, nCol, rLeft, sPriority, crText, nDrawTextFlags);
				}
			}
			return;

		case TDCA_RISK:
			{
				int nRisk = _ttoi(sText);
				
				if (nRisk >= 0)
				{
					CString sPriority = sText + Misc::Format(_T(" (%s)"), CEnString(IDS_PRIORITYRISK_SCALE[nRisk]));
					CInputListCtrl::DrawCellText(pDC, nRow, nCol, rText, sPriority, crText, nDrawTextFlags);
				}
			}
			return;

		case TDCA_ICON:
			DrawIcon(pDC, sText, rText);
			return;

		case TDCA_COLOR:
			{
				crText = _ttoi(sText);

				if (crText != CLR_NONE)
				{
					if (m_data.HasStyle(TDCS_TASKCOLORISBACKGROUND))
					{
						// Use the entire cell rect for the background colour
						CRect rCell;
						GetCellRect(nRow, nCol, rCell);

						pDC->FillSolidRect(rCell, crText);
						crText = GraphicsMisc::GetBestTextColor(crText);
					}

					CInputListCtrl::DrawCellText(pDC, nRow, nCol, rText, CEnString(IDS_COLOR_SAMPLETEXT), crText, nDrawTextFlags);
				}
			}
			return;

		case TDCA_ALLOCTO:
		case TDCA_CATEGORY:
		case TDCA_TAGS:
			{
				int nMixed = sText.Find('|');

				if (nMixed != -1)
				{
					CInputListCtrl::DrawCellText(pDC, nRow, nCol, rText, sText.Left(nMixed), crText, nDrawTextFlags);
					return;
				}
			}
			break;

		case TDCA_FILELINK:
			if (!sText.IsEmpty())
			{
				// Just draw the first file without its path
				CString sFile(sText);
				Misc::Split(sFile, CString(), Misc::GetListSeparator());

				CRect rRest(rText);

				if (CFileIcons::Draw(pDC, FileMisc::GetExtension(sFile), GetIconPos(rText)))
					rRest.left += ICON_SIZE + 2;

				CInputListCtrl::DrawCellText(pDC, nRow, nCol, rRest, FileMisc::GetFileNameFromPath(sFile), crText, nDrawTextFlags);
			}
			return;

		default:
			if (TDCCUSTOMATTRIBUTEDEFINITION::IsCustomAttribute(nAttribID) &&
				m_aCustomAttribDefs.GetAttributeDataType(nAttribID) == TDCCA_ICON)
			{
				CStringArray aIcons;
				int nNumIcons = Misc::Split(sText, aIcons);

				CRect rIcon(rText);

				for (int nIcon = 0; nIcon < nNumIcons; nIcon++)
				{
					DrawIcon(pDC, aIcons[nIcon], rIcon);
					rIcon.left += GraphicsMisc::ScaleByDPIFactor(16) + 2;
				}
				return;
			}
			break;
		}
	}

	CInputListCtrl::DrawCellText(pDC, nRow, nCol, rText, sText, crText, nDrawTextFlags);
}

CPoint CTDLTaskAttributeListCtrl::GetIconPos(const CRect& rText)
{
	return CPoint(rText.left - 1, rText.top + ((rText.Height() - ICON_SIZE) / 2));
}

void CTDLTaskAttributeListCtrl::DrawIcon(CDC* pDC, const CString& sIcon, const CRect& rText)
{
	if (!sIcon.IsEmpty())
		m_ilIcons.Draw(pDC, sIcon, GetIconPos(rText), ILD_TRANSPARENT);
}

void CTDLTaskAttributeListCtrl::OnTextEditOK(NMHDR* pNMHDR, LRESULT* pResult)
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;

	if ((pDispInfo->item.iSubItem == VALUE_COL) &&
		(pDispInfo->item.iItem >= 0))
	{
		TDC_ATTRIBUTE nAttribID = GetAttributeID(pDispInfo->item.iItem, TRUE);
		
		NotifyParentEdit(nAttribID);
		RefreshSelectedTaskValue(nAttribID);
	}

	*pResult = 0;
}

BOOL CTDLTaskAttributeListCtrl::GetTimeEstimate(TDCTIMEPERIOD& timeEst) const
{
	return timeEst.Parse(GetValueText(TDCA_TIMEESTIMATE));
}

BOOL CTDLTaskAttributeListCtrl::GetTimeSpent(TDCTIMEPERIOD& timeSpent) const
{
	return timeSpent.Parse(GetValueText(TDCA_TIMESPENT));
}

int CTDLTaskAttributeListCtrl::GetAllocTo(CStringArray& aMatched, CStringArray& aMixed) const
{
	return ParseMultiSelValues(GetValueText(TDCA_ALLOCTO), aMatched, aMixed);
}

CString CTDLTaskAttributeListCtrl::GetAllocBy() const
{
	return GetValueText(TDCA_ALLOCBY);
}

CString CTDLTaskAttributeListCtrl::GetStatus() const
{
	return GetValueText(TDCA_STATUS);
}

int CTDLTaskAttributeListCtrl::GetCategories(CStringArray& aMatched, CStringArray& aMixed) const
{
	return ParseMultiSelValues(GetValueText(TDCA_CATEGORY), aMatched, aMixed);
}

int CTDLTaskAttributeListCtrl::GetDependencies(CTDCDependencyArray& aDepends) const
{
	return m_eDepends.GetDependencies(aDepends);
}

int CTDLTaskAttributeListCtrl::GetTags(CStringArray& aMatched, CStringArray& aMixed) const
{
	return ParseMultiSelValues(GetValueText(TDCA_TAGS), aMatched, aMixed);
}

int CTDLTaskAttributeListCtrl::GetFileLinks(CStringArray& aFiles) const
{
	return Misc::Split(GetValueText(TDCA_FILELINK), aFiles);
}

CString CTDLTaskAttributeListCtrl::GetExternalID() const
{
	return GetValueText(TDCA_EXTERNALID);
}

int CTDLTaskAttributeListCtrl::GetPercent() const
{
	return _ttoi(GetValueText(TDCA_PERCENT));
}

int CTDLTaskAttributeListCtrl::GetPriority() const
{
	CString sValue = GetValueText(TDCA_PRIORITY);

	return (sValue.IsEmpty() ? TDC_NOPRIORITYORISK : _ttoi(sValue));
}

int CTDLTaskAttributeListCtrl::GetRisk() const
{
	CString sValue = GetValueText(TDCA_RISK);

	return (sValue.IsEmpty() ? TDC_NOPRIORITYORISK : _ttoi(sValue));
}

BOOL CTDLTaskAttributeListCtrl::GetCost(TDCCOST& cost) const
{
	return cost.Parse(GetValueText(TDCA_COST));
}

BOOL CTDLTaskAttributeListCtrl::GetFlag() const
{
	return !GetValueText(TDCA_FLAG).IsEmpty();
}

BOOL CTDLTaskAttributeListCtrl::GetLock() const
{
	return !GetValueText(TDCA_LOCK).IsEmpty();
}

CString CTDLTaskAttributeListCtrl::GetVersion() const
{
	return GetValueText(TDCA_VERSION);
}

COleDateTime CTDLTaskAttributeListCtrl::GetStartDate() const
{
	COleDateTime date;
	CDateHelper::DecodeDate(GetValueText(TDCA_STARTDATE), date, FALSE);

	return date;
}

COleDateTime CTDLTaskAttributeListCtrl::GetDueDate() const
{
	COleDateTime date;
	CDateHelper::DecodeDate(GetValueText(TDCA_DUEDATE), date, FALSE);

	return date;
}

COleDateTime CTDLTaskAttributeListCtrl::GetDoneDate() const
{
	COleDateTime date;
	CDateHelper::DecodeDate(GetValueText(TDCA_DONEDATE), date, FALSE);

	return date;
}

COleDateTime CTDLTaskAttributeListCtrl::GetStartTime() const
{
	return (CTimeHelper::DecodeClockTime(GetValueText(TDCA_STARTTIME)) / 24);
}

COleDateTime CTDLTaskAttributeListCtrl::GetDueTime() const
{
	return (CTimeHelper::DecodeClockTime(GetValueText(TDCA_DUETIME)) / 24);
}

COleDateTime CTDLTaskAttributeListCtrl::GetDoneTime() const
{
	return (CTimeHelper::DecodeClockTime(GetValueText(TDCA_DONETIME)) / 24);
}

BOOL CTDLTaskAttributeListCtrl::GetCustomAttributeData(const CString& sAttribID, TDCCADATA& data) const
{
	TDC_ATTRIBUTE nAttribID = m_aCustomAttribDefs.GetAttributeID(sAttribID);

	CString sValue = GetValueText(nAttribID);

	return (sValue.IsEmpty() ? CLR_NONE : _ttoi(sValue));
}

CString CTDLTaskAttributeListCtrl::FormatMultiSelItems(const CStringArray& aMatched, const CStringArray& aMixed)
{
	CString sValue = Misc::FormatArray(aMatched);
	
	if (aMixed.GetSize())
		sValue += ('|' + Misc::FormatArray(aMixed));

	return sValue;
}

int CTDLTaskAttributeListCtrl::ParseMultiSelValues(const CString& sValues, CStringArray& aMatched, CStringArray& aMixed)
{
	CString sMatched(sValues), sMixed;

	Misc::Split(sMatched, sMixed, '|');
	Misc::Split(sMatched, aMatched);
	Misc::Split(sMixed, aMixed);

	return aMatched.GetSize();
}

void CTDLTaskAttributeListCtrl::PrepareMultiSelCombo(int nRow, const CStringArray& aDefValues, const CStringArray& aUserValues)
{
	m_cbMultiSelection.ResetContent();
	m_cbMultiSelection.AddStrings(aDefValues);
	m_cbMultiSelection.AddUniqueItems(aUserValues);

	CStringArray aMatched, aMixed;
	ParseMultiSelValues(GetItemText(nRow, VALUE_COL), aMatched, aMixed);

	m_cbMultiSelection.SetChecked(aMatched, aMixed);
}

void CTDLTaskAttributeListCtrl::PrepareSingleSelCombo(int nRow, const CStringArray& aDefValues, const CStringArray& aUserValues)
{
	m_cbSingleSelection.ResetContent();
	m_cbSingleSelection.AddStrings(aDefValues);
	m_cbSingleSelection.AddUniqueItems(aUserValues);

	m_cbSingleSelection.SelectString(-1, GetItemText(nRow, VALUE_COL));
}

void CTDLTaskAttributeListCtrl::PrepareControl(CWnd& ctrl, int nRow, int nCol)
{
	if (nCol != VALUE_COL)
	{
		ASSERT(0);
		return;
	}

	TDC_ATTRIBUTE nAttribID = GetAttributeID(nRow);

	switch (nAttribID)
	{
	case TDCA_ALLOCBY:	PrepareSingleSelCombo(nRow, m_tldDefault.aAllocBy, m_tldAll.aAllocBy);	break;
	case TDCA_STATUS: 	PrepareSingleSelCombo(nRow, m_tldDefault.aStatus, m_tldAll.aStatus);	break;
	case TDCA_VERSION: 	PrepareSingleSelCombo(nRow, m_tldDefault.aVersion, m_tldAll.aVersion);	break;
		break;

	case TDCA_ALLOCTO:	PrepareMultiSelCombo(nRow, m_tldDefault.aAllocTo, m_tldAll.aAllocTo);	break;
	case TDCA_CATEGORY: PrepareMultiSelCombo(nRow, m_tldDefault.aCategory, m_tldAll.aCategory);	break;
	case TDCA_TAGS:		PrepareMultiSelCombo(nRow, m_tldDefault.aTags, m_tldAll.aTags);			break;

	case TDCA_FILELINK:
		{
			CStringArray aFiles;
			m_taskCtrl.GetSelectedTaskFileLinks(aFiles, TRUE);

			m_cbFileLinks.SetFileList(aFiles);
			m_cbFileLinks.SetButtonBorderWidth(0);
			m_cbFileLinks.SetDefaultButton(0);
		}
		break;

	case TDCA_ICON: 
	case TDCA_FLAG: 
	case TDCA_LOCK: 
	case TDCA_COLOR:
	case TDCA_RECURRENCE:
	case TDCA_TASKNAME:
	case TDCA_EXTERNALID: 
		// Nothing to do
		break;

	case TDCA_PERCENT:
		m_editBox.SetMask(_T("0123456789"));
		m_editBox.SetSpinBuddy(&m_spinPercent);
		break;

	case TDCA_DEPENDENCY:
		{
			CTDCDependencyArray aDepends;
			m_taskCtrl.GetSelectedTaskDependencies(aDepends);

			m_eDepends.SetDependencies(aDepends);
		}
		break;

	case TDCA_COST:
		m_editBox.SetMask(_T("-0123456789.@"), ME_LOCALIZEDECIMAL);
		break;

	case TDCA_PRIORITY:
		{
			CDWordArray aColors;

			m_taskCtrl.GetPriorityColors(aColors);
			m_cbPriority.SetColors(aColors);

			m_cbPriority.SetSelectedPriority(_ttoi(GetItemText(nRow, nCol)));
		}
		break;

	case TDCA_RISK:
		m_cbRisk.SetSelectedRisk(_ttoi(GetItemText(nRow, nCol)));
		break;

	case TDCA_TIMEESTIMATE:
		PrepareTimePeriodEdit(nRow);
		break;

	case TDCA_TIMESPENT:
		{
			PrepareTimePeriodEdit(nRow);

			m_eTimePeriod.InsertButton(0, ID_BTN_TIMETRACK, m_iconTrackTime, CEnString(IDS_TDC_STARTSTOPCLOCK), 15);
			m_eTimePeriod.InsertButton(1, ID_BTN_ADDLOGGEDTIME, m_iconAddTime, CEnString(IDS_TDC_ADDLOGGEDTIME), 15);
		}
		break;

	case TDCA_DONEDATE:
		PrepareDatePicker(nRow, TDCA_NONE);
		break;

	case TDCA_DUEDATE:
		PrepareDatePicker(nRow, TDCA_STARTDATE);
		break;

	case TDCA_STARTDATE:
		PrepareDatePicker(nRow, TDCA_DUEDATE);
		break;

	case TDCA_DONETIME:
	case TDCA_DUETIME:
	case TDCA_STARTTIME:
		PrepareTimeCombo(nRow);
		break;

	default:
		if (TDCCUSTOMATTRIBUTEDEFINITION::IsCustomAttribute(nAttribID))
		{
			int nCust = m_aCustomAttribDefs.Find(nAttribID);
			ASSERT(nCust != -1);

			if (nCust != -1)
			{
				CString sAttribID = m_aCustomAttribDefs[nCust].sUniqueID;
				TDCCADATA data;

				if (m_taskCtrl.GetSelectedTaskCustomAttributeData(sAttribID, data))
				{
					if (m_aCustomAttribDefs[nCust].IsList())
					{
						// TODO
					}
					else
					{
						switch (m_aCustomAttribDefs[nCust].GetDataType())
						{
						case TDCCA_STRING:
						case TDCCA_FRACTION:
						case TDCCA_INTEGER:
						case TDCCA_DOUBLE:
						case TDCCA_ICON:
						case TDCCA_FILELINK:
							// TODO
							break;

						case TDCCA_CALCULATION:
							// TODO
							break;

						case TDCCA_TIMEPERIOD:
							// TODO
							break;

						case TDCCA_DATE:
							// TODO
							break;

						case TDCCA_BOOL:
							// TODO
							break;
						}
					}
				}
			}
		}
		else if (nAttribID > CUSTOMTIMEATTRIBOFFSET)
		{
			nAttribID = (TDC_ATTRIBUTE)(nAttribID - CUSTOMTIMEATTRIBOFFSET);

			CString sAttribID = m_aCustomAttribDefs.GetAttributeTypeID(nAttribID);
			TDCCADATA data;

			if (m_taskCtrl.GetSelectedTaskCustomAttributeData(sAttribID, data))
			{
				ASSERT(m_aCustomAttribDefs.GetAttributeDataType(sAttribID) == TDCCA_DATE);

				// TODO
			}
		}
		break;
	}
}

void CTDLTaskAttributeListCtrl::PrepareDatePicker(int nRow, TDC_ATTRIBUTE nFallbackDate)
{
	CString sValue = GetItemText(nRow, VALUE_COL);

	if (sValue.IsEmpty() && 
		(nFallbackDate != TDCA_NONE) && 
		(TDC::MapAttributeToDate(nFallbackDate) != TDCD_NONE))
	{
		sValue = GetValueText(nFallbackDate);
	}

	COleDateTime date;

	if (CDateHelper::DecodeDate(sValue, date, FALSE))
		m_datePicker.SetTime(date);
	else
		m_datePicker.SendMessage(DTM_SETSYSTEMTIME, GDT_NONE, 0);
}

void CTDLTaskAttributeListCtrl::PrepareTimeCombo(int nRow)
{
	CString sValue = GetItemText(nRow, VALUE_COL);

	if (sValue.IsEmpty())
		m_cbTimeOfDay.Set24HourTime(-1);
	else
		m_cbTimeOfDay.Set24HourTime(CTimeHelper::DecodeClockTime(sValue));

	DWORD dwStyle = m_cbTimeOfDay.GetStyle();
	Misc::SetFlag(dwStyle, TCB_ISO, m_data.HasStyle(TDCS_SHOWDATESINISO));

	m_cbTimeOfDay.SetStyle(dwStyle);
}

void CTDLTaskAttributeListCtrl::PrepareTimePeriodEdit(int nRow)
{
	CString sValue = GetItemText(nRow, VALUE_COL);
	TH_UNITS nUnits = THU_NULL;
	double dValue = 0.0;
		
	if (CTimeHelper::DecodeOffset(sValue, dValue, nUnits, FALSE))
		m_eTimePeriod.SetTime(dValue, nUnits);

	m_eTimePeriod.SetBorderWidth(0);
	m_eTimePeriod.SetDefaultButton(0);
}

CString CTDLTaskAttributeListCtrl::GetValueText(TDC_ATTRIBUTE nAttribID) const 
{ 
	return GetItemText(FindItemFromData(nAttribID), VALUE_COL); 
}

CWnd* CTDLTaskAttributeListCtrl::GetEditControl(int nRow, BOOL bBtnClick)
{
	// Sanity check
	if ((nRow < 0) || (nRow > GetItemCount()))
		return NULL;

	TDC_ATTRIBUTE nAttribID = GetAttributeID(nRow);

	switch (nAttribID)
	{
	case TDCA_ALLOCBY:
	case TDCA_STATUS:
	case TDCA_VERSION:
		return &m_cbSingleSelection;

	case TDCA_ALLOCTO:
	case TDCA_CATEGORY:
	case TDCA_TAGS:
		return &m_cbMultiSelection;

	case TDCA_FILELINK:
		return &m_cbFileLinks;

	case TDCA_FLAG:
	case TDCA_ICON:
	case TDCA_LOCK:
	case TDCA_COLOR:
	case TDCA_RECURRENCE:
		// Not required
		return NULL;

	case TDCA_PERCENT:
	case TDCA_EXTERNALID:
	case TDCA_COST:
	case TDCA_TASKNAME:
		// Use base class edit control
		break;

	case TDCA_PRIORITY:
		return &m_cbPriority;

	case TDCA_RISK:
		return &m_cbRisk;

	case TDCA_TIMEESTIMATE:
	case TDCA_TIMESPENT:
		return &m_eTimePeriod;

	case TDCA_DONEDATE:
	case TDCA_DUEDATE:
	case TDCA_STARTDATE:
		return &m_datePicker;

	case TDCA_DONETIME:
	case TDCA_DUETIME:
	case TDCA_STARTTIME:
		return &m_cbTimeOfDay;

	case TDCA_DEPENDENCY:
		return (bBtnClick ? NULL : &m_eDepends);

	default:
		if (TDCCUSTOMATTRIBUTEDEFINITION::IsCustomAttribute(nAttribID))
		{
			int nCust = m_aCustomAttribDefs.Find(nAttribID);
			ASSERT(nCust != -1);

			if (nCust != -1)
			{
				CString sAttribID = m_aCustomAttribDefs[nCust].sUniqueID;
				TDCCADATA data;

				if (m_taskCtrl.GetSelectedTaskCustomAttributeData(sAttribID, data))
				{
					if (m_aCustomAttribDefs[nCust].IsList())
					{
						// TODO
					}
					else
					{
						switch (m_aCustomAttribDefs[nCust].GetDataType())
						{
						case TDCCA_STRING:
						case TDCCA_FRACTION:
						case TDCCA_INTEGER:
						case TDCCA_DOUBLE:
						case TDCCA_ICON:
						case TDCCA_FILELINK:
							// Use base class edit control
							break;

						case TDCCA_CALCULATION:
							// Not editable
							return NULL; 

						case TDCCA_TIMEPERIOD:
							return &m_eTimePeriod;

						case TDCCA_DATE:
							return &m_datePicker;
							break;

						case TDCCA_BOOL:
							// Not required
							return NULL;
						}
					}
				}
			}
		}
		else if (nAttribID > CUSTOMTIMEATTRIBOFFSET)
		{
			nAttribID = (TDC_ATTRIBUTE)(nAttribID - CUSTOMTIMEATTRIBOFFSET);

			CString sAttribID = m_aCustomAttribDefs.GetAttributeTypeID(nAttribID);
			TDCCADATA data;

			if (m_taskCtrl.GetSelectedTaskCustomAttributeData(sAttribID, data))
			{
				ASSERT(m_aCustomAttribDefs.GetAttributeDataType(sAttribID) == TDCCA_DATE);

				// TODO
			}
		}
		break;
	}

	// all else
	return CInputListCtrl::GetEditControl();
}

void CTDLTaskAttributeListCtrl::EditCell(int nRow, int nCol, BOOL bBtnClick)
{
	if (!CanEditCell(nRow, nCol))
		return;

	CWnd* pCtrl = GetEditControl(nRow, bBtnClick);
	TDC_ATTRIBUTE nAttribID = GetAttributeID(nRow);

	if (pCtrl == CInputListCtrl::GetEditControl())
	{
		PrepareControl(m_editBox, nRow, nCol);
		CInputListCtrl::EditCell(nRow, nCol, bBtnClick);

		return;
	}
	else if (pCtrl == &m_eTimePeriod)
	{
		ShowControl(*pCtrl, nRow, nCol, bBtnClick);

		if (bBtnClick)
		{
			m_eTimePeriod.ShowUnitsPopupMenu();
			HideControl(*pCtrl);
		}

		return;
	}
	else if (pCtrl != NULL)
	{
		ShowControl(*pCtrl, nRow, nCol, bBtnClick);
		return;
	}

	// All other attributes not handled by the base class
	CString sValue = GetItemText(nRow, VALUE_COL);

	switch (nAttribID)
	{
	case TDCA_FLAG:
	case TDCA_LOCK:
		SetItemText(nRow, VALUE_COL, (sValue.IsEmpty() ? _T("+") : _T(""))); // Toggle checkbox
		NotifyParentEdit(nAttribID);
		break;

	case TDCA_ICON:
	case TDCA_COLOR:
	case TDCA_RECURRENCE:
		if (GetParent()->SendMessage(WM_TDCM_EDITTASKATTRIBUTE, nAttribID))
			RefreshSelectedTaskValue(nRow);
		break;

	case TDCA_DEPENDENCY:
		if (bBtnClick)
		{
			if (GetParent()->SendMessage(WM_TDCM_EDITTASKATTRIBUTE, nAttribID))
				RefreshSelectedTaskValue(nRow);
		}
		else
		{
			CInputListCtrl::EditCell(nRow, nCol, bBtnClick);
		}
		break;

	case TDCA_TASKNAME:
		// TODO
		break;

	default:
		if (TDCCUSTOMATTRIBUTEDEFINITION::IsCustomAttribute(nAttribID))
		{
			int nCust = m_aCustomAttribDefs.Find(nAttribID);
			ASSERT(nCust != -1);

			if (nCust != -1)
			{
				CString sAttribID = m_aCustomAttribDefs[nCust].sUniqueID;
				TDCCADATA data;

				if (m_taskCtrl.GetSelectedTaskCustomAttributeData(sAttribID, data))
				{
					if (m_aCustomAttribDefs[nCust].IsList())
					{
						// TODO
					}
					else
					{
						switch (m_aCustomAttribDefs[nCust].GetDataType())
						{
						case TDCCA_STRING:
						case TDCCA_FRACTION:
						case TDCCA_INTEGER:
						case TDCCA_DOUBLE:
						case TDCCA_ICON:
						case TDCCA_FILELINK:
							// TODO
							break;

						case TDCCA_CALCULATION:
							// TODO
							break;

						case TDCCA_TIMEPERIOD:
							// TODO
							break;

						case TDCCA_DATE:
							// TODO
							break;

						case TDCCA_BOOL:
							// TODO
							break;
						}
					}
				}
			}
		}
		else if (nAttribID > CUSTOMTIMEATTRIBOFFSET)
		{
			nAttribID = (TDC_ATTRIBUTE)(nAttribID - CUSTOMTIMEATTRIBOFFSET);

			CString sAttribID = m_aCustomAttribDefs.GetAttributeTypeID(nAttribID);
			TDCCADATA data;

			if (m_taskCtrl.GetSelectedTaskCustomAttributeData(sAttribID, data))
			{
				ASSERT(m_aCustomAttribDefs.GetAttributeDataType(sAttribID) == TDCCA_DATE);

				// TODO
			}
		}
		break;
	}
}

void CTDLTaskAttributeListCtrl::HideAllControls(const CWnd* pWndIgnore)
{
	CInputListCtrl::HideAllControls(pWndIgnore);

	HideControl(m_datePicker, pWndIgnore);
	HideControl(m_cbMultiSelection, pWndIgnore);
	HideControl(m_cbSingleSelection, pWndIgnore);
	HideControl(m_cbTimeOfDay, pWndIgnore);
	HideControl(m_cbPriority, pWndIgnore);
	HideControl(m_cbRisk, pWndIgnore);
	HideControl(m_eDepends, pWndIgnore);
	HideControl(m_eTimePeriod, pWndIgnore);
	HideControl(m_cbFileLinks, pWndIgnore);

	m_eTimePeriod.DeleteButton(ID_BTN_ADDLOGGEDTIME);
	m_eTimePeriod.DeleteButton(ID_BTN_TIMETRACK);
	
	if (pWndIgnore != &m_editBox)
		m_editBox.SetSpinBuddy(NULL);
}

void CTDLTaskAttributeListCtrl::OnComboCloseUp(UINT nCtrlID) 
{ 
	SetFocus();
	HideControl(*GetDlgItem(nCtrlID));
}

void CTDLTaskAttributeListCtrl::OnComboEditCancel(UINT nCtrlID)
{
	int nRow = GetCurSel();
	TDC_ATTRIBUTE nAttribID = GetAttributeID(nRow);

	CWnd* pCombo = GetDlgItem(nCtrlID);
	HideControl(*pCombo);
}

void CTDLTaskAttributeListCtrl::OnComboEditChange(UINT nCtrlID)
{
	HideControl(*GetDlgItem(nCtrlID));

	int nRow = GetCurSel();
	TDC_ATTRIBUTE nAttribID = GetAttributeID(nRow);

	CString sNewItemText;

	switch (nCtrlID)
	{
	case IDC_SINGLESEL_COMBO:
		sNewItemText = CDialogHelper::GetSelectedItem(m_cbSingleSelection);
		break;

	case IDC_MULTISEL_COMBO:
		{
			CStringArray aMatched, aMixed;
			m_cbMultiSelection.GetChecked(aMatched, aMixed);

			sNewItemText = FormatMultiSelItems(aMatched, aMixed);
		}
		break;

	case IDC_TIME_PICKER:
		sNewItemText = CTimeHelper::FormatClockTime(m_cbTimeOfDay.Get24HourTime() / 24);
		break;

	case IDC_PRIORITY_COMBO:
		sNewItemText = Misc::Format(m_cbPriority.GetSelectedPriority());
		break;

	case IDC_RISK_COMBO:
		sNewItemText = Misc::Format(m_cbRisk.GetSelectedRisk());
		break;

	case IDC_FILELINK_COMBO:
		{
			CStringArray aFiles;
			
			if (m_cbFileLinks.GetFileList(aFiles))
				sNewItemText = Misc::FormatArray(aFiles);
		}
		break;

	default:
		ASSERT(0);
		return;
	}

	if (sNewItemText != GetItemText(nRow, VALUE_COL))
	{
		SetItemText(nRow, VALUE_COL, sNewItemText);
		NotifyParentEdit(nAttribID);
	}
}

LRESULT CTDLTaskAttributeListCtrl::NotifyParentEdit(TDC_ATTRIBUTE nAttribID)
{
	UpdateWindow();

	return GetParent()->SendMessage(WM_TDCN_ATTRIBUTEEDITED, nAttribID);
}

void CTDLTaskAttributeListCtrl::OnDependsChange()
{
	// Received after a manual edit of the task IDs
	int nRow = GetCurSel();
	ASSERT(GetAttributeID(nRow) == TDCA_DEPENDENCY);

	HideControl(m_eDepends);
	SetItemText(nRow, VALUE_COL, m_eDepends.FormatDependencies());
	NotifyParentEdit(TDCA_DEPENDENCY);
}

void CTDLTaskAttributeListCtrl::OnTimePeriodChange()
{
	// Received after a manual edit of the task IDs
	int nRow = GetCurSel();
	TDC_ATTRIBUTE nAttribID = GetAttributeID(nRow);

	TDCTIMEPERIOD tp(m_eTimePeriod.GetTime(), m_eTimePeriod.GetUnits());

	HideControl(m_eTimePeriod);
	SetItemText(nRow, VALUE_COL, tp.Format(2));
	NotifyParentEdit(nAttribID);
}

void CTDLTaskAttributeListCtrl::OnCancelEdit()
{
	int nRow = GetCurSel();
	TDC_ATTRIBUTE nAttribID = GetAttributeID(nRow);

	// Revert changes before default handling
	switch (nAttribID)
	{
	case TDCA_DEPENDENCY:
		m_eDepends.SetWindowText(m_eDepends.FormatDependencies());
		break;

	case TDCA_TIMEESTIMATE:
	case TDCA_TIMESPENT:
		PrepareTimePeriodEdit(nRow);
		break;
	}

	CInputListCtrl::OnCancelEdit();
}

void CTDLTaskAttributeListCtrl::OnDateCloseUp(NMHDR* pNMHDR, LRESULT* pResult) 
{ 
	UNREFERENCED_PARAMETER(pNMHDR);
	ASSERT(pNMHDR->idFrom == IDC_DATE_PICKER);

	HideControl(m_datePicker); 
	*pResult = 0;
}

void CTDLTaskAttributeListCtrl::OnDateChange(NMHDR* pNMHDR, LRESULT* pResult)
{
	UNREFERENCED_PARAMETER(pNMHDR);
	ASSERT(pNMHDR->idFrom == IDC_DATE_PICKER);

	// Only handle this if the calendar is closed
	if (!m_datePicker.IsCalendarVisible())
	{
		// Note: Don't hide the date picker because the user 
		// may be editing the date components manually

		int nRow = GetCurSel();
		TDC_ATTRIBUTE nAttribID = GetAttributeID(nRow);

		CString sNewItemText;
		COleDateTime date;

		if (m_datePicker.GetTime(date))
			sNewItemText = m_formatter.GetDateOnly(date, TRUE);
		
		if (sNewItemText != GetItemText(nRow, VALUE_COL))
		{
			SetItemText(nRow, VALUE_COL, sNewItemText);
			NotifyParentEdit(nAttribID);
		}
	}

	*pResult = 0;
}

LRESULT CTDLTaskAttributeListCtrl::OnAutoComboAddDelete(WPARAM wp, LPARAM lp)
{
	int nRow = GetCurSel();
	TDC_ATTRIBUTE nAttribID = GetAttributeID(nRow);

	switch (nAttribID)
	{
	case TDCA_ALLOCBY:	CDialogHelper::GetComboBoxItems(m_cbSingleSelection, m_tldAll.aAllocBy);break;
	case TDCA_STATUS:	CDialogHelper::GetComboBoxItems(m_cbSingleSelection, m_tldAll.aStatus);	break;
	case TDCA_VERSION:	CDialogHelper::GetComboBoxItems(m_cbSingleSelection, m_tldAll.aVersion);break;

	case TDCA_ALLOCTO:	CDialogHelper::GetComboBoxItems(m_cbMultiSelection, m_tldAll.aAllocTo);	break;
	case TDCA_CATEGORY: CDialogHelper::GetComboBoxItems(m_cbMultiSelection, m_tldAll.aCategory);break;
	case TDCA_TAGS:		CDialogHelper::GetComboBoxItems(m_cbMultiSelection, m_tldAll.aTags);	break;

	case TDCA_FILELINK:
		return 0L;
	}

	return GetParent()->SendMessage(WM_TDCN_AUTOITEMADDEDDELETED, nAttribID);
}

BOOL CTDLTaskAttributeListCtrl::DeleteSelectedCell()
{
	if (m_nCurCol == VALUE_COL)
	{
		TDC_ATTRIBUTE nAttribID = GetAttributeID(GetCurSel());
		
		if (GetParent()->SendMessage(WM_TDCM_CLEARTASKATTRIBUTE, nAttribID))
		{
			SetItemText(GetCurSel(), m_nCurCol, _T(""));
			return TRUE;
		}
	}

	// else
	return FALSE;
}
