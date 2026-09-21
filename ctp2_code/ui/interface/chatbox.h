#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __CHATBOX_H__
#define __CHATBOX_H__

#include "ui/aui_utils/primitives.h"
#include "gfx/gfx_utils/colorset.h"
#include "ui/aui_ctp2/c3window.h"
#include <memory>

#define k_CHATBOX_LINE_LENGTH			60




















class aui_Surface;
class aui_BitmapFont;
class aui_Window;
class C3TextField;
class c3_HyperTextBox;
class ChatBox;

class ChatWindow : public C3Window
{
public:
	ChatWindow
    (
		AUI_ERRCODE *   retval,
		uint32          id,
		MBCHAR *        ldlBlock,
		sint32          bpp,
		AUI_WINDOW_TYPE type    = AUI_WINDOW_TYPE_STANDARD,
        ChatBox *       parent  = nullptr
    );
	~ChatWindow() override;

	virtual AUI_ERRCODE InitCommonLdl(MBCHAR *ldlBlock);
	AUI_ERRCODE DrawThis(
		aui_Surface *surface = nullptr,
		sint32 x = 0,
		sint32 y = 0 ) override;

	BOOL CheckForEasterEggs(MBCHAR *s);

	c3_HyperTextBox	*   GetTextBox() const { return m_textBox.get(); }
	C3TextField	*       GetTextField() const { return m_textField.get(); }
	ChatBox	*           GetChatBox() const { return m_chatBox; }

	void				ColorizeString(MBCHAR *destString, size_t destSize, MBCHAR *srcString, COLORREF colorRef);

	static void ChatCallback(aui_Control *control, uint32 action, uint32 data, void *cookie) ;

private:
	std::unique_ptr<c3_HyperTextBox>	m_textBox;
	std::unique_ptr<C3TextField>		m_textField;
	ChatBox				*m_chatBox;
};

class ChatBox {
public:
	static void Initialize();
	static void Cleanup();

	ChatBox();
	~ChatBox();

	BOOL IsActive() { return m_active; }
	void SetActive(BOOL active);

	void AddText(MBCHAR *text);
	void AddLine(sint32 playerNum, MBCHAR *text);

private:

	std::unique_ptr<ChatWindow>	m_chatWindow;
	BOOL				m_active;
};

ChatBox * chatbox_Get();
void      chatbox_Set(ChatBox *p);

#endif
