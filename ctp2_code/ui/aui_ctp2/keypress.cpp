//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Key press handling
// Id           : $Id$
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
// _PLAYTEST
// - Generates version with utilities for playtesting
//
// _DEBUG
// - Generate debug version when set.
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Added music screen key by ahenobarb.
// - Start the great library with the current research project of the player.
// - Disabled restart key in network, hot seat and email gmase, by
//   Martin G�hmann.
// - Opening the score tab of the info window does not close other windows
//   anymore like the other tabs. - Aug 7th 2005 Martin G�hmann
// - Strongly modal windows like the DipWizzard cannot closed anymore by
//   by keypresses that open other windows. (20-10-2007 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"

#include "gfx/gfx_utils/pixelutils.h"
#include "gfx/spritesys/UnitSpriteGroup.h"

#include "ui/aui_common/aui.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ui/aui_common/aui_window.h"
#include "ui/interface/radarwindow.h"
#include "ui/aui_ctp2/statuswindow.h"
#include "ui/aui_utils/primitives.h"
#include "ui/aui_ctp2/pattern.h"

#include "ctp/ctp2_utils/c3debug.h"
#include "ctp/ctp2_utils/c3cmdline.h"

#include "ui/aui_ctp2/keymap.h"

#include "gs/gameobj/player.h"
#include "gs/fileio/gamefile.h"

#include "net/general/network.h"
#include "net/general/net_action.h"
#include "net/general/net_info.h"
#include "net/general/net_rand.h"

#include "gfx/tilesys/tiledmap.h"
#include "ui/aui_ctp2/background.h"
#include "gs/gameobj/XY_Coordinates.h"
#include "gs/world/World.h"
#include "ctp/civ3_main.h"
#include "gs/gameobj/pollution.h"
#include "ui/aui_ctp2/SelItem.h"
#include "gs/utility/newturncount.h"
#include "gs/utility/TurnCnt.h"
#include "gs/outcom/AICause.h"
#include "ui/aui_ctp2/radarmap.h"
#include "gs/utility/DataCheck.h"

#include "ui/interface/km_screen.h"
#include "ui/interface/screenutils.h"
#include "ui/interface/optionswindow.h"
#include "gfx/tilesys/resourcemap.h"

#include "ctp/civapp.h"
#include "gs/gameobj/Order.h"

#include "ui/interface/controlpanelwindow.h"
#include "ui/aui_ctp2/c3_listbox.h"
#include "ui/aui_common/aui_ranger.h"

#include "ui/interface/loadsavewindow.h"
#include "gs/gameobj/ArmyPool.h"
#include "gs/gameobj/GameSettings.h"
#include "ui/interface/messagemodal.h"
#include "gs/gameobj/MessagePool.h"
#include "gs/slic/SlicButton.h"
#include "ui/interface/messagewindow.h"
#include "ui/interface/messageactions.h"
#include "ui/interface/workwindow.h"
#include "ui/interface/workwin.h"

#include "gs/slic/SlicEngine.h"

#include "ui/slic_debug/segmentlist.h"

#include "gs/gameobj/UnitData.h"
#include "gs/gameobj/citydata.h"

#include "gs/events/GameEventManager.h"
#include "gs/gameobj/CTP2Combat.h"
#include "ui/interface/battle.h"

#include "ui/interface/armymanagerwindow.h"
#include "ui/aui_ctp2/ctp2_Window.h"
#include "ui/interface/optionwarningscreen.h"

#include "ui/interface/dipwizard.h"
#include "ui/aui_ctp2/keypress.h"
#include "ui/interface/sciencevictorydialog.h"
#include "ui/interface/citywindow.h"
#include "ui/interface/infowindow.h"
#include "ui/interface/cityespionage.h"
#include "ui/interface/trademanager.h"
#include "ui/interface/EditQueue.h"
#include "ui/interface/soundscreen.h"
// music added by ahenobarb
#include "ui/interface/musicscreen.h"
#include "ui/interface/graphicsscreen.h"
#include "ui/interface/gameplayoptions.h"
#include "OrderRecord.h"
#include "ui/interface/ProfileEdit.h"
#include "gs/world/cellunitlist.h"
#include "gs/gameobj/ArmyData.h"
#include "ui/interface/MainControlPanel.h"
#include "ui/interface/UnitControlPanel.h"

extern BOOL			gSuspended;


extern BOOL			g_helpMode;



extern HWND gHwnd;

extern sint32		g_is_rand_test;


#ifdef _PLAYTEST
#endif

KEYMAP *theKeyMap = nullptr;

extern DataCheck	*g_DataCheck;
extern sint32		g_debugOwner;


extern sint32		g_isKMScreen;




#include "gs/database/profileDB.h"

#include "gfx/spritesys/director.h"

#include "ui/aui_ctp2/c3_utilitydialogbox.h"
c3_UtilityPlayerListPopup *g_networkPlayersScreen = nullptr;
extern c3_UtilityTextMessagePopup		*g_utilityTextMessage;

#include "ui/interface/chatbox.h"

#include "gs/database/StrDB.h"

#include "ui/aui_ctp2/keyboardhandler.h"

extern MessageModal *g_modalMessage;
extern MessageWindow	*g_currentMessageWindow;

