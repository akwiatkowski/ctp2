//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : The Slic Engine
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
// - Readded the new slicfunctions of the patch, by Martin G�hmann.
// - Added FreeAllSlaves slicfunction by The Big Mc.
// - Added PlantSpecificGood and RemoveGood slicfunctions by MrBaggins.
// - Data blanked out at screen in between 2 players in hotseat play.
// - Great library history cleared between 2 players in hotseat play.
// - Corrected reported memory leak.
// - Added database access to all databases in the new database format,
//   even if it does not make sense, by Martin G�hmann.
// - New slic functions added by Martin G�hmann:
//   - CargoCapacity     Gets number of additional units a unit can carry.
//   - MaxCargoSize      Gets the maximum number of units a unit can carry.
//   - CargoSize         Gets the current number of units a unit is carrying.
//   - GetUnitFromCargo  Gets the i'th unit a unit is carrying.
//   - GetContinent      Gets the continent ID of an location.
//   - IsWater           Gets whether a location is water.
// - Enable end turn button when unblanking.
// - Removed a syntax error by Klaus Kaan
// - Function by Solver: IsOnSameContinent - Checks if two locations are
//   on same continent.
// - Added AddSlaves function modelled after the AddPops function.
// - Prevented crash with a missing Slic file.
// - Memory leaks repaired.
// - Redesigned to prevent memory leaks and crashes.
// - Reuse SlicSegment pool between SlicEngine sessions.
// - Added slic civilisation database support.
// - Added slic risk database support. (Sep 15th 2005 Martin G�hmann)
// - Added City Capture options by E 6.09.2006
// - Added database slic access of difficulty, pollution and global warming
//   databases. (July 15th 2006 Martin G�hmann)
// - PopContext refills the builtins when it restores the old context so
//   that slic does not forget the values of the builtins. (Sep 24th 2006 Martin G�hmann)
// - Added GetContinentSize slic function. (Dec 24th 2006 Martin G�hmann)
// - Added slic database access to the new map icon database. (27-Mar-2007 Martin G�hmann)
// - Added slic database access to the new map database. (27-Mar-2007 Martin G�hmann)
// - Added slic database access to the new concept database. (31-Mar-2007 Martin G�hmann)
// - Added slic database access to the new const database. (29-Oct-2007 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/slic/SlicEngine.h"

#include <iterator>
#include <list>
#include <memory>
#include "ctp/ctp2_utils/c3errors.h"
#include "gs/slic/SlicObject.h"
#include "gs/slic/SlicSegment.h"
#include "gs/slic/slicif.h"
#include "gs/slic/StringHash.h"
#include "gs/slic/SlicFunc.h"
#include "gs/slic/slicfuncai.h"
#include "gs/slic/SlicSymTab.h"
#include "gs/gameobj/player.h"					// player_arr_Get()
#include "gs/gameobj/Unit.h"
#include "gs/fileio/CivPaths.h"				// civpaths_Get()
#include "gs/core/game_observer.h"             // gameobservers_Get()
#include "gs/core/game.h"                      // Ctp2::Game (trampoline target)
#include "ctp/civapp.h"                        // civapp_Get → CivApp::GetGame
#include "gs/core/player_view.h"               // player_view::VisiblePlayer
#include "gs/gameobj/TradeOffer.h"
#include "gs/gameobj/Agreement.h"
#include "gs/gameobj/MessagePool.h"			// messagepool_Get()
#include "gs/gameobj/BldQue.h"
#include "gs/slic/SlicRecord.h"
#include "ctp/ctp2_utils/pointerlist.h"
#include "gs/utility/SimpleDynArr.h"
#include "gs/fileio/gamefile.h"
#include "gs/core/tiledmap_observer.h"
#include "gs/slic/SlicConst.h"
#include "gs/slic/SlicStruct.h"
#include "gs/slic/SlicNamedSymbol.h"
#include "gs/slic/SlicBuiltin.h"
#include "gs/slic/SlicBuiltinEnum.h"
#include "gs/slic/SlicArray.h"
#include "gs/slic/SlicFrame.h"
#include "UnitRecord.h"
#include "AdvanceRecord.h"
#include "TerrainRecord.h"
#include "BuildingRecord.h"
#include "WonderRecord.h"
#include "FeatRecord.h"
#include "OrderRecord.h"
#include "TerrainImprovementRecord.h"
#include "GovernmentRecord.h"
#include "StrategyRecord.h"
#include "DiplomacyRecord.h"
#include "PersonalityRecord.h"
#include "AdvanceBranchRecord.h"
#include "AdvanceListRecord.h"
#include "AgeCityStyleRecord.h"
#include "AgeRecord.h"
#include "BuildingBuildListRecord.h"
#include "BuildListSequenceRecord.h"
#include "CitySizeRecord.h"
#include "CityStyleRecord.h"
#include "ConceptRecord.h"
#include "ConstRecord.h"
#include "DiplomacyProposalRecord.h"
#include "DiplomacyThreatRecord.h"
#include "EndGameObjectRecord.h"
#include "GoalRecord.h"
#include "IconRecord.h"
#include "PopRecord.h"
#include "ImprovementListRecord.h"
#include "SoundRecord.h"
#include "SpecialAttackInfoRecord.h"
#include "SpecialEffectRecord.h"
#include "SpriteRecord.h"
#include "UnitBuildListRecord.h"
#include "WonderBuildListRecord.h"
#include "WonderMovieRecord.h"
#include "CivilisationRecord.h"
#include "RiskRecord.h"
#include "DifficultyRecord.h"
#include "PollutionRecord.h"
#include "GlobalWarmingRecord.h"
#include "MapRecord.h"
#include "MapIconRecord.h"
#include "gs/slic/SlicDBConduit.h"
#include "gs/slic/SlicModFunction.h"
#include "gs/events/GameEventManager.h"
#include "gs/utility/Globals.h"
#include "ResourceRecord.h"
#include "gs/gameobj/CriticalMessagesPrefs.h"
#include "gs/database/profileDB.h"
#include <memory>

// SlicEngine storage lives in Ctp2::Game; accessors trampoline through CivApp.
SlicEngine * slicengine_Get() {
    CivApp * app = civapp_Get();
    Ctp2::Game * game = app ? app->GetGame() : nullptr;
    return game ? game->GetSlicPtr() : nullptr;
}
void slicengine_Set(SlicEngine *p) {
    CivApp * app = civapp_Get();
    Ctp2::Game * game = app ? app->GetGame() : nullptr;
    if (game) game->SetSlicPtr(p); else std::unique_ptr<SlicEngine>{p};
}

char g_slic_filename[_MAX_PATH];
char g_tutorial_filename[_MAX_PATH];

namespace
{
    size_t const            CONST_HASH_SIZE                     = 10;
    size_t const            k_DB_HASH_SIZE                      = 32;
    sint32 const            k_SLIC_DEFAULT_TIMER_GRANULARITY    = 5;
    MBCHAR const            KEY_UNDEFINED                       = 0;
    sint32 const            NOT_IN_USE                          = -1;
    PLAYER_INDEX const      SINGLE_PLAYER_DEFAULT               = 1;
} // namespace

SlicEngine::SlicEngine()
:   m_tutorialActive        (FALSE),
	m_tutorialPlayer        (SINGLE_PLAYER_DEFAULT),
	m_currentMessage        (std::make_unique<Message>()),
	m_segmentHash           (std::make_unique<SlicSegmentHash>(k_SEGMENT_HASH_SIZE)),
	m_functionHash          (nullptr),
	m_uiHash                (std::make_unique<StringHash<SlicUITrigger>>(k_SEGMENT_HASH_SIZE)),
	m_dbHash                (nullptr),
	m_symTab                (std::make_unique<SlicSymTab>(0)),
	m_context               (nullptr),
	m_disabledClasses       (std::make_unique<SimpleDynamicArray<sint32>>()),
	m_eyepointMessage       (),
	m_timerGranularity      (k_SLIC_DEFAULT_TIMER_GRANULARITY),
	m_doResearchOnUnblank   (FALSE),
	m_researchOwner         (NOT_IN_USE),
	m_constHash             (std::make_unique<StringHash<SlicConst>>(CONST_HASH_SIZE)),
	m_builtins              (std::make_unique<SlicSymbolData const *[]>(SLIC_BUILTIN_MAX).release()),
	m_builtin_desc          (std::make_unique<SlicStructDescription *[]>(SLIC_BUILTIN_MAX).release()),
	m_loadGameName          (nullptr),
	m_currentKeyTrigger     (KEY_UNDEFINED),
	m_blankScreen           (false),
	m_atBreak               (false),
	m_breakContext          (nullptr),
	m_breakRequested        (false)
{
	for (auto & m_triggerList : m_triggerLists)
	{
		m_triggerList = std::make_unique<PointerList<SlicSegment>>().release();
	}

	std::fill(m_records, m_records + k_MAX_PLAYERS, static_cast<PointerList<SlicRecord> *>(nullptr));
	std::fill(m_timer, m_timer + k_NUM_TIMERS, NOT_IN_USE);
	std::fill(m_triggerKey, m_triggerKey + k_MAX_TRIGGER_KEYS, KEY_UNDEFINED);
	std::fill(m_builtins, m_builtins + SLIC_BUILTIN_MAX, static_cast<SlicSymbolData const *>(nullptr));
	std::fill(m_builtin_desc, m_builtin_desc + SLIC_BUILTIN_MAX, static_cast<SlicStructDescription *>(nullptr));
	std::fill(m_researchText, m_researchText + 256, 0);
	std::fill(m_modFunc, m_modFunc + mod_MAX, static_cast<SlicModFunc *>(nullptr));

	AddStructs(true);
	AddBuiltinFunctions();
	AddDatabases();
}

SlicEngine::~SlicEngine()
{
    if (m_context)
	{
        m_context->Release();
        m_context = nullptr;
	}

    if (m_breakContext)
	{
        m_breakContext->Release();
        m_breakContext = nullptr;
	}

    // Entries are reference counted, so they are Released rather than deleted;
    // the list itself now frees its own nodes.
    while (SlicObject * obj = m_contextStack.RemoveTail())
	{
        obj->Release();
	}

	// m_loadGameName: reference only
    KillCurrentMessage();

	size_t  i;

    for (i = 0; i < TRIGGER_LIST_MAX; ++i)
    {
        if (m_triggerLists[i])
        {
	        m_triggerLists[i]->DeleteAll();
            std::unique_ptr<PointerList<SlicSegment>>(m_triggerLists[i]);
        }
	}

	for (i = 0; i < k_MAX_PLAYERS; ++i)
    {
		if (m_records[i])
        {
			m_records[i]->DeleteAll();
			std::unique_ptr<PointerList<SlicRecord>>(m_records[i]);
		}
	}

    if (m_disabledClasses)
    {
	    m_disabledClasses->Clear();
        m_disabledClasses.reset();
    }

	    m_uiExecuteObjects.DeleteAll();

	for (i = 0; i < mod_MAX; ++i)
    {
	    std::unique_ptr<SlicModFunc>(m_modFunc[i]);
    }


	for (i = 0; i < SLIC_BUILTIN_MAX; ++i)
    {
		std::unique_ptr<SlicStructDescription>(m_builtin_desc[i]);
        // m_builtins[i] not deleted: managed through m_symTab
	}
    std::unique_ptr<SlicStructDescription *[]>(m_builtin_desc);
    std::unique_ptr<SlicSymbolData const *[]>(m_builtins);

	slicif_cleanup();

    if (MessagePool *mp = messagepool_Get())
    {
		mp->NotifySlicReload();
	}
}

/// Reset the Slic handling and reload from file.
/// \param  a_File  Name of the top level Slic file to load
/// \result File loaded and parsed successfully
bool SlicEngine::Reload(std::basic_string<MBCHAR> const & a_File)
{
    slicengine_Set(std::make_unique<SlicEngine>().release());  // Set() deletes the previous instance

    SlicEngine * eng = slicengine_Get();
    bool isParsedOk = eng->Load(a_File, k_NORMAL_FILE);
    if (isParsedOk)
    {
        eng->Link();
    }
    return isParsedOk;
}

void SlicEngine::PostSerialize()
{
	m_symTab->PostSerialize();
	m_segmentHash->LinkTriggerSymbols(m_uiHash.get());

	AddModFuncs();
}

SlicSegment *SlicEngine::GetSegment(const char *id)
{
	return m_segmentHash->Access(id);
}

SlicFunc *SlicEngine::GetFunction(const char *name)
{
    return m_functionHash ? m_functionHash->Access(name) : nullptr;
}

SlicNamedSymbol *SlicEngine::GetSymbol(sint32 index)
{
	return m_symTab->Access(index);
}

SlicNamedSymbol *SlicEngine::GetSymbol(const char *name)
{
	return m_symTab->StringHash<SlicNamedSymbol>::Access(name);
}

SlicNamedSymbol *SlicEngine::GetOrMakeSymbol(const char *name)
{
	SlicNamedSymbol *sym = m_symTab->StringHash<SlicNamedSymbol>::Access(name);
	if(!sym) {
		sym = std::make_unique<SlicNamedSymbol>(name).release();
		m_symTab->Add(sym);
	}
	return sym;
}

