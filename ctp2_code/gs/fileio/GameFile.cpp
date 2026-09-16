//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Game file handling
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
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Repaired multiple memory leaks.
// - Readded Activision patch new magic number 66.
// - Fixed autosave directory name for scenarios to match normal directory.
// - Fixed the scenario savegame bug (but not for autosave, that still needs to be done)
// - Replaced old civilisation database by new one. (Aug 20th 2005 Martin G�hmann)
// - Made progress bar more fluently. (Aug 22nd 2005 Martin G�hmann)
// - Removed old sprite state databases. (Aug 29th 2005 Martin G�hmann)
// - Removed old difficulty database. (April 29th 2006 Martin G�hmann)
// - Removed old pollution database. (July 15th 2006 Martin G�hmann)
// - Removed old gobal warming database. (July 15th 2006 Martin G�hmann)
// - Removed old concept database. (31-Mar-2007 Martin G�hmann)
// - Removed old const database. (5-Aug-2007 Martin G�hmann)
// - Replaced CIV_INDEX by sint32. (2-Jan-2008 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/fileio/gamefile.h"

#include "gs/gameobj/AchievementTracker.h"
#include "AdvanceRecord.h"
#include "gs/gameobj/Advances.h"
#include "AgeRecord.h"
#include "gs/gameobj/AgreementPool.h"         // agreementpool_Get()
#include <algorithm>
#include <vector>
#include "gs/gameobj/ArmyPool.h"
#include "gs/gameobj/BldQue.h"
#include "BuildingRecord.h"
#include "ctp/ctp2_utils/c3errors.h"
#include "ctp/ctp2_utils/c3files.h"
#include "gs/world/Cell.h"
#include "gs/gameobj/citydata.h"
#include "ctp/civ3_main.h"
#include "ctp/civapp.h"
#include "gs/gameobj/CivilisationPool.h"       // civilisationpool_Get()
#include "CivilisationRecord.h"
#include "gs/fileio/CivPaths.h"               // civpaths_Get()
#include "gs/fileio/civscenarios.h"
#if CTP2_ENABLE_NETWORKING
#include "ui/netshell/netshell.h"               // gamesetup_Get()
#endif
#include "ai/ctpai.h"
#include "gs/gameobj/Diffcly.h"
#include "gs/gameobj/DiplomaticRequestPool.h"  // diplomaticrequestpool_Get()
#include "gs/gameobj/EventTracker.h"
#include "gs/gameobj/Exclusions.h"
#include "gs/gameobj/FeatTracker.h"
#include "gs/utility/gameinit.h"
#include "gs/fileio/json_save.h"               // json_save::LoadJson (G-2)
#include "ctp/ctp2_utils/bounded_json.h"       // ReadBoundedJson
#include <cctype>                              // std::isspace (G-2)
#include <cstdio>                              // std::fgetc (G-2)
#include <fstream>
#include "gs/gameobj/GameSettings.h"
#include "gs/gameobj/Gold.h"
#include "gs/gameobj/installation.h"
#include "gs/gameobj/installationpool.h"       // installationpool_Get()
#include "gs/gameobj/MessagePool.h"            // messagepool_Get()
Pixel16 pixelutils_Convert565to555(Pixel16);  // forward decl, was gfx/gfx_utils/pixelutils.h
#include "gs/gameobj/player.h"                 // player_Get
#include "gs/gameobj/pollution.h"
#include "gs/database/profileDB.h"              // profiledb_Get()
#include "gs/utility/RandGen.h"                // rand_ptr()
#include "robot/utility/RoboInit.h"
#include "gs/gameobj/Sci.h"
#include "gs/slic/SlicEngine.h"
#include "gs/core/audio_types.h"
#include "gs/core/progress_observer.h"
#include "gs/core/player_view.h"
#include "gs/core/audio_observer.h"
#include "gs/database/StrDB.h"                  // g_theStringDB
#include "gs/gameobj/TaxRate.h"
#include "TerrainRecord.h"
#include "gs/gameobj/TerrImprovePool.h"        // terrimprovepool_Get()
#include "gs/database/thronedb.h"               // g_theThroneDB
#include "gs/gameobj/TopTen.h"
#include "gs/gameobj/TradeBids.h"
#include "gs/gameobj/TradeOfferPool.h"
#include "gs/gameobj/TradePool.h"
#include "gs/utility/TurnCnt.h"                // turn_Get()
#include "gs/gameobj/UnitData.h"
#include "gs/gameobj/UnitPool.h"               // UnitPool
#include "gs/database/UVDB.h"
#include "WonderRecord.h"
#include "gs/gameobj/WonderTracker.h"
#include "gs/world/World.h"                  // world_Get()
#include "gs/gameobj/Wormhole.h"
#include <zlib.h>

#ifndef WIN32
#include <sys/types.h>
#include <dirent.h>
#endif

extern  OzoneDatabase               *g_theUVDB;
extern sint32                       g_isGridOn;











// G-4: USE_FORMAT_67 ifdefs collapsed.  CTP0066 is the canonical magic
// for binary CTP2 saves; CTP0067 was an unreleased Apolyton variant
// that never shipped.  With JSON as the default save format (G-3) the
// binary path only needs to recognise files that already exist on disk
// — i.e. CTP0049..CTP0066.
#define k_GAME_MAGIC_VALUE		"CTP0066"


struct MagicValue {
	char const *string;
	sint32 version;
};

#define k_NUM_MAGIC_VALUES 18
MagicValue s_magicValue[k_NUM_MAGIC_VALUES] = {
	{ "CTP0049", 49},
	{ "CTP0050", 50},
	{ "CTP0051", 51},
	{ "CTP0052", 52},
	{ "CTP0053", 53},
	{ "CTP0054", 54},
	{ "CTP0055", 55},
	{ "CTP0056", 56},
	{ "CTP0057", 57},
	{ "CTP0058", 58},
	{ "CTP0059", 59},
	{ "CTP0060", 60},
	{ "CTP0061", 61},
	{ "CTP0062", 62},
	{ "CTP0063", 63},   // Activision Alexander the Great scenario
	{ "CTP0064", 64},
	{ "CTP0065", 65},   // Activision CTP2 unpatched
	{ "CTP0066", 66}    // Activision CTP2 patched (canonical binary magic)
};

sint32 gamefile_CurrentVersion()
{
	return s_magicValue[k_NUM_MAGIC_VALUES - 1].version;
}

static sint32 g_saveFileVersion = -1;

sint32 save_file_version_Get() { return g_saveFileVersion; }
void   save_file_version_Set(sint32 v) { g_saveFileVersion = v; }
static sint32 g_startInfoType = STARTINFOTYPE_NONE;

sint32 start_info_type_Get()     { return g_startInfoType; }
void   start_info_type_Set(sint32 v) { g_startInfoType = v; }
static sint32 g_isScenario = FALSE;

sint32 is_scenario_Get()     { return g_isScenario; }
void   is_scenario_Set(sint32 v) { g_isScenario = v; }
static sint32 g_useScenarioCivs = 2;

sint32 scenario_civs_Get() { return g_useScenarioCivs; }
void   scenario_civs_Set(sint32 v) { g_useScenarioCivs = v; }

static sint32 g_showUnitLabels = FALSE;

bool show_unit_labels_Get()        { return g_showUnitLabels != FALSE; }
void show_unit_labels_Set(bool value)  { g_showUnitLabels = value ? TRUE : FALSE; }

