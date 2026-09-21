//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Collection of control panels during actual play.
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
// - Blank function added to hide the data of the previous player for hotseat
//   games.
//
//----------------------------------------------------------------------------

#include <memory>
#include "ctp/c3.h"
#include "ui/interface/MainControlPanel.h"

#include "ui/aui_common/aui_progressbar.h"
#include "ui/aui_common/aui_uniqueid.h"
#include "ui/interface/CityControlPanel.h"
#include "ui/interface/ControlTabPanel.h"
#include "ui/interface/DomesticControlPanel.h"
#include "ui/interface/EndTurnButton.h"
#include "gs/gameobj/Events.h"
#include "gs/events/GameEventManager.h"
#include "gs/events/GameEventUser.h"
#include "ui/interface/MessageControlPanel.h"
#include "ui/interface/ShortcutPad.h"
#include "ui/interface/StatusBar.h"
#include "ui/interface/TilesControlPanel.h"
#include "ui/interface/TurnYearStatus.h"
#include "ui/interface/UnitControlPanel.h"
#include "ui/interface/ZoomPad.h"

static std::unique_ptr<MainControlPanel> g_mainControlPanel;

MainControlPanel * maincontrolpanel_Get()
{
	return g_mainControlPanel.get();
}

static aui_ProgressBar * s_progressBar;

STDEHANDLER(MainControlPanel_BeginTurn)
{
	PLAYER_INDEX player = 0;
	if (args->GetPlayer(0, player))
	{
		MainControlPanel::UpdatePlayer(player);
	}

	return GEV_HD_Continue;
}

void MainControlPanel::InitializeEvents()
{
	gevmanager_Get()->AddCallback(GEV_BeginTurn, GEV_PRI_Post, &s_MainControlPanel_BeginTurn);
}

void MainControlPanel::Initialize(MBCHAR const *ldlBlock)
{
	if (!g_mainControlPanel)
	{
		g_mainControlPanel = std::make_unique<MainControlPanel>(ldlBlock);
	}
}

//----------------------------------------------------------------------------
//
// Name       : MainControlPanel::Blank
//
// Description: Blank out the data of the previous player in between turns for
//              hotseat play.
//
// Parameters : -
//
// Globals    : g_mainControlPanel	: control panel to blank
//
// Returns    : -
//
// Remark(s)  : -
//
//----------------------------------------------------------------------------
void MainControlPanel::Blank()
{
	if (g_mainControlPanel)
	{
		g_mainControlPanel->m_controlTabPanel->Blank();
	}
}

void MainControlPanel::CleanUp()
{
	g_mainControlPanel.reset();
}

void MainControlPanel::Update()
{
	if(g_mainControlPanel) {
		g_mainControlPanel->m_controlTabPanel->Update();
	}
}

void MainControlPanel::UpdateCityList()
{
	if(g_mainControlPanel) {
		g_mainControlPanel->m_controlTabPanel->UpdateCityList();
	}
}

void MainControlPanel::UpdatePlayer(PLAYER_INDEX player)
{
	if(g_mainControlPanel) {

		g_mainControlPanel->m_endTurnButton->UpdatePlayer(player);
		g_mainControlPanel->m_turnYearStatus->UpdatePlayer(player);
	}
}

void MainControlPanel::UpdateZoom()
{
	// Does nothing but could be reimplemented
}

void MainControlPanel::SelectedCity()
{
	if(g_mainControlPanel) {
		g_mainControlPanel->m_controlTabPanel->SelectedCity();
	}
}

void MainControlPanel::SelectedUnit()
{
	if(g_mainControlPanel) {
		g_mainControlPanel->m_controlTabPanel->SelectedUnit();
	}
}

void MainControlPanel::UnitPanelActivated()
{
	if(g_mainControlPanel) {
		g_mainControlPanel->m_controlTabPanel->UnitPanelActivated();
	}
}

void MainControlPanel::CityPanelActivated()
{
	if(g_mainControlPanel) {
		g_mainControlPanel->m_controlTabPanel->CityPanelActivated();
	}
}

aui_ProgressBar* MainControlPanel::GetProgressBar()
{
	return s_progressBar;
}

MainControlPanel::MainControlPanel(MBCHAR const *ldlBlock)
:
    m_controlTabPanel   (std::make_unique<ControlTabPanel>(ldlBlock)),
    m_endTurnButton     (std::make_unique<EndTurnButton>(ldlBlock)),
    m_shortcutPad       (std::make_unique<ShortcutPad>(ldlBlock)),
    m_statusBar         (std::make_unique<StatusBar>(ldlBlock)),
    m_turnYearStatus    (std::make_unique<TurnYearStatus>(ldlBlock))
{
	TurnYearStatus::BuildTurnLengthOverride();
}

MainControlPanel::~MainControlPanel()
{
	TurnYearStatus::CleanupTurnLengthOverride();
}

bool MainControlPanel::GetSelectedCargo(CellUnitList &cargo)
{
	return g_mainControlPanel &&
	       g_mainControlPanel->m_controlTabPanel->GetSelectedCargo(cargo);
}

void MainControlPanel::SwitchToTransportView()
{
	if(g_mainControlPanel) {
		g_mainControlPanel->m_controlTabPanel->SwitchToTransportView();
	}
}