SlicParameterSymbol *SlicEngine::GetParameterSymbol(const char *name, sint32 parameterIndex)
{
	SlicNamedSymbol *namedSym = m_symTab->StringHash<SlicNamedSymbol>::Access(name);

	SlicParameterSymbol *sym = (SlicParameterSymbol *)namedSym;

	if(!sym) {
		sym = std::make_unique<SlicParameterSymbol>(name, parameterIndex).release();
		m_symTab->Add(sym);
	}
	Assert(sym->GetSerializeType() == SLIC_SYM_SERIAL_PARAMETER);
	if(sym->GetSerializeType() != SLIC_SYM_SERIAL_PARAMETER) {
		return nullptr;
	}

	return sym;
}

void SlicEngine::Execute(SlicObject *obj)
{
	Assert(obj && !m_atBreak);
	if (!obj || m_atBreak)
		return;

	PushContext(obj);

	if (obj->IsValid()  &&
        ((obj->GetSegment()->GetFilenum() != k_TUTORIAL_FILE) || profiledb_Get()->IsTutorialAdvice()) &&
		(!critical_messages_prefs_Get() || critical_messages_prefs_Get()->IsEnabled(obj->GetSegment()->GetName()))
       )
    {
		obj->Execute();
	}

    if (m_atBreak)
    {
        // No action: keep context active to Continue later.
    }
    else
    {
	    PopContext();
    }
}

