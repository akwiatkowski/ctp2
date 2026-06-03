//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
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
// Compiler flags
//
// _NO_GAME_WATCH
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Replaced CIV_INDEX by sint32. (2-Jan-2008 Martin G�hmann)
//
//----------------------------------------------------------------------------

#if defined(HAVE_PRAGMA_ONCE)
#pragma once
#endif

#ifndef GAMEFILE_H__
#define GAMEFILE_H__

class GameFile;
class GameInfo;
class GameMapFile;
class GameMapInfo;
class SaveInfo;
class SaveMapInfo;

size_t const	SAVE_LEADER_NAME_SIZE	= 6;

enum GAMEFILE_ERR
{
	GAMEFILE_ERR_LOAD_OK,
	GAMEFILE_ERR_LOAD_FAILED,
	GAMEFILE_ERR_INCORRECT_VERSION,
	GAMEFILE_ERR_STORE_OK,
	GAMEFILE_ERR_STORE_FAILED,
	GAMEFILE_ERR_MAX
};

enum SAVEINFOLOAD
{
	SAVEINFOLOAD_NONE = -1,

	SAVEINFOLOAD_BASIC,
	SAVEINFOLOAD_EXTENDED,

	SAVEINFOLOAD_MAX
};

#include "gs/core/pixel_types.h"
#include "ctp/ctp2_utils/pointerlist.h"
#include "ui/netshell/ns_gamesetup.h"
#include "gs/world/MapPoint.h"
#include "gs/fileio/StartingPosition.h"
#include "gs/fileio/civscenarios.h"
#include "gs/gameobj/CivilisationPool.h"
class CivArchive;

#include <string>

struct CivGuid {
	sint32 civIndex;
	GUID guid;
};


class SaveInfo {
public:
	SaveInfo();
	SaveInfo(SaveInfo *copyMe);
	~SaveInfo();

	MBCHAR					gameName[_MAX_PATH];
	MBCHAR					fileName[_MAX_PATH];
	MBCHAR					pathName[_MAX_PATH];
	MBCHAR					leaderName[k_MAX_NAME_LEN];
	MBCHAR					civName[k_MAX_NAME_LEN];
	MBCHAR					note[_MAX_PATH];
	sint32					radarMapWidth;
	sint32					radarMapHeight;
	Pixel16					*radarMapData;
	sint32					powerGraphWidth;
	sint32					powerGraphHeight;
	Pixel16					*powerGraphData;
	sint32					numCivs;
	MBCHAR					civList[k_MAX_PLAYERS][k_MAX_NAME_LEN];
	CivGuid                 networkGUID[k_MAX_PLAYERS];

	nf_GameSetup gameSetup;

	struct OptionScreenSettings
	{

		BOOL	tutorialadvice;
		BOOL	leftrightclickmove;
		BOOL	autocycleturn;
		BOOL	autocycleunits;
		BOOL	battleview;
		BOOL	monument;

		BOOL	walk;
		BOOL	goods;
		BOOL	attack;
		BOOL	trade;
		BOOL	wonder;
		BOOL	library;
		BOOL	message;
		BOOL	water;
		BOOL	movie;
		BOOL	grid;

		sint32	sfxVolume;
		sint32	musicVolume;
		sint32	voiceVolume;

		BOOL	autoRepeat;
		BOOL	randomOrder;
		BOOL	musicOn;
	} options;

	BOOL isScenario;
	STARTINFOTYPE startInfoType;
	sint32 numPositions;
	StartingPosition positions[k_MAX_START_POINTS];

	SAVEINFOLOAD	loadType;




	std::string			scenarioName;


	sint32		playerCivIndexList[k_MAX_PLAYERS];




	BOOL	showLabels;
	sint32	startingPlayer;
};

class GameInfo {
public:
	GameInfo();
	~GameInfo();
	MBCHAR					name[_MAX_PATH];
	MBCHAR					path[_MAX_PATH];
	PointerList<SaveInfo>	*files;
};

class GameFile
{
public:
	GameFile();

	uint32 SaveDB(CivArchive &archive);
	// G-4c-2 prep: the legacy CivArchive binary-format save/restore
	// routines.  Public entry points (SaveGame / RestoreGame) dispatch
	// to these only when g_useJsonSave is false (or when the file's
	// first non-whitespace byte isn't '{').  Renamed so the binary
	// path is a clear deletion target once existing .c2g scenarios
	// have been migrated to JSON via the converter (Phase G-1).
	uint32 SaveLegacyBinary(MBCHAR const * filepath, SaveInfo *info);
	uint32 RestoreLegacyBinary(MBCHAR const * filepath);

