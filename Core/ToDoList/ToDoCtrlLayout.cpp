// ToDoCtrlSplitting.cpp: implementation of the CToDoCtrlSplitting class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "ToDoCtrlLayout.h"

#include "..\shared\Misc.h"
#include "..\shared\GraphicsMisc.h"
#include "..\shared\HoldRedraw.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////

const int SPLITSIZE = GraphicsMisc::ScaleByDPIFactor(6);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CToDoCtrlLayout::CToDoCtrlLayout(CWnd* pParent, CWnd* pAttributes, CWnd* pComments)
	:
	m_pParent(pParent),
	m_pAttributes(pAttributes),
	m_pComments(pComments),
	m_nAttribsPos(TDCUIL_RIGHT),
	m_nCommentsPos(TDCUIL_BOTTOM),
	m_bAllowStacking(FALSE),
	m_bStackCommentsAbove(FALSE),
	m_nMaxState(TDCMS_NORMAL),
	m_splitterHorz(0, SSP_HORZ, 30, SPLITSIZE),
	m_splitterVert(0, SSP_VERT, 30, SPLITSIZE)
{
	ASSERT(m_pParent);
	ASSERT(m_pAttributes);
	ASSERT(m_pComments);
}

CToDoCtrlLayout::~CToDoCtrlLayout()
{

}

BOOL CToDoCtrlLayout::ModifyLayout(TDC_UILOCATION nAttribsPos,
								   TDC_UILOCATION nCommentsPos)
{
	ASSERT(::IsWindow(*m_pParent));
	ASSERT(::IsWindow(*m_pAttributes));
	ASSERT(::IsWindow(*m_pComments));

	// Check for change
	if ((m_nAttribsPos == nAttribsPos) && (m_nCommentsPos == nAttribsPos))
		return FALSE;

	RebuildSplitters();
	return TRUE;
}

BOOL CToDoCtrlLayout::ModifyLayout(BOOL bAllowStacking,
								   BOOL bStackCommentAbove)
{
	ASSERT(::IsWindow(*m_pParent));
	ASSERT(::IsWindow(*m_pAttributes));
	ASSERT(::IsWindow(*m_pComments));

	// We may only need to rebuild if both are on the same side
	BOOL bRebuild = ((m_nAttribsPos == m_nCommentsPos) &&
					(Misc::StateChanged(m_bAllowStacking, bAllowStacking) ||
					 Misc::StateChanged(m_bStackCommentsAbove, bStackCommentAbove)));

	m_bAllowStacking = bAllowStacking;
	m_bStackCommentsAbove = bStackCommentAbove;

	if (bRebuild)
		RebuildSplitters();

	return bRebuild;
}

void CToDoCtrlLayout::Resize(const CRect& rect)
{
	// Resize the primary splitter only
	if (m_splitterHorz.GetSafeHwnd() && (m_splitterHorz.GetParent() == m_pParent))
	{
		m_splitterHorz.MoveWindow(rect);
	}
	else if (m_splitterVert.GetSafeHwnd() && (m_splitterVert.GetParent() == m_pParent))
	{
		m_splitterVert.MoveWindow(rect);
	}
}

BOOL CToDoCtrlLayout::IsCommentsVisible(BOOL bActually) const
{
	if (m_nMaxState == TDCMS_MAXCOMMENTS)
		return TRUE; // always

	BOOL bVisible = ((m_nMaxState == TDCMS_NORMAL) || ((m_nMaxState == TDCMS_MAXTASKLIST) && m_bShowCommentsAlways));

	// check optionally for actual size
	if (bActually)
	{
		CRect rComments;
		m_pComments->GetWindowRect(rComments);

		bVisible &= ((rComments.Width() > 0) && (rComments.Height() > 0));
	}

	return bVisible;
}

BOOL CToDoCtrlLayout::SetMaximiseState(TDC_MAXSTATE nState, BOOL bShowCommentsAlways)
{
	if (m_nMaxState == nState)
	{
		if (!Misc::StateChanged(m_bShowCommentsAlways, bShowCommentsAlways))
			return FALSE;

		switch (nState)
		{
		case TDCMS_MAXCOMMENTS:
		case TDCMS_NORMAL:
			// Save state but requires no change to splitters
			m_bShowCommentsAlways = bShowCommentsAlways;
			return FALSE;
		}
	}

	m_bShowCommentsAlways = bShowCommentsAlways;
	m_nMaxState = nState;

	RebuildSplitters();

	return TRUE;
}

