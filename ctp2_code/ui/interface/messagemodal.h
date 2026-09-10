//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : User interface modal message
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
// - Event handlers declared in a notation that is more standard C++.
// - #pragma once commented out.
//
//----------------------------------------------------------------------------

#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif

#ifndef __MESSAGEMODAL_H__
#define __MESSAGEMODAL_H__

#include <memory>
#include <vector>

#include "ui/interface/messagewindow.h"
#include "ui/aui_ctp2/c3_popupwindow.h"

#define k_MODAL_BUTTON_SPACING			4
#define k_MODAL_BUTTON_TEXT_PADDING			15
#define k_MODAL_BUTTON_DEFAULT_WIDTH		70

class aui_Static;
class aui_Button;
class aui_HyperTextBox;
class Message;

class ctp2_Button;

class MessageEyePointStandard;
class MessageEyePointDropdown;
class MessageEyePointListbox;

class MessageModalResponseAction;

class MessageModal : public c3_PopupWindow
{
public:
	MessageModal(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR *ldlBlock,
		sint32 bpp,
		Message data,
		AUI_WINDOW_TYPE type = AUI_WINDOW_TYPE_FLOATING );

	~MessageModal() override;

	virtual AUI_ERRCODE InitCommon( MBCHAR *ldlBlock, Message data );

	void BringBorderToTop();
	AUI_ERRCODE AddBordersToUI();
	AUI_ERRCODE RemoveBordersFromUI();

	Message				*GetMessage( ) { return &m_message; }

protected:
	void	MouseLGrabInside(aui_MouseEvent * mouseData) override;
	void	MouseLDragAway(aui_MouseEvent * mouseData) override;

	AUI_ERRCODE CreateWindowEdges( MBCHAR *ldlBlock );
	AUI_ERRCODE CreateResponses( MBCHAR *ldlBlock );
	AUI_ERRCODE CreateStandardTextBox( MBCHAR *ldlBlock );

	AUI_ERRCODE CreateEyePointBox( MBCHAR *ldlBlock );
	AUI_ERRCODE CreateStandardEyePointBox( MBCHAR *ldlBlock );
	AUI_ERRCODE CreateDropdownEyePointBox( MBCHAR *ldlBlock );
	AUI_ERRCODE CreateListboxEyePointBox( MBCHAR *ldlBlock );

private:
	std::unique_ptr<aui_HyperTextBox>	m_messageText;

	Message							m_message;

	// Exactly one eye-point helper is created (per the message's style);
	// unique_ptr members replace the old raw-pointer union and fix the
	// leak where ~MessageModal never freed it.
	std::unique_ptr<MessageEyePointStandard>	m_eyePointStandard;
	std::unique_ptr<MessageEyePointDropdown>	m_eyePointDropdown;
	std::unique_ptr<MessageEyePointListbox>		m_eyePointListbox;

	aui_Static						*m_leftBar;
	aui_Static						*m_rightBar;

	C3Window						*m_topBar;
	C3Window						*m_bottomBar;

	// Response buttons and their actions, in creation order; both owned
	// here even though the buttons are also registered with the window.
	std::vector<std::unique_ptr<ctp2_Button>>				m_responseButtons;
	std::vector<std::unique_ptr<MessageModalResponseAction>>	m_responseActions;

	POINT	m_offsetTop;
	POINT	m_offsetBottom;

};

int     messagemodal_CreateModalMessage(Message data);
void    messagemodal_DestroyModalMessage();
void    messagemodal_PrepareDestroyWindow();

#endif