	static bool LoadExtendedGameInfo(FILE *saveFile, SaveInfo *info);
	static bool LoadBasicGameInfo(FILE *saveFile, SaveInfo *info);
	static void SaveExtendedGameInfo(FILE *saveFile, SaveInfo *info);

	static void SetProfileFromExtendedInfo(SaveInfo *info);
	static void GetExtendedInfoFromProfile(SaveInfo *info);

	static void RestoreGame(MBCHAR const *filename);
	static void SaveGame(MBCHAR const * filename, SaveInfo *info);
	static void RestoreScenarioGame(MBCHAR const *name);

	static bool ValidateGameFile(MBCHAR const * path, SaveInfo *info);
	static bool FetchExtendedSaveInfo(MBCHAR const * path, SaveInfo *info);
	static PointerList<GameInfo> * BuildSaveList(C3SAVEDIR dir);
};




class SaveMapInfo {
public:
	SaveMapInfo();

	MBCHAR					gameMapName[_MAX_PATH];
	MBCHAR					fileName[_MAX_PATH];
	MBCHAR					pathName[_MAX_PATH];
	MBCHAR					note[_MAX_PATH];
	sint32					radarMapWidth;
	sint32					radarMapHeight;
	Pixel16					*radarMapData;
};

class GameMapInfo {
public:
	GameMapInfo();
	MBCHAR					name[_MAX_PATH];
	MBCHAR					path[_MAX_PATH];
	PointerList<SaveMapInfo>	*files;
};

class GameMapFile
{
public:
	GameMapFile() ;

	uint32 Save(const MBCHAR *filepath, SaveMapInfo *info) ;
	uint32 Restore(const MBCHAR *filepath) ;

	static bool LoadExtendedGameMapInfo(FILE *saveFile, SaveMapInfo *info);
	static void SaveExtendedGameMapInfo(FILE *saveFile, SaveMapInfo *info);

	static void SetProfileFromExtendedInfo(SaveMapInfo *info);
	static void GetExtendedInfoFromProfile(SaveMapInfo *info);

	static void RestoreGameMap(const MBCHAR *filename) ;
	static void SaveGameMap(const MBCHAR *filename, SaveMapInfo *info) ;

	static bool ValidateGameMapFile(MBCHAR const * path, SaveMapInfo *info);
	static PointerList<GameMapInfo> *BuildSaveMapList(C3SAVEDIR dir);
};

// Save-file version: -1 if no save loaded yet; otherwise the schema
// version pulled from the file's magic value (see s_magicValue table
// in GameFile.cpp).  Almost every gs/ Serialize() reads this to gate
// backward-compat branches.  Definition is file-scope `static` in
// GameFile.cpp; external readers go through save_file_version_Get().
sint32 save_file_version_Get(void);
void   save_file_version_Set(sint32 v);
// Scenario start-info type (NOLOCS / CIVSFIXED / POSITIONSFIXED / CIVS /
// NONE).  Definition is file-scope `static` in GameFile.cpp; the
// scenario editor + scenario window set it, the world-init plumbing
// reads it.
sint32 start_info_type_Get(void);
void   start_info_type_Set(sint32 v);
// Scenario-mode flag.  Definition is file-scope `static` in GameFile.cpp;
// scenario editor / window / loaders set it; gameplay code reads it to
// gate scenario-only branches.
sint32 is_scenario_Get(void);
void   is_scenario_Set(sint32 v);
// Number of scenario civs the player picked.  Definition in GameFile.cpp
// as file-scope static; the UI (loadsavescreen, allinonewindow) and
// gameinit's scenario-load path read/write through the accessors.
sint32 scenario_civs_Get(void);
void   scenario_civs_Set(sint32 v);
// Scenario name buffer.  Definition in GameFile.cpp as file-scope
// `static`; callers treat the returned pointer as a writable buffer of
// k_SCENARIO_NAME_MAX bytes (used directly by strcpy / memset / char
// indexing across the UI + civ3_main bootstrap).
MBCHAR * scenario_name_buf(void);

// Lifecycle: definition lives in gs/fileio/GameFile.cpp as file-scope
// `static`.  Read sites use show_unit_labels_Get(); the scenario editor
// + GameFile load/save paths flip it via show_unit_labels_Set().
bool show_unit_labels_Get(void);
void show_unit_labels_Set(bool value);

// Scenario starting-player slot (-1 = unset).  Definition is file-scope
// `static` in GameFile.cpp; the scenario editor sets it via _Set, the
// save/load info plumbing reads it via _Get.
sint32 starting_player_Get(void);
void   starting_player_Set(sint32 v);

sint32 gamefile_CurrentVersion();

#endif
