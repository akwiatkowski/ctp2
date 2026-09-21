//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Loads Scenarios
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
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Fixed memory leak in LoadScenarioPackData, by Martin G�hmann.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"

#include "ctp/ctp2_utils/pointerlist.h"

#include "gs/fileio/CivPaths.h"
#include "gs/fileio/civscenarios.h"
#include "gs/fileio/gamefile.h"

#include <memory>

#ifndef WIN32
#include <sys/types.h>
#include <dirent.h>
#endif



static std::unique_ptr<CivScenarios> g_civScenarios;

CivScenarios * civscenarios_Get()
{
	return g_civScenarios.get();
}


void CivScenarios::Initialize()
{
	g_civScenarios = std::make_unique<CivScenarios>();
}


void CivScenarios::Cleanup()
{
	g_civScenarios.reset();
}


CivScenarios::CivScenarios()
{
	LoadData();
}


CivScenarios::~CivScenarios()
{
	if (!m_scenarioPacks.empty())
		ClearData();
}


void CivScenarios::LoadScenarioData(Scenario *scenario, MBCHAR *scenPath)
{
	FILE				*scenFile;
	MBCHAR				scenFileName[_MAX_PATH];

	scenario->m_name[0] = '\0';
	scenario->m_description[0] = '\0';

	snprintf(scenFileName, sizeof(scenFileName), "%s%s%s", scenPath, FILE_SEP, k_SCENARIO_INFO_FILENAME);

	scenFile = fopen(scenFileName, "r");

	if (scenFile) {
		if (!fgets(scenario->m_name, k_SCENARIO_NAME_MAX-1, scenFile)) return;
		scenario->m_name[k_SCENARIO_NAME_MAX-1]='\0';
		scenario->m_name[strlen(scenario->m_name)-1] = '\0';
		if (!fgets(scenario->m_description, k_SCENARIO_DESC_MAX-1, scenFile)) return;
		scenario->m_description[k_SCENARIO_DESC_MAX-1] = '\0';
		if(scenario->m_description[strlen(scenario->m_description)-1]!=0x0A)
		{
			MBCHAR tempbuf[_MAX_PATH];
			do
			{
				memset(tempbuf,0,sizeof(tempbuf));
				if(!fgets(tempbuf, _MAX_PATH, scenFile))
					break;
			} while(tempbuf[strlen(tempbuf)-1]!=0x0A);
		}
		else
		{
			scenario->m_description[strlen(scenario->m_description)-1] = '\0';
		}
		fclose(scenFile);
	}

}


void CivScenarios::LoadScenarioPackData(ScenarioPack *pack, MBCHAR *packPath)
{
	FILE				*listFile;
	MBCHAR				listFileName[_MAX_PATH];
	sint32				i;
#ifdef WIN32
	struct _stat		tmpstat;
#else
	struct stat		tmpstat;
#endif

	pack->m_scenarios.clear();
	pack->m_name[0] = '\0';
	pack->m_description[0] = '\0';

	snprintf(listFileName, sizeof(listFileName), "%s%s%s", packPath, FILE_SEP, k_SCENARIO_PACK_LIST_FILENAME);

	listFile = fopen(listFileName, "r");
	if (!listFile) return;

	if (!fgets(pack->m_name, k_SCENARIO_PACK_NAME_MAX-1, listFile)) return;
	pack->m_name[k_SCENARIO_PACK_NAME_MAX-1] = '\0';
	pack->m_name[strlen(pack->m_name)-1] = '\0';

	if (!fgets(pack->m_description, k_SCENARIO_PACK_DESC_MAX-1, listFile)) return;
	pack->m_description[k_SCENARIO_PACK_DESC_MAX-1] = '\0';
	if(pack->m_description[strlen(pack->m_description)-1]!=0x0A)
	{
		MBCHAR tempbuf[_MAX_PATH];
		do
		{
			memset(tempbuf,0,sizeof(tempbuf));
			if(!fgets(tempbuf, _MAX_PATH, listFile))
				break;
		} while(tempbuf[strlen(tempbuf)-1]!=0x0A);
	}
	else
	{
		pack->m_description[strlen(pack->m_description)-1] = '\0';
	}

	sint32		numScenarios;
	fscanf(listFile, "%d", &numScenarios);

	fclose(listFile);

	if (numScenarios <= 0) return;

	auto scenList = std::make_unique<PointerList<MBCHAR>>();
	for (i=0; i<numScenarios; i++) {
		MBCHAR		scenPath[_MAX_PATH];
		MBCHAR		scenListName[_MAX_PATH];
		int		r;

		snprintf(scenPath, sizeof(scenPath), "%s%s%s%.4d", packPath, FILE_SEP, k_SCENARIO_FOLDER_PREFIX, i);
		snprintf(scenListName, sizeof(scenListName), "%s%s%s", scenPath, FILE_SEP, k_SCENARIO_INFO_FILENAME);

#ifdef WIN32
		r = _stat(scenListName, &tmpstat);
#else
		r = stat(scenListName, &tmpstat);
#endif
		if (!r) {
			auto scenarioPath = std::make_unique<MBCHAR[]>(strlen(scenPath)+1);
			strlcpy(scenarioPath.get(), scenPath, strlen(scenPath)+1);
			scenList->AddTail(scenarioPath.release());
		}
	}

	if (scenList->GetCount() > 0)
	{
		numScenarios = scenList->GetCount();

		pack->m_scenarios.resize(numScenarios);

		PointerList<MBCHAR>::Walker walker(scenList.get());

		i=0;
		while (walker.IsValid()) {
			strlcpy(pack->m_scenarios[i].m_path, walker.GetObj(), sizeof(pack->m_scenarios[i].m_path));
			LoadScenarioData(&pack->m_scenarios[i], walker.GetObj());

			walker.Next();
			i++;
		}
//Added by Martin G�hmann
	}

	//This must be deleted always
	scenList->DeleteAll();
}