void SlicEngine::AddBuiltinFunctions()
{
	if (m_functionHash)
		return; // Already added

	m_functionHash = std::make_unique<StringHash<SlicFunc>>(k_SEGMENT_HASH_SIZE);

	m_functionHash->Add(std::make_unique<Slic_PrintInt>().release());
	m_functionHash->Add(std::make_unique<Slic_PrintText>().release());
	m_functionHash->Add(std::make_unique<Slic_Text>().release());
	m_functionHash->Add(std::make_unique<Slic_Message>().release());
	m_functionHash->Add(std::make_unique<Slic_AddMessage>().release());
	m_functionHash->Add(std::make_unique<Slic_MessageAll>().release());
	m_functionHash->Add(std::make_unique<Slic_MessageAllBut>().release());
	m_functionHash->Add(std::make_unique<Slic_EyePoint>().release());
	m_functionHash->Add(std::make_unique<Slic_DisableTrigger>().release());
	m_functionHash->Add(std::make_unique<Slic_EnableTrigger>().release());
	m_functionHash->Add(std::make_unique<Slic_Return1>().release());
	m_functionHash->Add(std::make_unique<Slic_Return0>().release());
	m_functionHash->Add(std::make_unique<Slic_HasAdvance>().release());
	m_functionHash->Add(std::make_unique<Slic_IsContinentBiggerThan>().release());
	m_functionHash->Add(std::make_unique<Slic_IsHostile>().release());
	m_functionHash->Add(std::make_unique<Slic_TradePoints>().release());
	m_functionHash->Add(std::make_unique<Slic_TradeRoutes>().release());
	m_functionHash->Add(std::make_unique<Slic_HasSameGoodAsTraded>().release());
	m_functionHash->Add(std::make_unique<Slic_AddCity>().release());
	m_functionHash->Add(std::make_unique<Slic_IsSecondRowUnit>().release());
	m_functionHash->Add(std::make_unique<Slic_IsFlankingUnit>().release());
	m_functionHash->Add(std::make_unique<Slic_IsBombardingUnit>().release());
	m_functionHash->Add(std::make_unique<Slic_IsWormholeProbe>().release());
	m_functionHash->Add(std::make_unique<Slic_IsUnderseaCity>().release());
	m_functionHash->Add(std::make_unique<Slic_IsSpaceCity>().release());
	m_functionHash->Add(std::make_unique<Slic_IsSpaceUnit>().release());
	m_functionHash->Add(std::make_unique<Slic_IsWonderType>().release());
	m_functionHash->Add(std::make_unique<Slic_IsCounterBombardingUnit>().release());
	m_functionHash->Add(std::make_unique<Slic_IsCleric>().release());
	m_functionHash->Add(std::make_unique<Slic_IsSlaver>().release());
	m_functionHash->Add(std::make_unique<Slic_IsActiveDefender>().release());
	m_functionHash->Add(std::make_unique<Slic_IsDiplomat>().release());
	m_functionHash->Add(std::make_unique<Slic_IsInRegion>().release());
	m_functionHash->Add(std::make_unique<Slic_UnitHasFlag>().release());
	m_functionHash->Add(std::make_unique<Slic_UnitsInCell>().release());
	m_functionHash->Add(std::make_unique<Slic_PlayerCityCount>().release());
	m_functionHash->Add(std::make_unique<Slic_RegardLevel>().release());
	m_functionHash->Add(std::make_unique<Slic_ChangeRegardLevel>().release());
	m_functionHash->Add(std::make_unique<Slic_Kill>().release());
	m_functionHash->Add(std::make_unique<Slic_DeactivateTutorial>().release());
	m_functionHash->Add(std::make_unique<Slic_ControlsRegion>().release());
	m_functionHash->Add(std::make_unique<Slic_DemandWarFromAllies>().release());
	m_functionHash->Add(std::make_unique<Slic_Accept>().release());
	m_functionHash->Add(std::make_unique<Slic_Reject>().release());
	m_functionHash->Add(std::make_unique<Slic_KnowledgeRank>().release());
	m_functionHash->Add(std::make_unique<Slic_MilitaryRank>().release());
	m_functionHash->Add(std::make_unique<Slic_TradeRank>().release());
	m_functionHash->Add(std::make_unique<Slic_GoldRank>().release());
	m_functionHash->Add(std::make_unique<Slic_PopulationRank>().release());
	m_functionHash->Add(std::make_unique<Slic_CitiesRank>().release());
	m_functionHash->Add(std::make_unique<Slic_GeographicRank>().release());
	m_functionHash->Add(std::make_unique<Slic_SpaceRank>().release());
	m_functionHash->Add(std::make_unique<Slic_UnderseaRank>().release());
	m_functionHash->Add(std::make_unique<Slic_EyeDropdown>().release());
	m_functionHash->Add(std::make_unique<Slic_CaptureCity>().release());
	m_functionHash->Add(std::make_unique<Slic_CaptureRegion>().release());
	m_functionHash->Add(std::make_unique<Slic_LeaveRegion>().release());
	m_functionHash->Add(std::make_unique<Slic_Surrender>().release());
	m_functionHash->Add(std::make_unique<Slic_Research>().release());
	m_functionHash->Add(std::make_unique<Slic_MessageType>().release());
	m_functionHash->Add(std::make_unique<Slic_Caption>().release());
	m_functionHash->Add(std::make_unique<Slic_Duration>().release());
	m_functionHash->Add(std::make_unique<Slic_BreakAgreement>().release());
	m_functionHash->Add(std::make_unique<Slic_AcceptTradeOffer>().release());
	m_functionHash->Add(std::make_unique<Slic_DontAcceptTradeOffer>().release());
	m_functionHash->Add(std::make_unique<Slic_SetGovernment>().release());
	m_functionHash->Add(std::make_unique<Slic_StealRandomAdvance>().release());
	m_functionHash->Add(std::make_unique<Slic_StealSpecificAdvance>().release());
	m_functionHash->Add(std::make_unique<Slic_DisableTileImprovementButton>().release());
	m_functionHash->Add(std::make_unique<Slic_DisableScreensButton>().release());
	m_functionHash->Add(std::make_unique<Slic_EnableTileImprovementButton>().release());
	m_functionHash->Add(std::make_unique<Slic_EnableScreensButton>().release());
	m_functionHash->Add(std::make_unique<Slic_OpenCiv>().release());
	m_functionHash->Add(std::make_unique<Slic_OpenCity>().release());
	m_functionHash->Add(std::make_unique<Slic_OpenUnit>().release());
	m_functionHash->Add(std::make_unique<Slic_OpenScience>().release());
	m_functionHash->Add(std::make_unique<Slic_OpenDiplomacy>().release());
	m_functionHash->Add(std::make_unique<Slic_OpenTrade>().release());
	m_functionHash->Add(std::make_unique<Slic_OpenInfo>().release());
	m_functionHash->Add(std::make_unique<Slic_OpenOptions>().release());
	m_functionHash->Add(std::make_unique<Slic_OpenCivTab>().release());
	m_functionHash->Add(std::make_unique<Slic_OpenMaxTab>().release());
	m_functionHash->Add(std::make_unique<Slic_OpenLaborTab>().release());
	m_functionHash->Add(std::make_unique<Slic_OpenProductionTab>().release());
	m_functionHash->Add(std::make_unique<Slic_OpenCityTab>().release());
	m_functionHash->Add(std::make_unique<Slic_ExitToShell>().release());
	m_functionHash->Add(std::make_unique<Slic_SendTradeBid>().release());
	m_functionHash->Add(std::make_unique<Slic_AcceptTradeBid>().release());
	m_functionHash->Add(std::make_unique<Slic_RejectTradeBid>().release());
	m_functionHash->Add(std::make_unique<Slic_BreakAlliance>().release());
	m_functionHash->Add(std::make_unique<Slic_AddOrder>().release());
	m_functionHash->Add(std::make_unique<Slic_EndTurn>().release());
	m_functionHash->Add(std::make_unique<Slic_FinishBuilding>().release());
	m_functionHash->Add(std::make_unique<Slic_Abort>().release());
	m_functionHash->Add(std::make_unique<Slic_Show>().release());
    m_functionHash->Add(std::make_unique<Slic_DoAutoUnload>().release());
    m_functionHash->Add(std::make_unique<Slic_DoLandInOcean>().release());
    m_functionHash->Add(std::make_unique<Slic_DoOutOfFuel>().release());
    m_functionHash->Add(std::make_unique<Slic_DoPillageOwnLand>().release());
    m_functionHash->Add(std::make_unique<Slic_DoSellImprovement>().release());
    m_functionHash->Add(std::make_unique<Slic_DoCertainRevolution>().release());
    m_functionHash->Add(std::make_unique<Slic_DoFreeSlaves>().release());
    m_functionHash->Add(std::make_unique<Slic_DoCannotAffordMaintenance>().release());
    m_functionHash->Add(std::make_unique<Slic_DoCannotAffordSupport>().release());
    m_functionHash->Add(std::make_unique<Slic_DoCityWillStarve>().release());
    m_functionHash->Add(std::make_unique<Slic_DoYouWillBreakRoute>().release());
	m_functionHash->Add(std::make_unique<Slic_TerrainType>().release());
	m_functionHash->Add(std::make_unique<Slic_LibraryUnit>().release());
	m_functionHash->Add(std::make_unique<Slic_LibraryBuilding>().release());
	m_functionHash->Add(std::make_unique<Slic_LibraryWonder>().release());
	m_functionHash->Add(std::make_unique<Slic_LibraryAdvance>().release());
	m_functionHash->Add(std::make_unique<Slic_LibraryTerrain>().release());
	m_functionHash->Add(std::make_unique<Slic_LibraryConcept>().release());
	m_functionHash->Add(std::make_unique<Slic_LibraryGovernment>().release());
	m_functionHash->Add(std::make_unique<Slic_LibraryTileImprovement>().release());
	m_functionHash->Add(std::make_unique<Slic_UnitCount>().release());
	m_functionHash->Add(std::make_unique<Slic_UnitType>().release());
	m_functionHash->Add(std::make_unique<Slic_KillMessages>().release());
	m_functionHash->Add(std::make_unique<Slic_MessageClass>().release());
	m_functionHash->Add(std::make_unique<Slic_KillClass>().release());
	m_functionHash->Add(std::make_unique<Slic_CityHasBuilding>().release());
	m_functionHash->Add(std::make_unique<Slic_Title>().release());
	m_functionHash->Add(std::make_unique<Slic_NetworkAccept>().release());
	m_functionHash->Add(std::make_unique<Slic_NetworkEject>().release());

	m_functionHash->Add(std::make_unique<Slic_Attract>().release());
	m_functionHash->Add(std::make_unique<Slic_StopAttract>().release());

	m_functionHash->Add(std::make_unique<Slic_DontSave>().release());
	m_functionHash->Add(std::make_unique<Slic_IsUnitSelected>().release());
	m_functionHash->Add(std::make_unique<Slic_IsCitySelected>().release());
	m_functionHash->Add(std::make_unique<Slic_BuildingType>().release());
	m_functionHash->Add(std::make_unique<Slic_IsHumanPlayer>().release());
	m_functionHash->Add(std::make_unique<Slic_DisableClose>().release());
	m_functionHash->Add(std::make_unique<Slic_EnableCloseClass>().release());
	m_functionHash->Add(std::make_unique<Slic_EnableCloseMessage>().release());
	m_functionHash->Add(std::make_unique<Slic_AddGoods>().release());
	m_functionHash->Add(std::make_unique<Slic_GoodType>().release());
	m_functionHash->Add(std::make_unique<Slic_GoodCount>().release());
	m_functionHash->Add(std::make_unique<Slic_GoodCountTotal>().release());
	m_functionHash->Add(std::make_unique<Slic_GoodVisibutik>().release());
	m_functionHash->Add(std::make_unique<Slic_StartTimer>().release());
	m_functionHash->Add(std::make_unique<Slic_StopTimer>().release());
	m_functionHash->Add(std::make_unique<Slic_DisableMessageClass>().release());
	m_functionHash->Add(std::make_unique<Slic_EnableMessageClass>().release());
	m_functionHash->Add(std::make_unique<Slic_CreateUnit>().release());
	m_functionHash->Add(std::make_unique<Slic_Random>().release());
	m_functionHash->Add(std::make_unique<Slic_AddCityByIndex>().release());
	m_functionHash->Add(std::make_unique<Slic_DetachRobot>().release());
	m_functionHash->Add(std::make_unique<Slic_AttachRobot>().release());
	m_functionHash->Add(std::make_unique<Slic_Cities>().release());
	m_functionHash->Add(std::make_unique<Slic_ForceRegard>().release());
	m_functionHash->Add(std::make_unique<Slic_AddPops>().release());
	m_functionHash->Add(std::make_unique<Slic_KillUnit>().release());
	m_functionHash->Add(std::make_unique<Slic_PlaySound>().release());
	m_functionHash->Add(std::make_unique<Slic_CreateCity>().release());
	m_functionHash->Add(std::make_unique<Slic_ExtractLocation>().release());
	m_functionHash->Add(std::make_unique<Slic_CreateCoastalCity>().release());
	m_functionHash->Add(std::make_unique<Slic_FindCoastalCity>().release());
	m_functionHash->Add(std::make_unique<Slic_Terraform>().release());
	m_functionHash->Add(std::make_unique<Slic_PlantGood>().release());
	m_functionHash->Add(std::make_unique<Slic_GetRandomNeighbor>().release());
	m_functionHash->Add(std::make_unique<Slic_GrantAdvance>().release());
	m_functionHash->Add(std::make_unique<Slic_AddUnit>().release());
	m_functionHash->Add(std::make_unique<Slic_AllUnitsCanBeExpelled>().release());
	m_functionHash->Add(std::make_unique<Slic_AddExpelOrder>().release());
	m_functionHash->Add(std::make_unique<Slic_GetMessageClass>().release());
	m_functionHash->Add(std::make_unique<Slic_SetPlayer>().release());
	m_functionHash->Add(std::make_unique<Slic_CityCollectingGood>().release());
	m_functionHash->Add(std::make_unique<Slic_GetNearestWater>().release());
	m_functionHash->Add(std::make_unique<Slic_IsPlayerAlive>().release());
	m_functionHash->Add(std::make_unique<Slic_GameOver>().release());
	m_functionHash->Add(std::make_unique<Slic_SaveGame>().release());
	m_functionHash->Add(std::make_unique<Slic_LoadGame>().release());
	m_functionHash->Add(std::make_unique<Slic_HasRiver>().release());
	m_functionHash->Add(std::make_unique<Slic_SetScience>().release());
	m_functionHash->Add(std::make_unique<Slic_SetResearching>().release());
	m_functionHash->Add(std::make_unique<Slic_IsInZOC>().release());
	m_functionHash->Add(std::make_unique<Slic_DisableChooseResearch>().release());
	m_functionHash->Add(std::make_unique<Slic_EnableChooseResearch>().release());
	m_functionHash->Add(std::make_unique<Slic_QuitToLobby>().release());
	m_functionHash->Add(std::make_unique<Slic_KillEyepointMessage>().release());
	m_functionHash->Add(std::make_unique<Slic_ClearBuildQueue>().release());
	m_functionHash->Add(std::make_unique<Slic_BreakLeaveOurLands>().release());
	m_functionHash->Add(std::make_unique<Slic_BreakNoPiracy>().release());
	m_functionHash->Add(std::make_unique<Slic_UseDirector>().release());
	m_functionHash->Add(std::make_unique<Slic_ClearOrders>().release());
	m_functionHash->Add(std::make_unique<Slic_SetTimerGranularity>().release());

	m_functionHash->Add(std::make_unique<Slic_SetUnit>().release());
	m_functionHash->Add(std::make_unique<Slic_SetUnitByIndex>().release());
	m_functionHash->Add(std::make_unique<Slic_SetCity>().release());
	m_functionHash->Add(std::make_unique<Slic_SetCityByIndex>().release());
	m_functionHash->Add(std::make_unique<Slic_SetLocation>().release());
	m_functionHash->Add(std::make_unique<Slic_MakeLocation>().release());
	m_functionHash->Add(std::make_unique<Slic_SetOrder>().release());
	m_functionHash->Add(std::make_unique<Slic_Flood>().release());
	m_functionHash->Add(std::make_unique<Slic_Ozone>().release());
	m_functionHash->Add(std::make_unique<Slic_GodMode>().release());
	m_functionHash->Add(std::make_unique<Slic_ExecuteAllOrders>().release());
	m_functionHash->Add(std::make_unique<Slic_CatchUp>().release());
	m_functionHash->Add(std::make_unique<Slic_Deselect>().release());
	m_functionHash->Add(std::make_unique<Slic_Preference>().release());
	m_functionHash->Add(std::make_unique<Slic_SetPreference>().release());
	m_functionHash->Add(std::make_unique<Slic_AddMovement>().release());
	m_functionHash->Add(std::make_unique<Slic_ToggleVeteran>().release());
	m_functionHash->Add(std::make_unique<Slic_IsVeteran>().release());

	m_functionHash->Add(std::make_unique<Slic_CantAttackUnit>().release());
	m_functionHash->Add(std::make_unique<Slic_CantAttackCity>().release());
	m_functionHash->Add(std::make_unique<Slic_CityCantRiotOrRevolt>().release());
	m_functionHash->Add(std::make_unique<Slic_SelectUnit>().release());
	m_functionHash->Add(std::make_unique<Slic_SelectCity>().release());
	m_functionHash->Add(std::make_unique<Slic_CantEndTurn>().release());
	m_functionHash->Add(std::make_unique<Slic_Heal>().release());
	m_functionHash->Add(std::make_unique<Slic_AddGold>().release());
	m_functionHash->Add(std::make_unique<Slic_SetActionKey>().release());
	m_functionHash->Add(std::make_unique<Slic_GetCityByLocation>().release());
	m_functionHash->Add(std::make_unique<Slic_GetNeighbor>().release());

	m_functionHash->Add(std::make_unique<Slic_DamageUnit>().release());

	m_functionHash->Add(std::make_unique<Slic_IsUnitInBuildList>().release());
    m_functionHash->Add(std::make_unique<Slic_IsBuildingInBuildList>().release());
    m_functionHash->Add(std::make_unique<Slic_IsWonderInBuildList>().release());
    m_functionHash->Add(std::make_unique<Slic_IsEndgameInBuildList>().release());
    m_functionHash->Add(std::make_unique<Slic_IsBuildingAtHead>().release());
    m_functionHash->Add(std::make_unique<Slic_IsWonderAtHead>().release());

    m_functionHash->Add(std::make_unique<Slic_AddUnitToBuildList>().release());
    m_functionHash->Add(std::make_unique<Slic_AddBuildingToBuildList>().release());
    m_functionHash->Add(std::make_unique<Slic_AddWonderToBuildList>().release());
    m_functionHash->Add(std::make_unique<Slic_AddEndgameToBuildList>().release());
    m_functionHash->Add(std::make_unique<Slic_KillUnitFromBuildList>().release());
    m_functionHash->Add(std::make_unique<Slic_KillBuildingFromBuildList>().release());
    m_functionHash->Add(std::make_unique<Slic_KillWonderFromBuildList>().release());
    m_functionHash->Add(std::make_unique<Slic_KillEndgameFromBuildList>().release());

	m_functionHash->Add(std::make_unique<Slic_SetPW>().release());
	m_functionHash->Add(std::make_unique<Slic_Stacked>().release());

	m_functionHash->Add(std::make_unique<Slic_SetString>().release());
	m_functionHash->Add(std::make_unique<Slic_SetStringByDBIndex>().release());
	m_functionHash->Add(std::make_unique<Slic_GetStringDBIndex>().release());
	m_functionHash->Add(std::make_unique<Slic_UnitHasUserFlag>().release());

	m_functionHash->Add(std::make_unique<Slic_BlankScreen>().release());
	m_functionHash->Add(std::make_unique<Slic_AddCenter>().release());
	m_functionHash->Add(std::make_unique<Slic_AddEffect>().release());
	m_functionHash->Add(std::make_unique<Slic_PlayerCivilization>().release());
	m_functionHash->Add(std::make_unique<Slic_CivilizationIndex>().release());
	m_functionHash->Add(std::make_unique<Slic_ExitToDesktop>().release());

	m_functionHash->Add(std::make_unique<Slic_Import>().release());
	m_functionHash->Add(std::make_unique<Slic_Export>().release());

	m_functionHash->Add(std::make_unique<Slic_GetUnitFromArmy>().release());
	m_functionHash->Add(std::make_unique<Slic_GetUnitByIndex>().release());
	m_functionHash->Add(std::make_unique<Slic_GetArmyByIndex>().release());
	m_functionHash->Add(std::make_unique<Slic_GetCityByIndex>().release());





	m_functionHash->Add(std::make_unique<Slic_LogRegardEvent>().release());
	m_functionHash->Add(std::make_unique<Slic_GetPublicRegard>().release());
	m_functionHash->Add(std::make_unique<Slic_GetEffectiveRegard>().release());
	m_functionHash->Add(std::make_unique<Slic_GetTrust>().release());
	m_functionHash->Add(std::make_unique<Slic_SetTrust>().release());
	m_functionHash->Add(std::make_unique<Slic_RecomputeRegard>().release());
	m_functionHash->Add(std::make_unique<Slic_ConsiderResponse>().release());
	m_functionHash->Add(std::make_unique<Slic_SetResponse>().release());;
	m_functionHash->Add(std::make_unique<Slic_ConsiderMotivation>().release());
	m_functionHash->Add(std::make_unique<Slic_ConsiderNewProposal>().release());
	m_functionHash->Add(std::make_unique<Slic_SetNewProposal>().release());
	m_functionHash->Add(std::make_unique<Slic_ConsiderStrategicState>().release());
	m_functionHash->Add(std::make_unique<Slic_ComputeCurrentStrategy>().release());
	m_functionHash->Add(std::make_unique<Slic_ConsiderDiplomaticState>().release());
	m_functionHash->Add(std::make_unique<Slic_ChangeDiplomaticState>().release());
	m_functionHash->Add(std::make_unique<Slic_GetTradeFrom>().release());
	m_functionHash->Add(std::make_unique<Slic_GetTributeFrom>().release());
	m_functionHash->Add(std::make_unique<Slic_GetGoldSurplusPercent>().release());
	m_functionHash->Add(std::make_unique<Slic_CanBuySurplus>().release());
	m_functionHash->Add(std::make_unique<Slic_GetAdvanceLevelPercent>().release());
	m_functionHash->Add(std::make_unique<Slic_AtWarCount>().release());
	m_functionHash->Add(std::make_unique<Slic_EffectiveAtWarCount>().release());
	m_functionHash->Add(std::make_unique<Slic_AtWarWith>().release());
	m_functionHash->Add(std::make_unique<Slic_EffectiveWarWith>().release());






	m_functionHash->Add(std::make_unique<Slic_HasAgreementWithAnyone>().release());
	m_functionHash->Add(std::make_unique<Slic_HasAgreement>().release());
	m_functionHash->Add(std::make_unique<Slic_CancelAgreement>().release());
	m_functionHash->Add(std::make_unique<Slic_TurnsSinceLastWar>().release());
	m_functionHash->Add(std::make_unique<Slic_TurnsAtWar>().release());
	m_functionHash->Add(std::make_unique<Slic_GetLastHotwarAttack>().release());
	m_functionHash->Add(std::make_unique<Slic_GetLastColdwarAttack>().release());




	m_functionHash->Add(std::make_unique<Slic_GetNuclearLaunchTarget>().release());
	m_functionHash->Add(std::make_unique<Slic_TargetNuclearAttack>().release());


	m_functionHash->Add(std::make_unique<Slic_GetMapHeight>().release());
	m_functionHash->Add(std::make_unique<Slic_GetMapWidth>().release());

	m_functionHash->Add(std::make_unique<Slic_AddFeat>().release());

	m_functionHash->Add(std::make_unique<Slic_IsFortress>().release());

	m_functionHash->Add(std::make_unique<Slic_Distance>().release());
	m_functionHash->Add(std::make_unique<Slic_SquaredDistance>().release());
	m_functionHash->Add(std::make_unique<Slic_HasGood>().release());

	m_functionHash->Add(std::make_unique<Slic_GetRiotLevel>().release());
	m_functionHash->Add(std::make_unique<Slic_GetRevolutionLevel>().release());

	m_functionHash->Add(std::make_unique<Slic_CityFoodDelta>().release());
	m_functionHash->Add(std::make_unique<Slic_PlayerWagesExp>().release());
	m_functionHash->Add(std::make_unique<Slic_PlayerWorkdayExp>().release());
	m_functionHash->Add(std::make_unique<Slic_PlayerRationsExp>().release());
	m_functionHash->Add(std::make_unique<Slic_PlayerWorkdayLevel>().release());
	m_functionHash->Add(std::make_unique<Slic_PlayerRationsLevel>().release());
	m_functionHash->Add(std::make_unique<Slic_PlayerWagesLevel>().release());
	m_functionHash->Add(std::make_unique<Slic_CityStarvationTurns>().release());

	m_functionHash->Add(std::make_unique<Slic_GetUnitsAtLocation>().release());
	m_functionHash->Add(std::make_unique<Slic_GetUnitFromCell>().release());


	m_functionHash->Add(std::make_unique<Slic_TradePointsInUse>().release());

	m_functionHash->Add(std::make_unique<Slic_CityIsValid>().release());
	m_functionHash->Add(std::make_unique<Slic_GetCurrentYear>().release());
	m_functionHash->Add(std::make_unique<Slic_GetCurrentRound>().release());

	m_functionHash->Add(std::make_unique<Slic_CellOwner>().release());

	m_functionHash->Add(std::make_unique<Slic_CityIsNamed>().release());

	m_functionHash->Add(std::make_unique<Slic_StringCompare>().release());
	m_functionHash->Add(std::make_unique<Slic_CityNameCompare>().release());
	m_functionHash->Add(std::make_unique<Slic_ChangeGlobalRegard>().release());
	m_functionHash->Add(std::make_unique<Slic_SetCityVisible>().release());

	m_functionHash->Add(std::make_unique<Slic_IsCivilian>().release());

	m_functionHash->Add(std::make_unique<Slic_GetArmyFromUnit>().release());

	m_functionHash->Add(std::make_unique<Slic_FinishImprovements>().release());

	m_functionHash->Add(std::make_unique<Slic_RemoveAdvance>().release());
	m_functionHash->Add(std::make_unique<Slic_PlayerGold>().release());
	m_functionHash->Add(std::make_unique<Slic_ClearBattleFlag>().release());

	m_functionHash->Add(std::make_unique<Slic_MinimizeAction>().release());

	m_functionHash->Add(std::make_unique<Slic_SetAllCitiesVisible>().release());

	m_functionHash->Add(std::make_unique<Slic_IsUnitAtHead>().release());
	m_functionHash->Add(std::make_unique<Slic_OpenScenarioEditor>().release());

	//Readded Slic functions of CTP2.1 by Martin G�hmann
	m_functionHash->Add(std::make_unique<Slic_DestroyBuilding>().release());
	m_functionHash->Add(std::make_unique<Slic_OpenBuildQueue>().release());
	m_functionHash->Add(std::make_unique<Slic_TileHasImprovement>().release());
	m_functionHash->Add(std::make_unique<Slic_PlayerHasWonder>().release());
	m_functionHash->Add(std::make_unique<Slic_WonderOwner>().release());
	m_functionHash->Add(std::make_unique<Slic_CityHasWonder>().release());
	m_functionHash->Add(std::make_unique<Slic_ArmyIsValid>().release());
	m_functionHash->Add(std::make_unique<Slic_GetBorderIncursionBy>().release());
	m_functionHash->Add(std::make_unique<Slic_GetLastNewProposalType>().release());
	m_functionHash->Add(std::make_unique<Slic_GetLastNewProposalArg>().release());
	m_functionHash->Add(std::make_unique<Slic_GetLastNewProposalTone>().release());
	m_functionHash->Add(std::make_unique<Slic_GetLastResponseType>().release());
	m_functionHash->Add(std::make_unique<Slic_GetLastCounterResponseType>().release());
	m_functionHash->Add(std::make_unique<Slic_GetLastCounterResponseArg>().release());
	m_functionHash->Add(std::make_unique<Slic_GetLastThreatResponseType>().release());
	m_functionHash->Add(std::make_unique<Slic_GetLastThreatResponseArg>().release());
	m_functionHash->Add(std::make_unique<Slic_GetAgreementDuration>().release());
	m_functionHash->Add(std::make_unique<Slic_GetNewProposalPriority>().release());
	m_functionHash->Add(std::make_unique<Slic_GetNextAdvance>().release());
	m_functionHash->Add(std::make_unique<Slic_GetDesiredAdvanceFrom>().release());
	m_functionHash->Add(std::make_unique<Slic_GetLastBorderIncursion>().release());
	m_functionHash->Add(std::make_unique<Slic_GetPersonalityType>().release());
	m_functionHash->Add(std::make_unique<Slic_GetAtRiskCitiesValue>().release());
	m_functionHash->Add(std::make_unique<Slic_GetRelativeStrength>().release());
	m_functionHash->Add(std::make_unique<Slic_GetDesireWarWith>().release());
	m_functionHash->Add(std::make_unique<Slic_RoundPercentReduction>().release());
	m_functionHash->Add(std::make_unique<Slic_RoundGold>().release());
	m_functionHash->Add(std::make_unique<Slic_GetPollutionLevelPromisedTo>().release());
	m_functionHash->Add(std::make_unique<Slic_GetPiracyIncomeFrom>().release());
	m_functionHash->Add(std::make_unique<Slic_GetProjectedScience>().release());
	m_functionHash->Add(std::make_unique<Slic_CanFormAlliance>().release());
	m_functionHash->Add(std::make_unique<Slic_GetStopResearchingAdvance>().release());
	m_functionHash->Add(std::make_unique<Slic_GetNanoWeaponsCount>().release());
	m_functionHash->Add(std::make_unique<Slic_GetBioWeaponsCount>().release());
	m_functionHash->Add(std::make_unique<Slic_GetNuclearWeaponsCount>().release());
	m_functionHash->Add(std::make_unique<Slic_FindCityToExtortFrom>().release());
	m_functionHash->Add(std::make_unique<Slic_GetEmbargo>().release());
	m_functionHash->Add(std::make_unique<Slic_SetEmbargo>().release());
	m_functionHash->Add(std::make_unique<Slic_GetTotalValue>().release());
	m_functionHash->Add(std::make_unique<Slic_GetNewProposalResult>().release());
	m_functionHash->Add(std::make_unique<Slic_GetCounterProposalResult>().release());
	m_functionHash->Add(std::make_unique<Slic_GetMostAtRiskCity>().release());
	m_functionHash->Add(std::make_unique<Slic_GetRoundsToNextDisaster>().release());
	m_functionHash->Add(std::make_unique<Slic_GetCurrentPollutionLevel>().release());
	// New slicfunction by The Big Mc
	m_functionHash->Add(std::make_unique<Slic_FreeAllSlaves>().release());
	m_functionHash->Add(std::make_unique<Slic_AddSlaves>().release());
	// New good functions by MrBaggins
	m_functionHash->Add(std::make_unique<Slic_PlantSpecificGood>().release());
	m_functionHash->Add(std::make_unique<Slic_RemoveGood>().release());
	// Added by Peter Triggs
	m_functionHash->Add(std::make_unique<Slic_DeclareWar>().release());
	// Added by Martin G�hmann
	m_functionHash->Add(std::make_unique<Slic_CargoCapacity>().release());
	m_functionHash->Add(std::make_unique<Slic_MaxCargoSize>().release());
	m_functionHash->Add(std::make_unique<Slic_CargoSize>().release());
	m_functionHash->Add(std::make_unique<Slic_GetUnitFromCargo>().release());
	m_functionHash->Add(std::make_unique<Slic_GetContinent>().release());
	m_functionHash->Add(std::make_unique<Slic_GetContinentSize>().release());
	m_functionHash->Add(std::make_unique<Slic_IsWater>().release());
	//Added by Solver
	m_functionHash->Add(std::make_unique<Slic_IsOnSameContinent>().release());
	//Added by E
	m_functionHash->Add(std::make_unique<Slic_KillCity>().release());
	m_functionHash->Add(std::make_unique<Slic_Pillage>().release());
	m_functionHash->Add(std::make_unique<Slic_Plunder>().release());
	m_functionHash->Add(std::make_unique<Slic_Liberate>().release());
	m_functionHash->Add(std::make_unique<Slic_AddPW>().release());
	//m_functionHash->Add(std::make_unique<Slic_PuppetGovt>().release());
	//Added by Maq
	m_functionHash->Add(std::make_unique<Slic_CreateBuilding>().release());
	m_functionHash->Add(std::make_unique<Slic_CreateWonder>().release());
	m_functionHash->Add(std::make_unique<Slic_UnitMovementLeft>().release());
	m_functionHash->Add(std::make_unique<Slic_GetStoredProduction>().release());

}

