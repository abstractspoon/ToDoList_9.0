// BurndownChart.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "BurndownChart.h"
#include "BurndownStatic.h"
#include "BurndownGraphs.h"

#include "..\shared\datehelper.h"
#include "..\shared\holdredraw.h"
#include "..\shared\enstring.h"
#include "..\shared\graphicsmisc.h"

/////////////////////////////////////////////////////////////////////////////

const int MIN_XSCALE_SPACING = 50; // pixels

static BURNDOWN_CHARTSCALE SCALES[] = 
{
	BCS_DAY,		
	BCS_WEEK,	
	BCS_MONTH,	
	BCS_2MONTH,	
	BCS_QUARTER,	
	BCS_HALFYEAR,
	BCS_YEAR,	
};
static int NUM_SCALES = sizeof(SCALES) / sizeof(int);

////////////////////////////////////////////////////////////////////////////////
// CBurndownChart

CBurndownChart::CBurndownChart(const CStatsItemArray& data) 
	: 
	m_data(data),
	m_nScale(BCS_DAY),
	m_nChartType(BCT_INCOMPLETETASKS)
{
}

CBurndownChart::~CBurndownChart()
{
}

////////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CBurndownChart, CHMXChartEx)
	ON_WM_SIZE()
END_MESSAGE_MAP()

////////////////////////////////////////////////////////////////////////////////
// CBurndownChart message handlers

BOOL CBurndownChart::SetChartType(BURNDOWN_CHARTTYPE nType)
{
	if (nType != m_nChartType)
	{
		m_nChartType = nType;
		RebuildGraph(FALSE);

		return TRUE;
	}

	return FALSE;
}

BOOL CBurndownChart::SaveToImage(CBitmap& bmImage)
{
	CClientDC dc(this);
	CDC dcImage;

	if (dcImage.CreateCompatibleDC(&dc))
	{
		CRect rClient;
		GetClientRect(rClient);

		if (bmImage.CreateCompatibleBitmap(&dc, rClient.Width(), rClient.Height()))
		{
			CBitmap* pOldImage = dcImage.SelectObject(&bmImage);

			SendMessage(WM_PRINTCLIENT, (WPARAM)dcImage.GetSafeHdc(), PRF_ERASEBKGND);
			Invalidate(TRUE);

			dcImage.SelectObject(pOldImage);
		}
	}

	return (bmImage.GetSafeHandle() != NULL);
}

BURNDOWN_CHARTSCALE CBurndownChart::CalculateRequiredXScale() const
{
	// calculate new x scale
	int nDataWidth = GetDataArea().cx;
	int nNumDays = m_dtExtents.GetDayCount();

	// work thru the available scales until we find a suitable one
	for (int nScale = 0; nScale < NUM_SCALES; nScale++)
	{
		int nSpacing = MulDiv(SCALES[nScale], nDataWidth, nNumDays);

		if (nSpacing > MIN_XSCALE_SPACING)
			return SCALES[nScale];
	}

	return BCS_YEAR;
}

void CBurndownChart::RebuildXScale()
{
	ClearXScaleLabels();

	// calc new scale
	m_nScale = CalculateRequiredXScale();
	SetXLabelStep(m_nScale);

	// Get new start and end dates
	COleDateTime dtStart = GetGraphStartDate();
	COleDateTime dtEnd = GetGraphEndDate();

	// build ticks
	CDateHelper dh;
	int nNumDays = ((int)dtEnd.m_dt - (int)dtStart.m_dt);
	COleDateTime dtTick = dtStart;
	CString sTick;
	
	for (int nDay = 0; nDay <= nNumDays; nDay += m_nScale)
	{
		sTick = CDateHelper::FormatDate(dtTick);
		SetXScaleLabel(nDay, sTick);

		// next Tick date
		switch (m_nScale)
		{
		case BCS_DAY:
			dtTick.m_dt += 1.0;
			break;
			
		case BCS_WEEK:
			dh.OffsetDate(dtTick, 1, DHU_WEEKS);
			break;
			
		case BCS_MONTH:
			dh.OffsetDate(dtTick, 1, DHU_MONTHS);
			break;
			
		case BCS_2MONTH:
			dh.OffsetDate(dtTick, 2, DHU_MONTHS);
			break;
			
		case BCS_QUARTER:
			dh.OffsetDate(dtTick, 3, DHU_MONTHS);
			break;
			
		case BCS_HALFYEAR:
			dh.OffsetDate(dtTick, 6, DHU_MONTHS);
			break;
			
		case BCS_YEAR:
			dh.OffsetDate(dtTick, 1, DHU_YEARS);
			break;
			
		default:
			ASSERT(0);
		}
	}
}