void CivScenarios::LoadData()
{
	MBCHAR			    path[_MAX_PATH];
    MBCHAR              rootPath[_MAX_PATH];
#ifdef WIN32
	WIN32_FIND_DATA		fileData;
	HANDLE			    lpFileList;
	struct _stat		tmpstat;
#else
	struct stat		    tmpstat;
#endif
	 sint32			i;


	civpaths_Get()->GetScenarioRootPath(rootPath);

#ifdef WIN32
	snprintf(path, sizeof(path), "%s%s*.*", rootPath, FILE_SEP);
	lpFileList = FindFirstFile(path,&fileData);

	if (lpFileList == INVALID_HANDLE_VALUE) return;
#else
	DIR *dir = opendir(rootPath);
	if (!dir) return;
	struct dirent *dent = nullptr;
#endif

	auto packList = std::make_unique<PointerList<MBCHAR>>();
	do {
#ifndef WIN32
		dent = readdir(dir);
		if (!dent)
			continue;
		snprintf(path, sizeof(path), "%s%s%s", rootPath, FILE_SEP, dent->d_name);
		if (!stat(path, &tmpstat))
			continue;

		if (S_ISDIR(tmpstat.st_mode)) {
			MBCHAR *name = dent->d_name;
#else
		if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			MBCHAR *name = fileData.cFileName;
#endif
			MBCHAR		packListName[_MAX_PATH];
			int		r;

			snprintf(packListName, sizeof(packListName), "%s%s%s%s%s", rootPath, FILE_SEP, name, FILE_SEP, k_SCENARIO_PACK_LIST_FILENAME);
#ifdef WIN32
			r = _stat(packListName, &tmpstat);
#else
			r = stat(packListName, &tmpstat);
#endif
			if (!r) {
				auto fileListFileName = std::make_unique<MBCHAR[]>(strlen(name)+1);

				strlcpy(fileListFileName.get(), name, strlen(name)+1);

				packList->AddTail(fileListFileName.release());
			}
		}
#ifndef WIN32
	} while(dent);
	closedir(dir);
#else
	} while(FindNextFile(lpFileList,&fileData));
	FindClose(lpFileList);