extern sint32 g_modalWindow;

#include "ui/interface/debugwindow.h"
extern DebugWindow		*g_debugWindow;

#include "ui/interface/battleviewwindow.h"

PointerList<KeyboardHandler> g_keyboardHandlers;

void keypress_QuitCallback( sint32 val )
{
	if ( val ) {
	static SDL_Event quit = { 0 };
        quit.type = SDL_QUIT;
        quit.quit.type = SDL_QUIT;
        SDL_PushEvent(&quit);
	}
}

void keypress_RegisterHandler(KeyboardHandler *handler)
{
	g_keyboardHandlers.AddTail(handler);
}

void keypress_RemoveHandler(KeyboardHandler *handler)
{
	PointerList<KeyboardHandler>::Walker walk(&g_keyboardHandlers);
	while(walk.IsValid()) {
		if(walk.GetObj() == handler) {
			walk.Remove();
		} else {
			walk.Next();
		}
	}
}

void init_keymap() {
	if ( !theKeyMap )
		theKeyMap = new KEYMAP(FALSE);

}

void init_defaultKeymap() {
	if(!theKeyMap)
		theKeyMap = new KEYMAP(TRUE);
}

void cleanup_keymap()
{
	delete theKeyMap;
	theKeyMap = nullptr;
}
































sint32 g_keypress_stop_player;

BOOL	commandMode = FALSE;

