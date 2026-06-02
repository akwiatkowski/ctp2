#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __SCREENMANAGER_H__
#define __SCREENMANAGER_H__

class aui_Surface;

class ScreenManager
{
public:
	ScreenManager();
	~ScreenManager();

	void LockSurface(aui_Surface *surf);
	void UnlockSurface(void);

	BOOL IsLocked(void) const { return m_isLocked; }

	aui_Surface *GetSurface(void) const { return m_surface; }
	uint8 *GetSurfBase(void) const { return m_surfBase; }
	sint32 GetSurfWidth(void) const { return m_surfWidth; }
	sint32 GetSurfHeight(void) const { return m_surfHeight; }
	sint32 GetSurfPitch(void) const { return m_surfPitch; }

private:
	aui_Surface		*m_surface;
	uint8			*m_surfBase;
	sint32			m_surfWidth;
	sint32			m_surfHeight;
	sint32			m_surfPitch;
	BOOL			m_isLocked;
};

// App-singleton accessor pair, mirroring world_Get / c3ui_Get /
// tiledmap_Get.  The legacy g_screenManager pointer is now file-static
// in ctp/civ3_main.cpp; outside callers must go through these accessors.
ScreenManager * screenmanager_Get(void);
void            screenmanager_Set(ScreenManager *p);

#endif