void SlicEngine::Link()
{
    if (!m_segmentHash)
    {
        m_segmentHash = std::make_unique<SlicSegmentHash>(k_SEGMENT_HASH_SIZE);
    }

    if (!m_symTab)
    {
        m_symTab = std::make_unique<SlicSymTab>(0);
    }

    m_segmentHash->SetSize(slic_num_entries_Get());

    for (sint32 i = 0; i < slic_num_entries_Get(); i++)
    {
        SlicSegment * seg = std::make_unique<SlicSegment>(i).release();
        m_segmentHash->Add(seg->GetName(), seg);
    }

    m_segmentHash->LinkTriggerSymbols(m_uiHash.get());

    slicif_init();
    AddModFuncs();
}

extern "C" char slic_parser_error_text[1024];

bool SlicEngine::Load(std::basic_string<MBCHAR> const & a_File, sint32 filenum)
{
	slicconst_Initialize();

	sint32 symStart = m_symTab->GetNumEntries();

    AddBuiltinFunctions();
	slicif_init();
	slicif_set_file_num(filenum);

    bool isParsedOk =
        (SLIC_ERROR_OK ==
            slicif_run_parser(const_cast<MBCHAR *>(a_File.c_str()), symStart)
        );

	if (!isParsedOk)
    {
		c3errors_ErrorDialog(a_File.c_str(), "%s", slic_parser_error_text);
	}

	return isParsedOk;
}

void SlicEngine::AddTrigger(SlicSegment *trigger, TRIGGER_LIST which)
{
	sint32 t;
	for(t = sint32(which); t < sint32(TRIGGER_LIST_MAX); t++) {
		if(m_triggerLists[t]->Find(trigger))
			return;
	}

	PointerList<SlicSegment>::PointerListNode *node;

	for(t = sint32(which) - 1; t >= 0; t--) {
		if((node = m_triggerLists[t]->Find(trigger)) != nullptr) {
			m_triggerLists[t]->Remove(node);
		}
	}

	m_triggerLists[which]->AddTail(trigger);
}

void SlicEngine::SetCurrentMessage(const Message &message)
{
	*m_currentMessage = message;
}

void SlicEngine::GetCurrentMessage(Message &message) const
{
	message = *m_currentMessage;
}

void SlicEngine::KillCurrentMessage()
{
	MessagePool *mp = messagepool_Get();
	if(!m_currentMessage || !mp || !mp->IsValid(*m_currentMessage))
		return;
	m_currentMessage->Kill();
	*m_currentMessage = Message();
}

void SlicEngine::AddCurrentMessage()
{
	if(!messagepool_Get()->IsValid(*m_currentMessage))
		return;

	m_currentMessage->Minimize();
	*m_currentMessage = Message();
}

PointerList<SlicRecord> *SlicEngine::GetRecords(sint32 player)
{
	return m_records[player];
}

void SlicEngine::AddTutorialRecord(sint32 player, MBCHAR *title, MBCHAR *text,
								   SlicSegment *segment)
{
    if (!m_records[player])
    {
        m_records[player] = std::make_unique<PointerList<SlicRecord>>().release();
    }

    for
    (
        PointerList<SlicRecord>::Walker walk(m_records[player]);
        walk.IsValid();
        walk.Next()
    )
    {
        if (walk.GetObj()->GetSegment() == segment)
        {
            return;
	  }
    }

    m_records[player]->AddTail(std::make_unique<SlicRecord>(player, title, text, segment).release());

    if (title && gameobservers_Get()) {
        gameobservers_Get()->NotifyTutorialAddRecord(
            title, m_records[player]->GetCount() - 1);
    }
}

bool SlicEngine::IsTimerExpired(sint32 timer) const
{
	Assert(timer >= 0);
	Assert(timer < k_NUM_TIMERS);

	if (timer < 0 || timer >= k_NUM_TIMERS)
		return false;

	if (m_timer[timer] < 0)
		return false;

	return time(nullptr) >= static_cast<time_t>(m_timer[timer]);
}

void SlicEngine::StartTimer(sint32 timer, time_t duration)
{
	Assert(timer >= 0);
	Assert(timer < k_NUM_TIMERS);

	if (timer < 0 || timer >= k_NUM_TIMERS)
		return;

	m_timer[timer] = static_cast<sint32>(time(nullptr) + duration);
    Assert(static_cast<time_t>(m_timer[timer]) == (time(nullptr) + duration));
}

void SlicEngine::StopTimer(sint32 timer)
{
	Assert(timer >= 0);
	Assert(timer < k_NUM_TIMERS);

	if(timer < 0 || timer >= k_NUM_TIMERS)
		return;

	m_timer[timer] = NOT_IN_USE;
}

void SlicEngine::SetTutorialActive(BOOL on)
{
	if(!on && m_tutorialActive) {
		RunTrigger(TRIGGER_LIST_TUTORIAL_OFF,
				   ST_END);
	}

	m_tutorialActive = on;
	if(!m_tutorialActive) {

		EnableMessageClass(k_NON_TUTORIAL_MESSAGE_CLASS);
		if(player_arr_Get() && player_Get(m_tutorialPlayer)) {
			sint32 i;
			DynamicArray<Message> *msgs = player_Get(m_tutorialPlayer)->m_messages.get();
			for(i = 0; i < msgs->Num(); i++) {
				if(msgs->Access(i).AccessData()->GetSlicSegment() &&
				   msgs->Access(i).AccessData()->GetSlicSegment()->GetFilenum() == k_TUTORIAL_FILE) {
					player_Get(m_tutorialPlayer)->m_messages->Access(i).AccessData()->DisableClose(FALSE);
					player_Get(m_tutorialPlayer)->m_messages->Access(i).Kill();
				}
			}
		}
	}
}

