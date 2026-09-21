#include "ctp/c3.h"

#include <memory>

#include "ui/aui_common/aui_mouse.h"

#include "ui/aui_ctp2/c3ui.h"

#include "ui/interface/cursormanager.h"


static std::unique_ptr<CursorManager> g_cursorManager;

CursorManager * cursormanager_Get() { return g_cursorManager.get(); }


void CursorManager::Initialize()
{
	Cleanup();

	g_cursorManager = std::make_unique<CursorManager>();
}


void CursorManager::Cleanup()
{
	g_cursorManager.reset();
}


CursorManager::CursorManager()
{
	m_curCursor = CURSORINDEX_DEFAULT;
	m_savedCursor = CURSORINDEX_MAX;
}


CursorManager::~CursorManager()
= default;


void CursorManager::SetCursor(CURSORINDEX cursor)
{
	if (cursor >= 0 && cursor < CURSORINDEX_MAX) {

		aui_Mouse		*theMouse;
		theMouse = c3ui_Get()->TheMouse();






		if(theMouse)
			theMouse->SetAnim((sint32)cursor);

		m_curCursor = cursor;
	}
}


void CursorManager::SaveCursor()
{
	m_savedCursor = m_curCursor;
}


void CursorManager::RestoreCursor()
{
	if (m_savedCursor == CURSORINDEX_MAX) {
		m_curCursor = CURSORINDEX_DEFAULT;
	} else {
		m_curCursor = m_savedCursor;
		m_savedCursor = CURSORINDEX_MAX;
	}
}
