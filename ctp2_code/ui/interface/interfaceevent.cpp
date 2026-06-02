//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Event handlers for the user interface.
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
// - None
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Prevent production errors when pressing F3 after end of turn.
// - The information window is no more closed on the begin of a new turn.
//   (Aug 7th 2005 Martin G�hmann)
// - The initial city interface is no more displayed if the visible player
//   is a robot. (26-Jan-2008 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ui/interface/interfaceevent.h"
#include "gs/gameobj/Events.h"
#include "gs/events/GameEventManager.h"
#include "gs/events/GameEventUser.h"
#include "gs/gameobj/Unit.h"
#include "ui/aui_ctp2/SelItem.h"
#include "ui/aui_ctp2/c3_utilitydialogbox.h"
#include "ui/interface/controlpanelwindow.h"
#include "ui/interface/MainControlPanel.h"
#include "gs/database/profileDB.h"
#include "ui/interface/citywindow.h"
#include "gs/gameobj/ArmyData.h"
#include "gs/gameobj/Player.h"
#include "gfx/spritesys/director.h"
#include "ui/interface/EditQueue.h"
#include "ui/interface/screenutils.h"
#include "sound/soundmanager.h"
#include "net/general/network.h"
#include "sound/gamesounds.h"

extern sint32               g_modalWindow;

STDEHANDLER(InterfaceCreateCityEvent)
{
	Unit city;
	if(!args->GetCity(0, city)) return GEV_HD_Continue;

	if(city.GetOwner() == selitem_Get()->GetVisiblePlayer()) {
		if(profiledb_Get()->GetAutoRenameCities()) {
			c3_utilitydialogbox_NameCity(city);
		}
	}

	return GEV_HD_Continue;
}

STDEHANDLER(InterfaceMakePopEvent)
{
#if 0
	if(profiledb_Get()->GetAutoOpenCityWindow()) {
		static Unit city;
		if(!args->GetCity(0, city)) return GEV_HD_Continue;
		if(city.GetOwner() == selitem_Get()->GetVisiblePlayer() &&
		   city.CD()->PopCount() == 1) {

			EditQueue::Display(CityWindow::GetCityData(city));
		}
	}
#endif
	return GEV_HD_Continue;
}

STDEHANDLER(InterfaceOpenInitialCityInterfaceEvent)
{
	if(profiledb_Get()->GetAutoOpenCityWindow()) {
		static Unit city;
		if(!args->GetCity(0, city)) return GEV_HD_Continue;
		if( city.GetOwner() == selitem_Get()->GetVisiblePlayer()
		&& !player_Get(city.GetOwner())->IsRobot()
		){
			EditQueue::Display(CityWindow::GetCityData(city));
		}
	}
	return GEV_HD_Continue;
}

STDEHANDLER(InterfaceUpdateCityEvent)
{
	static Unit city, selCity;
	if(!args->GetCity(0, city)) return GEV_HD_Continue;

	if(selitem_Get()->GetSelectedCity(selCity) && city.m_id == selCity.m_id) {
		controlpanelwindow_Update(&city);
	}
	return GEV_HD_Continue;
}

STDEHANDLER(InterfaceStartMovePhaseEvent)
{
	sint32 pl;
	if(!args->GetPlayer(0, pl)) return GEV_HD_Continue;
	if(pl == selitem_Get()->GetVisiblePlayer()) {
		static Unit selCity;
		if(selitem_Get()->GetSelectedCity(selCity)) {
			controlpanelwindow_Update(&selCity);
		}
	}
	return GEV_HD_Continue;
}

STDEHANDLER(InterfaceCityWindowUnitNotification)
{
	CityWindow::NotifyUnitChange();
	return GEV_HD_Continue;
}

STDEHANDLER(InterfaceUpdateCityProjection)
{
	sint32 pl;
	if(!args->GetPlayer(0, pl)) return GEV_HD_Continue;

	if(pl == selitem_Get()->GetVisiblePlayer()) {
		MainControlPanel::SelectedCity();
		// Reenable opening the city window - see InterfacePreBeginTurnEvent.
		if (g_modalWindow > 0)
		{
			--g_modalWindow;
		}
	}

	return GEV_HD_Continue;
}