void SlicEngine::EnableMessageClass(sint32 mclass)
{
	for (sint32 i = m_disabledClasses->Num() - 1; i >= 0; --i)
	{
		if (m_disabledClasses->Access(i) == mclass)
		{
			m_disabledClasses->DelIndex(i);
		}
	}
}

void SlicEngine::DisableMessageClass(sint32 mclass)
{
	m_disabledClasses->Insert(mclass);
}

bool SlicEngine::IsMessageClassDisabled(sint32 mclass) const
{
	for (sint32 i = 0; i < m_disabledClasses->Num(); ++i)
	{
		if (m_disabledClasses->Access(i) == mclass)
		{
			return true;
		}
	}
	return false;
}

void SlicEngine::ProcessUITriggers()
{
    for
    (
        SlicObject *    obj = m_uiExecuteObjects.RemoveHead();
        obj;
        obj = m_uiExecuteObjects.RemoveHead()
    )
    {
        Execute(obj);
    }
}

void SlicEngine::RunYearlyTriggers()
{
    for
    (
        PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_YEARLY]);
        walk.IsValid();
        walk.Next()
    )
    {
        if (walk.GetObj()->IsEnabled())
        {
		Execute(std::make_unique<SlicObject>(walk.GetObj()));
	  }
    }
}

void SlicEngine::RunPlayerTriggers(PLAYER_INDEX player)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_PLAYER]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->AddPlayer(player);
			Execute(std::move(obj));
		}
		walk.Next();
    }
}

void SlicEngine::RunCityTriggers(const Unit &city)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_CITY]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->AddCity(city);
			obj->AddPlayer(city.GetOwner());
			Execute(std::move(obj));
		}
		walk.Next();
    }
}

void SlicEngine::RunCityPopTriggers(const Unit &city)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_CITY_POP]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->AddCity(city);
			obj->AddCivilisation(city.GetOwner());
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunClickedUnitTriggers(const Unit &unit)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_CLICKED_UNIT]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->AddUnit(unit);
			obj->AddCivilisation(unit.GetOwner());
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunSelectedUnitTriggers(const Unit &unit)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_UNIT_SELECTED]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->AddUnit(unit);
			obj->AddCivilisation(unit.GetOwner());
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunDeselectedUnitTriggers(const Unit &unit)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_UNIT_DESELECTED]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->AddUnit(unit);
			obj->AddPlayer(unit.GetOwner());
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunDeselectedCityTriggers(const Unit &city)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_CITY_DESELECTED]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->AddCity(city);
			obj->AddPlayer(city.GetOwner());
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunIdleTriggers(sint32 seconds)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_IDLE]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->SetIdle(seconds);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunUnitMovedTriggers(const Unit &u)
{

	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_UNIT_MOVED]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->AddUnit(u);
			obj->AddCivilisation(u.GetOwner());
			Execute(std::move(obj));
			if(!u.IsValid())
				return;
		}
		walk.Next();
	}
}

void SlicEngine::RunAllUnitsMovedTriggers()
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_ALL_UNITS_MOVED]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->AddCivilisation(m_tutorialPlayer);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunCityBuiltTriggers(const Unit &city)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_CITY_BUILT]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->AddCity(city);
			obj->AddPlayer(city.GetOwner());
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunUnitBuiltTriggers(const Unit &u, const Unit &city)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_UNIT_BUILT]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			if(!u.IsValid()) {
				return;
			}
			if(!city.IsValid()) {
				return;
			}
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->AddUnit(u);
			obj->AddCity(city);
			obj->AddPlayer(u.GetOwner());
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunDiscoveryTriggers(AdvanceType adv, PLAYER_INDEX p)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_DISCOVERY]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->AddAdvance(adv);
			obj->AddPlayer(p);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunContactTriggers(const Unit &unit1, const Unit &unit2)
{
	if (    unit1.IsValid()
         &&	unit2.IsValid()
         && player_Get(unit1.GetOwner())
         && player_Get(unit2.GetOwner())
       )
    {
	    PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_CONTACT]);
	    while(walk.IsValid()) {
		    if(walk.GetObj()->IsEnabled()) {
			    auto obj = std::make_unique<SlicObject>(walk.GetObj());
			    obj->AddUnit(unit1);
			    obj->AddUnit(unit2);
			    obj->AddPlayer(unit1.GetOwner());
			    obj->AddPlayer(unit2.GetOwner());
			    Execute(std::move(obj));
		    }
		    walk.Next();
	    }
    }
}

void SlicEngine::RunAttackTriggers(const Unit &unit1, const Unit &unit2)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_ATTACK]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->AddUnit(unit1);
			obj->AddUnit(unit2);
			obj->AddPlayer(unit1.GetOwner());
			obj->AddPlayer(unit2.GetOwner());
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunTradeScreenTriggers()
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_TRADE_SCREEN]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			if (player_Get(player_view::VisiblePlayer()) != nullptr) {
				auto obj = std::make_unique<SlicObject>(walk.GetObj());
				obj->AddPlayer(player_view::VisiblePlayer());
				Execute(std::move(obj));
			}
		}
		walk.Next();
	}
}

void SlicEngine::RunSameGoodTriggers(const Unit &city1, const Unit &city2)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_SAME_GOOD]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			if (player_Get(player_view::VisiblePlayer()) != nullptr) {
				auto obj = std::make_unique<SlicObject>(walk.GetObj());
				obj->AddPlayer(player_view::VisiblePlayer());
				obj->AddCity(city1);
				obj->AddCity(city2);
				Execute(std::move(obj));
			}
		}
		walk.Next();
	}
}

bool SlicEngine::SpecialSameGoodEnabled() const
{
	return !m_triggerLists[TRIGGER_LIST_SAME_GOOD]->IsEmpty();
}

void SlicEngine::RunSameGoodAsTradedTriggers(sint32 good, const Unit &city1)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_SAME_GOOD_AS_TRADED]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			if (player_Get(player_view::VisiblePlayer()) != nullptr) {
				auto obj = std::make_unique<SlicObject>(walk.GetObj());
				obj->AddCivilisation(*player_Get(player_view::VisiblePlayer())->m_civilisation);
				obj->AddCity(city1);
				Execute(std::move(obj));
			}
		}
		walk.Next();
	}
}

void SlicEngine::RunUnitQueueTriggers()
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_UNIT_QUEUE]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			if (player_Get(player_view::VisiblePlayer()) != nullptr) {
				auto obj = std::make_unique<SlicObject>(walk.GetObj());
				obj->AddCivilisation(*player_Get(player_view::VisiblePlayer())->m_civilisation);
				Execute(std::move(obj));
			}
		}
		walk.Next();
	}
}

void SlicEngine::RunProductionQueueTriggers()
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_PRODUCTION_QUEUE]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			if (player_Get(player_view::VisiblePlayer()) != nullptr) {
				auto obj = std::make_unique<SlicObject>(walk.GetObj());
				obj->AddCivilisation(*player_Get(player_view::VisiblePlayer())->m_civilisation);
				Execute(std::move(obj));
			}
		}
		walk.Next();
	}
}

void SlicEngine::RunDiplomaticScreenTriggers()
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_DIPLOMATIC_SCREEN]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			if (player_Get(player_view::VisiblePlayer()) != nullptr) {
				auto obj = std::make_unique<SlicObject>(walk.GetObj());
				obj->AddCivilisation(*player_Get(player_view::VisiblePlayer())->m_civilisation);
				Execute(std::move(obj));
			}
		}
		walk.Next();
	}
}

void SlicEngine::RunCreateStackTriggers()
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_CREATE_STACK]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			if (player_Get(player_view::VisiblePlayer()) != nullptr) {
				auto obj = std::make_unique<SlicObject>(walk.GetObj());
				obj->AddCivilisation(*player_Get(player_view::VisiblePlayer())->m_civilisation);
				Execute(std::move(obj));
			}
		}
		walk.Next();
	}
}

void SlicEngine::RunCreateMixedStackTriggers()
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_CREATE_MIXED_STACK]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			if (player_Get(player_view::VisiblePlayer()) != nullptr) {
				auto obj = std::make_unique<SlicObject>(walk.GetObj());
				obj->AddCivilisation(*player_Get(player_view::VisiblePlayer())->m_civilisation);
				Execute(std::move(obj));
			}
		}
		walk.Next();
	}
}

void SlicEngine::RunAutoArrangeOffTriggers()
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_AUTO_ARRANGE_OFF]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			if (player_Get(player_view::VisiblePlayer()) != nullptr) {
				auto obj = std::make_unique<SlicObject>(walk.GetObj());
				obj->AddCivilisation(*player_Get(player_view::VisiblePlayer())->m_civilisation);
				Execute(std::move(obj));
			}
		}
		walk.Next();
	}
}

void SlicEngine::RunBombardmentTriggers(const Unit &attacker,
										const Unit &defender)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_BOMBARDMENT]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());

			obj->AddCivilisation(*player_Get(attacker.GetOwner())->m_civilisation);
			obj->AddCivilisation(*player_Get(defender.GetOwner())->m_civilisation);
			obj->AddUnit(attacker);
			obj->AddUnit(defender);

			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunCounterBombardmentTriggers(const Unit &bombarder,
											   const Unit &counterbombarder)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_COUNTER_BOMBARDMENT]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());

			obj->AddCivilisation(*player_Get(bombarder.GetOwner())->m_civilisation);
			obj->AddCivilisation(*player_Get(counterbombarder.GetOwner())->m_civilisation);
			obj->AddUnit(bombarder);
			obj->AddUnit(counterbombarder);

			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunActiveDefenseTriggers(const Unit &defender, const Unit &aggressor)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_ACTIVE_DEFENSE]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());

			obj->AddCivilisation(*player_Get(defender.GetOwner())->m_civilisation);
			obj->AddCivilisation(*player_Get(aggressor.GetOwner())->m_civilisation);
			obj->AddUnit(defender);
			obj->AddUnit(aggressor);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunIndulgenceTriggers(const Unit &cleric, const Unit &city)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_INDULGENCES]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());

			obj->AddCivilisation(*player_Get(cleric.GetOwner())->m_civilisation);
			obj->AddCivilisation(*player_Get(city.GetOwner())->m_civilisation);
			obj->AddUnit(cleric);
			obj->AddCity(city);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunTerrorismTriggers(const Unit &terrorist, const Unit &target)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_TERRORISM]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());

			obj->AddCivilisation(*player_Get(terrorist.GetOwner())->m_civilisation);
			obj->AddCivilisation(*player_Get(target.GetOwner())->m_civilisation);
			obj->AddUnit(terrorist);
			obj->AddCity(target);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunConversionTriggers(const Unit &cleric, const Unit &city)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_CONVERSION]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());

			obj->AddCivilisation(*player_Get(cleric.GetOwner())->m_civilisation);
			obj->AddCivilisation(*player_Get(city.GetOwner())->m_civilisation);
			obj->AddUnit(cleric);
			obj->AddUnit(city);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunWonderStartedTriggers(const Unit &city, sint32 wondertype)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_WONDER_STARTED]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());

			obj->AddCivilisation(*player_Get(city.GetOwner())->m_civilisation);
			obj->AddCity(city);
			obj->AddWonder(wondertype);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunWonderFinishedTriggers(const Unit &city, sint32 wondertype)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_WONDER_FINISHED]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());

			obj->AddCivilisation(*player_Get(city.GetOwner())->m_civilisation);
			obj->AddCity(city);
			obj->AddWonder(wondertype);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}


void SlicEngine::RunEnslavementTriggers(const Unit &slaver, const Unit &city)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_ENSLAVEMENT]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());

			obj->AddCivilisation(*player_Get(slaver.GetOwner())->m_civilisation);
			obj->AddCivilisation(*player_Get(city.GetOwner())->m_civilisation);
			obj->AddUnit(slaver);
			obj->AddCity(city);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunSettlerEnslavedTriggers(const Unit &slaver,
											sint32 settlerOwner)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_SETTLERENSLAVED]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());

			obj->AddCivilisation(*player_Get(slaver.GetOwner())->m_civilisation);
			obj->AddCivilisation(*player_Get(settlerOwner)->m_civilisation);
			obj->AddUnit(slaver);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunVictoryEnslavementTriggers(const Unit &slaver,
											   sint32 slavee,
											   Unit &hc)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_VICTORYENSLAVEMENT]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());

			obj->AddCivilisation(*player_Get(slaver.GetOwner())->m_civilisation);
			obj->AddCivilisation(*player_Get(slavee)->m_civilisation);
			obj->AddUnit(slaver);
			obj->AddCity(hc);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunUnitLaunchedTriggers(const Unit &launchee)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_UNIT_LAUNCHED]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());

			obj->AddCivilisation(*player_Get(launchee.GetOwner())->m_civilisation);
			obj->AddUnit(launchee);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunUnitBeginTurnTriggers(const Unit &unit)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_UNIT_BEGIN_TURN]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());

			obj->AddCivilisation(*player_Get(unit.GetOwner())->m_civilisation);
			obj->AddUnit(unit);
			Execute(std::move(obj));
			if(!unit.IsValid())
				return;
			if(!player_Get(unit.GetOwner()))
				return;
		}
		walk.Next();
	}
}