static sint32 g_startingPlayer = -1;

sint32 starting_player_Get()     { return g_startingPlayer; }
void   starting_player_Set(sint32 v) { g_startingPlayer = v; }

static MBCHAR g_scenarioName[k_SCENARIO_NAME_MAX];

MBCHAR * scenario_name_buf() { return g_scenarioName; }




// Phase 0.C-3: JSON-only restore.  The legacy CivArchive binary path
// (RestoreLegacyBinary + format auto-detection) is gone; loading an
// old binary .c2g now fails with GAMEFILE_ERR_LOAD_FAILED.
static uint32 DispatchRestore(MBCHAR const *filepath)
{
	if (civapp_Get()->InitializeGame() != 0)
		return GAMEFILE_ERR_LOAD_FAILED;
	bool const ok = json_save::LoadJson(filepath);
	return ok ? GAMEFILE_ERR_LOAD_OK : GAMEFILE_ERR_LOAD_FAILED;
}

bool GameFile::RestoreGame(MBCHAR const * name)
{
	return DispatchRestore(name) == GAMEFILE_ERR_LOAD_OK;
}

bool GameFile::RestoreScenarioGame(MBCHAR const * name)
{
	return DispatchRestore(name) == GAMEFILE_ERR_LOAD_OK;
}

void GameFile::SaveGame(const MBCHAR *filename, SaveInfo *info)
{
	// SaveInfo (radar map snapshot, leader name, etc.) is metadata
	// the legacy binary path interleaved into the file header.  The JSON
	// schema doesn't carry these fields yet (Phase F-18 schema is
	// game-state-only); the load-save browser UI that consumes them is
	// not yet wired to JSON saves.  Out of scope here; the SaveInfo
	// argument is intentionally ignored.
	(void)info;
	json_save::SaveJson(filename);
}


GameFile::GameFile()
= default;

static uint32 CompressData(uint8 *inbuf, size_t insize,
                           uint8 **outbuf, size_t *outsize)
{
	uLong tsize = (uLong)(((double)insize * 1.01) + 12.5);

	*outbuf = new uint8[tsize];

	int err = compress2(*outbuf, &tsize, inbuf, insize, Z_DEFAULT_COMPRESSION);
	*outsize = tsize;

	return (err == Z_OK);
}







bool GameFile::LoadExtendedGameInfo(FILE *saveFile, SaveInfo *info)
{
#if !CTP2_ENABLE_NETWORKING
	(void)saveFile;
	(void)info;
	return false;
#else
	sint32		n;

	n = c3files_fread(info->gameName, sizeof(uint8), _MAX_PATH, saveFile);
	if (n != _MAX_PATH) {
		c3files_fclose(saveFile);
		return false;
	}

	n = c3files_fread(info->leaderName, sizeof(uint8), k_MAX_NAME_LEN, saveFile);
	if (n != k_MAX_NAME_LEN) {
		c3files_fclose(saveFile);
		return false;
	}

	n = c3files_fread(info->civName, sizeof(uint8), k_MAX_NAME_LEN, saveFile);
	if (n != k_MAX_NAME_LEN) {
		c3files_fclose(saveFile);
		return false;
	}

	n = c3files_fread(info->note, sizeof(uint8), _MAX_PATH, saveFile);
	if (n != _MAX_PATH) {
		c3files_fclose(saveFile);
		return false;
	}

	info->radarMapWidth = 0;
	info->radarMapHeight = 0;
	info->radarMapData.clear();

	n = c3files_fread(&info->radarMapWidth, sizeof(uint8), sizeof(info->radarMapWidth), saveFile);
	if (n != sizeof(info->radarMapWidth)) {
		c3files_fclose(saveFile);
		return false;
	}
	n = c3files_fread(&info->radarMapHeight, sizeof(uint8), sizeof(info->radarMapHeight), saveFile);
	if (n != sizeof(info->radarMapHeight)) {
		c3files_fclose(saveFile);
		return false;
	}

	if (info->radarMapHeight > 0 && info->radarMapWidth > 0) {
		info->radarMapData.resize(info->radarMapWidth * info->radarMapHeight);
		n = c3files_fread(info->radarMapData.data(), sizeof(uint8),
							sizeof(Pixel16) * info->radarMapWidth * info->radarMapHeight, saveFile);
		if (n != (sint32)(info->radarMapWidth * info->radarMapHeight * sizeof(Pixel16))) {
			c3files_fclose(saveFile);
			return false;
		}

		if (!is_565_Get()) {
			for (sint32 i=0; i<info->radarMapWidth * info->radarMapHeight; i++) {
				info->radarMapData[i] = pixelutils_Convert565to555(info->radarMapData[i]);
			}
		}
	}

	info->powerGraphWidth = 0;
	info->powerGraphHeight = 0;
	info->powerGraphData.clear();

	n = c3files_fread(&info->powerGraphWidth, sizeof(uint8), sizeof(info->powerGraphWidth), saveFile);
	if (n != sizeof(info->powerGraphWidth)) {
		c3files_fclose(saveFile);
		return false;
	}
	n = c3files_fread(&info->powerGraphHeight, sizeof(uint8), sizeof(info->powerGraphHeight), saveFile);
	if (n != sizeof(info->powerGraphHeight)) {
		c3files_fclose(saveFile);
		return false;
	}

	if (info->powerGraphHeight > 0 && info->powerGraphWidth > 0) {
		info->powerGraphData.resize(info->powerGraphWidth * info->powerGraphHeight);
		n = c3files_fread(info->powerGraphData.data(), sizeof(uint8),
							sizeof(Pixel16) * info->powerGraphWidth * info->powerGraphHeight, saveFile);
		if (n != (sint32)(info->powerGraphWidth * info->powerGraphHeight * sizeof(Pixel16))) {
			c3files_fclose(saveFile);
			return false;
		}

		if (!is_565_Get())
		{
			std::transform(info->powerGraphData.data(),
			               info->powerGraphData.data() + (info->powerGraphWidth * info->powerGraphHeight),
			               info->powerGraphData.data(),
			               pixelutils_Convert565to555
			              );
		}
	}

	sint32 numPlayers;
	n = c3files_fread(&numPlayers, sizeof(uint8), sizeof(sint32), saveFile);
	if (n != sizeof(sint32)) {
		c3files_fclose(saveFile);
		return false;
	}
	info->numCivs = numPlayers;

    sint32 has_robot;
	for (sint32 i=0; i<k_MAX_PLAYERS; i++) {
        c3files_fread(&has_robot, sizeof(uint8), sizeof(sint32), saveFile);

        if (has_robot) {
		    n = c3files_fread(info->civList[i], sizeof(uint8), k_MAX_NAME_LEN, saveFile);
		    if (n != k_MAX_NAME_LEN) {
			    c3files_fclose(saveFile);
			    return false;
		    }
        } else {
            info->civList[i][0] = '\0';
        }

		sint32 civindex;
		GUID guid;
		n = c3files_fread(&civindex, 1, sizeof(sint32), saveFile);
		if(n != sizeof(sint32)) {
			c3files_fclose(saveFile);
			return false;
		}

		n = c3files_fread(&guid, 1, sizeof(guid), saveFile);
		if(n != sizeof(guid)) {
			c3files_fclose(saveFile);
			return false;
		}

		info->networkGUID[i].civIndex = civindex;
		info->networkGUID[i].guid = guid;
	}

	n = c3files_fread(&info->gameSetup, sizeof(nf_GameSetup), 1, saveFile);
	if (n != 1) {
		c3files_fclose(saveFile);
		return false;
	}

	NETFunc::Session *  s       = (NETFunc::Session *) &info->gameSetup;
	dp_session_t *      sess    =
        (dp_session_t *)((uint8*)s + sizeof(NETFunc::Key));
	// GAMEID = 1504, defined in ui/netshell/netshell_game.h.  Inlined here
	// rather than including the UI-side header to keep gs/fileio/ free of
	// ui/ deps; revisit if the netshell session-type discriminator is ever
	// reused outside the save-browser path.
	sess->sessionType = 1504;

	n = c3files_fread(&info->options, sizeof(SaveInfo::OptionScreenSettings), 1, saveFile);
	if (n != 1) {
		c3files_fclose(saveFile);
		return false;
	}

	info->loadType = SAVEINFOLOAD_EXTENDED;


	n = c3files_fread(&info->isScenario, sizeof(info->isScenario), 1, saveFile);
	if(n != 1) {
		c3files_fclose(saveFile);
		return false;
	}

	n = c3files_fread(&info->startInfoType, sizeof(info->startInfoType), 1, saveFile);
	if(n != 1) {
		c3files_fclose(saveFile);
		return false;
	}

	n = c3files_fread(&info->numPositions, sizeof(info->numPositions), 1, saveFile);
	if(n != 1) {
		c3files_fclose(saveFile);
		return false;
	}

	n = c3files_fread(&info->positions, sizeof(StartingPosition), k_MAX_START_POINTS, saveFile);
	if(n != k_MAX_START_POINTS) {
		c3files_fclose(saveFile);
		return false;
	}




	MBCHAR name[k_SCENARIO_NAME_MAX];
	n = c3files_fread(name, sizeof(MBCHAR), k_SCENARIO_NAME_MAX, saveFile);
	if(n != k_SCENARIO_NAME_MAX) {
		c3files_fclose(saveFile);
		return false;
	}

	info->scenarioName = name;


	if (g_saveFileVersion >= 47) {


		n = c3files_fread(info->playerCivIndexList, sizeof(sint32), k_MAX_PLAYERS, saveFile);
		if(n != k_MAX_PLAYERS) {
			return false;
		}
	}

	if (g_saveFileVersion >= 50) {
		n = c3files_fread(&info->showLabels, sizeof(info->showLabels), 1, saveFile);
		if(n != 1) {
			c3files_fclose(saveFile);
			return false;
		}

		n = c3files_fread(&info->startingPlayer, sizeof(info->startingPlayer), 1, saveFile);
		if(n != 1) {
			c3files_fclose(saveFile);
			return false;
		}
	} else {
		info->showLabels = FALSE;
		info->startingPlayer = -1;
	}

	return true;
#endif
}