STDEHANDLER(InterfaceBeginTurnRecenter)
{
	sint32 pl;
	if(!args->GetPlayer(0, pl)) return GEV_HD_Continue;

	if(pl == selitem_Get()->GetVisiblePlayer())
	{
		Army a;
		Unit c;
		MapPoint pos(-1,-1);
		if(selitem_Get()->GetSelectedArmy(a))
		{
			pos = a->RetPos();
		}
		else if(selitem_Get()->GetSelectedCity(c))
		{
			pos = c.RetPos();
		}

		if(selitem_Get()->IsAutoCenterOn()
		&& pos.x >= 0
		&& !director_Get()->TileWillBeCompletelyVisible(pos.x, pos.y)
		){
			director_Get()->AddCenterMap(pos);
		}
	}
	return GEV_HD_Continue;
}

STDEHANDLER(InterfacePreBeginTurn)
{
	sint32 pl;
	if(!args->GetPlayer(0, pl)) return GEV_HD_Continue;

	if(pl == selitem_Get()->GetVisiblePlayer()) {
		close_AllScreensAndUpdateInfoScreen();
		// Prevent opening the city window during the production computations.
		// It will be reenabled in InterfaceUpdateCityProjection.
		++g_modalWindow;
		if(g_controlPanel)
			g_controlPanel->ClearTargetingMode();

		if(g_soundManager) {
			g_soundManager->TerminateAllLoopingSounds(SOUNDTYPE_SFX);
			g_soundManager->TerminateAllLoopingSounds(SOUNDTYPE_VOICE);
		}

		if(g_network.IsHost() && g_network.GetPlayerIndex() == pl) {

			if (g_soundManager) {
				g_soundManager->AddSound(SOUNDTYPE_SFX, (uint32)0,
				                         gamesounds_GetGameSoundID(GAMESOUNDS_NET_YOUR_TURN),
				                         0,
				                         0);
			}
		}
	}
	return GEV_HD_Continue;
}

void interfaceevent_Initialize()
{

	MainControlPanel::InitializeEvents();

	gevmanager_Get()->AddCallback(GEV_CreateCity, GEV_PRI_Post, &s_InterfaceCreateCityEvent);
	gevmanager_Get()->AddCallback(GEV_MakePop, GEV_PRI_Post, &s_InterfaceMakePopEvent);
	gevmanager_Get()->AddCallback(GEV_OpenInitialCityInterface, GEV_PRI_Post, &s_InterfaceOpenInitialCityInterfaceEvent);


	gevmanager_Get()->AddCallback(GEV_BuildUnit, GEV_PRI_Post, &s_InterfaceUpdateCityEvent);
	gevmanager_Get()->AddCallback(GEV_BuildBuilding, GEV_PRI_Post, &s_InterfaceUpdateCityEvent);
	gevmanager_Get()->AddCallback(GEV_BuildWonder, GEV_PRI_Post, &s_InterfaceUpdateCityEvent);
	gevmanager_Get()->AddCallback(GEV_BuildFront, GEV_PRI_Post, &s_InterfaceUpdateCityEvent);
	gevmanager_Get()->AddCallback(GEV_CityBeginTurn, GEV_PRI_Post, &s_InterfaceUpdateCityEvent);
	gevmanager_Get()->AddCallback(GEV_ZeroProduction, GEV_PRI_Post, &s_InterfaceUpdateCityEvent);
	gevmanager_Get()->AddCallback(GEV_RollOverProduction, GEV_PRI_Post, &s_InterfaceUpdateCityEvent);

	gevmanager_Get()->AddCallback(GEV_StartMovePhase, GEV_PRI_Post, &s_InterfaceStartMovePhaseEvent);

	gevmanager_Get()->AddCallback(GEV_DisbandUnit, GEV_PRI_Post, &s_InterfaceCityWindowUnitNotification);

	gevmanager_Get()->AddCallback(GEV_StartMovePhase, GEV_PRI_Post, &s_InterfaceUpdateCityProjection);
	gevmanager_Get()->AddCallback(GEV_StartMovePhase, GEV_PRI_Post, &s_InterfaceBeginTurnRecenter);

	gevmanager_Get()->AddCallback(GEV_BeginTurn, GEV_PRI_Pre, &s_InterfacePreBeginTurn);
}

void interfaceevent_Cleanup()
{
}
