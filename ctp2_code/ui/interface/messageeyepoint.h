#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __MESSAGE_EYEPOINT_H__
#define __MESSAGE_EYEPOINT_H__

class MessageEyePointDropdown;
class MessageEyePointListbox;
class MessageEyePointListItem;
class MessageEyePointStandard;

#include <memory>

#include "ui/aui_common/auitypes.h"       // AUI_ERRCODE
#include "ui/aui_ctp2/c3_listitem.h"
#include "os/include/ctp2_inttypes.h"  // sintN

class aui_Button;
class c3_DropDown;
class c3_Static;
class MessageData;
class MessageDropdownEyePointAction;
class MessageListboxEyePointAction;
class MessageModal;
class MessageWindow;
class MessageStandardEyePointAction;
// MBCHAR

class MessageEyePointListItem : public c3_ListItem
{
public:
	MessageEyePointListItem(AUI_ERRCODE *retval, MBCHAR const *name, sint32 index, MBCHAR const *ldlBlock);

	void Update() override;

	MBCHAR const * GetName( ) const { return m_name; }
	sint32 GetIndex( ) const { return m_index; }

	sint32 Compare(c3_ListItem *item2, uint32 column) override;

protected:
	MessageEyePointListItem() : c3_ListItem() {}

	AUI_ERRCODE InitCommonLdl(MBCHAR const * name, sint32 index, MBCHAR const * ldlBlock);

private:
	MBCHAR		m_name[_MAX_PATH];
	sint32		m_index;

};


class MessageEyePointStandard
{
public:
	MessageEyePointStandard( AUI_ERRCODE *retval,
							 MBCHAR const *ldlBlock,
							 MessageWindow *window );
	MessageEyePointStandard( AUI_ERRCODE *retval,
							 MBCHAR const *ldlBlock,
							 MessageModal *window );

	virtual ~MessageEyePointStandard();

	virtual AUI_ERRCODE InitCommon( MBCHAR const *ldlBlock, MessageWindow *window );
	virtual AUI_ERRCODE InitCommon( MBCHAR const *ldlBlock, MessageModal *window );

private:
	// The eye-point button and its action are both owned here; the parent
	// window's control list only borrows the button for layout/drawing.
	std::unique_ptr<aui_Button>						m_button;
	std::unique_ptr<MessageStandardEyePointAction>	m_action;
};


class MessageEyePointDropdown
{
public:
	MessageEyePointDropdown( AUI_ERRCODE *retval,
							 MBCHAR const *ldlBlock,
							 MessageWindow *window );
	MessageEyePointDropdown( AUI_ERRCODE *retval,
							 MBCHAR const *ldlBlock,
							 MessageModal *window );

	virtual ~MessageEyePointDropdown();

	virtual AUI_ERRCODE InitCommon(MBCHAR const * ldlBlock, MessageWindow *window );
	virtual AUI_ERRCODE InitCommon(MBCHAR const * ldlBlock, MessageModal *window );

private:
    AUI_ERRCODE InitCommonCommon
    (
        MBCHAR const *  ldlBlock,
        MessageData *   a_Message
    );

	std::unique_ptr<aui_Button>                     m_button;
	std::unique_ptr<c3_DropDown>                    m_dropdown;

	std::unique_ptr<MessageDropdownEyePointAction>  m_action;
	std::unique_ptr<MessageDropdownAction>          m_dropaction;
};


class MessageEyePointListbox
{
public:
	MessageEyePointListbox( AUI_ERRCODE *retval,
							MBCHAR const *ldlBlock,
							MessageWindow *window );
	MessageEyePointListbox( AUI_ERRCODE *retval,
							MBCHAR const *ldlBlock,
							MessageModal *window );

	virtual ~MessageEyePointListbox();

	virtual AUI_ERRCODE InitCommon( MBCHAR const *ldlBlock, MessageWindow *window );
	virtual AUI_ERRCODE InitCommon( MBCHAR const *ldlBlock, MessageModal *window );

private:
	// Left/right eye-point paging buttons and their paired actions; the
	// actions reference each other, the buttons live in the parent window.
	std::unique_ptr<aui_Button>						m_buttonLeft;
	std::unique_ptr<aui_Button>						m_buttonRight;
	std::unique_ptr<MessageListboxEyePointAction>	m_actionLeft;
	std::unique_ptr<MessageListboxEyePointAction>	m_actionRight;
};

#endif