bool GameFile::LoadBasicGameInfo(FILE *saveFile, SaveInfo *info)
{
#if !CTP2_ENABLE_NETWORKING
	(void)saveFile;
	(void)info;
	return false;
#else
	sint32		n;

	n = c3files_fread(info->gameName, sizeof(uint8), _MAX_PATH, saveFile);
	if (n != _MAX_PATH) {
		c3files_fclose(saveFile);
		return false;
	}

	n = c3files_fread(info->leaderName, sizeof(uint8), k_MAX_NAME_LEN, saveFile);
	if (n != k_MAX_NAME_LEN) {
		c3files_fclose(saveFile);
		return false;
	}

	n = c3files_fread(info->civName, sizeof(uint8), k_MAX_NAME_LEN, saveFile);
	if (n != k_MAX_NAME_LEN) {
		c3files_fclose(saveFile);
		return false;
	}

	n = c3files_fread(info->note, sizeof(uint8), _MAX_PATH, saveFile);
	if (n != _MAX_PATH) {
		c3files_fclose(saveFile);
		return false;
	}

	info->loadType = SAVEINFOLOAD_BASIC;




	if (g_saveFileVersion >= 42) {


		n = c3files_fread(&info->radarMapWidth, sizeof(uint8), sizeof(info->radarMapWidth), saveFile);
		if (n != sizeof(info->radarMapWidth)) {
			c3files_fclose(saveFile);
			return false;
		}

		n = c3files_fread(&info->radarMapHeight, sizeof(uint8), sizeof(info->radarMapHeight), saveFile);
		if (n != sizeof(info->radarMapHeight)) {
			c3files_fclose(saveFile);
			return false;
		}

		if (info->radarMapHeight > 0 && info->radarMapWidth > 0) {
			c3files_fseek(saveFile,
						sizeof(Pixel16) * info->radarMapWidth * info->radarMapHeight,
						SEEK_CUR);
		}

		n = c3files_fread(&info->powerGraphWidth, sizeof(uint8), sizeof(info->powerGraphWidth), saveFile);
		if (n != sizeof(info->powerGraphWidth)) {
			c3files_fclose(saveFile);
			return false;
		}
		n = c3files_fread(&info->powerGraphHeight, sizeof(uint8), sizeof(info->powerGraphHeight), saveFile);
		if (n != sizeof(info->powerGraphHeight)) {
			c3files_fclose(saveFile);
			return false;
		}

		if (info->powerGraphHeight > 0 && info->powerGraphWidth > 0) {
			c3files_fseek(saveFile,
						sizeof(Pixel16) * info->powerGraphWidth * info->powerGraphHeight,
						SEEK_CUR);
		}

		sint32 numPlayers;
		n = c3files_fread(&numPlayers, sizeof(uint8), sizeof(sint32), saveFile);
		if (n != sizeof(sint32)) {
			c3files_fclose(saveFile);
			return false;
		}
		info->numCivs = numPlayers;

		sint32 has_robot;
		for (sint32 i=0; i<k_MAX_PLAYERS; i++) {
			c3files_fread(&has_robot, sizeof(uint8), sizeof(sint32), saveFile);

			if (has_robot) {
				n = c3files_fread(info->civList[i], sizeof(uint8), k_MAX_NAME_LEN, saveFile);
				if (n != k_MAX_NAME_LEN) {
					c3files_fclose(saveFile);
					return false;
				}
			} else {
				info->civList[i][0] = '\0';
			}

			sint32 civindex;
			GUID guid;
			n = c3files_fread(&civindex, 1, sizeof(sint32), saveFile);
			if(n != sizeof(sint32)) {
				c3files_fclose(saveFile);
				return false;
			}

			n = c3files_fread(&guid, 1, sizeof(guid), saveFile);
			if(n != sizeof(guid)) {
				c3files_fclose(saveFile);
				return false;
			}

			info->networkGUID[i].civIndex = civindex;
			info->networkGUID[i].guid = guid;
		}

		n = c3files_fread(&info->gameSetup, sizeof(nf_GameSetup), 1, saveFile);
		if (n != 1) {
			c3files_fclose(saveFile);
			return false;
		}

		n = c3files_fread(&info->options, sizeof(SaveInfo::OptionScreenSettings), 1, saveFile);
		if (n != 1) {
			c3files_fclose(saveFile);
			return false;
		}




		n = c3files_fread(&info->isScenario, sizeof(info->isScenario), 1, saveFile);
		if(n != 1) {
			c3files_fclose(saveFile);
			return false;
		}

		n = c3files_fread(&info->startInfoType, sizeof(info->startInfoType), 1, saveFile);
		if(n != 1) {
			c3files_fclose(saveFile);
			return false;
		}
	}




	if (g_saveFileVersion >= 46) {

		MBCHAR name[k_SCENARIO_NAME_MAX];
		n = c3files_fread(name, sizeof(MBCHAR), k_SCENARIO_NAME_MAX, saveFile);
		if(n != k_SCENARIO_NAME_MAX) {
			c3files_fclose(saveFile);
			return false;
		}

		info->scenarioName = name;
	}


	if (g_saveFileVersion >= 47) {


		n = c3files_fread(info->playerCivIndexList, sizeof(sint32), k_MAX_PLAYERS, saveFile);
		if(n != k_MAX_PLAYERS) {
			return false;
		}
	}

	return true;
#endif
}