sint32 ui_HandleKeypress(WPARAM wParam, LPARAM lParam)
{
    WORLD_DIRECTION	d;
    BOOL			move = FALSE;
#ifdef _PLAYTEST
    int i;
#endif
	BOOL isMyTurn = !network_Get().IsActive() || network_Get().IsMyTurn();
	Unit city;

	if ( g_isKMScreen) {
		switch(wParam) {
			case VK_ESCAPE:
			case VK_LEFT + 256:
			case VK_RIGHT + 256:
			case VK_UP + 256:
			case VK_DOWN + 256:
			case 29:
			case 28:

				return TRUE;
			default:

				km_screen_remapKey( wParam, lParam );
				return TRUE;
		}
	}

#ifdef _PLAYTEST
	if(commandMode) {
		switch(wParam) {
			case '\r' + 128: wParam = '\r'; break;
			case '\t' + 128: wParam = '\t'; break;
			case 8 + 128: wParam = 8; break;
		}
		commandMode = command_line_Get().AddKey(static_cast<char>(wParam));
		return TRUE;
	}
#endif







	if (wParam == VK_ESCAPE) {

		extern OptionsWindow *g_optionsWindow;

		if(c3ui_Get()->TopWindow() && c3ui_Get()->TopWindow() == DipWizard::GetWindow()) {

		} else if(g_keyboardHandlers.GetTail()) {
			g_keyboardHandlers.GetTail()->kh_Close();
		} else if (civapp_Get()->IsGameLoaded()) {
			if(g_currentMessageWindow &&
			   g_currentMessageWindow->GetMessage() &&
			   (messagepool_Get()->IsValid(*g_currentMessageWindow->GetMessage()))) {
				g_currentMessageWindow->GetMessage()->Minimize();
			} else if(g_modalMessage) {

				Message *msg = g_modalMessage->GetMessage();
				if(msg && messagepool_Get()->IsValid(*msg)) {
					Assert(msg->IsAlertBox());
					MessageData *data = msg->AccessData();
					if(data->GetNumButtons() <= 2 && data->GetNumButtons() > 0) {
						data->GetButton(0)->Callback();
					}
				}
			} else if (g_optionsWindow && c3ui_Get()->GetWindow(g_optionsWindow->Id())) {

				optionsscreen_removeMyWindow(AUI_BUTTON_ACTION_EXECUTE);
			} else if(c3ui_Get()->TopWindow() && c3ui_Get()->TopWindow()->HandleKey(wParam)) {

			} else if(battleviewwindow_Get()) {
				battleview_ExitButtonActionCallback( nullptr, AUI_BUTTON_ACTION_EXECUTE, 0, nullptr);
			} else {

				optionsscreen_Initialize();
				optionsscreen_displayMyWindow(1);
			}
		}
		return TRUE;
	}

	aui_Window *topWindow = c3ui_Get()->TopWindow();
	if(topWindow && (!controlpanel_Get() || topWindow != controlpanel_Get()->GetWindow()) && topWindow != statuswindow_Get()) {
		if(topWindow->HandleKey(wParam))
			return 0;
	}


	if (!theKeyMap) return 0;

	if (slicengine_Get() && slicengine_Get()->RunKeyboardTrigger(static_cast<char>(wParam)))
	{
		return 0;
	}

	if (!civapp_Get()->IsGameLoaded()) {
		return TRUE;
	}

	KEY_FUNCTION	kf = theKeyMap->get_function(wParam);
	if (kf != KEY_FUNCTION_NOOP)
	{
		selitem_Get()->RegisterUIClick();
	}

	if (topWindow->IsStronglyModal() /*&& keypress_IsGameFunction(kf)*/) // I should not be able to open other windows if I have have open strongly modal windows like the DipWizzard
	{
		return 0;
	}


	switch (kf) {
#ifdef _PLAYTEST
	case KEY_FUNCTION_ENTER_COMMAND:
		segmentlist_Display();
		break;
	case KEY_FUNCTION_ENTER_COMMAND_ALTERNATE:
		commandMode = TRUE;
		move = FALSE;
		break;
#endif
    case KEY_FUNCTION_NOOP: return FALSE;
    case KEY_FUNCTION_MOVE_NORTHWEST: d = NORTHWEST; move = TRUE; break;
    case KEY_FUNCTION_MOVE_NORTH: d = NORTH; move = TRUE; break;
    case KEY_FUNCTION_MOVE_NORTHEAST: d = NORTHEAST; move = TRUE; break;
    case KEY_FUNCTION_MOVE_WEST: d = WEST; move = TRUE; break;
    case KEY_FUNCTION_MOVE_EAST: d = EAST; move = TRUE; break;
    case KEY_FUNCTION_MOVE_SOUTHWEST: d = SOUTHWEST; move = TRUE; break;
    case KEY_FUNCTION_MOVE_SOUTH: d = SOUTH; move = TRUE; break;
    case KEY_FUNCTION_MOVE_SOUTHEAST: d = SOUTHEAST; move = TRUE; break;

	case KEY_FUNCTION_ENTRENCH:
		if(isMyTurn) selitem_Get()->Entrench();
		break;









	case KEY_FUNCTION_UNLOAD_TRANS:
	{
		sint32 order;
		if(!g_theOrderDB->GetNamedItem("ORDER_UNLOAD", order))
			return 0;

		Army a;
		if(selitem_Get()->GetSelectedArmy(a))
			controlpanel_Get()->BeginOrderDelivery(g_theOrderDB->Access(order));
	}
	break;
	case KEY_FUNCTION_SETTLE:
	{
		selitem_Get()->Settle();
		move = FALSE;
		break;
	}
	case KEY_FUNCTION_SPACE_LAUNCH:
		if(isMyTurn) {
			sint32 index;
			if(g_theOrderDB->GetNamedItem("ORDER_SPACE_LAUNCH", index)) {
				controlpanel_Get()->BeginOrderDelivery(g_theOrderDB->Access(index));
			}

			return TRUE;
		}
		break;
	case KEY_FUNCTION_DESCEND:
		selitem_Get()->Descend();
		break;
	case KEY_FUNCTION_PILLAGE:
	{
		sint32 order;
		if(!g_theOrderDB->GetNamedItem("ORDER_PILLAGE", order))
			return 0;

		Army a;
		if(selitem_Get()->GetSelectedArmy(a) && a.CanPillage()) {
			controlpanel_Get()->BeginOrderDelivery(g_theOrderDB->Access(order));
		} else {
			selitem_Get()->Pillage();
			return 0;
		}
	}
		break;
	case KEY_FUNCTION_BOMBARD:
	{
		sint32 order;
		if(!g_theOrderDB->GetNamedItem("ORDER_BOMBARD", order))
			return 0;

		Army a;
		if(selitem_Get()->GetSelectedArmy(a) && a.CanBombard()) {
			controlpanel_Get()->BeginOrderDelivery(g_theOrderDB->Access(order));
		} else {
			return 0;
		}
	}





		break;
	case KEY_FUNCTION_EXPEL:
	{
		sint32 order;
		if(!g_theOrderDB->GetNamedItem("ORDER_EXPEL", order))
			return 0;

		Army a;
		if(selitem_Get()->GetSelectedArmy(a) && a.CanExpel()) {
			controlpanel_Get()->BeginOrderDelivery(g_theOrderDB->Access(order));
		} else {
			return 0;
		}
	}




		break;
	case KEY_FUNCTION_SLEEP:
		if(isMyTurn) {
			move = FALSE;

			selitem_Get()->Sleep();
		}
		break;
    case KEY_FUNCTION_NEXT_ITEM:
        selitem_Get()->NextItem();
		move = FALSE;
        break;

	case KEY_FUNCTION_PARADROP:
		if(isMyTurn) {
			MapPoint point;
			tiledmap_Get()->GetMouseTilePos(point);
			selitem_Get()->Paradrop(point);
			return TRUE;
		}
		break;

	case KEY_FUNCTION_INVESTIGATE_CITY:
        {
            MapPoint    point;
            tiledmap_Get()->GetMouseTilePos(point);
		    selitem_Get()->InvestigateCity(point);
        }
		break;

	case KEY_FUNCTION_PLANT_NUKE:
	{
		sint32 order;
		if(!g_theOrderDB->GetNamedItem("ORDER_PLANT_NUKE", order))
			return 0;

		Army a;
		double chance;
		double escape_chance;
		if(selitem_Get()->GetSelectedArmy(a) && a.CanPlantNuke(chance, escape_chance)) {
			controlpanel_Get()->BeginOrderDelivery(g_theOrderDB->Access(order));
		} else {
			return 0;
		}
	}

		break;
	case KEY_FUNCTION_BIOINFECT:
	{
		sint32 order;
		if(!g_theOrderDB->GetNamedItem("ORDER_BIO_INFECT", order))
			return 0;

		Army a;
		double chance;
		if(selitem_Get()->GetSelectedArmy(a) && a.CanBioInfect(chance)) {
			controlpanel_Get()->BeginOrderDelivery(g_theOrderDB->Access(order));
		} else {
			return 0;
		}
	}

		break;
	case KEY_FUNCTION_NANOTERROR:
	{
		sint32 order;
		if(!g_theOrderDB->GetNamedItem("ORDER_NANO_INFECT", order))
			return 0;

		Army a;
		double chance;
		if(selitem_Get()->GetSelectedArmy(a) && a.CanNanoInfect(chance)) {
			controlpanel_Get()->BeginOrderDelivery(g_theOrderDB->Access(order));
		} else {
			return 0;
		}
	}

		break;
	case KEY_FUNCTION_CREATE_PARK:
	{
		sint32 order;
		if(!g_theOrderDB->GetNamedItem("ORDER_CREATE_PARK", order))
			return 0;

		Army a;
		if(selitem_Get()->GetSelectedArmy(a) && a.AccessData()->CanCreatePark()) {
			controlpanel_Get()->BeginOrderDelivery(g_theOrderDB->Access(order));
		} else {
			return 0;
		}
	}

		break;
	case KEY_FUNCTION_REFORM:
	{
		sint32 order;
		if(!g_theOrderDB->GetNamedItem("ORDER_REFORM", order))
			return 0;

		Army a;
		if(selitem_Get()->GetSelectedArmy(a) && a.AccessData()->CanReformCity()) {
			controlpanel_Get()->BeginOrderDelivery(g_theOrderDB->Access(order));
		} else {
			return 0;
		}
	}

		break;

	case KEY_FUNCTION_OPEN_WORK_VIEW:
		ArmyManagerWindow::Toggle();
		break;
	case KEY_FUNCTION_OPEN_CITY_VIEW:
		if(!g_modalWindow) {
			close_AllScreens();
			open_CityView();
		}
		break;
	case KEY_FUNCTION_OPEN_CITY_STATUS:
		if ( !g_modalWindow ) {
			close_AllScreens();
			open_CityStatus();
		}
		break;
	case KEY_FUNCTION_OPEN_CIV_STATUS:
		if ( !g_modalWindow ) {
			close_AllScreens();
			open_CivStatus();
		}
		break;
	case KEY_FUNCTION_OPEN_SCIENCE_STATUS:
		if ( !g_modalWindow ) {
			close_AllScreens();
			open_ScienceStatus();
		}
		break;
	case KEY_FUNCTION_OPEN_UNIT_STATUS:
		if ( !g_modalWindow ) {
			close_AllScreens();
			open_UnitStatus();
		}
		break;
	case KEY_FUNCTION_OPEN_TRADE_STATUS:
		if ( !g_modalWindow ) {
			close_AllScreens();
			TradeManager::Display();
			TradeManager::SetMode(TRADE_MANAGER_MARKET);

		}
		break;
	case KEY_FUNCTION_OPEN_DIPLOMACY:
		if ( !g_modalWindow ) {
			close_AllScreens();
			open_Diplomacy();
		}
		break;
	case KEY_FUNCTION_OPEN_INFO_SCREEN:
		if ( !g_modalWindow ) {
			InfoWindow::SelectScoreTab();
			InfoWindow::Open();
		}
		break;
	case KEY_FUNCTION_OPEN_GREAT_LIBRARY:
		if ( !g_modalWindow ) {
			close_AllScreens();
			open_GreatLibrary();
		}
		break;
	case KEY_FUNCTION_OPEN_OPTIONS_SCREEN:
		if ( !g_modalWindow ) {
			close_AllScreens();
			open_OptionsScreen(1);
		}
		break;

	case KEY_FUNCTION_OPEN_SCENARIO_EDITOR:
		if(!g_modalWindow
		&& !turn_Get()->IsEmail()
		){
			close_AllScreens();
			optionsscreen_mapeditorPress(nullptr, AUI_BUTTON_ACTION_EXECUTE, 0, nullptr);
		}
		break;


    case KEY_FUNCTION_ZOOM_IN1:
        if (tiledmap_Get()) {
			tiledmap_Get()->ZoomIn();
		}
		break;

    case KEY_FUNCTION_ZOOM_OUT1:
        if (tiledmap_Get()) {
			tiledmap_Get()->ZoomOut();
        }

		break;

	case KEY_FUNCTION_CENTER_MAP:
	{
		PLAYER_INDEX s_player;
		ID s_item;
		SELECT_TYPE s_state;
		selitem_Get()->GetTopCurItem(s_player, s_item, s_state);

        MapPoint pos;
		switch(s_state) {
			case SELECT_TYPE_LOCAL_ARMY:
			{
				Army army(s_item);
				army.GetPos(pos);
				radar_map_Get()->CenterMap(pos);
				tiledmap_Get()->Refresh();
				tiledmap_Get()->InvalidateMap();
				break;
			}
			case SELECT_TYPE_LOCAL_CITY:
			{
				Unit unit(s_item);
				unit.GetPos(pos);
				radar_map_Get()->CenterMap(pos);
				tiledmap_Get()->Refresh();
				tiledmap_Get()->InvalidateMap();

				break;
			}
			default:
				break;
		}
	}
	break;

	case KEY_FUNCTION_SAVE_GAME:
		if(network_Get().IsActive()) {
			if(network_Get().IsHost()) {

				is_scenario_Set(FALSE);
				loadsavescreen_displayMyWindow(LSS_SAVE_MP);
			}
		} else {

			is_scenario_Set(FALSE);

			loadsavescreen_displayMyWindow(LSS_SAVE_GAME);
		}
		break;
	case KEY_FUNCTION_LOAD_GAME:
		if(!network_Get().IsActive()) {
			loadsavescreen_displayMyWindow(LSS_LOAD_GAME);
		}
		break;
	case KEY_FUNCTION_REMAP_KEYBOARD:
		open_KeyMappingScreen();
		break;

    case KEY_FUNCTION_KEYBOARD_SELECT_UNIT:
         selitem_Get()->KeyboardSelectFirstUnit();
		 move = FALSE;
		 break;

#ifdef _PLAYTEST
    case KEY_FUNCTION_RAND_TEST:
       g_is_rand_test=TRUE;
         return TRUE;
		 break;
#endif

    case KEY_FUNCTION_ENDTURN:
#ifdef _PLAYTEST
        if (selitem_Get()->GetCurPlayer() != selitem_Get()->GetVisiblePlayer())
           break;

		if(network_Get().IsActive()) {
			turn_Get()->NetworkEndTurn();
		} else {
            selitem_Get()->Deselect(selitem_Get()->GetCurPlayer());




			NewTurnCount::StartNextPlayer(true);












			tiledmap_Get()->InvalidateMix();
			tiledmap_Get()->InvalidateMap();
			tiledmap_Get()->Refresh();
			radar_map_Get()->Update();
			turn_Get()->InformMessages();
		}
		move = FALSE;
        break;
#endif

    case KEY_FUNCTION_NEXT_ROUND:


		if(g_modalMessage) {
			Message *msg = g_modalMessage->GetMessage();
			if(messagepool_Get()->IsValid(*msg)) {
				Assert(msg->IsAlertBox());
				MessageData *data = msg->AccessData();
				if(data->GetNumButtons() <= 2 && data->GetNumButtons() > 0) {
					data->GetButton(0)->Callback();
				}
			}
		} else if(g_utilityTextMessage) {
			g_utilityTextMessage->m_callback(FALSE);
			g_utilityTextMessage->RemoveWindow();
		} else {

			if(selitem_Get()->GetVisiblePlayer() == selitem_Get()->GetCurPlayer()) {
				DPRINTF(k_DBG_GAMESTATE, ("Keypress end turn, %d\n", selitem_Get()->GetCurPlayer()));
				selitem_Get()->RegisterManualEndTurn();
				director_Get()->AddEndTurn();
			}
			else
			{


			}
		}
        break;

    case KEY_FUNCTION_SAVE_WORLD :
		if (civapp_Get()->IsGameLoaded() && !network_Get().IsClient()) {
			civapp_Get()->AutoSave(selitem_Get()->GetVisiblePlayer(), true);






		}
		move = FALSE ;
		break ;

    case KEY_FUNCTION_LOAD_WORLD :
		if (civapp_Get()->IsGameLoaded() && !network_Get().IsActive()) {
			{
				civapp_Get()->PostLoadQuickSaveAction(selitem_Get()->GetVisiblePlayer());





			}
			move = FALSE ;
		}
		break ;

    case KEY_FUNCTION_QUIT:
		optionwarningscreen_displayMyWindow(OWS_QUIT);

        return 1;
        break;

#ifdef _PLAYTEST
    case KEY_FUNCTION_GAMESTATE_DEBUG:
        world_Get()->GamestateDebug();
        for (i=0; i<2; i++) {
            player_Get(i)->GamestateDebug();
        }
        return TRUE;
		break;
#endif

    case KEY_FUNCTION_GROUP_ARMY:
	{
		Army a;
		if(selitem_Get()->GetSelectedArmy(a)) {
			if(network_Get().IsClient())
            {
				CellUnitList units;
				Cell *cell = world_Get()->GetCell(a->RetPos());
				for (sint32 i = 0; i < cell->GetNumUnits(); i++) {
					if(cell->AccessUnit(i).GetArmy().m_id != a.m_id) {
						units.Insert(cell->AccessUnit(i));
					}
				}
				if(units.Num() > 0) {
					network_Get().SendGroupRequest(units, a);
				}
			} else {
				gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_GroupOrder,
									   GEA_Army, a.m_id,
									   GEA_End);
			}
		}

		return TRUE;
	}
	break;
	case KEY_FUNCTION_UNGROUP_ARMY:
	{
		Army a;
		if(selitem_Get()->GetSelectedArmy(a)) {
			if(network_Get().IsClient()) {
				network_Get().SendUngroupRequest(a, *a.AccessData());
			} else {
				gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_UngroupOrder,
									   GEA_Army, a.m_id,
									   GEA_End);
			}
		}

		return TRUE;
	}
	break;
	case KEY_FUNCTION_PROCESS_UNIT_ORDERS:
		if(isMyTurn) {
			gevmanager_Get()->Pause();
			for
            (
                sint32 i = 0;
                i < player_Get(selitem_Get()->GetVisiblePlayer())->m_all_armies->Num();
                i++
            )
            {
				gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_BeginTurnExecute,
									   GEA_Army, player_Get(selitem_Get()->GetVisiblePlayer())->m_all_armies->Access(i),
									   GEA_End);
			}
			gevmanager_Get()->Resume();

		}
		break;
	case KEY_FUNCTION_EXECUTE_ORDERS:
		if(isMyTurn)
		{
			PLAYER_INDEX s_player;
			ID s_item;
			SELECT_TYPE s_state;
			selitem_Get()->GetTopCurItem(s_player, s_item, s_state);
			Army army(s_item);
			if (s_state == SELECT_TYPE_LOCAL_ARMY && army.IsValid())
            {
				army.ExecuteOrders();
			}
		}
		break;

