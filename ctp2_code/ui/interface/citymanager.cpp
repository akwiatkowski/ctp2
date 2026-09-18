#include "ctp/c3.h"

#include <memory>
#include "ui/aui_common/aui.h"
#include "ui/aui_common/aui_window.h"
#include "ui/aui_common/aui_ui.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_common/aui_blitter.h"

#include "ui/aui_ctp2/ctp2_button.h"

#include "ui/interface/UIUtils.h"

#include "ui/interface/citymanager.h"
#include "ui/aui_ctp2/c3ui.h"

#include "gs/database/StrDB.h"


static std::unique_ptr<CityManagerWindow> s_cityManagerWindow;

void CityManagerWindow::Open()
{
	if (!s_cityManagerWindow)
    {
	    AUI_ERRCODE err = AUI_ERRCODE_OK;
		s_cityManagerWindow.reset(new CityManagerWindow(&err,
													aui_UniqueId(),
													const_cast<MBCHAR *>("CITY_MANAGER_WINDOW")));
		Assert(err == AUI_ERRCODE_OK);

		c3ui_Get()->AddWindow(s_cityManagerWindow.get());
	}

	s_cityManagerWindow->Show();
}

void CityManagerWindow::Cleanup()
{
	if (s_cityManagerWindow)
    {
		s_cityManagerWindow->Hide();
        if (c3ui_Get())
        {
		    c3ui_Get()->RemoveWindow(s_cityManagerWindow->Id());
        }
	}

    s_cityManagerWindow.reset();
}

CityManagerWindow::CityManagerWindow(AUI_ERRCODE *retval,
									 uint32 id,
									 MBCHAR *ldlBlock)
	: aui_Window(retval, id,
				 uiutils_ChooseLdl(ldlBlock, const_cast<MBCHAR *>("CITY_MANAGER_WINDOW")),
				 16,
				 AUI_WINDOW_TYPE_STANDARD)
{
	m_bg = nullptr;
	ldlBlock = uiutils_ChooseLdl(ldlBlock, const_cast<MBCHAR *>("CITY_MANAGER_WINDOW"));

	*retval = InitCommonLdl(ldlBlock);

}

CityManagerWindow::~CityManagerWindow()
{
	m_ok.reset();
	m_cancel.reset();

	if(m_bg) {
		aui_ui_Get()->UnloadImage(m_bg);
		m_bg = nullptr;
	}
}

void CityManagerWindowButtonCallback(aui_Control *control, uint32 action, uint32 data, void *cookie)
{
	if(action != (uint32)AUI_BUTTON_ACTION_EXECUTE) return;
	if(s_cityManagerWindow)
		s_cityManagerWindow->Hide();
}

AUI_ERRCODE CityManagerWindow::InitCommonLdl(MBCHAR *ldlBlock)
{
    if (!aui_Ldl::IsValid(ldlBlock))
    {
		return AUI_ERRCODE_HACK;
	}

	MBCHAR controlBlock[k_AUI_LDL_MAXBLOCK + 1];
	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", ldlBlock, "OK_BUTTON");

	AUI_ERRCODE ret;
	m_ok.reset(new ctp2_Button(&ret, aui_UniqueId(), controlBlock,
						   "CTP2_BUTTON_TEXT_RIGHT_LARGE",
						   386, 414,
						   100, 20,

						   CityManagerWindowButtonCallback,
						   this));
	AddControl(m_ok.get());

	snprintf(controlBlock, sizeof(controlBlock), "%s.%s", ldlBlock, "CANCEL_BUTTON");
	m_cancel.reset(new ctp2_Button(&ret, aui_UniqueId(), controlBlock,
							   "CTP2_BUTTON_TEXT_RIGHT_LARGE",
							   526, 414,
							   100, 20,

							   CityManagerWindowButtonCallback,
							   this));
	AddControl(m_cancel.get());


	m_bg = aui_ui_Get()->LoadImage("CM.tga");

	Assert(m_bg);
	if(m_bg) {
		Resize(m_bg->TheSurface()->Width(), m_bg->TheSurface()->Height());
	}

	SetDraggable(TRUE);

	return AUI_ERRCODE_OK;
}

AUI_ERRCODE CityManagerWindow::DrawThis(aui_Surface *surface, sint32 x, sint32 y)
{
	if(IsHidden()) return AUI_ERRCODE_OK;

	if(!surface) surface = m_surface;

	AUI_ERRCODE err = AUI_ERRCODE_OK;

	if(m_bg) {
		aui_Surface *srcSurf = m_bg->TheSurface();
		RECT srcRect = {0, 0, srcSurf->Width(), srcSurf->Height()};

		err =
			aui_ui_Get()->TheBlitter()->Blt(surface,
									0, 0,
									srcSurf,
									&srcRect,
									k_AUI_BLITTER_FLAG_COPY);
	}

	RECT rect = {0,0,m_width,m_height};

	if(surface == m_surface)
		AddDirtyRect(&rect);

	return err;
}