void GameFile::SaveExtendedGameInfo(FILE *saveFile, SaveInfo *info)
{
#if !CTP2_ENABLE_NETWORKING
	(void)saveFile;
	(void)info;
	return;
#else
	MBCHAR const *functionName = "GameFile::SaveExtendedGameInfo";
	MBCHAR const *errorString = "Unable to write save file.";

	sint32		n;




	n = c3files_fwrite(info->gameName, sizeof(MBCHAR), _MAX_PATH, saveFile);
	if (n != _MAX_PATH) {
		c3errors_FatalDialog(functionName, errorString);
		return;
	}

	n = c3files_fwrite(info->leaderName, sizeof(MBCHAR), k_MAX_NAME_LEN, saveFile);
	if (n != k_MAX_NAME_LEN) {
		c3errors_FatalDialog(functionName, errorString);
		return;
	}

	n = c3files_fwrite(info->civName, sizeof(MBCHAR), k_MAX_NAME_LEN, saveFile);
	if (n != k_MAX_NAME_LEN) {
		c3errors_FatalDialog(functionName, errorString);
		return;
	}

	n = c3files_fwrite(info->note, sizeof(MBCHAR), _MAX_PATH, saveFile);
	if (n != _MAX_PATH) {
		c3errors_FatalDialog(functionName, errorString);
		return;
	}

	n = c3files_fwrite(&info->radarMapWidth, sizeof(uint8), sizeof(info->radarMapWidth), saveFile);
	if (n != sizeof(info->radarMapWidth)) {
		c3errors_FatalDialog(functionName, errorString);
		return;
	}
	n = c3files_fwrite(&info->radarMapHeight, sizeof(uint8), sizeof(info->radarMapHeight), saveFile);
	if (n != sizeof(info->radarMapHeight)) {
		c3errors_FatalDialog(functionName, errorString);
		return;
	}
	if (info->radarMapWidth > 0 && info->radarMapHeight > 0) {

		if (!is_565_Get()) {
			for (sint32 i=0; i<info->radarMapWidth * info->radarMapHeight; i++) {
				Pixel16		pixel = info->radarMapData[i];
				info->radarMapData[i] = ((pixel & 0x7FE0) << 1) | (pixel & 0x001F);

			}
		}

		n = c3files_fwrite(info->radarMapData.data(), sizeof(uint8),
							sizeof(Pixel16) * info->radarMapHeight * info->radarMapWidth,
							saveFile);
		if (n != (sint32)(sizeof(Pixel16) * info->radarMapHeight * info->radarMapWidth)) {
			c3errors_FatalDialog(functionName, errorString);
			return;
		}
	}

	n = c3files_fwrite(&info->powerGraphWidth, sizeof(uint8), sizeof(info->powerGraphWidth), saveFile);
	if (n != sizeof(info->powerGraphWidth)) {
		c3errors_FatalDialog(functionName, errorString);
		return;
	}
	n = c3files_fwrite(&info->powerGraphHeight, sizeof(uint8), sizeof(info->powerGraphHeight), saveFile);
	if (n != sizeof(info->powerGraphHeight)) {
		c3errors_FatalDialog(functionName, errorString);
		return;
	}

	if (info->powerGraphHeight > 0 && info->powerGraphWidth > 0) {

		if (!is_565_Get()) {
			for (sint32 i=0; i<info->powerGraphWidth * info->powerGraphHeight; i++) {
				Pixel16		pixel = info->powerGraphData[i];
				info->powerGraphData[i] = ((pixel & 0x7FE0) << 1) | (pixel & 0x001F);
			}
		}

		n = c3files_fwrite(info->powerGraphData.data(), sizeof(uint8),
							sizeof(Pixel16) * info->powerGraphHeight * info->powerGraphWidth,
							saveFile);
		if (n != (sint32)(sizeof(Pixel16) * info->powerGraphHeight * info->powerGraphWidth)) {
			c3errors_FatalDialog(functionName, errorString);
			return;
		}
	}

	sint32		numPlayers = k_MAX_PLAYERS;

	n = c3files_fwrite(&numPlayers, sizeof(uint8), sizeof(sint32), saveFile);
	if (n != sizeof(sint32)) {
		c3errors_FatalDialog(functionName, errorString);
		return;
	}

	for (sint32 i=0; i<k_MAX_PLAYERS; i++) {
		MBCHAR civName[k_MAX_NAME_LEN];
		// Zero the full buffer.  GetPluralCivName writes a short string +
		// null terminator; the remainder used to carry uninitialised
		// stack memory all the way through to the file, producing a
		// fresh ~500 bytes of non-determinism per player slot in every
		// save.  Caught by test_save_determinism.cpp.
		memset(civName, 0, sizeof(civName));

	    sint32 has_player;
		GUID guid;
		sint32 civindex;
		if (player_Get(i)) {
            has_player = 1;
            c3files_fwrite(&has_player, sizeof(uint8), sizeof(sint32), saveFile);

		    player_Get(i)->GetPluralCivName(civName);

		    n = c3files_fwrite(civName, sizeof(MBCHAR), k_MAX_NAME_LEN, saveFile);
		    if (n != k_MAX_NAME_LEN) {
			    c3errors_FatalDialog(functionName, errorString);
			    return;
		    }
			guid = player_Get(i)->m_networkGuid;
			civindex = player_Get(i)->m_civilisation->GetCivilisation();
        } else {
            has_player = 0;
            c3files_fwrite(&has_player, sizeof(uint8), sizeof(sint32), saveFile);
			memset(&guid, 0, sizeof(guid));
			civindex = -1;
        }
		c3files_fwrite(&civindex, 1, sizeof(sint32), saveFile);
		c3files_fwrite(&guid, 1, sizeof(GUID), saveFile);
	}

	n = c3files_fwrite(&info->gameSetup, sizeof(nf_GameSetup), 1, saveFile);
	if (n != 1) {
		c3errors_FatalDialog(functionName, errorString);
		return;
	}

	n = c3files_fwrite(&info->options, sizeof(SaveInfo::OptionScreenSettings), 1, saveFile);
	if (n != 1) {
		c3errors_FatalDialog(functionName, errorString);
		return;
	}




	n = c3files_fwrite(&info->isScenario, sizeof(info->isScenario), 1, saveFile);
	if(n != 1) {
		c3errors_FatalDialog(functionName, errorString);
		return;
	}

	n = c3files_fwrite(&info->startInfoType, sizeof(info->startInfoType), 1, saveFile);
	if(n != 1) {
		c3errors_FatalDialog(functionName, errorString);
		return;
	}

	n = c3files_fwrite(&info->numPositions, sizeof(info->numPositions), 1, saveFile);
	if(n != 1) {
		c3errors_FatalDialog(functionName, errorString);
		return;
	}

	n = c3files_fwrite(&info->positions, sizeof(StartingPosition), k_MAX_START_POINTS, saveFile);
	if(n != k_MAX_START_POINTS) {
		c3errors_FatalDialog(functionName, errorString);
		return;
	}




	MBCHAR name[k_SCENARIO_NAME_MAX];
	memset(name, 0, k_SCENARIO_NAME_MAX);

	if(strlen(g_scenarioName) > 0) // Problem in Multiplayer
	{
		strlcpy(name, g_scenarioName, sizeof(name));
	}

	n = c3files_fwrite(name, sizeof(MBCHAR), k_SCENARIO_NAME_MAX, saveFile);
	if(n != k_SCENARIO_NAME_MAX) {
		c3errors_FatalDialog(functionName, errorString);
		return;
	}




	n = c3files_fwrite(info->playerCivIndexList, sizeof(sint32), k_MAX_PLAYERS, saveFile);
	if(n != k_MAX_PLAYERS) {
		c3errors_FatalDialog(functionName, errorString);
		return;
	}





	n = c3files_fwrite(&info->showLabels, sizeof(info->showLabels), 1, saveFile);
	if(n != 1) {
		c3errors_FatalDialog(functionName, errorString);
		return;
	}

	n = c3files_fwrite(&info->startingPlayer, sizeof(info->startingPlayer), 1, saveFile);
	if(n != 1) {
		c3errors_FatalDialog(functionName, errorString);
		return;
	}
#endif
}

