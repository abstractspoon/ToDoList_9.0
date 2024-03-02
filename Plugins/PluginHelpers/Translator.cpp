// PluginHelpers.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "pluginhelpers.h"
#include "FormsUtil.h"
#include "Translator.h"

#include <Shared\wclassdefines.h>
#include <Interfaces\ITransText.h>

////////////////////////////////////////////////////////////////////////////////////////////////

using namespace System::Windows::Forms;
using namespace System::Collections::Generic;

using namespace Abstractspoon::Tdl::PluginHelpers;

////////////////////////////////////////////////////////////////////////////////////////////////

Translator::Translator(ITransText* pTransText) : m_pTransText(pTransText) 
{
} 

// private constructor
Translator::Translator() : m_pTransText(nullptr)
{

}

LPCWSTR Translator::GetClassName(CtrlType type)
{
	switch (type)
	{
		case CtrlType::Button:		return WC_BUTTON;
		case CtrlType::ComboBox:	return WC_COMBOBOX;
		case CtrlType::Dialog:		return WC_DIALOGBOX;
		case CtrlType::Header:		return WC_HEADER;
		case CtrlType::Label:		return WC_STATIC;
		case CtrlType::Menu:		return WC_MENU;
		case CtrlType::Tab:			return WC_TABCONTROL;
		case CtrlType::ToolTip:		return WC_TOOLTIPS;

		case CtrlType::CheckBox:	return L"checkbox";
		case CtrlType::GroupBox:	return L"groupbox";
		case CtrlType::RadioButton:	return L"radiobutton";
	}

	// All else + CtrlType::Text
	return L"text";
}

String^ Translator::Translate(String^ sText, CtrlType type)
{
	if (String::IsNullOrWhiteSpace(sText))
		return String::Empty;

	// Special cases
	switch (type)
	{
	case CtrlType::FileFilter:
		return Translate(sText, GetClassName(CtrlType::Text))->Trim('|');
	}

	return Translate(sText, GetClassName(type));
}

String^ Translator::Translate(String^ sText, LPCWSTR sClassName)
{
	if (String::IsNullOrWhiteSpace(sText))
		return String::Empty;

	// Allow pre-translate to operate regardless of
	// the embedded translator pointer
	if (m_mapPreTranslate != nullptr)
	{
		String^ sPreTrans;

		if (m_mapPreTranslate->TryGetValue(sText, sPreTrans))
			sText = sPreTrans;
	}

	if (m_pTransText == nullptr)
		return sText;

	LPWSTR szTemp = NULL;

	if (!m_pTransText->TranslateText(MS(sText), sClassName, szTemp))
		return sText;

	String^ sTextOut = gcnew String(szTemp);
	m_pTransText->FreeTextBuffer(szTemp);

	return sTextOut;
}

void Translator::Translate(Form^ window)
{
	// Window title
	window->Text = Translate(window->Text, CtrlType::Dialog);

	// children
	Translate(window->Controls);
}

void Translator::Translate(Form^ window, ToolTip^ tooltips)
{
	Translate(window);

	if (tooltips != nullptr)
	{
		int nItem = window->Controls->Count;

		while (nItem--)
		{
			auto ctrl = window->Controls[nItem];
			auto toolText = tooltips->GetToolTip(ctrl);

			if (!String::IsNullOrEmpty(toolText))
				tooltips->SetToolTip(ctrl, Translate(toolText, CtrlType::ToolTip));
		}
	}
}

void Translator::Translate(Control^ ctrl)
{
	// Special cases
	if (ISTYPE(ctrl, Windows::Forms::WebBrowser) ||
		ISTYPE(ctrl, TextBox) ||
		ISTYPE(ctrl, RichTextBox))
	{
		return;
	}

	if (ISTYPE(ctrl, ITranslatable))
	{
		Translate(ASTYPE(ctrl, ITranslatable));
	}
	else if (ISTYPE(ctrl, ToolStrip))
	{
		Translate(ASTYPE(ctrl, ToolStrip)->Items, false);
	}
	else if (ISTYPE(ctrl, ComboBox))
	{
		Translate(ASTYPE(ctrl, ComboBox));
	}
	else if (ISTYPE(ctrl, ListView))
	{
		Translate(ASTYPE(ctrl, ListView)->Columns);
	}
	else
	{
		auto typeArr = ctrl->GetType()->FullName->Split('.');
		auto className = typeArr[typeArr->Length - 1];
		
		ctrl->Text = Translate(ctrl->Text, MS(className));

		// children
		Translate(ctrl->Controls);
	}
}

void Translator::Translate(ITranslatable^ ctrl)
{
	if (ctrl != nullptr)
		ctrl->Translate(this);
}

void Translator::Translate(ToolStripItemCollection^ items, bool isMenu)
{
	int nItem = items->Count;

	while (nItem--)
	{
		auto item = items[nItem];

		item->Text = Translate(item->Text, (isMenu ? CtrlType::Menu : CtrlType::Text));
		item->ToolTipText = Translate(item->ToolTipText, CtrlType::ToolTip);

		// children
		auto dropItem = ASTYPE(item, ToolStripDropDownItem);

		if (dropItem != nullptr)
		{
			if (dropItem->DropDown != nullptr)
			{
				Translate(dropItem->DropDown); // RECURSIVE CALL
			}

			if (dropItem->HasDropDownItems)
			{
				Translate(dropItem->DropDownItems, isMenu); // RECURSIVE CALL
			}
		}
	}
}

void Translator::Translate(Control::ControlCollection^ items)
{
	int nItem = items->Count;

	while (nItem--)
		Translate(items[nItem]);
}

void Translator::Translate(Windows::Forms::ListView::ColumnHeaderCollection^ items)
{
	int nItem = items->Count;

	while (nItem--)
		items[nItem]->Text = Translate(items[nItem]->Text, CtrlType::Header);
}

void Translator::Translate(ComboBox^ combo)
{
	// Don't translate dynamic content
	if (combo->DropDownStyle != ComboBoxStyle::DropDownList)
		return;

	int nItem = combo->Items->Count;

	while (nItem--)
	{
		auto item = ASTYPE(combo->Items[nItem], String);

		if (item == nullptr)
			return;

		combo->Items[nItem] = Translate(item, CtrlType::ComboBox);
	}

	combo->Text = Translate(combo->Text, CtrlType::ComboBox);
	
	FormsUtil::RecalcDropWidth(combo);
}

void Translator::AddPreTranslation(String^ sText, String^ sTranslation)
{
	if (m_mapPreTranslate == nullptr)
		m_mapPreTranslate = gcnew Dictionary<String^, String^>();

	m_mapPreTranslate[sText] = sTranslation;
}

////////////////////////////////////////////////////////////////////////////////////////////////

