// FileMiscTest.cpp: implementation of the CMiscTest class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TDLTest.h"
#include "MiscTest.h"

#include "..\shared\Misc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////

const CString DEFSEP = (Misc::GetListSeparator() + ' ');
const int DEFSEPLEN = DEFSEP.GetLength();

const LPCTSTR NULLSTRING = NULL;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMiscTest::CMiscTest(const CTestUtils& utils) : CTDLTestBase(utils)
{

}

CMiscTest::~CMiscTest()
{
}

TESTRESULT CMiscTest::Run()
{
	ClearTotals();

	TestRegionalSettingsRetrievalPerformance();

	TestGetFormattedLength();
	TestFormatArray();
	TestHasPrefix();
	TestHasSuffix();
	TestAtof();
	TestRemoveDuplicates();

	return GetTotals();
}

void CMiscTest::TestGetFormattedLength()
{
	CTDCScopedTest test(*this, _T("Misc::GetFormattedLength"));

	{
		CStringArray aValues;
		aValues.Add(_T("a"));
		aValues.Add(_T("ab"));
		aValues.Add(_T("abc"));
		aValues.Add(_T("abcd"));

		// First 3 are equivalent and use default Windows separator
		ExpectEQ(Misc::GetFormattedLength(aValues), 10 + (3 * DEFSEPLEN));
		ExpectEQ(Misc::GetFormattedLength(aValues, NULL), 10 + (3 * DEFSEPLEN));
		ExpectEQ(Misc::GetFormattedLength(aValues, _T("")), 10 + (3 * DEFSEPLEN));

		ExpectEQ(Misc::GetFormattedLength(aValues, _T(",")), 13);
		ExpectEQ(Misc::GetFormattedLength(aValues, _T("...")), 19);
	}
	
	{
		CStringArray aValues;
		aValues.Add(_T("a"));
		aValues.Add(_T(""));
		aValues.Add(_T("ab"));
		aValues.Add(_T(""));
		aValues.Add(_T("abc"));
		aValues.Add(_T(""));
		aValues.Add(_T("abcd"));

		// Excluding empty items
		ExpectEQ(Misc::GetFormattedLength(aValues), 10 + (3 * DEFSEPLEN));
		ExpectEQ(Misc::GetFormattedLength(aValues, NULL), 10 + (3 * DEFSEPLEN));
		ExpectEQ(Misc::GetFormattedLength(aValues, _T("")), 10 + (3 * DEFSEPLEN));

		ExpectEQ(Misc::GetFormattedLength(aValues, _T(",")), 13);
		ExpectEQ(Misc::GetFormattedLength(aValues, _T("...")), 19);

		// Including empty items
		ExpectEQ(Misc::GetFormattedLength(aValues, NULL, TRUE), 10 + (6 * DEFSEPLEN));
		ExpectEQ(Misc::GetFormattedLength(aValues, _T(""), TRUE), 10 + (6 * DEFSEPLEN));

		ExpectEQ(Misc::GetFormattedLength(aValues, _T(","), TRUE), 16);
		ExpectEQ(Misc::GetFormattedLength(aValues, _T("..."), TRUE), 28);
	}
	
	{
		// Only empty items
		CStringArray aValues;
		aValues.Add(_T(""));
		aValues.Add(_T(""));
		aValues.Add(_T(""));

		// Excluding empty items
		ExpectEQ(Misc::GetFormattedLength(aValues), 0);
		ExpectEQ(Misc::GetFormattedLength(aValues, NULL), 0);
		ExpectEQ(Misc::GetFormattedLength(aValues, _T("")), 0);

		ExpectEQ(Misc::GetFormattedLength(aValues, _T(",")), 0);
		ExpectEQ(Misc::GetFormattedLength(aValues, _T("...")), 0);

		// Including empty items
		ExpectEQ(Misc::GetFormattedLength(aValues, NULL, TRUE), 2 * DEFSEPLEN);
		ExpectEQ(Misc::GetFormattedLength(aValues, _T(""), TRUE), 2 * DEFSEPLEN);

		ExpectEQ(Misc::GetFormattedLength(aValues, _T(","), TRUE), 2);
		ExpectEQ(Misc::GetFormattedLength(aValues, _T("..."), TRUE), 6);
	}
}