void CToDoCtrlLayout::RebuildSplitters()
{
	CLockUpdates lu(m_pParent->GetSafeHwnd());

	if (m_splitterHorz.GetSafeHwnd())
	{
		m_splitterHorz.DestroyWindow();
		m_splitterHorz.ClearPanes();
	}

	if (m_splitterVert.GetSafeHwnd())
	{
		m_splitterVert.DestroyWindow();
		m_splitterVert.ClearPanes();
	}

	switch (m_nMaxState)
	{
	case TDCMS_NORMAL:
		{
			switch (m_nCommentsPos)
			{
			case TDCUIL_LEFT: // Comments
				{
					switch (m_nAttribsPos)
					{
					case TDCUIL_LEFT: // Attributes
						if (m_bAllowStacking)
						{
							// .----..----------.      .----..----------.
							// | C  || T        |      | A  || T        |
							// |    ||          |      |    ||          |
							// �----�|          |  OR  �----�|          |
							// .----.|          |      .----.|          |
							// | A  ||          |      | C  ||          |
							// |    ||          |      |    ||          |
							// �----��----------�      �----��----------�
							m_splitterHorz.Create(SSP_HORZ, m_pParent, IDC_HORZSPLITTER);
							m_splitterVert.Create(SSP_VERT, &m_splitterHorz, IDC_VERTSPLITTER);

							m_splitterHorz.SetPaneCount(2);
							m_splitterHorz.SetPane(0, &m_splitterVert);
							m_splitterHorz.SetPane(1, NULL); // Tasks

							m_splitterVert.SetPaneCount(2);

							if (m_bStackCommentsAbove)
							{
								m_splitterVert.SetPane(0, m_pComments);
								m_splitterVert.SetPane(1, m_pAttributes);
							}
							else
							{
								m_splitterVert.SetPane(0, m_pAttributes);
								m_splitterVert.SetPane(1, m_pComments);
							}
						}
						else
						{
							// .----..----..----.
							// | C  || A  || T  |
							// |    ||    ||    |
							// |    ||    ||    |
							// |    ||    ||    |
							// |    ||    ||    |
							// |    ||    ||    |
							// �----��----��----�
							m_splitterHorz.Create(SSP_HORZ, m_pParent, IDC_HORZSPLITTER);

							m_splitterHorz.SetPaneCount(3);
							m_splitterHorz.SetPane(0, m_pComments);
							m_splitterHorz.SetPane(1, m_pAttributes);
							m_splitterHorz.SetPane(2, NULL); // Tasks
						}
						break;

					case TDCUIL_RIGHT: // Attributes
						{
							// .----..----..----.
							// | C  || T  || A  |
							// |    ||    ||    |
							// |    ||    ||    |
							// |    ||    ||    |
							// |    ||    ||    |
							// |    ||    ||    |
							// �----��----��----�
							m_splitterHorz.Create(SSP_HORZ, m_pParent, IDC_HORZSPLITTER);

							m_splitterHorz.SetPaneCount(3);
							m_splitterHorz.SetPane(0, m_pComments);
							m_splitterHorz.SetPane(1, NULL); // Tasks
							m_splitterHorz.SetPane(2, m_pAttributes);
						}
						break;

					case TDCUIL_BOTTOM: // Attributes
						{
							// .----. .----.
							// | C  | | T  |
							// |    | |    |
							// �----� �----�
							// .-----------.
							// |     A     |
							// |           |
							// �-----------�
							m_splitterVert.Create(SSP_VERT, m_pParent, IDC_VERTSPLITTER);
							m_splitterHorz.Create(SSP_HORZ, &m_splitterVert, IDC_HORZSPLITTER);

							m_splitterVert.SetPaneCount(2);
							m_splitterVert.SetPane(0, &m_splitterHorz);
							m_splitterVert.SetPane(1, m_pAttributes);

							m_splitterHorz.SetPaneCount(2);
							m_splitterHorz.SetPane(0, m_pComments);
							m_splitterHorz.SetPane(1, NULL); // Tasks
						}
						break;
					}
				}
				break;

			case TDCUIL_RIGHT: // Comments
				{
					switch (m_nAttribsPos)
					{
					case TDCUIL_LEFT: // Attributes
						{
							// .----..----..----.
							// | A  || T  || C  |
							// |    ||    ||    |
							// |    ||    ||    |
							// |    ||    ||    |
							// |    ||    ||    |
							// |    ||    ||    |
							// �----��----��----�
							m_splitterHorz.Create(SSP_HORZ, m_pParent, IDC_HORZSPLITTER);

							m_splitterHorz.SetPaneCount(3);
							m_splitterHorz.SetPane(0, m_pAttributes);
							m_splitterHorz.SetPane(1, NULL); // Tasks
							m_splitterHorz.SetPane(2, m_pComments);
						}
						break;

					case TDCUIL_RIGHT: // Attributes
						if (m_bAllowStacking)
						{
							// .----------..----.      .----------..----.
							// | T        || C  |      | T        || A  |
							// |          ||    |      |          ||    |
							// |          |�----�  OR  |          |�----�
							// |          |.----.      |          |.----.
							// |          || A  |      |          || C  |
							// |          ||    |      |          ||    |
							// �----------��----�      �----------��----�
							m_splitterHorz.Create(SSP_HORZ, m_pParent, IDC_HORZSPLITTER);
							m_splitterVert.Create(SSP_VERT, &m_splitterHorz, IDC_VERTSPLITTER);

							m_splitterHorz.SetPaneCount(2);
							m_splitterHorz.SetPane(0, NULL); // Tasks
							m_splitterHorz.SetPane(1, &m_splitterVert);

							m_splitterVert.SetPaneCount(2);

							if (m_bStackCommentsAbove)
							{
								m_splitterVert.SetPane(0, m_pComments);
								m_splitterVert.SetPane(1, m_pAttributes);
							}
							else
							{
								m_splitterVert.SetPane(0, m_pAttributes);
								m_splitterVert.SetPane(1, m_pComments);
							}
						}
						else
						{
							// .----..----..----.
							// | T  || A  || C  |
							// |    ||    ||    |
							// |    ||    ||    |
							// |    ||    ||    |
							// |    ||    ||    |
							// |    ||    ||    |
							// �----��----��----�
							m_splitterHorz.Create(SSP_HORZ, m_pParent, IDC_HORZSPLITTER);

							m_splitterHorz.SetPaneCount(3);
							m_splitterHorz.SetPane(0, NULL); // Tasks
							m_splitterHorz.SetPane(1, m_pAttributes);
							m_splitterHorz.SetPane(2, m_pComments);
						}
						break;

					case TDCUIL_BOTTOM: // Attributes
						{
							// .-----. .-----.
							// |  T  | |  C  |
							// |     | |     |
							// �-----� �-----�
							// .-------------.
							// |      A      |
							// |             |
							// �-------------�
							m_splitterVert.Create(SSP_VERT, m_pParent, IDC_VERTSPLITTER);
							m_splitterHorz.Create(SSP_HORZ, &m_splitterVert, IDC_HORZSPLITTER);

							m_splitterVert.SetPaneCount(2);
							m_splitterVert.SetPane(0, &m_splitterHorz);
							m_splitterVert.SetPane(1, m_pAttributes);

							m_splitterHorz.SetPaneCount(2);
							m_splitterHorz.SetPane(0, NULL); // Tasks
							m_splitterHorz.SetPane(1, m_pComments);
						}
						break;
					}
				}
				break;

			case TDCUIL_BOTTOM: // Comments
				{
					switch (m_nAttribsPos)
					{
					case TDCUIL_LEFT:
						{
							// .----..---------.
							// | A  || T       |
							// |    ||         |
							// |    |�---------�
							// |    |.---------.
							// |    || C       |
							// |    ||         |
							// �----��---------�
							m_splitterHorz.Create(SSP_HORZ, m_pParent, IDC_HORZSPLITTER);
							m_splitterVert.Create(SSP_VERT, &m_splitterHorz, IDC_VERTSPLITTER);

							m_splitterHorz.SetPaneCount(2);
							m_splitterHorz.SetPane(0, m_pAttributes);
							m_splitterHorz.SetPane(1, &m_splitterVert);

							m_splitterVert.SetPaneCount(2);
							m_splitterVert.SetPane(0, NULL); // Tasks
							m_splitterVert.SetPane(1, m_pComments);
						}
						break;

					case TDCUIL_RIGHT:
						{
							// .---------..----.
							// | T       || A  |
							// |         ||    |
							// �---------�|    |
							// .---------.|    |
							// | C       ||    |
							// |         ||    |
							// �---------��----�
							m_splitterHorz.Create(SSP_HORZ, m_pParent, IDC_HORZSPLITTER);
							m_splitterVert.Create(SSP_VERT, &m_splitterHorz, IDC_VERTSPLITTER);

							m_splitterHorz.SetPaneCount(2);
							m_splitterHorz.SetPane(0, &m_splitterVert);
							m_splitterHorz.SetPane(1, m_pAttributes);

							m_splitterVert.SetPaneCount(2);
							m_splitterVert.SetPane(0, NULL); // Tasks
							m_splitterVert.SetPane(1, m_pComments);
						}
						break;

					case TDCUIL_BOTTOM:
						{
							// .-------------.      .-------------.
							// |      T      |      |      T      |
							// |             |      |             |
							// �-------------�  OR  �-------------�
							// .-----. .-----.      .-----. .-----.
							// |  C  | |  A  |      |  A  | |  C  |
							// |     | |     |      |     | |     |
							// �-----� �-----�      �-----� �-----�
							if (m_bAllowStacking)
							{

								m_splitterVert.Create(SSP_VERT, m_pParent, IDC_VERTSPLITTER);
								m_splitterHorz.Create(SSP_HORZ, &m_splitterVert, IDC_HORZSPLITTER);

								m_splitterVert.SetPaneCount(2);
								m_splitterVert.SetPane(0, NULL); // Tasks
								m_splitterVert.SetPane(1, &m_splitterHorz);

								m_splitterHorz.SetPaneCount(2);

								if (m_bStackCommentsAbove)
								{
									m_splitterHorz.SetPane(0, m_pComments);
									m_splitterHorz.SetPane(1, m_pAttributes);
								}
								else
								{
									m_splitterHorz.SetPane(0, m_pAttributes);
									m_splitterHorz.SetPane(1, m_pComments);
								}
							}
							else
							{
								// .-------------.
								// |      T      |
								// �-------------�
								// .-------------.
								// |      A      |
								// �-------------�
								// .-------------.
								// |      C      |
								// �-------------�
								m_splitterVert.Create(SSP_VERT, m_pParent, IDC_VERTSPLITTER);

								m_splitterVert.SetPaneCount(3);
								m_splitterVert.SetPane(0, NULL); // Tasks
								m_splitterVert.SetPane(1, m_pAttributes);
								m_splitterVert.SetPane(2, m_pComments);
							}
						}
						break;
					}
				}
				break;
			}
		}
		break;

	case TDCMS_MAXCOMMENTS:
		{
			// No splitter required
		}
		break;

	case TDCMS_MAXTASKLIST:
		if (m_bShowCommentsAlways)
		{
			switch (m_nCommentsPos)
			{
			case TDCUIL_LEFT:
			case TDCUIL_RIGHT:
				{
					m_splitterHorz.Create(SSP_HORZ, m_pParent, IDC_HORZSPLITTER);
					m_splitterHorz.SetPaneCount(2);
					m_splitterHorz.SetPane(0, NULL); // Tasks
					m_splitterHorz.SetPane(1, m_pComments);
				}
				break;

			case TDCUIL_BOTTOM:
				{
					m_splitterVert.Create(SSP_VERT, m_pParent, IDC_VERTSPLITTER);
					m_splitterVert.SetPaneCount(2);
					m_splitterVert.SetPane(0, NULL); // Tasks
					m_splitterVert.SetPane(1, m_pComments);
				}
				break;
			}

		}
		else
		{
			// No splitter required
		}
		break;
	}
}