void GameFile::SetProfileFromExtendedInfo(SaveInfo *info)
{
	Assert(info && profiledb_Get());
	if (!info || !profiledb_Get()) return;

	if (g_isScenario)
    {
		MBCHAR	name[SAVE_LEADER_NAME_SIZE + 1];
		strlcpy(name, profiledb_Get()->GetLeaderName(), sizeof(name));
		// TODO: check if this is OK for japanese.
		profiledb_Get()->SetGameName(name);
	}
    else
    {
		profiledb_Get()->SetGameName(info->gameName);
	}


	if (info->startInfoType != STARTINFOTYPE_CIVS &&
		info->startInfoType != STARTINFOTYPE_POSITIONSFIXED) {
		profiledb_Get()->SetLeaderName(info->leaderName);
		profiledb_Get()->SetCivName(info->civName);
		profiledb_Get()->SetSaveNote(info->note);
	}

#if CTP2_ENABLE_NETWORKING
	nf_GameSetup temp = gamesetup_Get();
	gamesetup_Get() = info->gameSetup;

	memcpy(
		gamesetup_Get().GetTribeSlots(),
		temp.GetTribeSlots(),
		8  * sizeof( TribeSlot ) );

	gamesetup_Get().SetLaunched(true);
	gamesetup_Get().Pack();
#endif

	profiledb_Get()->SetTutorialAdvice(info->options.tutorialadvice);
















	g_isGridOn = info->options.grid;





	audio_observer::SetAutoRepeat(info->options.autoRepeat);
	if ( info->options.randomOrder )
		audio_observer::SetMusicStyle((sint32)MUSICSTYLE_RANDOM);
	else
		audio_observer::SetMusicStyle((sint32)MUSICSTYLE_NONE);
	if ( info->options.musicOn )
		audio_observer::EnableMusic();
	else
		audio_observer::DisableMusic();

	if(g_saveFileVersion >= 42) {
        if(!info->isScenario){// exclude starting new scenarios
            if (!info->scenarioName.empty()) {//same as in beginloadprocess
		        strlcpy(g_scenarioName, info->scenarioName.c_str(), sizeof(g_scenarioName));
			}
		}
		g_isScenario = info->isScenario;
		g_startInfoType = info->startInfoType;
	} else {
		g_isScenario = FALSE;
		g_startInfoType = STARTINFOTYPE_NONE;
	}

	if (g_saveFileVersion >= 50)
	{
		g_showUnitLabels = info->showLabels;
		g_startingPlayer = info->startingPlayer;
	} else {
		g_showUnitLabels = FALSE;
		g_startingPlayer = -1;
	}
}

void GameFile::GetExtendedInfoFromProfile(SaveInfo *info)
{
	Assert(info && profiledb_Get());
	if (!info || !profiledb_Get()) return;

	strlcpy(info->gameName, profiledb_Get()->GetGameName(), sizeof(info->gameName));
	strlcpy(info->leaderName, profiledb_Get()->GetLeaderName(), sizeof(info->leaderName));
	strlcpy(info->civName, profiledb_Get()->GetCivName(), sizeof(info->civName));
	strlcpy(info->note, profiledb_Get()->GetSaveNote(), sizeof(info->note));

#if CTP2_ENABLE_NETWORKING
	info->gameSetup = gamesetup_Get();

	memset(
		info->gameSetup.GetTribeSlots(),
		0,
		8  * sizeof( TribeSlot ) );

	if(g_isScenario) {
		memset(info->gameSetup.GetSavedTribeSlots(),
			   0,
			   8 * sizeof(TribeSlot));
	}
#endif

	info->options.tutorialadvice = profiledb_Get()->IsTutorialAdvice();
	info->options.leftrightclickmove = profiledb_Get()->IsUseLeftClick();
	info->options.autocycleturn = profiledb_Get()->IsAutoTurnCycle();
	info->options.autocycleunits = profiledb_Get()->IsAutoSelectFirstUnit();
	info->options.battleview = profiledb_Get()->IsZoomedCombatAlways();
	info->options.monument = profiledb_Get()->IsThroneRoom();


	info->options.walk = profiledb_Get()->IsUnitAnim();
	info->options.goods = profiledb_Get()->IsGoodAnim();

	info->options.trade = profiledb_Get()->IsTradeAnim();
	info->options.wonder = profiledb_Get()->IsWonderMovies();
	info->options.library = profiledb_Get()->IsLibraryAnim();
	info->options.message = profiledb_Get()->IsBounceMessage();

	info->options.movie = profiledb_Get()->IsFullScreenMovies();

	info->options.grid = g_isGridOn;

	info->options.sfxVolume = profiledb_Get()->GetSFXVolume();
	info->options.musicVolume = profiledb_Get()->GetMusicVolume();
	info->options.voiceVolume = profiledb_Get()->GetVoiceVolume();

	info->options.autoRepeat = audio_observer::IsAutoRepeat();
	info->options.randomOrder = audio_observer::GetMusicStyle() == (sint32)MUSICSTYLE_RANDOM;
	info->options.musicOn = audio_observer::IsMusicEnabled();






	info->isScenario = g_isScenario;
	info->startInfoType = (STARTINFOTYPE)g_startInfoType;

	memset(info->positions, 0, sizeof(info->positions));

	if(world_Get()) {
		info->numPositions = world_Get()->GetNumStartingPositions();
		sint32 i;
		for(i = 0; i < info->numPositions; i++) {
			info->positions[i].point = world_Get()->GetStartingPoint(i);
			info->positions[i].civIndex = world_Get()->GetStartingPointCiv(i);
		}
	}

	info->showLabels = g_showUnitLabels;
	info->startingPlayer = g_startingPlayer;
}