COleDateTime CBurndownChart::GetGraphStartDate() const
{
	switch (m_nChartType)
	{
	case BCT_INCOMPLETETASKS:
		return CIncompleteDaysGraph::GetGraphStartDate(m_dtExtents, m_nScale);

	case BCT_REMAININGDAYS:
		return CRemainingDaysGraph::GetGraphStartDate(m_dtExtents, m_nScale);
	}

	ASSERT(0);
	return m_dtExtents.GetStart();
}

COleDateTime CBurndownChart::GetGraphEndDate() const
{
	switch (m_nChartType)
	{
	case BCT_INCOMPLETETASKS:
		return CIncompleteDaysGraph::GetGraphEndDate(m_dtExtents, m_nScale);

	case BCT_REMAININGDAYS:
		return CRemainingDaysGraph::GetGraphEndDate(m_dtExtents, m_nScale);
	}

	ASSERT(0);
	return m_dtExtents.GetStart();
}

void CBurndownChart::OnSize(UINT nType, int cx, int cy) 
{
	CHMXChartEx::OnSize(nType, cx, cy);
	
	int nOldScale = m_nScale;
	RebuildXScale();
		
	// handle scale change
	if (m_nScale != nOldScale)
		RebuildGraph(FALSE);
}

void CBurndownChart::RebuildGraph(BOOL bUpdateExtents)
{
	CWaitCursor cursor;
	CHoldRedraw hr(*this);
	
	if (bUpdateExtents || (m_dtExtents.GetDayCount() == 0))
		m_data.GetDataExtents(m_dtExtents);
	
	ClearData();
	SetYText(CEnString(STATSDISPLAY[m_nChartType].nYAxisID));
	
	if (!m_data.IsEmpty())
		RebuildXScale();
	
	switch (STATSDISPLAY[m_nChartType].nDisplay)
	{
	case BCT_INCOMPLETETASKS:
		CIncompleteDaysGraph::BuildGraph(m_dtExtents, m_nScale, m_data, m_dataset);
		break;
		
	case BCT_REMAININGDAYS:
		CRemainingDaysGraph::BuildGraph(m_dtExtents, m_nScale, m_data, m_dataset);
		break;
	}

	CalcDatas();
}

void CBurndownChart::PreSubclassWindow()
{
	SetBkGnd(GetSysColor(COLOR_WINDOW));
	SetXLabelsAreTicks(true);
	SetXLabelAngle(45);
	SetYTicks(10);

	VERIFY(InitTooltip(TRUE));
}

int CBurndownChart::OnToolHitTest(CPoint point, TOOLINFO* pTI) const
{
	CString sTooltip;

	switch (m_nChartType)
	{
		case BCT_INCOMPLETETASKS:
			sTooltip = CIncompleteDaysGraph::GetTooltip(m_dataset, m_dtExtents, m_nScale, HitTest(point));
			break;

		case BCT_REMAININGDAYS:
			sTooltip = CRemainingDaysGraph::GetTooltip(m_dataset, m_dtExtents, m_nScale, HitTest(point));
			break;
	}

	if (!sTooltip.IsEmpty())
		return CToolTipCtrlEx::SetToolInfo(*pTI, this, sTooltip, MAKELONG(point.x, point.y), m_rectData);

	// else
	return CHMXChartEx::OnToolHitTest(point, pTI);
}

int CBurndownChart::HitTest(const CPoint& ptClient) const
{
	if (!m_rectData.Width() || !m_dtExtents.IsValid())
		return -1;

	if (!m_rectData.PtInRect(ptClient))
		return -1;

	int nNumData = m_dataset[0].GetDatasetSize();
	int nXOffset = (ptClient.x - m_rectData.left);

	return ((nXOffset * nNumData) / m_rectData.Width());
}