void CMiscTest::TestFormatArray()
{
	CTDCScopedTest test(*this, _T("Misc::FormatArray"));

	{
		CStringArray aValues;
		aValues.Add(_T("a"));
		aValues.Add(_T("ab"));
		aValues.Add(_T("abc"));
		aValues.Add(_T("abcd"));

		CString sDefCompare = _T("a") + DEFSEP + 
								_T("ab") + DEFSEP + 
								_T("abc") + DEFSEP + 
								_T("abcd");

		ExpectEQ(Misc::FormatArray(aValues), sDefCompare);
		ExpectEQ(Misc::FormatArray(aValues, NULLSTRING), sDefCompare);
		ExpectEQ(Misc::FormatArray(aValues, _T("")), sDefCompare);

		ExpectEQ(Misc::FormatArray(aValues, _T(",")), _T("a,ab,abc,abcd"));
		ExpectEQ(Misc::FormatArray(aValues, _T("...")), _T("a...ab...abc...abcd"));

		ExpectTrue(ActualLengthMatchesCalculation(aValues, NULL));
		ExpectTrue(ActualLengthMatchesCalculation(aValues, _T("")));
		ExpectTrue(ActualLengthMatchesCalculation(aValues, _T(",")));
		ExpectTrue(ActualLengthMatchesCalculation(aValues, _T("...")));
	}

	{
		CStringArray aValues;
		aValues.Add(_T("a"));
		aValues.Add(_T(""));
		aValues.Add(_T("ab"));
		aValues.Add(_T(""));
		aValues.Add(_T("abc"));
		aValues.Add(_T(""));
		aValues.Add(_T("abcd"));

		// Excluding empty items
		{
			CString sDefCompare = _T("a") + DEFSEP + 
									_T("ab") + DEFSEP + 
									_T("abc") + DEFSEP + 
									_T("abcd");

			ExpectEQ(Misc::FormatArray(aValues), sDefCompare);
			ExpectEQ(Misc::FormatArray(aValues, NULLSTRING), sDefCompare);
			ExpectEQ(Misc::FormatArray(aValues, _T("")), sDefCompare);

			ExpectEQ(Misc::FormatArray(aValues, _T(",")), _T("a,ab,abc,abcd"));
			ExpectEQ(Misc::FormatArray(aValues, _T("...")), _T("a...ab...abc...abcd"));

			ExpectTrue(ActualLengthMatchesCalculation(aValues, NULL));
			ExpectTrue(ActualLengthMatchesCalculation(aValues, _T("")));
			ExpectTrue(ActualLengthMatchesCalculation(aValues, _T(",")));
			ExpectTrue(ActualLengthMatchesCalculation(aValues, _T("...")));
		}

		// Including empty items
		{
			CString sDefCompare = _T("a") + DEFSEP + 
									_T("") + DEFSEP + 
									_T("ab") + DEFSEP + 
									_T("") + DEFSEP + 
									_T("abc") + DEFSEP + 
									_T("") + DEFSEP +
									_T("abcd");

			ExpectEQ(Misc::FormatArray(aValues, NULLSTRING, TRUE), sDefCompare);
			ExpectEQ(Misc::FormatArray(aValues, _T(""), TRUE), sDefCompare);

			ExpectEQ(Misc::FormatArray(aValues, _T(","), TRUE), _T("a,,ab,,abc,,abcd"));
			ExpectEQ(Misc::FormatArray(aValues, _T("..."), TRUE), _T("a......ab......abc......abcd"));

			ExpectTrue(ActualLengthMatchesCalculation(aValues, NULL, TRUE));
			ExpectTrue(ActualLengthMatchesCalculation(aValues, _T(""), TRUE));
			ExpectTrue(ActualLengthMatchesCalculation(aValues, _T(","), TRUE));
			ExpectTrue(ActualLengthMatchesCalculation(aValues, _T(", "), TRUE));
		}
	}

	{
		// Only empty items
		CStringArray aValues;
		aValues.Add(_T(""));
		aValues.Add(_T(""));
		aValues.Add(_T(""));

		// Excluding empty items
		{
			ExpectEQ(Misc::FormatArray(aValues), _T(""));
			ExpectEQ(Misc::FormatArray(aValues, NULLSTRING), _T(""));
			ExpectEQ(Misc::FormatArray(aValues, _T("")), _T(""));

			ExpectEQ(Misc::FormatArray(aValues, _T(",")), _T(""));
			ExpectEQ(Misc::FormatArray(aValues, _T("...")), _T(""));

			ExpectTrue(ActualLengthMatchesCalculation(aValues, NULL));
			ExpectTrue(ActualLengthMatchesCalculation(aValues, _T("")));
			ExpectTrue(ActualLengthMatchesCalculation(aValues, _T(",")));
			ExpectTrue(ActualLengthMatchesCalculation(aValues, _T("...")));
		}

		// Including empty items
		{
			CString sDefCompare = _T("") + DEFSEP + _T("") + DEFSEP + _T("");

			ExpectEQ(Misc::FormatArray(aValues, NULLSTRING, TRUE), sDefCompare);
			ExpectEQ(Misc::FormatArray(aValues, _T(""), TRUE), sDefCompare);

			ExpectEQ(Misc::FormatArray(aValues, _T(","), TRUE), _T(",,"));
			ExpectEQ(Misc::FormatArray(aValues, _T("..."), TRUE), _T("......"));

			ExpectTrue(ActualLengthMatchesCalculation(aValues, NULL, TRUE));
			ExpectTrue(ActualLengthMatchesCalculation(aValues, _T(""), TRUE));
			ExpectTrue(ActualLengthMatchesCalculation(aValues, _T(","), TRUE));
			ExpectTrue(ActualLengthMatchesCalculation(aValues, _T("..."), TRUE));
		}
	}
}