bool GameFile::ValidateGameFile(MBCHAR const * path, SaveInfo *info)
{
	MBCHAR		filepath[_MAX_PATH];
	snprintf(filepath, sizeof(filepath), "%s%s%s", path, FILE_SEP, info->fileName);

	FILE *  saveFile = c3files_fopen(C3DIR_DIRECT, filepath, "rb");
	if (saveFile == nullptr)
		return false;

	// JSON-only: skip leading whitespace and check the first non-ws byte.
	// nlohmann::json::dump(2) sorts keys alphabetically, so the "magic"
	// key can be deep in the file (line 38k+).  We can't rely on strstr
	// in a small read buffer — just verify the file is JSON.
	int c;
	do {
		c = fgetc(saveFile);
	} while (c != EOF && isspace(static_cast<unsigned char>(c)));

	c3files_fclose(saveFile);

	if (c == '{') {
		g_saveFileVersion = s_magicValue[k_NUM_MAGIC_VALUES - 1].version;
		info->loadType    = SAVEINFOLOAD_BASIC;
		return true;
	}

	return false;
}






bool GameFile::FetchExtendedSaveInfo(MBCHAR const * fullPath, SaveInfo *info)
{
	FILE * saveFile = c3files_fopen(C3DIR_DIRECT, fullPath, "rb");
	if (saveFile == nullptr)
		return false;

	MBCHAR  header[_MAX_PATH];
	size_t 	n = c3files_fread(header, sizeof(uint8), sizeof(k_GAME_MAGIC_VALUE), saveFile);
	if (n!=sizeof(k_GAME_MAGIC_VALUE)) {
		c3files_fclose(saveFile);
		return false;
	}

	g_saveFileVersion = -1;

	sint32 i;
	for(i = 0; i < k_NUM_MAGIC_VALUES; i++) {
		if(strcmp(header, s_magicValue[i].string) == 0) {
			g_saveFileVersion = s_magicValue[i].version;
			break;
		}
	}

	if(g_saveFileVersion < 0) {
		c3files_fclose(saveFile);
		return false;
	}


	bool success = LoadExtendedGameInfo(saveFile, info);

	c3files_fclose(saveFile);

	return success;
}

PointerList<GameInfo> *GameFile::BuildSaveList(C3SAVEDIR dir)
{
	PointerList<GameInfo> * list = new PointerList<GameInfo>;

	MBCHAR  dirPath[_MAX_PATH];
	MBCHAR  path[_MAX_PATH];
	if (!civpaths_Get()->GetSavePath(dir, dirPath)) return list;

#ifdef WIN32
	snprintf(path, sizeof(path), "%s%s*.*", dirPath, FILE_SEP);

	WIN32_FIND_DATA fileData;
	HANDLE lpDirList = FindFirstFile(path, &fileData);
	if (lpDirList == INVALID_HANDLE_VALUE) return list;
#else
	DIR * d = opendir(dirPath);
	if (!d) return list;

	struct stat     tmpstat;
	struct dirent * dent = nullptr;
#endif

	GameInfo			*gameInfo;

	do {
#ifndef WIN32
		dent = readdir(d);
		if (!dent) continue;

		snprintf(path, sizeof(path), "%s%s%s", dirPath, FILE_SEP, dent->d_name);
		if (stat(path, &tmpstat))
			continue;

		if (S_ISDIR(tmpstat.st_mode)) {
			MBCHAR *name = dent->d_name;
#else
		if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			MBCHAR *name = fileData.cFileName;
#endif
			if (!strcmp(name, ".")) continue;
			if (!strcmp(name, "..")) continue;

			gameInfo = new GameInfo();

			strlcpy(gameInfo->name, name, sizeof(gameInfo->name));

			snprintf(gameInfo->path, sizeof(gameInfo->path), "%s%s%s", dirPath, FILE_SEP, name);

			gameInfo->files = new PointerList<SaveInfo>;

			list->AddTail(gameInfo);

#ifdef WIN32
			WIN32_FIND_DATA		fileData2;
			HANDLE				lpFileList;

			snprintf(path, sizeof(path), "%s%s*.*", gameInfo->path, FILE_SEP);

			lpFileList = FindFirstFile(path, &fileData2);
			if (lpFileList == INVALID_HANDLE_VALUE) continue;
#else
			DIR *dir2 = opendir(path);
			struct dirent *dent2 = nullptr;

			if (!dir2) continue;
#endif
			struct SaveWithMtime {
				SaveInfo *info;
				time_t    mtime;
			};
			std::vector<SaveWithMtime> saves;
			do {
#ifndef WIN32
				dent2 = readdir(dir2);
				if (!dent2) continue;

				snprintf(path, sizeof(path), "%s%s%s", gameInfo->path, FILE_SEP, dent2->d_name);
				if (stat(path, &tmpstat)) continue;

				if (!S_ISDIR(tmpstat.st_mode)) {
					name = dent2->d_name;
					time_t mtime = tmpstat.st_mtime;
#else
				if (!(fileData2.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
					name = fileData2.cFileName;
					// Convert FILETIME to time_t
					ULARGE_INTEGER ull;
					ull.LowPart  = fileData2.ftLastWriteTime.dwLowDateTime;
					ull.HighPart = fileData2.ftLastWriteTime.dwHighDateTime;
					time_t mtime = (time_t)((ull.QuadPart - 116444736000000000ULL) / 10000000ULL);
#endif

					SaveInfo		*saveInfo = new SaveInfo();

					strlcpy(saveInfo->fileName, name, sizeof(saveInfo->fileName));

					snprintf(saveInfo->pathName, sizeof(saveInfo->pathName), "%s%s%s", gameInfo->path, FILE_SEP, saveInfo->fileName);

					if (!ValidateGameFile(gameInfo->path, saveInfo)) {
						delete saveInfo;
						continue;
					}

					saves.push_back({saveInfo, mtime});
				}
#ifdef WIN32
			} while (FindNextFile(lpFileList, &fileData2));
			FindClose(lpFileList);

			std::sort(saves.begin(), saves.end(),
				[](const SaveWithMtime &a, const SaveWithMtime &b) {
					return a.mtime > b.mtime; // descending: newest first
				});
			for (auto &swm : saves) {
				gameInfo->files->AddTail(swm.info);
			}
		}
	} while(FindNextFile(lpDirList,&fileData));
	FindClose(lpDirList);
#else
			} while (dent2);
			closedir(dir2);

			std::sort(saves.begin(), saves.end(),
				[](const SaveWithMtime &a, const SaveWithMtime &b) {
					return a.mtime > b.mtime; // descending: newest first
				});
			for (auto &swm : saves) {
				gameInfo->files->AddTail(swm.info);
			}
		}
	} while(dent);
	closedir(d);
#endif

	return list;
}

//----------------------------------------------------------------------------
//
// Name       : SaveInfo::SaveInfo
//
// Description: Constructor
//
// Parameters : -
//
// Globals    : -
//
// Returns    : -
//
// Remark(s)  : -
//
//----------------------------------------------------------------------------
SaveInfo::SaveInfo()
:
	radarMapWidth       (0),
	radarMapHeight      (0),
	powerGraphWidth     (0),
	powerGraphHeight    (0),
	numCivs             (0),