void SlicEngine::RunPopMovedTriggers(const Unit &city)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_POP_MOVED]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());

			obj->AddCivilisation(*player_Get(city.GetOwner())->m_civilisation);
			obj->AddCity(city);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunBuildFarmTriggers(sint32 owner, const MapPoint &point,
									 sint32 type)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_BUILD_FARM]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());

			obj->AddCivilisation(*player_Get(owner)->m_civilisation);
			obj->AddLocation(point);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunBuildRoadTriggers(sint32 owner, const MapPoint &point,
									  sint32 type)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_BUILD_ROAD]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());

			obj->AddCivilisation(*player_Get(owner)->m_civilisation);
			obj->AddLocation(point);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunBuildMineTriggers(sint32 owner, const MapPoint &point,
									  sint32 type)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_BUILD_MINE]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());

			obj->AddCivilisation(*player_Get(owner)->m_civilisation);
			obj->AddLocation(point);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunBuildInstallationTriggers(sint32 owner, const MapPoint &point,
									  sint32 type)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_BUILD_INSTALLATION]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());

			obj->AddCivilisation(*player_Get(owner)->m_civilisation);
			obj->AddLocation(point);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunBuildTransformTriggers(sint32 owner, const MapPoint &point,
									  sint32 type)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_BUILD_TRANSFORM]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());

			obj->AddCivilisation(*player_Get(owner)->m_civilisation);
			obj->AddLocation(point);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunScienceRateTriggers(sint32 owner)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_SCIENCE_RATE]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->AddCivilisation(*player_Get(owner)->m_civilisation);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunCityCapturedTriggers(sint32 newowner, sint32 oldowner, const Unit &city)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_CITY_CAPTURED]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->AddCivilisation(*player_Get(newowner)->m_civilisation);
			obj->AddCivilisation(*player_Get(oldowner)->m_civilisation);
			obj->AddCity(city);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunTradeOfferTriggers(const TradeOffer &offer)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_TRADE_OFFER]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->AddGood(offer.GetOfferResource());
			obj->AddGold(offer.GetAskingResource());
			obj->AddCivilisation(*player_Get(offer.GetFromCity().GetOwner())->m_civilisation);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunTreatyBrokenTriggers(sint32 pl1, sint32 pl2, const Agreement &ag)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_TREATY_BROKEN]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->AddCivilisation(*player_Get(pl1)->m_civilisation);
			obj->AddCivilisation(*player_Get(pl2)->m_civilisation);

			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunUnitDeadTriggers(const Unit &unit, PLAYER_INDEX killedBy)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_UNIT_DEAD]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->AddCivilisation(*player_Get(unit.GetOwner())->m_civilisation);
			if(killedBy >= 0) {
				obj->AddCivilisation(killedBy);
			}
			obj->AddUnit(unit);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunOutOfFuelTriggers(const Unit &unit)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_UNIT_DEAD_OUTOFFUEL]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->AddCivilisation(*player_Get(unit.GetOwner())->m_civilisation);
			obj->AddUnit(unit);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}
void SlicEngine::RunUnitCantBeSupportedTriggers(const Unit &unit)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_UNIT_DEAD_CANT_SUPPORT]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->AddCivilisation(*player_Get(unit.GetOwner())->m_civilisation);
			obj->AddUnit(unit);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}
void SlicEngine::RunMiscUnitDeathTriggers(const Unit &unit)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_UNIT_DEAD_MISC]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->AddCivilisation(*player_Get(unit.GetOwner())->m_civilisation);
			obj->AddUnit(unit);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunDiscoveryTradedTriggers(sint32 pl1, sint32 pl2, AdvanceType adv)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_DISCOVERY_TRADED]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->AddCivilisation(*player_Get(pl1)->m_civilisation);
			obj->AddCivilisation(*player_Get(pl2)->m_civilisation);
			obj->AddAdvance(adv);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunUITriggers(const MBCHAR *controlName)
{
	DPRINTF(k_DBG_UI, ("SLIC: control %s used\n", controlName));

	SlicUITrigger * trig =
		controlName ? m_uiHash->Access(controlName) : nullptr;

	if(trig) {
		SlicSegment *seg = trig->GetSegment();
		if(seg && seg->IsEnabled()) {
			m_uiExecuteObjects.AddTail(std::make_unique<SlicObject>(seg).release());

		}
	}
}

void SlicEngine::RunHelpTriggers(const MBCHAR *helpName)
{
	DPRINTF(k_DBG_UI, ("SLIC: help for component %s requested\n", helpName));
	if(!helpName)
		return;

	SlicSegment *seg = m_segmentHash->Access(helpName);
	if(seg && seg->IsEnabled()) {
		auto obj = std::make_unique<SlicObject>(seg);
		{
			sint32 visible = player_view::VisiblePlayer();
			if (visible >= 0) obj->AddRecipient(visible);
		}
		Execute(std::move(obj));
	}
}

void SlicEngine::RunPopMovedOffGoodTriggers(const Unit &city)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_POP_MOVED_OFF_GOOD]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());

			obj->AddCivilisation(*player_Get(city.GetOwner())->m_civilisation);
			obj->AddCity(city);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunWastingWorkTriggers(const Unit &city)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_CITY_BUILDINGNOTHING]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());

			obj->AddCivilisation(*player_Get(city.GetOwner())->m_civilisation);
			obj->AddCity(city);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunPublicWorksTaxTriggers(sint32 owner)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_PUBLIC_WORKS_TAX]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());

			obj->AddCivilisation(owner);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunWonderAlmostDoneTriggers(const Unit &city, sint32 wonder)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_WONDER_ALMOST_DONE]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->AddCity(city);
			obj->AddCivilisation(city.GetOwner());
			obj->AddWonder(wonder);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunGovernmentChangedTriggers(sint32 player)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_GOVERNMENT_CHANGED]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->AddCivilisation(player);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunTradeRouteTriggers(TradeRoute &route, sint32 gold)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_TRADE_ROUTE]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->AddCivilisation(route.GetOwner());
			obj->AddCity(route.GetSource());
			obj->AddCity(route.GetDestination());
			obj->AddGold(gold);
			sint32 resource;
			ROUTE_TYPE type;
			route.GetSourceResource(type, resource);
			obj->AddGood(resource);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunForeignTradeRouteTriggers(TradeRoute &route, sint32 gold)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_FOREIGN_TRADE_ROUTE]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->AddCivilisation(route.GetSource().GetOwner());
			obj->AddCivilisation(route.GetDestination().GetOwner());
			obj->AddCity(route.GetSource());
			obj->AddCity(route.GetDestination());
			obj->AddGold(gold);
			sint32 resource;
			ROUTE_TYPE type;
			route.GetSourceResource(type, resource);
			obj->AddGood(resource);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunUnitDoneMovingTriggers(const Unit &unit)
{

	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_UNIT_DONE_MOVING]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->AddUnit(unit);
			obj->AddCivilisation(*player_Get(unit.GetOwner())->m_civilisation);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunPiracyTriggers(const TradeRoute &route, const Unit &unit)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_PIRACY]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());

			obj->AddCivilisation(route.GetSource().GetOwner());

			obj->AddCivilisation(route.GetDestination().GetOwner());

			obj->AddCivilisation(*player_Get(unit.GetOwner())->m_civilisation);
			obj->AddUnit(unit);

			obj->AddCity(route.GetSource());

			obj->AddCity(route.GetDestination());

			ROUTE_TYPE type;
			sint32 resource;
			route.GetSourceResource(type, resource);
			Assert(type == ROUTE_TYPE_RESOURCE);
			obj->AddGood(resource);

			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunPillageTriggers(const Unit &unit, PLAYER_INDEX pillagee)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_PIRACY]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->AddUnit(unit);
			obj->AddCivilisation(unit.GetOwner());
			obj->AddCivilisation(pillagee);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunCitySelectedTriggers(const Unit &city)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_CITY_SELECTED]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->AddCity(city);
			obj->AddCivilisation(city.GetOwner());
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunClickedOnUnexploredTriggers(const MapPoint &pos)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_CLICKED_UNEXPLORED]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->AddLocation(pos);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunZOCTriggers()
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_ZOC]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunCantSettleMovementTriggers(const Unit &unit)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_CANT_SETTLE_MOVEMENT]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->AddUnit(unit);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunBuildingBuiltTriggers(const Unit &city, sint32 building)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_BUILDING_BUILT]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->AddBuilding(building);
			obj->AddCity(city);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunAgeChangeTriggers(sint32 player)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_AGE_CHANGE]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->AddCivilisation(player);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunTimerTriggers()
{
	for (sint32 i = 0; i < k_NUM_TIMERS; ++i)
	{
		if (IsTimerExpired(i))
		{
			m_timer[i] = NOT_IN_USE;
            gevmanager_Get()->AddEvent(GEV_INSERT_Tail,GEV_TimerExpired,GEA_Int,i, GEA_End);
		}
	}
}

void SlicEngine::RunWorkViewTriggers()
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_WORK_VIEW]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunSentCeaseFireTriggers(sint32 owner, sint32 recipient)
{
	PointerList<SlicSegment>::Walker walk(m_triggerLists[TRIGGER_LIST_CEASE_FIRE]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			obj->AddCivilisation(owner);
			obj->AddCivilisation(recipient);
			Execute(std::move(obj));
		}
		walk.Next();
	}
}

void SlicEngine::RunTrigger(sint32 tlist, ...)
{
	Assert(tlist >= TRIGGER_LIST_YEARLY);
	Assert(tlist < TRIGGER_LIST_MAX);
	if(tlist < TRIGGER_LIST_YEARLY || tlist >= TRIGGER_LIST_MAX)
		return;

	Unit u;
	sint32 player;
	sint32 good;
	sint32 gold;
	sint32 advance;
	MapPoint pos;
	char *str;

	BOOL abort = FALSE;

	PointerList<SlicSegment>::Walker walk(m_triggerLists[tlist]);
	while(walk.IsValid()) {
		if(walk.GetObj()->IsEnabled()) {
			auto obj = std::make_unique<SlicObject>(walk.GetObj());
			BOOL done = FALSE;
			SLIC_TAG tag;
			va_list vl;
			va_start(vl, tlist);
			do {
				tag = va_arg(vl, SLIC_TAG);
				switch(tag) {
					case ST_END:
						done = TRUE;
						break;
					case ST_UNIT:
						u = va_arg(vl, Unit);
						if (u.IsValid())
                        {
							obj->AddUnit(u);
						} else {
							abort = TRUE;
						}
						break;
					case ST_CITY:
						u = va_arg(vl, Unit);
						if (u.IsValid())
                        {
							obj->AddCity(u);
						} else {
							abort = TRUE;
						}
						break;
					case ST_PLAYER:
						player = va_arg(vl, sint32);
						if(player_Get(player)) {
							obj->AddCivilisation(player);
						} else {
							abort = TRUE;
						}
						break;
					case ST_GOOD:
						good = va_arg(vl, sint32);
						obj->AddGood(good);
						break;
					case ST_GOLD:
						gold = va_arg(vl, sint32);
						obj->AddGold(gold);
						break;
					case ST_LOCATION:
						pos = va_arg(vl, MapPoint);
						obj->AddLocation(pos);
						break;
					case ST_ACTION:
						str = va_arg(vl, char*);
						obj->AddAction(str);
						break;
					case ST_ADVANCE:
						advance = va_arg(vl, sint32);
						obj->AddAdvance(advance);
						break;
					default:
					{
						BOOL Unknown_Tag = FALSE;
						Assert(Unknown_Tag);
						break;
					}
				}
			} while(!done && !abort);
			va_end(vl);
			if(!abort) {
				Execute(std::move(obj));
			}
		}
		if(abort)
			break;
		walk.Next();
	}
}

void SlicEngine::RecreateTutorialRecord()
{
	if (!gameobservers_Get()) return;

	if (m_tutorialActive && m_records[m_tutorialPlayer]) {
		sint32 c = 0;
		PointerList<SlicRecord>::Walker walk(m_records[m_tutorialPlayer]);
		while (walk.IsValid()) {
			gameobservers_Get()->NotifyTutorialAddRecord(
				walk.GetObj()->AccessTitle(), c++);
			walk.Next();
		}
	}
}

MBCHAR SlicEngine::GetTriggerKey(sint32 index)
{
	Assert(index >= 0);
	Assert(index < k_MAX_TRIGGER_KEYS);
	if(index >= 0 && index < k_MAX_TRIGGER_KEYS)
		return m_triggerKey[index];
	return 0;
}

void SlicEngine::SetTriggerKey(sint32 index, MBCHAR key)
{
	Assert(index >= 0);
	Assert(index < k_MAX_TRIGGER_KEYS);
	if(index >= 0 && index < k_MAX_TRIGGER_KEYS)
		m_triggerKey[index] = key;
}

bool SlicEngine::IsKeyPressed(MBCHAR key) const
{
	return m_currentKeyTrigger && (m_currentKeyTrigger == m_triggerKey[static_cast<unsigned char>(key)]);
}

bool SlicEngine::RunKeyboardTrigger(MBCHAR key)
{
	for (char i : m_triggerKey)
	{
		if (i == key)
		{
			m_currentKeyTrigger = key;
			RunTrigger(TRIGGER_LIST_KEY_PRESSED, ST_END);
			m_currentKeyTrigger = KEY_UNDEFINED;
			return true;
		}
	}

	return false;
}