#endif

	if (packList->GetCount() <= 0) {
		return;
	}

	sint32 numScenarioPacks = packList->GetCount();
	m_scenarioPacks.resize(numScenarioPacks);

	PointerList<MBCHAR>::Walker walker(packList.get());

	for (i=0; i<numScenarioPacks; i++) {
		MBCHAR		packPath[_MAX_PATH];

		std::unique_ptr<MBCHAR[]> fileListFileName(walker.GetObj());
		snprintf(packPath, sizeof(packPath), "%s%s%s", rootPath, FILE_SEP, walker.GetObj());

		strlcpy(m_scenarioPacks[i].m_path, packPath, sizeof(m_scenarioPacks[i].m_path));
		m_scenarioPacks[i].m_index = i;
		LoadScenarioPackData(&m_scenarioPacks[i], packPath);


		walker.Next();
	}



}


void CivScenarios::ClearData()
{
	m_scenarioPacks.clear();
}


void CivScenarios::ReloadData()
{
	if (!m_scenarioPacks.empty()) {
		ClearData();
	}

	LoadData();
}

ScenarioPack *CivScenarios::GetScenarioPack(sint32 which)
{
	sint32 numScenarioPacks = static_cast<sint32>(m_scenarioPacks.size());
	Assert(which >= 0 && which < numScenarioPacks);
	if (which < 0 || which >= numScenarioPacks) return nullptr;

	return &m_scenarioPacks[which];
}

ScenarioPack *CivScenarios::GetScenarioPackByPath(const MBCHAR *path)
{
	sint32 p;
	sint32 numScenarioPacks = static_cast<sint32>(m_scenarioPacks.size());
	for(p = 0; p < numScenarioPacks; p++) {
		if(!stricmp(path, m_scenarioPacks[p].m_path)) {
			return &m_scenarioPacks[p];
		}
	}
	return nullptr;
}


BOOL CivScenarios::FindScenario(MBCHAR *scenarioName, ScenarioPack **pack, Scenario **scen)
{

	sint32 numScenarioPacks = static_cast<sint32>(m_scenarioPacks.size());
	for (sint32 i=0; i<numScenarioPacks; i++) {
		ScenarioPack *scenarioPack = &m_scenarioPacks[i];

		if (!scenarioPack) continue;

		sint32 numScenarios = static_cast<sint32>(scenarioPack->m_scenarios.size());
		for (sint32 j=0; j<numScenarios; j++) {
			Scenario *scenario = &scenarioPack->m_scenarios[j];

			if (!scenario) continue;

			if (!_stricoll(scenarioName, scenario->m_name)) {
				*pack = scenarioPack;
				*scen = scenario;

				return TRUE;
			}
		}
	}

	*pack = nullptr;
	*scen = nullptr;

	return FALSE;
}

BOOL CivScenarios::FindScenarioFromSaveFile(MBCHAR *saveName, ScenarioPack **pack, Scenario **scen)
{
	MBCHAR path[_MAX_PATH];
	strlcpy(path, saveName, sizeof(path));
	MBCHAR *lastBackslash = strrchr(path, FILE_SEPC);
	if(!lastBackslash)
		return FALSE;

	*lastBackslash = '\0';

	Scenario tmpscen;
	tmpscen.m_name[0] = 0;
	LoadScenarioData(&tmpscen, path);
	if(tmpscen.m_name[0] != 0) {
		return FindScenario(tmpscen.m_name, pack, scen);
	}

	return FALSE;
}




BOOL CivScenarios::ScenarioHasSavedGame(Scenario *scen)
{
	MBCHAR	tempPath[_MAX_PATH];

	if (!scen) return FALSE;

	snprintf(tempPath, sizeof(tempPath), "%s%s%s",
						scen->m_path, FILE_SEP,
						k_SCENARIO_DEFAULT_SAVED_GAME_NAME);

	if (c3files_PathIsValid(tempPath)) {
		return TRUE;
	}

	return FALSE;
}

SaveInfo *CivScenarios::LoadSaveInfo(Scenario *scen)
{
	MBCHAR	tempPath[_MAX_PATH];

	if (!scen) return nullptr;

	snprintf(tempPath, sizeof(tempPath), "%s%s%s",
						scen->m_path, FILE_SEP,
						k_SCENARIO_DEFAULT_SAVED_GAME_NAME);

	if (c3files_PathIsValid(tempPath)) {
		auto info = std::make_unique<SaveInfo>();
		if(!GameFile::FetchExtendedSaveInfo(tempPath, info.get())) {
			return nullptr;
		} else {
			return info.release();
		}
	}

	return nullptr;
}

