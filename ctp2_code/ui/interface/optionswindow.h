#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef OPTIONSWINDOW_FLAG
#define OPTIONSWINDOW_FLAG

#include <memory>
#include "ui/aui_ctp2/c3_popupwindow.h"

class c3_Static;
class c3_Button;
class ctp2_Button;

sint32 optionsscreen_displayMyWindow(sint32 from);
sint32 optionsscreen_removeMyWindow(uint32 action);
AUI_ERRCODE optionsscreen_Initialize( );
void optionsscreen_Cleanup();

void optionsscreen_graphicsPress(aui_Control *control, uint32 action, uint32 data, void *cookie );
void optionsscreen_soundPress(aui_Control *control, uint32 action, uint32 data, void *cookie );
void optionsscreen_musicPress(aui_Control *control, uint32 action, uint32 data, void *cookie );
void optionsscreen_newgamePress(aui_Control *control, uint32 action, uint32 data, void *cookie );
void optionsscreen_savegamePress(aui_Control *control, uint32 action, uint32 data, void *cookie );
void optionsscreen_savescenarioPress(aui_Control *control, uint32 action, uint32 data, void *cookie );
void optionsscreen_loadgamePress(aui_Control *control, uint32 action, uint32 data, void *cookie );
void optionsscreen_restartPress(aui_Control *control, uint32 action, uint32 data, void *cookie );
void optionsscreen_quitPress(aui_Control *control, uint32 action, uint32 data, void *cookie );
void optionsscreen_returnPress(aui_Control *control, uint32 action, uint32 data, void *cookie );
void optionsscreen_gameplayPress(aui_Control *control, uint32 action, uint32 data, void *cookie );
void optionsscreen_mapeditorPress(aui_Control *control, uint32 action, uint32 data, void *cookie );
void optionsscreen_keyboardPress(aui_Control *control, uint32 action, uint32 data, void *cookie );
void optionsscreen_quitToShellPress(aui_Control *control, uint32 action, uint32 data, void *cookie );

class OptionsWindow : public c3_PopupWindow
{
public:
	OptionsWindow(
		AUI_ERRCODE *retval,
		uint32 id,
		MBCHAR *ldlBlock,
		sint32 bpp,
		AUI_WINDOW_TYPE type = AUI_WINDOW_TYPE_STANDARD,
		bool bevel = true);
	~OptionsWindow() override;

	sint32 EnableButtons( );
	sint32 DisableButtons( );

	void RemoveQuitToWindowsButton( );
	void AddQuitToWindowsButton( );

	ctp2_Button *SaveGameButton() const { return m_savegame.get(); }
	ctp2_Button *LoadGameButton() const { return m_loadgame.get(); }
	ctp2_Button *QuitToShellButton() const { return m_quittoshell.get(); }

	ctp2_Button *GraphicsButton() const { return m_graphics.get(); }
	ctp2_Button *SoundButton() const { return m_sound.get(); }
	ctp2_Button *MusicButton() const { return m_music.get(); }
	ctp2_Button *NewGameButton() const { return m_newgame.get(); }
	ctp2_Button *RestartButton() const { return m_restart.get(); }
	ctp2_Button *GamePlayButton() const { return m_gameplay.get(); }
	ctp2_Button *MapEditorButton() const { return m_mapeditor.get(); }
	ctp2_Button *KeyboardButton() const { return m_keyboard.get(); }

private:

	// One declaration per control, so each can carry its own ownership.
	std::unique_ptr<ctp2_Button>	m_graphics;
	std::unique_ptr<ctp2_Button>	m_sound;
	std::unique_ptr<ctp2_Button>	m_music;
	std::unique_ptr<ctp2_Button>	m_newgame;
	std::unique_ptr<ctp2_Button>	m_savegame;
	std::unique_ptr<ctp2_Button>	m_loadgame;
	std::unique_ptr<ctp2_Button>	m_restart;
	std::unique_ptr<ctp2_Button>	m_gameplay;
	std::unique_ptr<ctp2_Button>	m_mapeditor;
	std::unique_ptr<ctp2_Button>	m_keyboard;
	std::unique_ptr<ctp2_Button>	m_quittoshell;

	std::unique_ptr<c3_Static>	m_configHeader;
	std::unique_ptr<c3_Static>	m_gameHeader;

};

#endif
