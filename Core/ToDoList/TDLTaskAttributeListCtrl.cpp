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
#include "..\shared\HoldRedraw.h"
#include "..\shared\Localizer.h"

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
	IDC_DATE_CTRL,
};

/////////////////////////////////////////////////////////////////////////////

const int CUSTOMTIMEATTRIBOFFSET = (TDCA_LAST_ATTRIBUTE + 1);

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
	m_cbMultiSelection(ACBS_ALLOWDELETE | ACBS_AUTOCOMPLETE)
{
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

	ON_NOTIFY_RANGE(DTN_CLOSEUP, 0, 0xffff, OnDateCloseUp)
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

	// Add attributes
	Populate();

	// Create edit fields
	CreateControl(m_cbSingleSelection, IDC_SINGLESEL_COMBO, (CBS_DROPDOWN | CBS_SORT));
	CreateControl(m_cbMultiSelection, IDC_MULTISEL_COMBO, (CBS_DROPDOWN | CBS_SORT));
	CreateControl(m_dtc, IDC_DATE_CTRL, DTS_SHOWNONE);
	
	CLocalizer::EnableTranslation(m_cbSingleSelection, FALSE);
	CLocalizer::EnableTranslation(m_cbMultiSelection, FALSE);

	return 0;
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
		// Text fields
	case TDCA_TASKNAME:
	case TDCA_COST:
	case TDCA_EXTERNALID:
	case TDCA_PERCENT:
	case TDCA_TIMEESTIMATE:
	case TDCA_TIMESPENT:
		return ILCT_TEXT;

		// Date fields
	case TDCA_DONEDATE:
	case TDCA_DUEDATE:
	case TDCA_STARTDATE:
		return ILCT_DATE;

		// Drop lists
	case TDCA_PRIORITY:
	case TDCA_COLOR:
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

		// Browse-like fields
	case TDCA_ICON:
	case TDCA_RECURRENCE:
	case TDCA_DEPENDENCY:
		return ILCT_BROWSE;

		// Checkbox fields
	case TDCA_FLAG:
	case TDCA_LOCK:
		return ILCT_CHECK;

		// Read-only fields
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
	if ((nCol == ATTRIB_COL) || m_data.HasStyle(TDCS_READONLY))
		return FALSE;

	// else
	TDC_ATTRIBUTE nAttribID = GetAttributeID(nRow);

	switch (nAttribID)
	{
		// Read-only fields
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
			return FALSE;

		case TDCA_LOCK:
			return TRUE;

		case TDCA_STARTTIME:
			return !GetValueText(TDCA_STARTDATE).IsEmpty();

		case TDCA_DUETIME:
			return !GetValueText(TDCA_DUEDATE).IsEmpty();

		case TDCA_DONETIME:
			return !GetValueText(TDCA_DONEDATE).IsEmpty();
	}

	if (m_taskCtrl.SelectionHasLocked())
		return FALSE;

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

void CTDLTaskAttributeListCtrl::SetAutoListData(const TDCAUTOLISTDATA& tldAll)
{
	m_tldAll.Copy(tldAll, TDCA_ALL);
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
	case TDCA_EXTERNALID:	sValue = m_taskCtrl.GetSelectedTaskExtID(); break;
	case TDCA_ALLOCBY:		sValue = m_taskCtrl.GetSelectedTaskAllocBy(); break;
	case TDCA_STATUS:		sValue = m_taskCtrl.GetSelectedTaskStatus(); break;
	case TDCA_VERSION:		sValue = m_taskCtrl.GetSelectedTaskVersion(); break;
	case TDCA_ICON:			sValue = m_taskCtrl.GetSelectedTaskIcon(); break;

	case TDCA_FLAG:			sValue = m_taskCtrl.IsSelectedTaskFlagged() ? _T("+") : _T(""); break;
	case TDCA_LOCK:			sValue = m_taskCtrl.IsSelectedTaskLocked() ? _T("+") : _T(""); break;

	case TDCA_ALLOCTO:		m_taskCtrl.GetSelectedTaskAllocTo(aMatched, aMixed); break;
	case TDCA_CATEGORY:		m_taskCtrl.GetSelectedTaskCategories(aMatched, aMixed); break;
	case TDCA_TAGS:			m_taskCtrl.GetSelectedTaskTags(aMatched, aMixed); break;
	case TDCA_FILELINK:		m_taskCtrl.GetSelectedTaskFileLinks(aMatched, FALSE); break;

	case TDCA_COST:
		{
			TDCCOST cost;
			if (m_taskCtrl.GetSelectedTaskCost(cost))
				sValue = cost.Format(2);
		}
		break;

	case TDCA_PERCENT:
		{
			int nValue = m_taskCtrl.GetSelectedTaskPercent();

			if (nValue != -1)
				sValue = Misc::Format(nValue);
		}
		break;

	case TDCA_PRIORITY:
		{
			int nPriority = m_taskCtrl.GetSelectedTaskPriority();

			if (nPriority >= 0)
				sValue = Misc::Format(nPriority);
		}
		break;

	case TDCA_RISK:
		{
			int nRisk = m_taskCtrl.GetSelectedTaskRisk();

			if (nRisk >= 0)
				sValue = Misc::Format(nRisk);
		}
		break;

	case TDCA_TIMEESTIMATE:
		{
			TDCTIMEPERIOD tp;
			if (m_taskCtrl.GetSelectedTaskTimeEstimate(tp))
				sValue = tp.Format(2);
		}
		break;

	case TDCA_TIMESPENT:
		{
			TDCTIMEPERIOD tp;
			if (m_taskCtrl.GetSelectedTaskTimeSpent(tp))
				sValue = tp.Format(2);
		}
		break;

	case TDCA_DONEDATE:
	case TDCA_DUEDATE:
	case TDCA_STARTDATE:
		{
			COleDateTime date = m_taskCtrl.GetSelectedTaskDate(TDC::MapAttributeToDate(nAttribID));
			sValue = CDateHelper::FormatDate(date);
		}
		break;

	case TDCA_LASTMODDATE:
	case TDCA_CREATIONDATE:
		{
			COleDateTime date = m_taskCtrl.GetSelectedTaskDate(TDC::MapAttributeToDate(nAttribID));
			sValue = CDateHelper::FormatDate(date, DHFD_TIME);
		}
		break;

	case TDCA_DONETIME:
	case TDCA_DUETIME:
	case TDCA_STARTTIME:
		{
			COleDateTime date = m_taskCtrl.GetSelectedTaskDate(TDC::MapAttributeToDate(nAttribID));

			if (CDateHelper::DateHasTime(date))
				sValue = CTimeHelper::FormatClockTime(date);
		}
		break;

	case TDCA_COLOR:
		{
			COLORREF color = m_taskCtrl.GetSelectedTaskColor();

			if (color != CLR_NONE)
				sValue = Misc::Format(color);
		}
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
			if (!sText.IsEmpty())
			{
				int nPriority = m_taskCtrl.GetSelectedTaskPriority();
				ASSERT(nPriority >= 0);

				CRect rBox(rText);
				rBox.DeflateRect(0, 1, 0, 2);
				rBox.right = rBox.left + rBox.Height();

				COLORREF crFill = m_taskCtrl.GetPriorityColor(nPriority);
				COLORREF crBorder = GraphicsMisc::Darker(crFill, 0.5);

				GraphicsMisc::DrawRect(pDC, rBox, crFill, crBorder);

				CRect rLeft(rText);
				rLeft.left += rText.Height();

				// TODO - Get text from Priority combo
				CInputListCtrl::DrawCellText(pDC, nRow, nCol, rLeft, sText, crText, nDrawTextFlags);
				return;
			}
			break;

		case TDCA_ICON:
			if (!sText.IsEmpty())
			{
				m_ilIcons.Draw(pDC, sText, rText.TopLeft(), ILD_TRANSPARENT);
				return;
			}
			break;

		case TDCA_COLOR:
			if (!sText.IsEmpty())
			{
				crText = _ttoi(sText);

				if (m_data.HasStyle(TDCS_TASKCOLORISBACKGROUND))
				{
					pDC->FillSolidRect(rText, crText);
					crText = GraphicsMisc::GetBestTextColor(crText);
				}

				CInputListCtrl::DrawCellText(pDC, nRow, nCol, rText, CEnString(IDS_COLOR_SAMPLETEXT), crText, nDrawTextFlags);
				return;
			}
			break;

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

		default:
			if (TDCCUSTOMATTRIBUTEDEFINITION::IsCustomAttribute(nAttribID) &&
				m_aCustomAttribDefs.GetAttributeDataType(nAttribID) == TDCCA_ICON)
			{
				CStringArray aIcons;
				int nNumIcons = Misc::Split(sText, aIcons);

				CRect rIcon(rText);

				for (int nIcon = 0; nIcon < nNumIcons; nIcon++)
				{
					m_ilIcons.Draw(pDC, aIcons[nIcon], rIcon.TopLeft(), ILD_TRANSPARENT);
					rIcon.left += GraphicsMisc::ScaleByDPIFactor(16);
				}
				return;
			}
			break;
		}
	}

	CInputListCtrl::DrawCellText(pDC, nRow, nCol, rText, sText, crText, nDrawTextFlags);
}