void SlicEngine::BlankScreen(bool blank)
{
	if (m_blankScreen == blank) return;

	m_blankScreen = blank;
	tiledmap_observer::InvalidateMap();
	tiledmap_observer::Refresh();

	if (!blank) {
		// Fire any deferred research-advance dialog before re-rendering
		// the control panel so the great-library snaps to that advance.
		CheckPendingResearch();
	}

	sint32 visible = player_view::VisiblePlayer();
	sint32 researching = (visible >= 0 && player_Get(visible))
		? player_Get(visible)->m_advances->GetResearching()
		: -1;
	if (gameobservers_Get()) {
		gameobservers_Get()->NotifyBlankScreenChanged(blank, visible, researching);
	}
}

void SlicEngine::CheckPendingResearch()
{
	if(m_doResearchOnUnblank) {
		m_doResearchOnUnblank = FALSE;
		if (m_researchOwner == player_view::VisiblePlayer() && gameobservers_Get()) {
			gameobservers_Get()->NotifyResearchAdvanceDialog(
				m_researchOwner, -1, m_researchText);
		}
	}
}

void SlicEngine::AddResearchOnUnblank(sint32 owner, MBCHAR *text)
{
	strlcpy(m_researchText, text, sizeof(m_researchText));
	m_researchOwner = owner;
	m_doResearchOnUnblank = TRUE;
}

SlicSymbolData *SlicEngine::CheckForBuiltinWithIndex(MBCHAR *name, sint32 &index)
{
	return FALSE;
}

void SlicEngine::AddConst(const MBCHAR *name, sint32 value)
{
	m_constHash->Add(std::make_unique<SlicConst>(name, value).release());
}

bool SlicEngine::FindConst(const MBCHAR *name, sint32 *value) const
{
	SlicConst * sc = m_constHash->Access(name);
	if (sc)
	{
		*value = sc->GetValue();
		return true;
	}
	return false;
}

void SlicEngine::AddStructArray(bool createSymbols, SlicStructDescription *desc, SLIC_BUILTIN which)
{
	m_builtin_desc[which] = desc;

	if (createSymbols)
    {
        SlicBuiltinNamedSymbol *    newSymbol =
		    std::make_unique<SlicBuiltinNamedSymbol>(which, desc->GetName(), std::make_unique<SlicArray>(desc).release()).release();
		m_builtins[which] = newSymbol;  // not deleted, managed through m_symTab
        m_symTab->Add(newSymbol);
	}
}

void SlicEngine::AddStruct(bool createSymbols, SlicStructDescription *desc, SLIC_BUILTIN which)
{
    m_builtin_desc[which] = desc;

    if (createSymbols)
    {
        SlicBuiltinNamedSymbol *    newSymbol =
		    std::make_unique<SlicBuiltinNamedSymbol>(which, desc->GetName(), desc).release();
        m_builtins[which] = newSymbol;  // not deleted, managed through m_symTab
		m_symTab->Add(newSymbol);
	}
}

void SlicEngine::AddStructs(bool createSymbols)
{
	AddStruct(createSymbols, std::make_unique<SlicStruct_Global>().release(), SLIC_BUILTIN_GLOBAL);

	AddStructArray(createSymbols, std::make_unique<SlicStruct_Unit>().release(), SLIC_BUILTIN_UNIT);
	AddStructArray(createSymbols, std::make_unique<SlicStruct_City>().release(), SLIC_BUILTIN_CITY);
	AddStructArray(createSymbols, std::make_unique<SlicStruct_Player>().release(), SLIC_BUILTIN_PLAYER);
	AddStructArray(createSymbols, std::make_unique<SlicStruct_Army>().release(), SLIC_BUILTIN_ARMY);
	AddStructArray(createSymbols, std::make_unique<SlicStruct_Location>().release(), SLIC_BUILTIN_LOCATION);
	AddStructArray(createSymbols, std::make_unique<SlicStruct_Government>().release(), SLIC_BUILTIN_GOVERNMENT);
	AddStructArray(createSymbols, std::make_unique<SlicStruct_Advance>().release(), SLIC_BUILTIN_ADVANCE);
	AddStructArray(createSymbols, std::make_unique<SlicStruct_Action>().release(), SLIC_BUILTIN_ACTION);
	AddStructArray(createSymbols, std::make_unique<SlicStruct_Value>().release(), SLIC_BUILTIN_VALUE);
	AddStructArray(createSymbols, std::make_unique<SlicStruct_Improvement>().release(), SLIC_BUILTIN_IMPROVEMENT);
	AddStructArray(createSymbols, std::make_unique<SlicStruct_Building>().release(), SLIC_BUILTIN_BUILDING);
	AddStructArray(createSymbols, std::make_unique<SlicStruct_Wonder>().release(), SLIC_BUILTIN_WONDER);
	AddStructArray(createSymbols, std::make_unique<SlicStruct_UnitRecord>().release(), SLIC_BUILTIN_UNITRECORD);
	AddStructArray(createSymbols, std::make_unique<SlicStruct_Gold>().release(), SLIC_BUILTIN_GOLD);
	AddStructArray(createSymbols, std::make_unique<SlicStruct_Good>().release(), SLIC_BUILTIN_GOOD);

	if(createSymbols && false) {
		m_symTab->Add(std::make_unique<SlicNamedSymbol>("special", std::make_unique<SlicArray>(SS_TYPE_SYM, SLIC_SYM_STRUCT).release()).release());
		m_symTab->Add(std::make_unique<SlicNamedSymbol>("discovery", std::make_unique<SlicArray>(SS_TYPE_SYM, SLIC_SYM_STRUCT).release()).release());
		m_symTab->Add(std::make_unique<SlicNamedSymbol>("gold", std::make_unique<SlicArray>(SS_TYPE_SYM, SLIC_SYM_STRUCT).release()).release());
		m_symTab->Add(std::make_unique<SlicNamedSymbol>("pop", std::make_unique<SlicArray>(SS_TYPE_SYM, SLIC_SYM_STRUCT).release()).release());
	}

}

void SlicEngine::AddBuiltinSymbol(SlicBuiltinNamedSymbol *sym)
{
    size_t const index = sym->GetBuiltin();

    Assert(index < SLIC_BUILTIN_MAX);
	m_builtins[index] = sym;
}

SlicSymbolData const * SlicEngine::GetBuiltinSymbol(SLIC_BUILTIN which) const
{
	Assert(which >= 0);
	Assert(which < SLIC_BUILTIN_MAX);
	if(which < 0 || which >= SLIC_BUILTIN_MAX)
		return nullptr;

	return m_builtins[which];
}

void SlicEngine::AddSymbol(SlicNamedSymbol *sym)
{
	m_symTab->Add(sym);
}

void SlicEngine::SetContext(SlicObject *obj)
{
	if (m_context)
	{
		m_context->Release();
	}

	m_context = obj;

	if (obj)
	{
		obj->AddRef();
		obj->FillBuiltins();
	}
}

SlicStructDescription *SlicEngine::GetStructDescription(SLIC_SYM which)
{
	switch(which) {
		case SLIC_SYM_UNIT: return m_builtin_desc[SLIC_BUILTIN_UNIT];
		case SLIC_SYM_CITY: return m_builtin_desc[SLIC_BUILTIN_CITY];
		case SLIC_SYM_ARMY: return m_builtin_desc[SLIC_BUILTIN_ARMY];
		case SLIC_SYM_LOCATION: return m_builtin_desc[SLIC_BUILTIN_LOCATION];
		case SLIC_SYM_PLAYER: return m_builtin_desc[SLIC_BUILTIN_PLAYER];
		case SLIC_SYM_POP: return m_builtin_desc[SLIC_BUILTIN_POP];

		default:
			return nullptr;
	}
}

SlicStructDescription *SlicEngine::GetStructDescription(SLIC_BUILTIN which)
{
	Assert(which >= 0);
	Assert(which < SLIC_BUILTIN_MAX);
	if(which < 0 || which >= SLIC_BUILTIN_MAX)
		return nullptr;
	return m_builtin_desc[which];
}

void SlicEngine::Break(SlicSegment *segment, sint32 offset, SlicObject *context, SlicStack *stack,
					   MessageData *message)
{

	Assert(!m_atBreak);
	if(m_atBreak)
		return;

	m_breakRequested = false;
	m_breakContext = context;
	m_breakContext->CopyFromBuiltins();
	m_breakContext->AddRef();

	m_atBreak = true;

#ifdef CTP2_ENABLE_SLICDEBUG
	// Forward-declared here so we don't pull ui/slic_debug/sourcelist.h
	// into the simulation core just for the debug-only call.
	extern void sourcelist_RegisterBreak(SlicSegment *segment, sint32 offset);
	sourcelist_RegisterBreak(segment, offset);
#endif
}

void SlicEngine::Continue()
{

	Assert(m_atBreak);
	if(!m_atBreak)
		return;

	m_atBreak = false;

	SetContext(m_breakContext);
	m_breakContext->Continue();
	m_breakContext->Release();
    m_breakContext = nullptr;

	if (!m_atBreak)
    {
	    for
        (
            SlicObject * oldContext = m_contextStack.RemoveTail();
            oldContext;
            oldContext = m_contextStack.RemoveTail()
        )
        {
			SetContext(oldContext);
			oldContext->Continue();
		}
	}

	SetContext(nullptr);
}

void SlicEngine::RequestBreak()
{
	m_breakRequested = true;
}

bool SlicEngine::BreakRequested()
{
	return m_breakRequested;
}

void SlicEngine::PushContext(SlicObject * obj)
{
	if (m_context)
	{
		m_contextStack.AddTail(m_context);
	}

	m_context = obj;

	if (obj)
	{
		obj->AddRef();
		obj->FillBuiltins();
	}
}

void SlicEngine::PopContext()
{
	if (m_context)
	{
		m_context->Release();
	}

	m_context = m_contextStack.RemoveTail();

	if(m_context){
		// TODO check whether builtins filling is superflous.
		m_context->FillBuiltins(); // Builtins were overwritten in PushContext
	}
}

