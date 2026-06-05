//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : The Status Bar
// Id           : $Id$
//
//----------------------------------------------------------------------------
//
// Disclaimer
//
// THIS FILE IS NOT GENERATED OR SUPPORTED BY ACTIVISION.
//
// This material has been developed at apolyton.net by the Apolyton CtP2
// Source Code Project. Contact the authors at ctp2source@apolyton.net.
//
//----------------------------------------------------------------------------
//
// Compiler flags
//
// - None
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - None
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"

#include "ui/interface/StatusBar.h"

#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_ctp2/ctp2_Static.h"

std::string StatusBar::m_text;
std::list<StatusBar*> StatusBar::m_list;
const aui_Control *StatusBar::m_owner = nullptr;

void StatusBar::SetText(const MBCHAR *text, const aui_Control *owner)
{
	if (text) {
		if (m_text == text) return;   // identical content — no observers to wake
		m_text = text;
	}
	// Note: original logic only repainted on text != nullptr; preserve that
	// (a nullptr 'text' call still updates m_owner + dispatches Update()).
	m_owner = owner;

	for (auto & i : m_list)
		i->Update();
}

StatusBar::StatusBar(MBCHAR *ldlBlock) :
m_statusBar(static_cast<ctp2_Static*>(aui_Ldl::GetObject(ldlBlock, "StatusBar")))
{

	Assert(m_statusBar);

	m_list.push_back(this);

}

StatusBar::~StatusBar()
{

	m_list.remove(this);

}

void StatusBar::Update()
{
	m_statusBar->SetText(m_text.c_str());
}