CIV_SCEN_ERR CivScenarios::MakeNewPack(MBCHAR *dirName, MBCHAR *packName, MBCHAR *packDesc)
{
	MBCHAR				path[_MAX_PATH],
					rootPath[_MAX_PATH];
#ifdef WIN32
	struct _stat		tmpstat;
#else
	struct stat		tmpstat;
#endif

	civpaths_Get()->GetScenarioRootPath(rootPath);

	snprintf(path, sizeof(path), "%s%s%s", rootPath, FILE_SEP, dirName);
#ifdef WIN32
	if(!_stat(path, &tmpstat)) {
#else
	if(!stat(path, &tmpstat)) {
#endif
		return CIV_SCEN_DIR_EXISTS;
	}

	c3files_CreateDirectory(path);

	strncat(path, FILE_SEP "packlist.txt", sizeof(path) - strlen(path) - 1);
	FILE *packlist = fopen(path, "w");
	Assert(packlist);
	if(!packlist)
		return CIV_SCEN_CANT_CREATE_FILE;

	fprintf(packlist, "%s\n", packName);
	fprintf(packlist, "%s\n", packDesc);
	fprintf(packlist, "0\n");

	fclose(packlist);

	ReloadData();

	return CIV_SCEN_OK;
}

CIV_SCEN_ERR CivScenarios::UpdatePacklist(ScenarioPack *pack)
{
	MBCHAR path[_MAX_PATH];
	snprintf(path, sizeof(path), "%s%spacklist.txt", pack->m_path, FILE_SEP);

	FILE *packList = fopen(path, "w");

	Assert(packList);
	if(!packList) {
		return CIV_SCEN_CANT_CREATE_FILE;
	}

	fprintf(packList, "%s\n", pack->m_name);
	fprintf(packList, "%s\n", pack->m_description);
	fprintf(packList, "%d\n", static_cast<sint32>(pack->m_scenarios.size()));

	fclose(packList);

	return CIV_SCEN_OK;
}

CIV_SCEN_ERR CivScenarios::MakeNewScenario(ScenarioPack *pack, MBCHAR *scenName, MBCHAR *scenDesc)
{
#ifdef WIN32
	struct _stat		tmpstat;
#else
	struct stat		tmpstat;
#endif

	MBCHAR scenPath[_MAX_PATH];
	sint32 nextScenarioIndex = static_cast<sint32>(pack->m_scenarios.size());
	snprintf(scenPath, sizeof(scenPath), "%s%sscen%04d", pack->m_path, FILE_SEP, nextScenarioIndex);
#ifdef WIN32
	if(!_stat(scenPath, &tmpstat)) {
#else
	if(!stat(scenPath, &tmpstat)) {
#endif

		return CIV_SCEN_DIR_EXISTS;
	}

	c3files_CreateDirectory(scenPath);

	MBCHAR descPath[_MAX_PATH];
	snprintf(descPath, sizeof(descPath), "%s%sscenario.txt", scenPath, FILE_SEP);
	FILE *scenFile = fopen(descPath, "w");
	Assert(scenFile);
	if(!scenFile)
		return CIV_SCEN_CANT_CREATE_FILE;

	fprintf(scenFile, "%s\n", scenName);
	fprintf(scenFile, "%s\n", scenDesc);
	fclose(scenFile);

	MBCHAR dataPath[_MAX_PATH];
	snprintf(dataPath, sizeof(dataPath), "%s%sdefault", scenPath, FILE_SEP);
	c3files_CreateDirectory(dataPath);

	strncat(dataPath, FILE_SEP "gamedata", sizeof(dataPath) - strlen(dataPath) - 1);
	c3files_CreateDirectory(dataPath);

	MBCHAR filePath[_MAX_PATH];
	snprintf(filePath, sizeof(filePath), "%s%sscenario.slc", dataPath, FILE_SEP);

	FILE *script = fopen(filePath, "w");
	Assert(script);
	if(!script)
		return CIV_SCEN_CANT_CREATE_FILE;

	fprintf(script, "// Scenario script for %s\n", scenName);
	fclose(script);

	pack->m_scenarios.resize(pack->m_scenarios.size() + 1);
	UpdatePacklist(pack);

	ReloadData();

	return CIV_SCEN_OK;
}
