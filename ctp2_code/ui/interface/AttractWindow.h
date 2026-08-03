#ifndef __ATTRACT_WINDOW_H__
#define __ATTRACT_WINDOW_H__

#include "ui/aui_ctp2/c3window.h"
#include "ctp/ctp2_utils/pointerlist.h"

struct AttractRegion {
	aui_Region *m_region;
	uint32 m_startTime;
};

class AttractWindow : public C3Window {
public:
	static void Initialize();
	static void Cleanup();

public:
	AttractWindow(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR *ldlBlock,
		sint32 bpp,
		AUI_WINDOW_TYPE type = AUI_WINDOW_TYPE_POPUP);

	~AttractWindow() override;

	virtual AUI_ERRCODE InitCommon();

	AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 ) override;
	AUI_ERRCODE Idle() override;
	void AppIdle();

public:
	void DrawAttractiveStuff();
	void ClearWindow();
	void HighlightControl(MBCHAR *ldlName);
	void RemoveControl(MBCHAR *ldlName);

	void RemoveRegion(aui_Region *region);
	void AddRegion(aui_Region *region);

private:
	RECT		m_screenAttractRect;
	POINT		m_attractPoint;
	sint32		m_attractStage;
	uint32		m_finishTime;

	PointerList<AttractRegion> m_regions;
};

// g_attractWindow demoted to file-scope `static` in AttractWindow.cpp.
// External callers go through attractwindow_Get() (returns NULL when the
// attract overlay has not been initialized).
AttractWindow * attractwindow_Get();

#endif