void SlicEngine::AddDatabases()
{
	if(m_dbHash)
		return;

	m_dbHash = std::make_unique<StringHash<SlicDBInterface>>(k_DB_HASH_SIZE);

	m_dbHash->Add(std::make_unique<SlicDBConduit<UnitRecord, UnitRecordAccessorInfo>>("UnitDB", g_theUnitDB,
																		g_UnitRecord_Accessors,
																		g_Unit_Tokens,
																		k_Num_UnitRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<AdvanceRecord, AdvanceRecordAccessorInfo>>("AdvanceDB", g_theAdvanceDB,
																			  g_AdvanceRecord_Accessors,
																			  g_Advance_Tokens,
																			  k_Num_AdvanceRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<TerrainRecord, TerrainRecordAccessorInfo>>("TerrainDB", g_theTerrainDB,
																			  g_TerrainRecord_Accessors,
																			  g_Terrain_Tokens,
																			  k_Num_TerrainRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<BuildingRecord, BuildingRecordAccessorInfo>>("BuildingDB", g_theBuildingDB,
																				g_BuildingRecord_Accessors,
																				g_Building_Tokens,
																				k_Num_BuildingRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<WonderRecord, WonderRecordAccessorInfo>>("WonderDB", g_theWonderDB,
																			g_WonderRecord_Accessors,
																			g_Wonder_Tokens,
																			k_Num_WonderRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<FeatRecord, FeatRecordAccessorInfo>>("FeatDB", g_theFeatDB,
																		g_FeatRecord_Accessors,
																		g_Feat_Tokens,
																		k_Num_FeatRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<ResourceRecord, ResourceRecordAccessorInfo>>("ResourceDB", g_theResourceDB,
																				g_ResourceRecord_Accessors,
																				g_Resource_Tokens,
																				k_Num_ResourceRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<OrderRecord, OrderRecordAccessorInfo>>("OrderDB", g_theOrderDB,
																		  g_OrderRecord_Accessors,
																		  g_Order_Tokens,
																		  k_Num_OrderRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<TerrainImprovementRecord,
									TerrainImprovementRecordAccessorInfo>>("TerrainImprovementDB",
																		  g_theTerrainImprovementDB,
																		  g_TerrainImprovementRecord_Accessors,
																		  g_TerrainImprovement_Tokens,
																		  k_Num_TerrainImprovementRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<GovernmentRecord,
									GovernmentRecordAccessorInfo>>("GovernmentDB", g_theGovernmentDB,
																  g_GovernmentRecord_Accessors,
																  g_Government_Tokens,
																  k_Num_GovernmentRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<StrategyRecord,
									StrategyRecordAccessorInfo>>("StrategyDB", g_theStrategyDB,
																  g_StrategyRecord_Accessors,
																  g_Strategy_Tokens,
																  k_Num_StrategyRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<DiplomacyRecord,
									DiplomacyRecordAccessorInfo>>("DiplomacyDB", g_theDiplomacyDB,
																  g_DiplomacyRecord_Accessors,
																  g_Diplomacy_Tokens,
																  k_Num_DiplomacyRecord_Tokens).release());
	//The rest of the new databases available through slic added by Martin G�hmann
	m_dbHash->Add(std::make_unique<SlicDBConduit<PersonalityRecord,
									PersonalityRecordAccessorInfo>>("PersonalityDB", g_thePersonalityDB,
																  g_PersonalityRecord_Accessors,
																  g_Personality_Tokens,
																  k_Num_PersonalityRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<AdvanceBranchRecord,
									AdvanceBranchRecordAccessorInfo>>("AdvanceBranchDB", g_theAdvanceBranchDB,
																  g_AdvanceBranchRecord_Accessors,
																  g_AdvanceBranch_Tokens,
																  k_Num_AdvanceBranchRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<AdvanceListRecord,
									AdvanceListRecordAccessorInfo>>("AdvanceListDB", g_theAdvanceListDB,
																  g_AdvanceListRecord_Accessors,
																  g_AdvanceList_Tokens,
																  k_Num_AdvanceListRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<AgeCityStyleRecord,
									AgeCityStyleRecordAccessorInfo>>("AgeCityStyleDB", g_theAgeCityStyleDB,
																  g_AgeCityStyleRecord_Accessors,
																  g_AgeCityStyle_Tokens,
																  k_Num_AgeCityStyleRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<AgeRecord, AgeRecordAccessorInfo>>("AgeDB", g_theAgeDB,
																  g_AgeRecord_Accessors,
																  g_Age_Tokens,
																  k_Num_AgeRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<BuildingBuildListRecord,
									BuildingBuildListRecordAccessorInfo>>("BuildingBuildListDB", g_theBuildingBuildListDB,
																  g_BuildingBuildListRecord_Accessors,
																  g_BuildingBuildList_Tokens,
																  k_Num_BuildingBuildListRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<BuildListSequenceRecord,
									BuildListSequenceRecordAccessorInfo>>("BuildListSequenceDB", g_theBuildListSequenceDB,
																  g_BuildListSequenceRecord_Accessors,
																  g_BuildListSequence_Tokens,
																  k_Num_BuildListSequenceRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<CitySizeRecord,
									CitySizeRecordAccessorInfo>>("CitySizeDB", g_theCitySizeDB,
																  g_CitySizeRecord_Accessors,
																  g_CitySize_Tokens,
																  k_Num_CitySizeRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<CityStyleRecord,
									CityStyleRecordAccessorInfo>>("CityStyleDB", g_theCityStyleDB,
																  g_CityStyleRecord_Accessors,
																  g_CityStyle_Tokens,
																  k_Num_CityStyleRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<DiplomacyProposalRecord,
									DiplomacyProposalRecordAccessorInfo>>("DiplomacyProposalDB", g_theDiplomacyProposalDB,
																  g_DiplomacyProposalRecord_Accessors,
																  g_DiplomacyProposal_Tokens,
																  k_Num_DiplomacyProposalRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<DiplomacyThreatRecord,
									DiplomacyThreatRecordAccessorInfo>>("DiplomacyThreatDB", g_theDiplomacyThreatDB,
																  g_DiplomacyThreatRecord_Accessors,
																  g_DiplomacyThreat_Tokens,
																  k_Num_DiplomacyThreatRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<EndGameObjectRecord,
									EndGameObjectRecordAccessorInfo>>("EndGameObjectDB", g_theEndGameObjectDB,
																  g_EndGameObjectRecord_Accessors,
																  g_EndGameObject_Tokens,
																  k_Num_EndGameObjectRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<GoalRecord, GoalRecordAccessorInfo>>("GoalDB", g_theGoalDB,
																  g_GoalRecord_Accessors,
																  g_Goal_Tokens,
																  k_Num_GoalRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<IconRecord, IconRecordAccessorInfo>>("IconDB", g_theIconDB,
																  g_IconRecord_Accessors,
																  g_Icon_Tokens,
																  k_Num_IconRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<ImprovementListRecord,
									ImprovementListRecordAccessorInfo>>("ImprovementListDB", g_theImprovementListDB,
																  g_ImprovementListRecord_Accessors,
																  g_ImprovementList_Tokens,
																  k_Num_ImprovementListRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<PopRecord, PopRecordAccessorInfo>>("PopDB", g_thePopDB,
																  g_PopRecord_Accessors,
																  g_Pop_Tokens,
																  k_Num_PopRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<SoundRecord, SoundRecordAccessorInfo>>("SoundDB", g_theSoundDB,
																  g_SoundRecord_Accessors,
																  g_Sound_Tokens,
																  k_Num_SoundRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<SpecialAttackInfoRecord,
									SpecialAttackInfoRecordAccessorInfo>>("SpecialAttackInfoDB", g_theSpecialAttackInfoDB,
																  g_SpecialAttackInfoRecord_Accessors,
																  g_SpecialAttackInfo_Tokens,
																  k_Num_SpecialAttackInfoRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<SpecialEffectRecord,
									SpecialEffectRecordAccessorInfo>>("SpecialEffectDB", g_theSpecialEffectDB,
																  g_SpecialEffectRecord_Accessors,
																  g_SpecialEffect_Tokens,
																  k_Num_SpecialEffectRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<SpriteRecord, SpriteRecordAccessorInfo>>("SpriteDB", g_theSpriteDB,
																  g_SpriteRecord_Accessors,
																  g_Sprite_Tokens,
																  k_Num_SpriteRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<UnitBuildListRecord,
									UnitBuildListRecordAccessorInfo>>("UnitBuildListDB", g_theUnitBuildListDB,
																  g_UnitBuildListRecord_Accessors,
																  g_UnitBuildList_Tokens,
																  k_Num_UnitBuildListRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<WonderBuildListRecord,
									WonderBuildListRecordAccessorInfo>>("WonderBuildListDB", g_theWonderBuildListDB,
																  g_WonderBuildListRecord_Accessors,
																  g_WonderBuildList_Tokens,
																  k_Num_WonderBuildListRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<WonderMovieRecord,
									WonderMovieRecordAccessorInfo>>("WonderMovieDB", g_theWonderMovieDB,
																  g_WonderMovieRecord_Accessors,
																  g_WonderMovie_Tokens,
																  k_Num_WonderMovieRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<CivilisationRecord,
									CivilisationRecordAccessorInfo>>("CivilisationDB", g_theCivilisationDB,
																  g_CivilisationRecord_Accessors,
																  g_Civilisation_Tokens,
																  k_Num_CivilisationRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<RiskRecord,
									RiskRecordAccessorInfo>>("RiskDB", g_theRiskDB,
																  g_RiskRecord_Accessors,
																  g_Risk_Tokens,
																  k_Num_RiskRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<DifficultyRecord,
									DifficultyRecordAccessorInfo>>("DifficultyDB", g_theDifficultyDB,
																  g_DifficultyRecord_Accessors,
																  g_Difficulty_Tokens,
																  k_Num_DifficultyRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<PollutionRecord,
									PollutionRecordAccessorInfo>>("PollutionDB", g_thePollutionDB,
																  g_PollutionRecord_Accessors,
																  g_Pollution_Tokens,
																  k_Num_PollutionRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<GlobalWarmingRecord,
									GlobalWarmingRecordAccessorInfo>>("GlobalWarmingDB", g_theGlobalWarmingDB,
																	  g_GlobalWarmingRecord_Accessors,
																	  g_GlobalWarming_Tokens,
																	  k_Num_GlobalWarmingRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<MapIconRecord,
									MapIconRecordAccessorInfo>>("MapIconDB", g_theMapIconDB,
																g_MapIconRecord_Accessors,
																g_MapIcon_Tokens,
																k_Num_MapIconRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<MapRecord,
									MapRecordAccessorInfo>>("MapDB", g_theMapDB,
															g_MapRecord_Accessors,
															g_Map_Tokens,
															k_Num_MapRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<ConceptRecord,
									ConceptRecordAccessorInfo>>("ConceptDB", g_theConceptDB,
															   g_ConceptRecord_Accessors,
															   g_Concept_Tokens,
															   k_Num_ConceptRecord_Tokens).release());
	m_dbHash->Add(std::make_unique<SlicDBConduit<ConstRecord,
									ConstRecordAccessorInfo>>("ConstDB", g_theConstDB,
															   g_ConstRecord_Accessors,
															   g_Const_Tokens,
															   k_Num_ConstRecord_Tokens).release());
}

SlicDBInterface *SlicEngine::GetDBConduit(const char *name)
{
	return m_dbHash ? m_dbHash->Access(name) : nullptr;
}

#define SMF_2A(name, a1, a2) m_modFunc[name] = std::make_unique<SlicModFunc>(#name, a1, a2, ST_END).release();
#define SMF_3A(name, a1, a2, a3) m_modFunc[name] = std::make_unique<SlicModFunc>(#name, a1, a2, a3, ST_END).release();

void SlicEngine::AddModFuncs()
{
	for (auto & i : m_modFunc)
	{
		std::unique_ptr<SlicModFunc>{i};
		i = nullptr;
	}

	SMF_2A(mod_CanPlayerHaveAdvance, ST_PLAYER, ST_ADVANCE);
	SMF_2A(mod_CanCityBuildUnit, ST_CITY, ST_INT);
	SMF_2A(mod_CanCityBuildBuilding, ST_CITY, ST_INT);
	SMF_2A(mod_CanCityBuildWonder, ST_CITY, ST_INT);

	SMF_2A(mod_CityHappiness, ST_CITY, ST_INT);
	SMF_3A(mod_UnitAttack, ST_UNIT, ST_UNIT, ST_INT);
	SMF_3A(mod_UnitRangedAttack, ST_UNIT, ST_UNIT, ST_INT);
	SMF_3A(mod_UnitDefense, ST_UNIT, ST_UNIT, ST_INT);
}

sint32 SlicEngine::CallMod(MOD_FUNC modFunc, sint32 def, ...)
{
	Assert(modFunc > mod_INVALID);
	Assert(modFunc < mod_MAX);

	SlicModFunc *mf = m_modFunc[modFunc];
	if(!mf) return def;

	if(!mf->GetSegment())
		return def;

	va_list vl;
	va_start(vl, def);

	sint32 arg;
	auto slicArgs = std::make_unique<SlicArgList>();

	Unit u;
	Army a;
	sint32 val;
	MapPoint pos;

	for(arg = 0; arg < mf->GetNumArgs(); arg++) {
		SlicSymbolData *sym;
		switch(mf->GetArg(arg)) {
			case ST_UNIT:
				u.m_id = va_arg(vl, uint32);
				sym = std::make_unique<SlicSymbolData>(SLIC_SYM_UNIT).release();
				sym->SetUnit(u);
				slicArgs->AddArg(SA_TYPE_INT_VAR, sym);
				break;
			case ST_CITY:
				u.m_id = va_arg(vl, uint32);
				sym = std::make_unique<SlicSymbolData>(SLIC_SYM_CITY).release();
				sym->SetCity(u);
				slicArgs->AddArg(SA_TYPE_INT_VAR, sym);
				break;
			case ST_PLAYER:
				val = va_arg(vl, sint32);
				sym = std::make_unique<SlicSymbolData>(SLIC_SYM_PLAYER).release();
				sym->SetIntValue(val);
				slicArgs->AddArg(SA_TYPE_INT_VAR, sym);
				break;
			case ST_GOOD:
			case ST_GOLD:
			case ST_ADVANCE:
			case ST_INT:
				val = va_arg(vl, sint32);
				sym = std::make_unique<SlicSymbolData>(SLIC_SYM_IVAR).release();
				sym->SetIntValue(val);
				slicArgs->AddArg(SA_TYPE_INT_VAR, sym);
				break;
			case ST_LOCATION:
				pos = va_arg(vl, MapPoint);
				sym = std::make_unique<SlicSymbolData>(SLIC_SYM_LOCATION).release();
				sym->SetPos(pos);
				slicArgs->AddArg(SA_TYPE_INT_VAR, sym);
				break;
			case ST_ARMY:
				a.m_id = va_arg(vl, uint32);
				sym = std::make_unique<SlicSymbolData>(SLIC_SYM_ARMY).release();
				sym->SetArmy(a);
				slicArgs->AddArg(SA_TYPE_INT_VAR, sym);
				break;
			case ST_NONE:
			case ST_ACTION:
			case ST_POP:
			case ST_END:
				// Not valid CallMod argument types; skip.
				break;
		}
	}

	va_end(vl);

	SlicObject *obj;
	mf->GetSegment()->Call(slicArgs.get(), obj);

	slicArgs->ReleaseSymbols();

	sint32 result = obj->GetResult();
	obj->Release();

	return result;
}

sint32 SlicEngine::CallExcludeFunc(const MBCHAR *name, sint32 type, sint32 player)
{
	SlicSegment *seg = GetSegment(name);
	if(!seg) return false;

	auto slicArgs = std::make_unique<SlicArgList>();
	SlicSymbolData *sym;
	sym = std::make_unique<SlicSymbolData>(SLIC_SYM_IVAR).release();
	sym->SetIntValue(type);
	slicArgs->AddArg(SA_TYPE_INT_VAR, sym);

	sym = std::make_unique<SlicSymbolData>(SLIC_SYM_PLAYER).release();
	sym->SetIntValue(player);
	slicArgs->AddArg(SA_TYPE_INT_VAR, sym);

	SlicObject *obj;
	seg->Call(slicArgs.get(), obj);

    slicArgs->ReleaseSymbols();

	sint32 result = obj->GetResult();
	obj->Release();

	return result;
}

sint32 SlicEngine::GetCurrentLine() const
{
	if(m_context)
	{
		return m_context->GetFrame()->GetCurrentLine();
	}
	else
	{
		return -1;
	}
}

const char* SlicEngine::GetSegmentName() const
{
	if(m_context)
	{
		return m_context->GetFrame()->GetSlicSegment()->GetName();
	}
	else
	{
		return "";
	}
}

const char* SlicEngine::GetFileName() const
{
	if(m_context)
	{
		return m_context->GetFrame()->GetSlicSegment()->GetFilename();
	}
	else
	{
		return "";
	}
}
