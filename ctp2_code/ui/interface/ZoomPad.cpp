#include "ctp/c3.h"

#include "ui/interface/ZoomPad.h"

#include "gfx/tilesys/tiledmap.h"
#include "ui/aui_common/aui_ldl.h"
#include "ui/aui_ctp2/ctp2_button.h"

ZoomPad::ZoomPad(MBCHAR *ldlBlock) :
m_zoomIn(static_cast<ctp2_Button*>(aui_Ldl::GetObject(ldlBlock, "ZoomPad.ZoomInButton"))),
m_zoomOut(static_cast<ctp2_Button*>(aui_Ldl::GetObject(ldlBlock, "ZoomPad.ZoomOutButton")))
{

	Assert(m_zoomIn);
	Assert(m_zoomOut);

	m_zoomIn->SetActionFuncAndCookie(ZoomInButtonActionCallback, nullptr);
	m_zoomOut->SetActionFuncAndCookie(ZoomOutButtonActionCallback, nullptr);

	Update();
}


void ZoomPad::Update()
{

	if(tiledmap_Get()) {

		m_zoomIn->Enable(tiledmap_Get()->CanZoomIn());
		m_zoomOut->Enable(tiledmap_Get()->CanZoomOut());
	}
}

void ZoomPad::ZoomInButtonActionCallback(aui_Control *control, uint32 action,
										 uint32 data, void *cookie)
{

	if(action != static_cast<uint32>(AUI_BUTTON_ACTION_EXECUTE))
		return;

	if(tiledmap_Get()) {

		tiledmap_Get()->ZoomIn();
	}
}

void ZoomPad::ZoomOutButtonActionCallback(aui_Control *control, uint32 action,
										  uint32 data, void *cookie)
{

	if(action != static_cast<uint32>(AUI_BUTTON_ACTION_EXECUTE))
		return;

	if(tiledmap_Get()) {

		tiledmap_Get()->ZoomOut();
	}
}