BOOL CMiscTest::ActualLengthMatchesCalculation(const CStringArray& aValues, LPCTSTR szSep, BOOL bIncEmpty)
{
	return (Misc::FormatArray(aValues, szSep, bIncEmpty).GetLength() == 
			Misc::GetFormattedLength(aValues, szSep, bIncEmpty));
}

void CMiscTest::TestHasPrefix()
{
	CTDCScopedTest test(*this, _T("Misc::HasPrefix"));

	// Note: FALSE for last argument is more permissive
	{
		// Empty arguments
		ExpectFalse(Misc::HasPrefix(_T(""), _T(""), TRUE));
		ExpectFalse(Misc::HasPrefix(_T(""), _T(""), FALSE));
		ExpectFalse(Misc::HasPrefix(_T("abc"), _T(""), TRUE));
		ExpectFalse(Misc::HasPrefix(_T("abc"), _T(""), FALSE));
		ExpectFalse(Misc::HasPrefix(_T(""), _T("abc"), TRUE));
		ExpectFalse(Misc::HasPrefix(_T(""), _T("abc"), FALSE));
	}

	{
		// Variable case
		ExpectTrue(Misc::HasPrefix(_T("abc"), _T("a"), TRUE));
		ExpectTrue(Misc::HasPrefix(_T("abc"), _T("ab"), TRUE));
		ExpectTrue(Misc::HasPrefix(_T("abc"), _T("abc"), TRUE));

		ExpectTrue(Misc::HasPrefix(_T("abc"), _T("a"), FALSE));
		ExpectTrue(Misc::HasPrefix(_T("abc"), _T("ab"), FALSE));
		ExpectTrue(Misc::HasPrefix(_T("abc"), _T("abc"), FALSE));

		ExpectFalse(Misc::HasPrefix(_T("abc"), _T("A"), TRUE));
		ExpectFalse(Misc::HasPrefix(_T("abc"), _T("AB"), TRUE));
		ExpectFalse(Misc::HasPrefix(_T("abc"), _T("ABC"), TRUE));

		ExpectTrue(Misc::HasPrefix(_T("abc"), _T("A"), FALSE));
		ExpectTrue(Misc::HasPrefix(_T("abc"), _T("AB"), FALSE));
		ExpectTrue(Misc::HasPrefix(_T("abc"), _T("ABC"), FALSE));

		ExpectTrue(Misc::HasPrefix(_T("abc"), _T("A"), FALSE));
		ExpectTrue(Misc::HasPrefix(_T("abc"), _T("Ab"), FALSE));
		ExpectTrue(Misc::HasPrefix(_T("abc"), _T("AbC"), FALSE));
	}

	{
		// Leading whitespace
		ExpectFalse(Misc::HasPrefix(_T(" abc"), _T("c"), FALSE));
		ExpectFalse(Misc::HasPrefix(_T("\tabc"), _T("bc"), FALSE));
		ExpectFalse(Misc::HasPrefix(_T("\nabc"), _T("abc"), FALSE));

		ExpectFalse(Misc::HasPrefix(_T(" abc"), _T("C"), FALSE));
		ExpectFalse(Misc::HasPrefix(_T("\tabc"), _T("BC"), FALSE));
		ExpectFalse(Misc::HasPrefix(_T("\nabc"), _T("ABC"), FALSE));
	}
}