#if defined(CTP1_HAS_RISEN_FROM_THE_GRAVE)
	case KEY_FUNCTION_TOGGLE_SPACE:
	{
		if(!network_Get().IsActive()) {
			extern sint32 g_fog_toggle;
			g_fog_toggle = !g_fog_toggle;
			void WhackScreen();
			WhackScreen();
		}
		break;
	}
#endif

	case KEY_FUNCTION_TOGGLE_CITY_NAMES:
		profiledb_Get()->SetShowCityNames(!profiledb_Get()->GetShowCityNames());
		break;

	case KEY_FUNCTION_TOGGLE_TRADE_ROUTES:
		profiledb_Get()->SetShowTradeRoutes(!profiledb_Get()->GetShowTradeRoutes());
		break;
//#if 0
//	case KEY_FUNCTION_TOGGLE_ARMY_NAMES: //emod
//		profiledb_Get()->SetShowArmyNames(!profiledb_Get()->GetShowArmyNames());
//		break;
//#endif

#ifdef _DEBUG
#endif
	case KEY_FUNCTION_HELP_MODE_TOGGLE:
		break;

	case KEY_FUNCTION_CHAT_KEY:
		if (chatbox_Get())
        {
			chatbox_Get()->SetActive(!chatbox_Get()->IsActive());
		}
		break;

	case KEY_FUNCTION_UNIT_CITY_TOGGLE:
		selitem_Get()->UnitCityToggle();
		break;
	case KEY_FUNCTION_END_UNIT_TURN:
		selitem_Get()->EndUnitTurn();
		break;

	case KEY_FUNCTION_NETWORK_PLAYERS_SCREEN:
		if ( !g_networkPlayersScreen ) {
			g_networkPlayersScreen = new c3_UtilityPlayerListPopup((c3_UtilityPlayerListCallback *)network_PlayerListCallback);
		}
		g_networkPlayersScreen->DisplayWindow();
		if ( network_Get().IsHost() ) {
			g_networkPlayersScreen->EnableButtons();
		}
		else if ( network_Get().IsClient() ) {
			g_networkPlayersScreen->DisableButtons();
		}

		break;
	case KEY_FUNCTION_CIV_TAB:
		if(controlpanel_Get()) {
			controlpanel_Get()->SetTab(CP_TAB_CIV);
		}
		break;
	case KEY_FUNCTION_MSG_TAB:
		if(controlpanel_Get()) {
			controlpanel_Get()->SetTab(CP_TAB_MSGLOG);
		}
		break;
	case KEY_FUNCTION_CITY_TAB:
		if(controlpanel_Get()) {
			controlpanel_Get()->SetTab(CP_TAB_CITY);
		}
		break;
	case KEY_FUNCTION_UNIT_TAB:
		if(controlpanel_Get()) {
			controlpanel_Get()->SetTab(CP_TAB_UNIT);
		}
		break;
	case KEY_FUNCTION_TILE_TAB:
		if(controlpanel_Get()) {
			controlpanel_Get()->SetTab(CP_TAB_TILEIMP);
		}
		break;

	case KEY_FUNCTION_CONTROL_NEXT:
		break;
	case KEY_FUNCTION_CONTROL_PREV:
		break;
	case KEY_FUNCTION_CLOSE:
		if(g_currentMessageWindow) {
			if(messagepool_Get()->IsValid(*g_currentMessageWindow->GetMessage())) {
				g_currentMessageWindow->GetMessage()->Kill();
			}
		} else if(g_modalMessage) {
			Message *msg = g_modalMessage->GetMessage();
			if(messagepool_Get()->IsValid(*msg)) {
				Assert(msg->IsAlertBox());
				MessageData *data = msg->AccessData();
				if(data->GetNumButtons() <= 2 && data->GetNumButtons() > 0) {
					data->GetButton(0)->Callback();
				}
			}
		}

		break;
	case KEY_FUNCTION_YES:
	{
		if(g_modalMessage) {
			Message *msg = g_modalMessage->GetMessage();
			if(messagepool_Get()->IsValid(*msg)) {
				Assert(msg->IsAlertBox());
				MessageData *data = msg->AccessData();
				if(data->GetNumButtons() == 2) {
					data->GetButton(1)->Callback();
				}
			}
		} else if(g_utilityTextMessage) {
			g_utilityTextMessage->m_callback(TRUE);
			g_utilityTextMessage->RemoveWindow();
		}
		break;
	}
	case KEY_FUNCTION_NO:
	{
		if(g_modalMessage) {
			Message *msg = g_modalMessage->GetMessage();
			if(messagepool_Get()->IsValid(*msg)) {
				Assert(msg->IsAlertBox());
				MessageData *data = msg->AccessData();
				if(data->GetNumButtons() == 2) {
					data->GetButton(0)->Callback();
				}
			}
		} else if(g_utilityTextMessage) {
			g_utilityTextMessage->m_callback(FALSE);
			g_utilityTextMessage->RemoveWindow();
		}
		break;
	}
	case KEY_FUNCTION_CLEAR_QUEUE:
		if(controlpanel_Get()

			) {
			Unit city;
			if(selitem_Get()->GetSelectedCity(city)) {
				city.GetData()->GetCityData()->GetBuildQueue()->Clear();
			}
		}
		break;
	case KEY_FUNCTION_OPEN_MESSAGE:
		{

			
			if (g_modalWindow == 0) {
				if(g_currentMessageWindow) {
					if(g_currentMessageWindow->GetMinimizeAction()) {
						g_currentMessageWindow->GetMinimizeAction()->Execute(nullptr,
																		   AUI_BUTTON_ACTION_EXECUTE,
																		   0);
					}
				} else if(selitem_Get()) {
					sint32 visPlayer = selitem_Get()->GetVisiblePlayer();

					if(player_Get(visPlayer) && player_Get(visPlayer)->m_messages->Num() > 0) {
						sint32 m;
						for(m = 0; m < player_Get(visPlayer)->m_messages->Num(); m++) {
							if(!player_Get(visPlayer)->m_messages->Access(m).IsRead()) {
								player_Get(visPlayer)->m_messages->Access(m).Show();
								break;
							}
						}
					}
				}
			}
		}
		break;
	case KEY_FUNCTION_EXECUTE_EYEPOINT:
		{

			if(g_currentMessageWindow) {
				if(messagepool_Get()->IsValid(*g_currentMessageWindow->GetMessage())) {
					g_currentMessageWindow->GetMessage()->AccessData()->EyePointCallback(0);
				}
			}
			break;
		}
	case KEY_FUNCTION_MOVE_ORDER:
		controlpanel_Get()->BeginOrderDelivery(selitem_Get()->GetMoveOrder());
		break;

	case KEY_FUNCTION_TOGGLE_RADAR:
		radarwindow_Toggle();
		break;

	case KEY_FUNCTION_TOGGLE_CONTROL_PANEL:
		controlpanel_Get()->Toggle();
		break;

	case KEY_FUNCTION_TOGGLE_ALL:
		if(!c3ui_Get()->GetWindow(controlpanel_Get()->GetWindow()->Id())) {
			c3ui_Get()->AddWindow(controlpanel_Get()->GetWindow());
			radarwindow_Show();
		} else {
			c3ui_Get()->RemoveWindow(controlpanel_Get()->GetWindow()->Id());
			radarwindow_Hide();
		}
		break;

	case KEY_FUNCTION_TRADE_SUMMARY:
		if (!g_modalWindow) {
			close_AllScreens();
			TradeManager::Display();
			TradeManager::SetMode(TRADE_MANAGER_SUMMARY);
		}
		break;

	case KEY_FUNCTION_GAIA:
		if(!g_modalWindow) {
			close_AllScreens();
			ScienceVictoryDialog::Open();
		}
		break;

	case KEY_FUNCTION_BUILD_QUEUE:
	{
		if(!g_modalWindow && player_Get(selitem_Get()->GetVisiblePlayer())) {
			Unit city;
			if(selitem_Get()->GetSelectedCity(city)) {
				city = world_Get()->GetCity(selitem_Get()->GetCurSelectPos());
			} else if(player_Get(selitem_Get()->GetVisiblePlayer())->GetNumCities()) {
				city = player_Get(selitem_Get()->GetVisiblePlayer())->m_all_cities->Access(0);
			}
				if(city.IsValid()) {
					close_AllScreens();
					EditQueue::Display(CityWindow::GetCityData(city));
				}
		}
		break;
	}
	case KEY_FUNCTION_CITY_MANAGEMENT:
		if(!g_modalWindow && player_Get(selitem_Get()->GetVisiblePlayer())) {
			close_AllScreens();
			Unit city;
			if(selitem_Get()->GetSelectedCity(city)) {
				CityWindow::Display(city.GetData()->GetCityData());
			} else if(player_Get(selitem_Get()->GetVisiblePlayer())->GetNumCities()) {
				city = player_Get(selitem_Get()->GetVisiblePlayer())->m_all_cities->Access(0);
				selitem_Get()->SetSelectCity(city);
				CityWindow::Display(city.GetData()->GetCityData());
			}
		}
		break;

	case KEY_FUNCTION_NEW_PROPOSAL:
		if(!g_modalWindow) {
			if(DipWizard::CanInitiateRightNow()) {
				DipWizard::Display();
			}
		}
		break;

	case KEY_FUNCTION_TIMELINE:
		if(!g_modalWindow) {
			InfoWindow::SelectWonderTab();
			InfoWindow::Open();
		}
		break;

	case KEY_FUNCTION_RANK:
		if(!g_modalWindow) {
			InfoWindow::SelectRankingTab();
			InfoWindow::Open();
		}
		break;

	case KEY_FUNCTION_RESTART:
		//Added by Martin G�hmann to disable also the restart key in network
		//games, hot seat games and email games.
		if(!g_modalWindow
		&& !profiledb_Get()->IsScenario()
		&& !is_scenario_Get()
		&& !network_Get().IsActive()
		&& !turn_Get()->IsHotSeat()
		&& !turn_Get()->IsEmail()
		) {
			optionwarningscreen_displayMyWindow(OWS_RESTART) ;
		}
		break;

	case KEY_FUNCTION_NEW_GAME:
		if(!g_modalWindow) {

			optionwarningscreen_displayMyWindow(OWS_QUITTOSHELL);
		}
		break;

	case KEY_FUNCTION_SOUND_OPTIONS:
		if(!g_modalWindow) {
			soundscreen_displayMyWindow();
		}
		break;

	// MUSIC added by ahenobarb
	case KEY_FUNCTION_MUSIC_OPTIONS:
		if(!g_modalWindow) {
			musicscreen_displayMyWindow();
		}
		break;

	case KEY_FUNCTION_GRAPHICS_OPTIONS:
		if(!g_modalWindow) {
			graphicsscreen_displayMyWindow();
		}
		break;

	case KEY_FUNCTION_GAMEPLAY_OPTIONS:
		if(!g_modalWindow) {
			gameplayoptions_displayMyWindow();
		}
		break;
	case KEY_FUNCTION_ADVANCED_OPTIONS:
		if(!g_modalWindow) {
			ProfileEdit::Display();
		}
		break;
	default :
        Assert(FALSE);
    }

    if (move && isMyTurn) {

		PLAYER_INDEX s_player;
		ID s_item;
		SELECT_TYPE s_state;

		selitem_Get()->GetTopCurItem(s_player, s_item, s_state);

		switch(s_state) {
			case SELECT_TYPE_LOCAL_ARMY:
			{
				Army army(s_item);
				MapPoint pos;
				army.GetPos(pos);
				MapPoint newpos;

				if(pos.GetNeighborPosition(d, newpos)) {
					gevmanager_Get()->AddEvent(GEV_INSERT_Tail, GEV_MoveToOrder,
										   GEA_Army, army.m_id,
										   GEA_Direction, d,
										   GEA_End);

					selitem_Get()->DidKeyboardMove();
				}
				break;
			}

			default:
				break;
		}
	}

	return TRUE;
}

