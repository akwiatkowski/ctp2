#ifndef __NS_CHATBOX_H__
#define __NS_CHATBOX_H__

#include "ui/aui_ctp2/textbox.h"
#include <memory>
#include "ui/aui_common/aui_action.h"
#include "ui/netshell/netfunc.h"

class aui_TextField;
class aui_Button;


class ns_ChatBox : public TextBox, public NETFunc::Chat
{
	NETFunc::Player *player;
	bool bWhisper;
	bool bGroup;
public:

	ns_ChatBox(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR *ldlBlock,
		ControlActionCallback *ActionFunc = nullptr,
		void *cookie = nullptr );
	~ns_ChatBox() override;

	NETFunc::Player *GetPlayer();
	void SetPlayer(NETFunc::Player *p);
	bool IsWhisper();
	void SetWhisper(bool w);
	bool IsGroup();
	void SetGroup(bool g);

	void Receive(NETFunc::Player *p, TYPE t, char *m) override;

	AUI_ERRCODE RepositionItems( ) override;
	AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 ) override;

protected:
	std::unique_ptr<aui_TextBase>	m_textstyleSystem;
	std::unique_ptr<aui_TextBase>	m_textstyleChat;
	std::unique_ptr<aui_TextBase>	m_textstyleWhisper;

	AUI_ERRCODE InitCommonLdl( MBCHAR *ldlBlock );
	AUI_ERRCODE InitCommon( );
	AUI_ERRCODE	CreateComponents( );

public:
	aui_TextField	*GetInputField( ) const { return m_inputField.get(); }

protected:
	std::unique_ptr<aui_TextField>	m_inputField;

	AUI_ACTION_BASIC(InputFieldAction);
};

#endif