void CMiscTest::TestHasSuffix()
{
	CTDCScopedTest test(*this, _T("Misc::HasSuffix"));

	// Note: FALSE for last argument is more permissive
	{
		// Empty arguments
		ExpectFalse(Misc::HasSuffix(_T(""), _T(""), TRUE));
		ExpectFalse(Misc::HasSuffix(_T(""), _T(""), FALSE));
		ExpectFalse(Misc::HasSuffix(_T("abc"), _T(""), TRUE));
		ExpectFalse(Misc::HasSuffix(_T("abc"), _T(""), FALSE));
		ExpectFalse(Misc::HasSuffix(_T(""), _T("abc"), TRUE));
		ExpectFalse(Misc::HasSuffix(_T(""), _T("abc"), FALSE));
	}

	{
		// Variable case
		ExpectTrue(Misc::HasSuffix(_T("abc"), _T("c"), TRUE));
		ExpectTrue(Misc::HasSuffix(_T("abc"), _T("bc"), TRUE));
		ExpectTrue(Misc::HasSuffix(_T("abc"), _T("abc"), TRUE));

		ExpectTrue(Misc::HasSuffix(_T("abc"), _T("c"), FALSE));
		ExpectTrue(Misc::HasSuffix(_T("abc"), _T("bc"), FALSE));
		ExpectTrue(Misc::HasSuffix(_T("abc"), _T("abc"), FALSE));

		ExpectFalse(Misc::HasSuffix(_T("abc"), _T("C"), TRUE));
		ExpectFalse(Misc::HasSuffix(_T("abc"), _T("BC"), TRUE));
		ExpectFalse(Misc::HasSuffix(_T("abc"), _T("ABC"), TRUE));

		ExpectTrue(Misc::HasSuffix(_T("abc"), _T("C"), FALSE));
		ExpectTrue(Misc::HasSuffix(_T("abc"), _T("BC"), FALSE));
		ExpectTrue(Misc::HasSuffix(_T("abc"), _T("ABC"), FALSE));

		ExpectTrue(Misc::HasSuffix(_T("abc"), _T("C"), FALSE));
		ExpectTrue(Misc::HasSuffix(_T("abc"), _T("bC"), FALSE));
		ExpectTrue(Misc::HasSuffix(_T("abc"), _T("AbC"), FALSE));
	}

	{
		// Trailing whitespace
		ExpectFalse(Misc::HasSuffix(_T("abc "), _T("c"), FALSE));
		ExpectFalse(Misc::HasSuffix(_T("abc\t"), _T("bc"), FALSE));
		ExpectFalse(Misc::HasSuffix(_T("abc\n"), _T("abc"), FALSE));

		ExpectFalse(Misc::HasSuffix(_T("abc "), _T("C"), FALSE));
		ExpectFalse(Misc::HasSuffix(_T("abc\t"), _T("BC"), FALSE));
		ExpectFalse(Misc::HasSuffix(_T("abc\n"), _T("ABC"), FALSE));
	}
}

void  CMiscTest::TestAtof()
{
	CTDCScopedTest test(*this, _T("Misc::Atof"));
	
	TestAtof("C");
	TestAtof("en-GB");
	TestAtof("en-BE");
	TestAtof("fr-FR");
	TestAtof("fr-BE");
	TestAtof("zh-CN");
	TestAtof("hu-HU");
	TestAtof("ar-DZ");
}

void CMiscTest::TestAtof(const CString& sLocale)
{
	ExpectEQ(CTempLocale::Current(), _T("C"));
	{
		CTempLocale loc(LC_ALL, sLocale);

#if _MSC_VER > 1200
		// This test will fail under VC6 because of something in setlocale.
		// Note: This will resolved in 9.1 because we will be using VS2015 
		//for the release build
		ExpectEQ(CTempLocale::Current(), sLocale);
#endif

		ExpectEQ(Misc::Atof(_T("10.12345")), 10.12345);
		ExpectEQ(Misc::Atof(_T("-10.12345")), -10.12345);

		ExpectEQ(Misc::Atof(_T("10,12345")), 10.12345);
		ExpectEQ(Misc::Atof(_T("-10,12345")), -10.12345);

		ExpectEQ(Misc::Atof(_T("10'12345")), 10.0);
		ExpectEQ(Misc::Atof(_T("-10'12345")), -10.0);
	}
	ExpectEQ(CTempLocale::Current(), _T("C"));
}

