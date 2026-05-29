//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Engine-side queries for "which player is being viewed"
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/core/player_view.h"
#include "gs/utility/gstypes.h"  // k_MAX_PLAYERS

namespace player_view {

static VisiblePlayerFn    s_visiblePlayer    = nullptr;
static CurPlayerFn        s_curPlayer        = nullptr;
static PlayerAfterFn      s_playerAfter      = nullptr;
static InitFn             s_init             = nullptr;
static InitFromArchiveFn  s_initFromArchive  = nullptr;
static CleanupFn          s_cleanup          = nullptr;
static SetCurrentPlayerFn s_setCurrentPlayer = nullptr;
static SetVisiblePlayerFn s_setVisiblePlayer = nullptr;
static RefreshFn          s_refresh          = nullptr;
static SetSelectUnitFn    s_setSelectUnit    = nullptr;
static SetSelectCityFn    s_setSelectCity    = nullptr;
static EnterArmyMoveFn    s_enterArmyMove    = nullptr;
static SetAutoUnloadFn    s_setAutoUnload    = nullptr;
static DeselectFn         s_deselect         = nullptr;
static IsArmySelectedFn   s_isArmySelected   = nullptr;
static IsCitySelectedFn   s_isCitySelected   = nullptr;
static GetSelectedIdFn    s_getSelectedArmyId = nullptr;
static GetSelectedIdFn    s_getSelectedCityId = nullptr;
static GetTopCurItemFn    s_getTopCurItem     = nullptr;
static IsModalActiveFn    s_isModalActive     = nullptr;
static NextPlayerFn       s_nextPlayer        = nullptr;
static AddPlayerFn        s_addPlayer         = nullptr;
static RegisterRemovedArmyFn s_registerRemovedArmy = nullptr;
static RegisterRemovedCityFn s_registerRemovedCity = nullptr;
static NextRoundFn            s_nextRound            = nullptr;
static RegisterManualEndTurnFn s_registerManualEndTurn = nullptr;
static IsAutoCenterOnFn       s_isAutoCenterOn       = nullptr;
static GetPlayerOnScreenFn    s_getPlayerOnScreen    = nullptr;
static ForceDirectorSelectFn  s_forceDirectorSelect  = nullptr;
static EnterMovePathFn        s_enterMovePath        = nullptr;
static GetCurrentYearStringFn       s_getCurrentYearString       = nullptr;
static GetBuildQueueHeadNameFn      s_getBuildQueueHeadName      = nullptr;
static GetBuildQueueHeadStringIdFn  s_getBuildQueueHeadStringId  = nullptr;
static GetSelectedCargoFn           s_getSelectedCargo           = nullptr;

void RegisterVisiblePlayer(VisiblePlayerFn fn)        { s_visiblePlayer    = fn; }
void RegisterCurPlayer(CurPlayerFn fn)                { s_curPlayer        = fn; }
void RegisterPlayerAfter(PlayerAfterFn fn)            { s_playerAfter      = fn; }
void RegisterInit(InitFn fn)                          { s_init             = fn; }
void RegisterInitFromArchive(InitFromArchiveFn fn)    { s_initFromArchive  = fn; }
void RegisterCleanup(CleanupFn fn)                    { s_cleanup          = fn; }
void RegisterSetCurrentPlayer(SetCurrentPlayerFn fn)  { s_setCurrentPlayer = fn; }
void RegisterSetVisiblePlayer(SetVisiblePlayerFn fn)  { s_setVisiblePlayer = fn; }
void RegisterRefresh(RefreshFn fn)                    { s_refresh          = fn; }
void RegisterSetSelectUnit(SetSelectUnitFn fn)        { s_setSelectUnit    = fn; }
void RegisterSetSelectCity(SetSelectCityFn fn)        { s_setSelectCity    = fn; }
void RegisterEnterArmyMove(EnterArmyMoveFn fn)        { s_enterArmyMove    = fn; }
void RegisterSetAutoUnload(SetAutoUnloadFn fn)        { s_setAutoUnload    = fn; }
void RegisterDeselect(DeselectFn fn)                  { s_deselect         = fn; }
void RegisterIsArmySelected(IsArmySelectedFn fn)      { s_isArmySelected   = fn; }
void RegisterIsCitySelected(IsCitySelectedFn fn)      { s_isCitySelected   = fn; }
void RegisterGetSelectedArmyId(GetSelectedIdFn fn)    { s_getSelectedArmyId = fn; }
void RegisterGetSelectedCityId(GetSelectedIdFn fn)    { s_getSelectedCityId = fn; }
void RegisterGetTopCurItem(GetTopCurItemFn fn)        { s_getTopCurItem     = fn; }
void RegisterIsModalMessageActive(IsModalActiveFn fn) { s_isModalActive    = fn; }
void RegisterNextPlayer(NextPlayerFn fn)              { s_nextPlayer       = fn; }
void RegisterAddPlayer(AddPlayerFn fn)                { s_addPlayer        = fn; }
void RegisterRemovedArmy(RegisterRemovedArmyFn fn)    { s_registerRemovedArmy = fn; }
void RegisterRemovedCity(RegisterRemovedCityFn fn)    { s_registerRemovedCity = fn; }
void RegisterNextRound(NextRoundFn fn)                { s_nextRound            = fn; }
void RegisterRegisterManualEndTurn(RegisterManualEndTurnFn fn) { s_registerManualEndTurn = fn; }
void RegisterIsAutoCenterOn(IsAutoCenterOnFn fn)      { s_isAutoCenterOn       = fn; }
void RegisterGetPlayerOnScreen(GetPlayerOnScreenFn fn) { s_getPlayerOnScreen    = fn; }
void RegisterForceDirectorSelect(ForceDirectorSelectFn fn) { s_forceDirectorSelect = fn; }
void RegisterEnterMovePath(EnterMovePathFn fn)        { s_enterMovePath        = fn; }
void RegisterGetCurrentYearString(GetCurrentYearStringFn fn)             { s_getCurrentYearString       = fn; }
void RegisterGetBuildQueueHeadName(GetBuildQueueHeadNameFn fn)           { s_getBuildQueueHeadName      = fn; }
void RegisterGetBuildQueueHeadStringId(GetBuildQueueHeadStringIdFn fn)   { s_getBuildQueueHeadStringId  = fn; }
void RegisterGetSelectedCargo(GetSelectedCargoFn fn)                     { s_getSelectedCargo           = fn; }

sint32 VisiblePlayer()
{
	return s_visiblePlayer ? s_visiblePlayer() : -1;
}

sint32 CurPlayer()
{
	return s_curPlayer ? s_curPlayer() : 0;
}

sint32 PlayerAfter(sint32 player)
{
	if (s_playerAfter) return s_playerAfter(player);
	return (player + 1) % k_MAX_PLAYERS;
}

void Init(sint32 nPlayers)
{
	if (s_init) s_init(nPlayers);
}

void InitFromArchive(CivArchive *archive)
{
	if (s_initFromArchive) s_initFromArchive(archive);
}

void Cleanup()
{
	if (s_cleanup) s_cleanup();
}

void SetCurrentPlayer(sint32 player)
{
	if (s_setCurrentPlayer) s_setCurrentPlayer(player);
}

void SetVisiblePlayer(sint32 player)
{
	if (s_setVisiblePlayer) s_setVisiblePlayer(player);
}

void Refresh()
{
	if (s_refresh) s_refresh();
}

void SetSelectUnit(const Unit &unit)
{
	if (s_setSelectUnit) s_setSelectUnit(unit);
}

void SetSelectCity(const Unit &city)
{
	if (s_setSelectCity) s_setSelectCity(city);
}

void EnterArmyMove(sint32 player, const MapPoint &pos)
{
	if (s_enterArmyMove) s_enterArmyMove(player, pos);
}

void SetAutoUnload(bool on)
{
	if (s_setAutoUnload) s_setAutoUnload(on);
}

void Deselect(sint32 player)
{
	if (s_deselect) s_deselect(player);
}

bool IsArmySelected()
{
	return s_isArmySelected ? s_isArmySelected() : false;
}

bool IsCitySelected()
{
	return s_isCitySelected ? s_isCitySelected() : false;
}

sint32 GetSelectedArmyId()
{
	return s_getSelectedArmyId ? s_getSelectedArmyId() : 0;
}

sint32 GetSelectedCityId()
{
	return s_getSelectedCityId ? s_getSelectedCityId() : 0;
}

void GetTopCurItem(sint32 &player, sint32 &item, sint32 &state)
{
	if (s_getTopCurItem) {
		s_getTopCurItem(player, item, state);
	} else {
		player = 0;
		item = 0;
		state = 0;
	}
}

bool IsModalMessageActive()
{
	return s_isModalActive ? s_isModalActive() : false;
}

void NextPlayer()
{
	if (s_nextPlayer) s_nextPlayer();
}

void AddPlayer(sint32 player)
{
	if (s_addPlayer) s_addPlayer(player);
}

void ArmyRemoved(sint32 player, const Army &army)
{
	if (s_registerRemovedArmy) s_registerRemovedArmy(player, army);
}

void CityRemoved(sint32 player, const Unit &city)
{
	if (s_registerRemovedCity) s_registerRemovedCity(player, city);
}

bool IsAutoCenterOn()
{
	return s_isAutoCenterOn ? s_isAutoCenterOn() : false;
}

sint32 GetPlayerOnScreen()
{
	return s_getPlayerOnScreen ? s_getPlayerOnScreen() : -1;
}

void ForceDirectorSelect(const Army &army)
{
	if (s_forceDirectorSelect) s_forceDirectorSelect(army);
}

void EnterMovePath(sint32 owner, Army &army,
                   const MapPoint &src, const MapPoint &dest)
{
	if (s_enterMovePath) s_enterMovePath(owner, army, src, dest);
}

void NextRound()
{
	if (s_nextRound) s_nextRound();
}

void RegisterManualEndTurn()
{
	if (s_registerManualEndTurn) s_registerManualEndTurn();
}

void GetCurrentYearString(char *out, size_t cap)
{
	if (s_getCurrentYearString) {
		s_getCurrentYearString(out, cap);
	} else if (out && cap > 0) {
		out[0] = '\0';
	}
}

void GetBuildQueueHeadName(const CityData *city, char *out, size_t cap)
{
	if (s_getBuildQueueHeadName) {
		s_getBuildQueueHeadName(city, out, cap);
	} else if (out && cap > 0) {
		out[0] = '\0';
	}
}

sint32 GetBuildQueueHeadStringId(const CityData *city)
{
	return s_getBuildQueueHeadStringId ? s_getBuildQueueHeadStringId(city) : -1;
}

bool GetSelectedCargo(CellUnitList &out)
{
	return s_getSelectedCargo ? s_getSelectedCargo(out) : false;
}

} // namespace player_view