// nf_GameSetup gameSetup;
// struct OptionScreenSettings options
	isScenario          (false),
	startInfoType       (STARTINFOTYPE_NONE),
	numPositions        (0),
	loadType            (SAVEINFOLOAD_NONE),
	showLabels          (false),
	startingPlayer      (CIV_INDEX_VANDALS)
{
	// Zero each string buffer in full, not just the first byte.  Saves
	// write these via c3files_fwrite(buf, 1, sizeof(buf), ...) — anything
	// past the null terminator goes to disk as heap garbage and shows
	// up as a fresh ~2 KB of non-determinism per save header.  Caught
	// by test_save_determinism.cpp.
	memset(gameName,   0, sizeof(gameName));
	memset(fileName,   0, sizeof(fileName));
	memset(pathName,   0, sizeof(pathName));
	memset(leaderName, 0, sizeof(leaderName));
	memset(civName,    0, sizeof(civName));
	memset(note,       0, sizeof(note));

	for (sint32 i=0; i<k_MAX_PLAYERS; i++) {
		memset(civList[i], 0, sizeof(civList[i]));
		memset(&networkGUID[i], 0, sizeof(CivGuid));
	}

	std::fill_n(playerCivIndexList, k_MAX_PLAYERS, CIV_INDEX_VANDALS);
}

//----------------------------------------------------------------------------
//
// Name       : SaveInfo::SaveInfo
//
// Description: Copy constructor
//
// Parameters : copyMe			: pointer to object to copy
//
// Globals    : -
//
// Returns    : -
//
// Remark(s)  : * The pointer data in copyMe that has been allocated on the
//                heap is not shared, but freshly allocated. This enables
//                both objects to be deallocated independently.
//
//----------------------------------------------------------------------------
SaveInfo::SaveInfo(SaveInfo *copyMe)
{
	*this = *copyMe;

	if (!(copyMe->radarMapWidth > 0 &&
		  copyMe->radarMapHeight > 0)) {

		radarMapData.clear();
	}

	if (!(copyMe->powerGraphWidth > 0 &&
		  copyMe->powerGraphHeight > 0)) {

		powerGraphData.clear();
	}

}

//----------------------------------------------------------------------------
//
// Name       : SaveInfo::~SaveInfo
//
// Description: Destructor
//
// Parameters : -
//
// Globals    : -
//
// Returns    : -
//
// Remark(s)  : * powerGraphData, radarMapData, and scenarioName are allocated
//                with new [] in GameFile::LoadBasicGameInfo and
//                GameFile::LoadBasicGameInfo.
//
//----------------------------------------------------------------------------
SaveInfo::~SaveInfo()
{
}

//----------------------------------------------------------------------------

GameInfo::GameInfo()
:
	files   (nullptr)
{
	name[0] = '\0';
	path[0] = '\0';
}

GameInfo::~GameInfo()
{
	if (files)
    {
		files->DeleteAll();
	}
	delete files;
}





SaveMapInfo::SaveMapInfo()
:
	radarMapWidth    (0),
	radarMapHeight   (0)
{
	gameMapName[0] = '\0';
	fileName[0] = '\0';
	pathName[0] = '\0';
	note[0] = '\0';
}

GameMapInfo::GameMapInfo()
:
    files   (nullptr)
{
	name[0] = '\0';
	path[0] = '\0';
}


namespace {

// JSON map-save format (replaces the CTPMAP__ binary layout).  One
// nlohmann document: header block (name/note/radar preview, what the
// old SaveExtendedGameMapInfo interleaved into the file head) plus the
// full world cell state via the existing World bridge.  Pre-JSON map
// files are not loadable — same policy as .c2g saves and .MAP scenarios.
char const * const kGameMapMagic          = "CTP2-GAMEMAP";
int          const kGameMapSchemaVersion  = 1;
std::streamoff const kMaxGameMapBytes     = 128 * 1024 * 1024;

nlohmann::json SaveMapInfoToJson(SaveMapInfo const &info)
{
	nlohmann::json radarData = nlohmann::json::array();
	for (Pixel16 px : info.radarMapData)
		radarData.push_back(px);

	return nlohmann::json{
		{"game_map_name", utf8_safe(info.gameMapName)},
		{"note",          utf8_safe(info.note)},
		{"radar_map", nlohmann::json{
			{"width",  info.radarMapWidth},
			{"height", info.radarMapHeight},
			{"data",   std::move(radarData)}}},
	};
}

// Fills the display fields only — fileName/pathName are set by the
// caller (BuildSaveMapList) and intentionally left alone.  Throws
// nlohmann::json::exception on malformed input.
void SaveMapInfoFromJson(nlohmann::json const &j, SaveMapInfo &info)
{
	std::string const name = latin1_safe(j.at("game_map_name").get<std::string>());
	std::string const note = latin1_safe(j.at("note").get<std::string>());
	strlcpy(info.gameMapName, name.c_str(), sizeof(info.gameMapName));
	strlcpy(info.note,        note.c_str(), sizeof(info.note));

	auto const &radar  = j.at("radar_map");
	sint32 const width  = radar.at("width").get<sint32>();
	sint32 const height = radar.at("height").get<sint32>();
	auto const &data   = radar.at("data");
	// Radar previews are small thumbnails; cap well above any sane size.
	if (width < 0 || height < 0 || width > 4096 || height > 4096
	    || !data.is_array()
	    || data.size() != static_cast<size_t>(width) * static_cast<size_t>(height))
	{
		throw nlohmann::json::other_error::create(
			532, "invalid radar map in game map file", &j);
	}

	info.radarMapWidth  = width;
	info.radarMapHeight = height;
	info.radarMapData.clear();
	info.radarMapData.reserve(data.size());
	for (auto const &v : data)
	{
		auto const px = v.get<int64_t>();
		if (px < 0 || px > 0xffff)
		{
			throw nlohmann::json::other_error::create(
				532, "invalid radar pixel in game map file", &j);
		}
		info.radarMapData.push_back(static_cast<Pixel16>(px));
	}
}

} // namespace

void GameMapFile::RestoreGameMap(const MBCHAR *filename)
{
    GameMapFile().Restore(filename);
}

void GameMapFile::SaveGameMap(const MBCHAR *filename, SaveMapInfo *info)
{
    GameMapFile().Save(filename, info);
}

GameMapFile::GameMapFile()
= default;

uint32 GameMapFile::Save(const MBCHAR *filepath, SaveMapInfo *info)
{
    World *w = world_Get();
    if (!w)
        return GAMEFILE_ERR_STORE_FAILED;

    SaveMapInfo localInfo;
    if (!info)
    {
        if (profiledb_Get())
            GetExtendedInfoFromProfile(&localInfo);
        info = &localInfo;
    }

    nlohmann::json doc;
    doc["magic"]          = kGameMapMagic;
    doc["schema_version"] = kGameMapSchemaVersion;
    doc["info"]           = SaveMapInfoToJson(*info);
    doc["world"]          = *w;

    std::ofstream out(filepath);
    if (!out)
    {
        c3errors_ErrorDialogFromDB("SAVE_ERROR", "SAVE_FAILED_TO_SAVE");
        return GAMEFILE_ERR_STORE_FAILED;
    }
    out << doc.dump(2);
    if (!out.good())
    {
        c3errors_FatalDialogFromDB("SAVE_ERROR", "SAVE_UNABLE_TO_WRITE_SAVEGAME");
        return GAMEFILE_ERR_STORE_FAILED;
    }
    return GAMEFILE_ERR_STORE_OK;
}