void CMiscTest::TestRemoveDuplicates()
{
	CTDCScopedTest test(*this, _T("Misc::RemoveDuplicates"));

	// Case sensitive
	{
		// No duplicates exist
		{
			CStringArray aItems;

			aItems.Add(_T("abc"));
			aItems.Add(_T("abC"));
			aItems.Add(_T("Abc"));
			aItems.Add(_T("ABC"));
			aItems.Add(_T("aBC"));
			aItems.Add(_T("AbC"));
			aItems.Add(_T("ABc"));

			ExpectEQ(Misc::RemoveDuplicates(aItems, TRUE), 0);
			ExpectEQ(aItems.GetSize(), 7);

			ExpectEQ(aItems[0], _T("abc"));
			ExpectEQ(aItems[1], _T("abC"));
			ExpectEQ(aItems[2], _T("Abc"));
			ExpectEQ(aItems[3], _T("ABC"));
			ExpectEQ(aItems[4], _T("aBC"));
			ExpectEQ(aItems[5], _T("AbC"));
			ExpectEQ(aItems[6], _T("ABc"));
		}

		// Some duplicates exist
		{
			CStringArray aItems;

			aItems.Add(_T("abc"));
			aItems.Add(_T("abC"));
			aItems.Add(_T("abc")); // dupe
			aItems.Add(_T("Abc"));
			aItems.Add(_T("ABC"));
			aItems.Add(_T("aBC"));
			aItems.Add(_T("ABC")); // dupe
			aItems.Add(_T("AbC"));
			aItems.Add(_T("ABc"));
			aItems.Add(_T("AbC")); // dupe

			ExpectEQ(Misc::RemoveDuplicates(aItems, TRUE), 3);
			ExpectEQ(aItems.GetSize(), 7);
			
			ExpectEQ(aItems[0], _T("abc"));
			ExpectEQ(aItems[1], _T("abC"));
			ExpectEQ(aItems[2], _T("Abc"));
			ExpectEQ(aItems[3], _T("ABC"));
			ExpectEQ(aItems[4], _T("aBC"));
			ExpectEQ(aItems[5], _T("AbC"));
			ExpectEQ(aItems[6], _T("ABc"));
		}
	}

	// Case Insensitive
	{
		CStringArray aItems;

		aItems.Add(_T("abc"));
		aItems.Add(_T("abC"));
		aItems.Add(_T("Abc"));
		aItems.Add(_T("ABC"));
		aItems.Add(_T("aBC"));
		aItems.Add(_T("AbC"));
		aItems.Add(_T("ABc"));

		ExpectEQ(Misc::RemoveDuplicates(aItems, FALSE), 6);
		ExpectEQ(aItems.GetSize(), 1);

		ExpectEQ(aItems[0], _T("abc")); // only the first item remains
	}
}

///////////////////////////////////////////////////////////////////////

void CMiscTest::TestRegionalSettingsRetrievalPerformance()
{
	if (!m_utils.HasCommandlineFlag('p'))
	{
		_tprintf(_T("Add '-p' to run CMiscTest::TestRegionalSettingsPerformance\n"));
		return;
	}

	CTDCScopedTest test(*this, _T("RegionalSettingsRetrievalPerformance"));

	CString sSetting;
	const int NUM_LOOPS = 100000;
	DWORD dwTickStart = GetTickCount();

	for (int nLoop = 0; nLoop <= NUM_LOOPS; nLoop++)
	{
		sSetting = Misc::GetListSeparator();
		sSetting = Misc::GetDecimalSeparator();
		sSetting = Misc::GetDefCharset();
		sSetting = Misc::GetAM();
		sSetting = Misc::GetPM();
		sSetting = Misc::GetTimeSeparator();
		sSetting = Misc::GetTimeFormat();
		sSetting = Misc::GetShortDateFormat();
		sSetting = Misc::GetDateSeparator();
	}

	_tprintf(_T("Test took %ld ms to retrieve 9 settings %d times\n"), (GetTickCount() - dwTickStart), NUM_LOOPS);
}
