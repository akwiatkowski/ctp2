//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : File paths
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
// _MSC_VER
// - Compiler version (for the Microsoft C++ compiler only)
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Added option to use multiple data directories.
// - FindFile can ignore files in scenario paths. (9-Apr-2007 Martin G�hmann)
//
//----------------------------------------------------------------------------

#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif

#ifndef __CIVPATHS_H__
#define __CIVPATHS_H__ 1

#include "ctp/ctp2_utils/c3files.h"
#include <string>
#include <vector>	// list did not work: crashes on begin() for empty list








class CivPaths {
private:
	std::string m_hdPath;
	std::string m_cdPath;

	std::string m_defaultPath;
	std::string m_localizedPath;

	std::string m_dataPath;			                // original data path (...\ctp2_data)
	std::vector<std::string> m_extraDataPaths;      // searched before m_dataPath
	std::string m_scenariosPath;

	std::string m_savePath;
	std::string m_saveGamePath;
	std::string m_saveQueuePath;
	std::string m_saveMPPath;
	std::string m_saveSCENPath;
	std::string m_saveMapPath;
	std::string m_saveClipsPath;

	std::string m_assetPaths[C3DIR_MAX];

	std::string m_curScenarioPath;

	std::string m_curScenarioPackPath;

	MBCHAR m_desktopPath[_MAX_PATH];

public:

	CivPaths ();

	virtual ~CivPaths();

	void CreateSaveFolders(const MBCHAR *path);

	void InitCDPath();

	MBCHAR *GetSavePath(C3SAVEDIR dir, MBCHAR *path);




	MBCHAR *FindFile(C3DIR dir, const MBCHAR *filename, MBCHAR *path,
                     bool silent = false, bool check_prjfile = true, bool checkScenario = true);

	MBCHAR *GetSpecificPath(C3DIR dir, MBCHAR *path, BOOL local);

	MBCHAR *GetScenarioRootPath(MBCHAR *path);

	void	SetCurScenarioPath(const MBCHAR *path);

	const MBCHAR *GetCurScenarioPath() const;

	void	ClearCurScenarioPath();




	void	SetCurScenarioPackPath(const MBCHAR *path);

	const MBCHAR * GetCurScenarioPackPath() const;

	void	ClearCurScenarioPackPath();











    bool        FindPath(C3DIR dir, int num, MBCHAR *path);

	const MBCHAR *  GetSavePathString() const { return m_savePath.c_str(); }

	const MBCHAR *  GetDesktopPath();

	std::vector<std::string> const &
                    GetExtraDataPaths() const;
	void	        InsertExtraDataPath(MBCHAR const * path);
	void	        ResetExtraDataPaths();

protected:

	MBCHAR *    MakeAssetPath
    (
        MBCHAR *        fullPath,
        MBCHAR const *  s1,
        MBCHAR const *  s2,
        MBCHAR const *  s3,
        MBCHAR const *  s4,
        MBCHAR const *  s5
    ) const;

	MBCHAR *    MakeSavePath(MBCHAR *fullPath, MBCHAR *s1, MBCHAR *s2, MBCHAR *s3);
};







void CivPaths_InitCivPaths();


void CivPaths_CleanupCivPaths();






// civpaths_Get() is file-static in CivPaths.cpp; access via accessors.
CivPaths * civpaths_Get();
void       civpaths_Set(CivPaths *p);

#endif