void WhackScreen()
{
	tiledmap_Get()->Refresh();
	tiledmap_Get()->InvalidateMap();
	radar_map_Get()->Update();
}

bool keypress_IsGameFunction(KEY_FUNCTION kf)
{
	switch(kf) {
		case KEY_FUNCTION_MOVE_NORTH:
		case KEY_FUNCTION_MOVE_NORTHWEST:
		case KEY_FUNCTION_MOVE_NORTHEAST:
		case KEY_FUNCTION_MOVE_WEST:
		case KEY_FUNCTION_MOVE_EAST:
		case KEY_FUNCTION_MOVE_SOUTHWEST:
		case KEY_FUNCTION_MOVE_SOUTH:
		case KEY_FUNCTION_MOVE_SOUTHEAST:
		case KEY_FUNCTION_MOVE_UP_OR_DOWN:
		case KEY_FUNCTION_EXECUTE_ORDERS:
		case KEY_FUNCTION_GROUP_ARMY:
		case KEY_FUNCTION_UNGROUP_ARMY:
		case KEY_FUNCTION_ENTRENCH:
		case KEY_FUNCTION_UNLOAD_TRANS:
		case KEY_FUNCTION_SETTLE:
		case KEY_FUNCTION_SPACE_LAUNCH:
		case KEY_FUNCTION_DESCEND:
		case KEY_FUNCTION_PILLAGE:
		case KEY_FUNCTION_BOMBARD:
		case KEY_FUNCTION_EXPEL:
		case KEY_FUNCTION_SLEEP:
		case KEY_FUNCTION_END_UNIT_TURN:
		case KEY_FUNCTION_NEXT_ROUND:
		case KEY_FUNCTION_ENDTURN:
		case KEY_FUNCTION_PARADROP:
		case KEY_FUNCTION_INVESTIGATE_CITY:
		case KEY_FUNCTION_PLANT_NUKE:
		case KEY_FUNCTION_BIOINFECT:
		case KEY_FUNCTION_NANOTERROR:
		case KEY_FUNCTION_CREATE_PARK:
		case KEY_FUNCTION_REFORM:
		case KEY_FUNCTION_CENTER_MAP:
		case KEY_FUNCTION_PROCESS_UNIT_ORDERS:
		case KEY_FUNCTION_MOVE_ORDER:
		case KEY_FUNCTION_TOGGLE_RADAR:
		case KEY_FUNCTION_TOGGLE_CONTROL_PANEL:
		case KEY_FUNCTION_TOGGLE_ALL:
		case KEY_FUNCTION_LOAD_WORLD:
		case KEY_FUNCTION_SAVE_WORLD:
		case KEY_FUNCTION_ZOOM_IN1:
		case KEY_FUNCTION_ZOOM_OUT1:
			return true;
		default:
			return false;
	}
}
