#ifndef CITY_MANAGER_H__
#define CITY_MANAGER_H__

#include <memory>

class CityManagerWindow : public aui_Window
{
private:
	// Owned here; the window's control list borrows them for drawing.
	std::unique_ptr<ctp2_Button> m_ok, m_cancel;

	// Registry-owned: released via aui_ui_Get()->UnloadImage, never delete.
	aui_Image *m_bg;

public:
	CityManagerWindow(AUI_ERRCODE *retval,
					  uint32 id,
					  MBCHAR *ldlBlock);
	~CityManagerWindow() override;

	AUI_ERRCODE InitCommonLdl(MBCHAR *ldlBlock);
	AUI_ERRCODE DrawThis(aui_Surface *surface, sint32 x, sint32 y) override;

	static void Open();
	static void Cleanup();
};

#endif