uint32 GameMapFile::Restore(const MBCHAR *filepath)
{
    World *w = world_Get();
    if (!w)
        return GAMEFILE_ERR_LOAD_FAILED;

    std::ifstream in(filepath);
    if (!in)
    {
        c3errors_ErrorDialog("LOAD_ERROR", "LOAD_FAILED_TO_LOAD_GAME");
        return GAMEFILE_ERR_LOAD_FAILED;
    }

    nlohmann::json doc;
    try { doc = ReadBoundedJson(in, kMaxGameMapBytes); }
    catch (nlohmann::json::exception const &)
    {
        c3errors_ErrorDialog("LOAD_ERROR", "LOAD_FAILED_TO_LOAD_GAME");
        return GAMEFILE_ERR_LOAD_FAILED;
    }

    if (!doc.is_object() || !doc.contains("magic")
        || !doc["magic"].is_string() || doc["magic"] != kGameMapMagic)
    {
        return GAMEFILE_ERR_LOAD_FAILED;
    }
    if (!doc.contains("schema_version")
        || !doc["schema_version"].is_number_integer()
        || doc["schema_version"] != kGameMapSchemaVersion)
    {
        return GAMEFILE_ERR_INCORRECT_VERSION;
    }

    try
    {
        doc.at("world").get_to(*w);
    }
    catch (nlohmann::json::exception const &)
    {
        return GAMEFILE_ERR_LOAD_FAILED;
    }

    // SerializeJustMap semantics: a saved map is terrain only — strip
    // units, ownership and tile improvements the loaded cells carried.
    for (sint32 x = 0; x < w->GetWidth(); ++x)
    {
        for (sint32 y = 0; y < w->GetHeight(); ++y)
        {
            w->GetCell(x, y)->ClearUnitsNStuff();
        }
    }

    return GAMEFILE_ERR_LOAD_OK;
}

void GameMapFile::SetProfileFromExtendedInfo(SaveMapInfo *info)
{
	Assert(info && profiledb_Get());
}

void GameMapFile::GetExtendedInfoFromProfile(SaveMapInfo *info)
{
	Assert(info && profiledb_Get());
}


bool GameMapFile::ValidateGameMapFile(MBCHAR const * path, SaveMapInfo *info)
{
	MBCHAR		filepath[_MAX_PATH];
	snprintf(filepath, sizeof(filepath), "%s%s%s", path, FILE_SEP, info->fileName);

	std::ifstream in(filepath);
	if (!in)
		return false;

	nlohmann::json doc;
	try { doc = ReadBoundedJson(in, kMaxGameMapBytes); }
	catch (nlohmann::json::exception const &)
	{
		return false;
	}

	if (!doc.is_object() || !doc.contains("magic")
	    || !doc["magic"].is_string() || doc["magic"] != kGameMapMagic
	    || !doc.contains("schema_version")
	    || !doc["schema_version"].is_number_integer()
	    || doc["schema_version"] != kGameMapSchemaVersion)
	{
		return false;
	}

	// Parse into a scratch record so a malformed block can't leave the
	// caller's info half-filled.
	SaveMapInfo parsed;
	try { SaveMapInfoFromJson(doc.at("info"), parsed); }
	catch (nlohmann::json::exception const &)
	{
		return false;
	}

	// fileName/pathName belong to the caller (BuildSaveMapList) — copy
	// only the fields that come from the file.
	strlcpy(info->gameMapName, parsed.gameMapName, sizeof(info->gameMapName));
	strlcpy(info->note,        parsed.note,        sizeof(info->note));
	info->radarMapWidth  = parsed.radarMapWidth;
	info->radarMapHeight = parsed.radarMapHeight;
	info->radarMapData   = std::move(parsed.radarMapData);
	return true;
}

PointerList<GameMapInfo> *GameMapFile::BuildSaveMapList(C3SAVEDIR dir)
{
	PointerList<GameMapInfo> * list = new PointerList<GameMapInfo>;
	MBCHAR dirPath[_MAX_PATH];
	MBCHAR path[_MAX_PATH];
	if (!civpaths_Get()->GetSavePath(dir, dirPath)) return list;

#ifdef WIN32
	snprintf(path, sizeof(path), "%s%s*.*", dirPath, FILE_SEP);

	WIN32_FIND_DATA fileData;
	HANDLE          lpDirList = FindFirstFile(path, &fileData);
	if (lpDirList == INVALID_HANDLE_VALUE) return list;
#else
	DIR * d = opendir(dirPath);
	if (!d) return list;

	struct stat     tmpstat;
	struct dirent * dent = nullptr;
#endif

	GameMapInfo			*gameInfo;

	do {
#ifndef WIN32
		dent = readdir(d);
		if (!dent) continue;

		snprintf(path, sizeof(path), "%s%s%s", dirPath, FILE_SEP, dent->d_name);
		if (!stat(path, &tmpstat))
			continue;

		if (S_ISDIR(tmpstat.st_mode)) {
			MBCHAR *name = dent->d_name;
#else
		if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			MBCHAR *name = fileData.cFileName;
#endif
			if (!strcmp(name, ".")) continue;
			if (!strcmp(name, "..")) continue;

			gameInfo = new GameMapInfo();
			strlcpy(gameInfo->name, name, sizeof(gameInfo->name));
			snprintf(gameInfo->path, sizeof(gameInfo->path), "%s%s%s", dirPath, FILE_SEP, name);
			gameInfo->files = new PointerList<SaveMapInfo>;

			list->AddTail(gameInfo);

#ifdef WIN32
			WIN32_FIND_DATA		fileData2;
			HANDLE				lpFileList;

			snprintf(path, sizeof(path), "%s%s*.*", gameInfo->path, FILE_SEP);

			lpFileList = FindFirstFile(path, &fileData2);
			if (lpFileList == INVALID_HANDLE_VALUE) continue;
#else
			DIR *dir2 = opendir(path);
			struct dirent *dent2 = nullptr;

			if (!dir2) continue;
#endif
			do {
#ifndef WIN32
				dent2 = readdir(dir2);
				if (!dent2) continue;

				snprintf(path, sizeof(path), "%s%s%s", gameInfo->path, FILE_SEP, dent2->d_name);
				if (!stat(path, &tmpstat)) continue;

				if (!S_ISDIR(tmpstat.st_mode)) {
					name = dent2->d_name;
#else
				if (!(fileData2.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
					name = fileData2.cFileName;
#endif
					SaveMapInfo		*saveInfo = new SaveMapInfo();

					strlcpy(saveInfo->fileName, name, sizeof(saveInfo->fileName));

					snprintf(saveInfo->pathName, sizeof(saveInfo->pathName), "%s%s%s", gameInfo->path, FILE_SEP, saveInfo->fileName);

					if (!ValidateGameMapFile(gameInfo->path, saveInfo)) {
						delete saveInfo;
						continue;
					}

					gameInfo->files->AddTail(saveInfo);
				}
#ifdef WIN32
			} while (FindNextFile(lpFileList, &fileData2));
			FindClose(lpFileList);
		}
	} while(FindNextFile(lpDirList,&fileData));
	FindClose(lpDirList);
#else
			} while (dent2);
			closedir(dir2);
		}
	} while(dent);
	closedir(d);
#endif

	return list;
}
