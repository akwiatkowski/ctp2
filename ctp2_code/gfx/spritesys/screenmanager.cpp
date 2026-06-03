#include "ctp/c3.h"
#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_surface.h"

#include "gfx/spritesys/screenmanager.h"

ScreenManager::ScreenManager()
:
m_surface(nullptr),
m_surfBase(nullptr),
m_surfWidth(0),
m_surfHeight(0),
m_surfPitch(0),
m_isLocked(FALSE)
{
}

ScreenManager::~ScreenManager()
= default;

void ScreenManager::LockSurface(aui_Surface *surf)
{
	AUI_ERRCODE		errcode;

	Assert(surf != nullptr);
	if (surf == nullptr) return;

	m_surface = surf;

	errcode = surf->Lock(nullptr, (LPVOID *)&m_surfBase, 0 );
	Assert(errcode == AUI_ERRCODE_OK);
	if ( errcode != AUI_ERRCODE_OK ) return;

	m_surfWidth = surf->Width();
	m_surfHeight = surf->Height();
	m_surfPitch = surf->Pitch();

	m_isLocked = TRUE;
}

void ScreenManager::UnlockSurface()
{
	AUI_ERRCODE		errcode;

	Assert(m_surface);
	if (m_surface == nullptr) return;

	errcode = m_surface->Unlock(m_surfBase);
	Assert(errcode == AUI_ERRCODE_OK);

	m_surface = nullptr;
	m_surfBase = nullptr;
	m_surface = nullptr;
	m_surfWidth = 0;
	m_surfHeight = 0;
	m_surfPitch = 0;
	m_isLocked = FALSE;
}