void CTDLTaskAttributeListCtrl::OnTextEditOK(NMHDR* pNMHDR, LRESULT* pResult)
{
	LV_DISPINFO* pDispInfo = (LV_DISPINFO*)pNMHDR;

	if ((pDispInfo->item.iSubItem == VALUE_COL) &&
		(pDispInfo->item.iItem >= 0))
	{
		TDC_ATTRIBUTE nAttribID = GetAttributeID(pDispInfo->item.iItem, TRUE);
		GetParent()->SendMessage(WM_TDCN_ATTRIBUTEEDIT, nAttribID, 0);
	}

	*pResult = 0;
}

COLORREF CTDLTaskAttributeListCtrl::GetColor() const
{
	CString sValue = GetValueText(TDCA_COLOR);

	return (sValue.IsEmpty() ? CLR_NONE : _ttoi(sValue));
}

CString CTDLTaskAttributeListCtrl::GetIcon() const
{
	return GetValueText(TDCA_ICON);
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
	return aDepends.Parse(GetValueText(TDCA_DEPENDENCY));
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
	return _ttoi(GetValueText(TDCA_COLOR));
}

int CTDLTaskAttributeListCtrl::GetRisk() const
{
	return _ttoi(GetValueText(TDCA_COLOR));
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

BOOL CTDLTaskAttributeListCtrl::GetRecurrence(TDCRECURRENCE& tr) const
{
	CString sValue = GetValueText(TDCA_RECURRENCE);

	return (sValue.IsEmpty() ? CLR_NONE : _ttoi(sValue));
}

CString CTDLTaskAttributeListCtrl::GetVersion() const
{
	return GetValueText(TDCA_VERSION);
}

COleDateTime CTDLTaskAttributeListCtrl::GetStartDate() const
{
	return GetDate(TDCA_STARTDATE, TDCA_STARTTIME);
}

COleDateTime CTDLTaskAttributeListCtrl::GetDueDate() const
{
	return GetDate(TDCA_DUEDATE, TDCA_DUETIME);
}

COleDateTime CTDLTaskAttributeListCtrl::GetDoneDate() const
{
	return GetDate(TDCA_DONEDATE, TDCA_DONETIME);
}

COleDateTime CTDLTaskAttributeListCtrl::GetDate(TDC_ATTRIBUTE nDate, TDC_ATTRIBUTE nTime) const
{
	CString sDate = GetValueText(nDate);
	CString sTime = GetValueText(nTime);

	COleDateTime date;
	CDateHelper::DecodeDate(sDate + ' ' + sTime, date, !sTime.IsEmpty());

	return date;
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
		break;

	case TDCA_ICON: 
	case TDCA_FLAG: 
	case TDCA_LOCK: 
	case TDCA_COLOR:
	case TDCA_DEPENDENCY:
	case TDCA_RECURRENCE:
	case TDCA_TASKNAME:
	case TDCA_EXTERNALID: 
		// Nothing to do
		break;

	case TDCA_COST:
		m_editBox.SetMask(_T("-0123456789.@"), ME_LOCALIZEDECIMAL);
		break;

	case TDCA_PERCENT:
		m_editBox.SetMask(_T("-0123456789"));
		break;

	case TDCA_PRIORITY:
		break;

	case TDCA_RISK:
		break;

	case TDCA_TIMEESTIMATE:
		break;

	case TDCA_TIMESPENT:
		break;

	case TDCA_DONEDATE:
	case TDCA_DUEDATE:
	case TDCA_STARTDATE:
		break;

	case TDCA_DONETIME:
	case TDCA_DUETIME:
	case TDCA_STARTTIME:
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

CString CTDLTaskAttributeListCtrl::GetValueText(TDC_ATTRIBUTE nAttribID) const 
{ 
	return GetItemText(FindItemFromData(nAttribID), VALUE_COL); 
}


CWnd* CTDLTaskAttributeListCtrl::GetEditControl(int nRow)
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
		break;

	case TDCA_FLAG:
	case TDCA_ICON:
	case TDCA_LOCK:
		// Not required
		return NULL;

	case TDCA_EXTERNALID:
	case TDCA_COST:
	case TDCA_PERCENT:
	case TDCA_TASKNAME:
		// Use base class edit control
		break;

	case TDCA_PRIORITY:
		break;

	case TDCA_RISK:
		break;

	case TDCA_TIMEESTIMATE:
		break;

	case TDCA_TIMESPENT:
		break;

	case TDCA_DONEDATE:
	case TDCA_DUEDATE:
	case TDCA_STARTDATE:
		return &m_dtc;

	case TDCA_DONETIME:
	case TDCA_DUETIME:
	case TDCA_STARTTIME:
		break;

	case TDCA_COLOR:
		break;

	case TDCA_RECURRENCE:
		break;

	case TDCA_DEPENDENCY:
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

	// all else
	return CInputListCtrl::GetEditControl();
}

void CTDLTaskAttributeListCtrl::EditCell(int nRow, int nCol, BOOL bBtnClick)
{
	if (!CanEditCell(nRow, nCol))
		return;

	CWnd* pCtrl = GetEditControl(nRow);

	if (pCtrl == CInputListCtrl::GetEditControl())
	{
		CInputListCtrl::EditCell(nRow, nCol, bBtnClick);
		return;
	}
	else if (pCtrl != NULL)
	{
		ShowControl(*pCtrl, nRow, nCol, bBtnClick);
		return;
	}

	// All other attributes not handled by the base class
	TDC_ATTRIBUTE nAttribID = GetAttributeID(nRow);

	switch (nAttribID)
	{
	case TDCA_FLAG:
	case TDCA_LOCK:
		// Toggle checkbox
		SetItemText(nRow, VALUE_COL, GetItemText(nRow, VALUE_COL).IsEmpty() ? _T("+") : _T(""));
		GetParent()->SendMessage(WM_TDCN_ATTRIBUTEEDIT, nAttribID, 0);
		break;

	case TDCA_ICON:
		break;

	case TDCA_COLOR:
		break;

	case TDCA_RECURRENCE:
		break;

	case TDCA_DEPENDENCY:
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
	HideControl(m_dtc, pWndIgnore);
	HideControl(m_cbMultiSelection, pWndIgnore);
	HideControl(m_cbSingleSelection, pWndIgnore);

	CInputListCtrl::HideAllControls(pWndIgnore);
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

	// Revert any combo changes
	PrepareControl(*pCombo, nRow, VALUE_COL);
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

	default:
		ASSERT(0);
		return;
	}

	if (sNewItemText != GetItemText(nRow, VALUE_COL))
	{
		SetItemText(nRow, VALUE_COL, sNewItemText);
		GetParent()->SendMessage(WM_TDCN_ATTRIBUTEEDIT, nAttribID, 0);
	}
}

void CTDLTaskAttributeListCtrl::OnDateCloseUp(UINT /*nCtrlID*/, NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/) 
{ 
	HideControl(m_dtc); 
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
