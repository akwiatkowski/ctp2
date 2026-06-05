//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : The Status Bar
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
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Added CleanUp function for m_text, by Martin G�hmann.
//
//----------------------------------------------------------------------------
#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef ___BMH_STATUS_BAR_HEADER
#define ___BMH_STATUS_BAR_HEADER

#include <list>
#include <string>

class aui_Control;
class ctp2_Static;

class StatusBar {
public:

	static void SetText(const MBCHAR *text, const aui_Control *owner = nullptr);

	static const aui_Control *GetOwner() { return m_owner; }

	StatusBar(MBCHAR *ldlBlock);

	// std::string self-clears; this entry point is kept for the existing
	// 'shutdown' call sites but no longer touches a raw buffer.
	static void CleanUp(){
		m_text.clear();
	}

	~StatusBar();

private:

	void Update();

	// Was raw 'MBCHAR *m_text' + 'sint32 m_allocatedLen' growing-buffer
	// management.  std::string handles all of it; CleanUp's old
	// 'delete m_text;' was a delete-vs-new[] mismatch (UB).
	static std::string m_text;

	static std::list<StatusBar*> m_list;

	ctp2_Static	*m_statusBar;

	static const aui_Control *m_owner;
};

#endif
