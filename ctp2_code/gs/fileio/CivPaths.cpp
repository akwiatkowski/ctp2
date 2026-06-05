//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
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
// - None
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Added option to use multiple data directories.
// - Memory leak/crash fix
// - FindFile can ignore files in scenario paths. (9-Apr-2007 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ctp/ctp2_utils/c3files.h"
#include "gs/fileio/CivPaths.h"

#ifdef WIN32
#include <shlobj.h>
#endif

static CivPaths *g_civPaths = nullptr;
CivPaths * civpaths_Get()        { return g_civPaths; }
void       civpaths_Set(CivPaths *p) { g_civPaths = p; }

#include "gs/fileio/prjfile.h"
extern ProjectFile *g_ImageMapPF;


void CivPaths_InitCivPaths()
{
    delete g_civPaths;
	g_civPaths = new CivPaths;
}


void CivPaths_CleanupCivPaths()
{
    delete g_civPaths;
	g_civPaths = nullptr;
}


CivPaths::CivPaths ()
{
    std::fill(m_desktopPath, m_desktopPath + _MAX_PATH, 0);

    FILE *  fin = fopen("civpaths.txt", "r");
    Assert(fin);

	// fgets a single line into `dst`, trim trailing \n / \r.
	auto readPath = [fin](std::string &dst) {
		MBCHAR buf[_MAX_PATH];
		if (fgets(buf, _MAX_PATH, fin)) {
			size_t len = strlen(buf);
			while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
				buf[--len] = '\0';
			dst.assign(buf, len);
		}
	};
	// fscanf-%s into `dst`. Bounded by _MAX_PATH like the legacy path.
	auto scanPath = [fin](std::string &dst) {
		MBCHAR buf[_MAX_PATH];
		buf[0] = '\0';
		fscanf(fin, "%1023s", buf);
		dst = buf;
	};

	readPath(m_hdPath);
	readPath(m_cdPath);
	readPath(m_defaultPath);
	readPath(m_localizedPath);
	readPath(m_dataPath);
	readPath(m_scenariosPath);
	readPath(m_savePath);
	readPath(m_saveGamePath);
	readPath(m_saveQueuePath);
	readPath(m_saveMPPath);
	readPath(m_saveSCENPath);
	readPath(m_saveMapPath);
	scanPath(m_saveClipsPath);

	for (auto & m_assetPath : m_assetPaths)
    {
		scanPath(m_assetPath);
	}

	fclose(fin);

	MBCHAR	tempPath[_MAX_PATH];
	MBCHAR	fullPath[_MAX_PATH];
	MBCHAR	*s;

	snprintf(tempPath, sizeof(tempPath), "%s%s%s", m_hdPath.c_str(), FILE_SEP, m_savePath.c_str());
	s = _fullpath(fullPath, tempPath, _MAX_PATH);
	Assert(s != nullptr);

	CreateSaveFolders(fullPath);
}


CivPaths::~CivPaths() = default;

void CivPaths::CreateSaveFolders(const MBCHAR *path)
{
#ifdef WIN32
	SECURITY_ATTRIBUTES		sa;

	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	CreateDirectory((LPCTSTR)path, &sa);
#else
	mode_t mode = 0777;
	mkdir(path, mode);
#endif

	MBCHAR subFolderPath[_MAX_PATH];

	snprintf(subFolderPath, sizeof(subFolderPath), "%s%s%s", path, FILE_SEP, m_saveGamePath.c_str());
#ifdef WIN32
	CreateDirectory((LPCTSTR)subFolderPath, &sa);
#else
	mkdir(subFolderPath, mode);
#endif
	snprintf(subFolderPath, sizeof(subFolderPath), "%s%s%s", path, FILE_SEP, m_saveQueuePath.c_str());
#ifdef WIN32
	CreateDirectory((LPCTSTR)subFolderPath, &sa);
#else
	mkdir(subFolderPath, mode);
#endif
	snprintf(subFolderPath, sizeof(subFolderPath), "%s%s%s", path, FILE_SEP, m_saveMPPath.c_str());
#ifdef WIN32
	CreateDirectory((LPCTSTR)subFolderPath, &sa);
#else
	mkdir(subFolderPath, mode);
#endif
	snprintf(subFolderPath, sizeof(subFolderPath), "%s%s%s", path, FILE_SEP, m_saveSCENPath.c_str());
#ifdef WIN32
	CreateDirectory((LPCTSTR)subFolderPath, &sa);
#else
	mkdir(subFolderPath, mode);
#endif
	snprintf(subFolderPath, sizeof(subFolderPath), "%s%s%s", path, FILE_SEP, m_saveMapPath.c_str());
#ifdef WIN32
	CreateDirectory((LPCTSTR)subFolderPath, &sa);
#else
	mkdir(subFolderPath, mode);
#endif
	snprintf(subFolderPath, sizeof(subFolderPath), "%s%s%s", path, FILE_SEP, m_saveClipsPath.c_str());
#ifdef WIN32
	CreateDirectory((LPCTSTR)subFolderPath, &sa);
#else
	mkdir(subFolderPath, mode);
#endif
}

void CivPaths::InitCDPath()
{
	MBCHAR tempPath[_MAX_PATH];
	snprintf(tempPath, sizeof(tempPath), "%c:%s%s", c3files_GetCtpCdId(), FILE_SEP, m_cdPath.c_str());
	m_cdPath = tempPath;
}


MBCHAR *CivPaths::MakeSavePath(MBCHAR *fullPath, MBCHAR *s1, MBCHAR *s2, MBCHAR *s3)
{
	MBCHAR			tempPath[_MAX_PATH];
	MBCHAR			*s;
	int			r;
#ifdef WIN32
	struct _stat		tmpstat;
#else
	struct stat		tmpstat;
#endif

	{
		snprintf(tempPath, sizeof(tempPath), "%s%s%s%s%s", s1, FILE_SEP, s2, FILE_SEP, s3);

		s = _fullpath(fullPath, tempPath, _MAX_PATH);
		Assert(s != nullptr);

#ifdef WIN32
		r = _stat(fullPath, &tmpstat);
#else
		r = stat(fullPath, &tmpstat);
#endif

		if (!r) {
			// NOTE: do NOT convert this strcat to strncat. fullPath is a pointer
			// parameter with no accompanying size, so sizeof(fullPath) would be
			// the pointer size (4/8 bytes), not the buffer size — producing a
			// negative bound that wraps to a huge size_t. Callers guarantee
			// fullPath points to a _MAX_PATH buffer; appending one FILE_SEP
			// after _fullpath() succeeded is within bounds. See batch 49 revert.
			strcat(fullPath, FILE_SEP);
			return fullPath;
		}
		else return nullptr;
	}
}


MBCHAR *CivPaths::GetSavePath(C3SAVEDIR dir, MBCHAR *path)
{
	MBCHAR		fullPath[_MAX_PATH];

	// MakeSavePath takes mutable MBCHAR* params (legacy signature); the
	// strings are read-only inside, so a const_cast is safe here.
	auto cs = [](std::string const &s) { return const_cast<MBCHAR *>(s.c_str()); };

	switch (dir) {
	case C3SAVEDIR_GAME:
		if (MakeSavePath(fullPath, cs(m_hdPath), cs(m_savePath), cs(m_saveGamePath))) {
			// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
			strcpy(path, fullPath);
			return path;
		}
		break;
	case C3SAVEDIR_QUEUES:
		if (MakeSavePath(fullPath, cs(m_hdPath), cs(m_savePath), cs(m_saveQueuePath))) {
			// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
			strcpy(path, fullPath);
			return path;
		}
		break;
	case C3SAVEDIR_MP:
		if (MakeSavePath(fullPath, cs(m_hdPath), cs(m_savePath), cs(m_saveMPPath))) {
			// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
			strcpy(path, fullPath);
			return path;
		}
		break;
	case C3SAVEDIR_SCEN:
		if (MakeSavePath(fullPath, cs(m_hdPath), cs(m_savePath), cs(m_saveSCENPath))) {
			// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
			strcpy(path, fullPath);
			return path;
		}
		break;
	case C3SAVEDIR_MAP:
		if (MakeSavePath(fullPath, cs(m_hdPath), cs(m_savePath), cs(m_saveMapPath))) {
			// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
			strcpy(path, fullPath);
			return path;
		}
		break;
	case C3SAVEDIR_CLIPS:
		if(MakeSavePath(fullPath, cs(m_hdPath), cs(m_savePath), cs(m_saveClipsPath))) {
			// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
			strcpy(path, fullPath);
			return path;
		}
		break;
	default :
		Assert(FALSE);
	}

	return nullptr;
}


MBCHAR *CivPaths::MakeAssetPath
(
    MBCHAR *        fullPath,
    MBCHAR const *  s1,
    MBCHAR const *  s2,
    MBCHAR const *  s3,
    MBCHAR const *  s4,
    MBCHAR const *  s5
) const
{
	MBCHAR			tempPath[_MAX_PATH];
	MBCHAR			*s;
	int			r;
#ifdef WIN32
	struct _stat		tmpstat;
#else
	struct stat		tmpstat;
#endif

	snprintf(tempPath, sizeof(tempPath), "%s%s%s%s%s%s%s%s%s",
	        s1, FILE_SEP, s2, FILE_SEP, s3, FILE_SEP, s4, FILE_SEP, s5);

	s = _fullpath(fullPath, tempPath, _MAX_PATH);
	Assert(s != nullptr);

#ifdef WIN32
	r = _stat(fullPath, &tmpstat);
#else
	r = stat(fullPath, &tmpstat);
#endif

	if (!r) return fullPath;
	else return nullptr;
}




MBCHAR *CivPaths::FindFile(C3DIR dir, const MBCHAR *filename, MBCHAR *path,
                           bool silent, bool check_prjfile, bool checkScenario)
{
	MBCHAR			fullPath[_MAX_PATH];

	Assert(path != nullptr);

	Assert(dir < C3DIR_MAX);

	Assert(filename != nullptr);

	if (dir == C3DIR_DIRECT) {
		// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
		strcpy(path, filename);

		return path;
	}

	if(checkScenario){
		if (!m_curScenarioPath.empty()) {

			snprintf(fullPath, sizeof(fullPath), "%s%s%s%s%s%s%s", m_curScenarioPath.c_str(), FILE_SEP, m_localizedPath.c_str(), FILE_SEP, m_assetPaths[dir].c_str(), FILE_SEP, filename);
			if (c3files_PathIsValid(fullPath)) {

				// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
				strcpy(path, fullPath);
				return path;
			}
			snprintf(fullPath, sizeof(fullPath), "%s%s%s%s%s%s%s", m_curScenarioPath.c_str(), FILE_SEP, m_defaultPath.c_str(), FILE_SEP, m_assetPaths[dir].c_str(), FILE_SEP, filename);

			if (c3files_PathIsValid(fullPath)) {

				// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
				strcpy(path, fullPath);
				return path;
			}
		}

		if (!m_curScenarioPackPath.empty()) {

			snprintf(fullPath, sizeof(fullPath), "%s%s%s%s%s%s%s", m_curScenarioPackPath.c_str(), FILE_SEP, m_localizedPath.c_str(), FILE_SEP, m_assetPaths[dir].c_str(), FILE_SEP, filename);
			if (c3files_PathIsValid(fullPath)) {

				// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
				strcpy(path, fullPath);
				return path;
			}
			snprintf(fullPath, sizeof(fullPath), "%s%s%s%s%s%s%s", m_curScenarioPackPath.c_str(), FILE_SEP, m_defaultPath.c_str(), FILE_SEP, m_assetPaths[dir].c_str(), FILE_SEP, filename);

			if (c3files_PathIsValid(fullPath)) {

				// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
				strcpy(path, fullPath);
				return path;
			}
		}
	}

    // The extra data paths take priority over the regular one.
	for (auto const &extra : m_extraDataPaths)
	{
		if (MakeAssetPath(fullPath, m_hdPath.c_str(), extra.c_str(), m_localizedPath.c_str(), m_assetPaths[dir].c_str(), filename) ||
			MakeAssetPath(fullPath, m_hdPath.c_str(), extra.c_str(), m_defaultPath.c_str(),   m_assetPaths[dir].c_str(), filename)
		   )
		{
			// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
			strcpy(path, fullPath);
			return path;
		}
	}

	// When not found in the new data, try the original directories
	if (MakeAssetPath(fullPath, m_hdPath.c_str(), m_dataPath.c_str(), m_localizedPath.c_str(), m_assetPaths[dir].c_str(), filename)) {

		// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
		strcpy(path, fullPath);
		return path;
	}

	if (MakeAssetPath(fullPath, m_hdPath.c_str(), m_dataPath.c_str(), m_defaultPath.c_str(), m_assetPaths[dir].c_str(), filename)) {

		// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
		strcpy(path, fullPath);
		return path;
	}


	// The CD will only have the original content
	if (MakeAssetPath(fullPath, m_cdPath.c_str(), m_dataPath.c_str(), m_localizedPath.c_str(), m_assetPaths[dir].c_str(), filename)) {

		// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
		strcpy(path, fullPath);
		return path;
	}

	if (MakeAssetPath(fullPath, m_cdPath.c_str(), m_dataPath.c_str(), m_defaultPath.c_str(), m_assetPaths[dir].c_str(), filename)) {

		// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
		strcpy(path, fullPath);
		return path;
	}

    if (check_prjfile &&
        ((dir == C3DIR_PATTERNS) ||
			(dir == C3DIR_PICTURES))) {

		uint32 len = strlen(filename);

        if (len > 3) {

            strlcpy(fullPath, filename, sizeof(fullPath));
            fullPath[len-3] = 'r';
            fullPath[len-2] = 'i';
            fullPath[len-1] = 'm';

            if (g_ImageMapPF && g_ImageMapPF->exists(fullPath)) {

                // TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
                strcpy(path, filename);
                return path;
            }
        }
    }

    if (!silent)
        c3errors_ErrorDialog("Paths", "'%s' not found in asset tree.", filename);

	return nullptr;
}

//----------------------------------------------------------------------------
//
// Name       : CivPaths::FindPath
//
// Description: Get the next possible lookup path
//
// Parameters : dir		type of path to lookup (not really used)
//              num     "index" of path to lookup
//
// Returns    : bool    there may be more paths to lookup
//              path    filled with a found path (set to "" when not found)
//
// Remark(s)  : -
//
//----------------------------------------------------------------------------

bool CivPaths::FindPath(C3DIR dir, int num, MBCHAR * path)
{
	Assert(path != nullptr);
	Assert(dir < C3DIR_MAX);

	path[0] = 0;

	if (dir == C3DIR_DIRECT)
	{
		return false;
	}

	MBCHAR          tempPath[_MAX_PATH];
	tempPath[0] = 0;

	switch (num)
	{
	case 0:
		if (!m_curScenarioPath.empty())
		{
			snprintf(path, _MAX_PATH, "%s%s%s%s%s", m_curScenarioPath.c_str(), FILE_SEP,
				m_localizedPath.c_str(), FILE_SEP, m_assetPaths[dir].c_str());
		}
		break;

	case 1:
		if (!m_curScenarioPath.empty())
		{
			snprintf(path, _MAX_PATH, "%s%s%s%s%s", m_curScenarioPath.c_str(), FILE_SEP,
				m_defaultPath.c_str(), FILE_SEP, m_assetPaths[dir].c_str());
		}
		break;

	case 2:
		if (!m_curScenarioPath.empty())
		{
			snprintf(path, _MAX_PATH, "%s%s%s%s%s", m_curScenarioPackPath.c_str(), FILE_SEP,
				m_localizedPath.c_str(), FILE_SEP, m_assetPaths[dir].c_str());
		}
		break;

	case 3:
		if (!m_curScenarioPath.empty())
		{
			snprintf(path, _MAX_PATH, "%s%s%s%s%s", m_curScenarioPackPath.c_str(), FILE_SEP,
				m_defaultPath.c_str(), FILE_SEP, m_assetPaths[dir].c_str());
		}
		break;

    default:
        {
            size_t const    i   = (num - 4) / 2;

            if (i < m_extraDataPaths.size())
            {
                if (num & 1)    // even: language dependent, odd: default
                {
		            snprintf(tempPath, sizeof(tempPath), "%s%s%s%s%s%s%s", m_hdPath.c_str(), FILE_SEP,
			                m_extraDataPaths[i].c_str(), FILE_SEP, m_defaultPath.c_str(), FILE_SEP, m_assetPaths[dir].c_str());
                }
                else
                {
		            snprintf(tempPath, sizeof(tempPath), "%s%s%s%s%s%s%s", m_hdPath.c_str(), FILE_SEP,
			                m_extraDataPaths[i].c_str(), FILE_SEP, m_localizedPath.c_str(), FILE_SEP, m_assetPaths[dir].c_str());
                }
            }
            else
            {
                switch (num - 2 * m_extraDataPaths.size())
                {
	            case 4:
		            snprintf(tempPath, sizeof(tempPath), "%s%s%s%s%s%s%s", m_hdPath.c_str(), FILE_SEP,
			                m_dataPath.c_str(), FILE_SEP, m_localizedPath.c_str(), FILE_SEP, m_assetPaths[dir].c_str());
                    break;
        	    case 5:
		            snprintf(tempPath, sizeof(tempPath), "%s%s%s%s%s%s%s", m_hdPath.c_str(), FILE_SEP,
			                m_dataPath.c_str(), FILE_SEP, m_defaultPath.c_str(), FILE_SEP, m_assetPaths[dir].c_str());
                    break;
	            case 6:
					if (!m_cdPath.empty())
					{
						snprintf(tempPath, sizeof(tempPath), "%s%s%s%s%s%s%s", m_cdPath.c_str(), FILE_SEP,
			                m_dataPath.c_str(), FILE_SEP, m_localizedPath.c_str(), FILE_SEP, m_assetPaths[dir].c_str());
					}
                    break;
	            case 7:
					if (!m_cdPath.empty())
					{
						snprintf(tempPath, sizeof(tempPath), "%s%s%s%s%s%s%s", m_cdPath.c_str(), FILE_SEP,
			                m_dataPath.c_str(), FILE_SEP, m_defaultPath.c_str(), FILE_SEP, m_assetPaths[dir].c_str());
					}
                    break;

                default:
                    return false;
                } // switch
            }

            if (_fullpath(path, tempPath, _MAX_PATH) == nullptr)
            {
		        path[0] = 0;
            }
        } // scope default
    } // switch

    return true;
}


MBCHAR *CivPaths::GetSpecificPath(C3DIR dir, MBCHAR *path, BOOL local)
{
	Assert(path != nullptr);
	if (path == nullptr) return nullptr;
	Assert(dir < C3DIR_MAX);
	if (dir >= C3DIR_MAX) return nullptr;

	MBCHAR			tempPath[_MAX_PATH];
	if (local)
    {
		snprintf(tempPath, sizeof(tempPath), "%s%s%s%s%s%s%s", m_hdPath.c_str(), FILE_SEP, m_dataPath.c_str(), FILE_SEP, m_localizedPath.c_str(), FILE_SEP, m_assetPaths[dir].c_str());
    }
    else
    {
		snprintf(tempPath, sizeof(tempPath), "%s%s%s%s%s%s%s", m_hdPath.c_str(), FILE_SEP, m_dataPath.c_str(), FILE_SEP, m_defaultPath.c_str(), FILE_SEP, m_assetPaths[dir].c_str());
    }

	MBCHAR          fullPath[_MAX_PATH];
	MBCHAR const *  s = _fullpath(fullPath, tempPath, _MAX_PATH);
	Assert(s);
	if (s)
    {
		// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
		strcpy(path, fullPath);
	}
	return path;
}

MBCHAR *CivPaths::GetScenarioRootPath(MBCHAR *path)
{
	MBCHAR	        temp[_MAX_PATH];
	MBCHAR const *  s  = _fullpath(temp, m_scenariosPath.c_str(), _MAX_PATH);
	Assert(s);
	if (s)
    {
		// TODO(phase-2): strcpy → strlcpy — dst is `char *`, capacity unknown at call site
		strcpy(path, temp);
	}

	return path;
}

void CivPaths::SetCurScenarioPath(const MBCHAR *path)
{
	m_curScenarioPath = path ? path : "";
}

const MBCHAR *CivPaths::GetCurScenarioPath() const
{
	return m_curScenarioPath.empty() ? nullptr : m_curScenarioPath.c_str();
}

void CivPaths::ClearCurScenarioPath()
{
	m_curScenarioPath.clear();
}

void CivPaths::SetCurScenarioPackPath(const MBCHAR *path)
{
	m_curScenarioPackPath = path ? path : "";
}

const MBCHAR *CivPaths::GetCurScenarioPackPath() const
{
	return m_curScenarioPackPath.empty() ? nullptr : m_curScenarioPackPath.c_str();
}

void CivPaths::ClearCurScenarioPackPath()
{
	m_curScenarioPackPath.clear();
}





const MBCHAR *CivPaths::GetDesktopPath()
{
#ifdef WIN32
	MBCHAR		    tempStr[_MAX_PATH] = { 0 };
	ITEMIDLIST *    idList;
	HRESULT		    hr = SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &idList);

    Assert(hr == S_OK);
	if (    (hr != S_OK)
        ||  !SHGetPathFromIDList(idList, tempStr)
       )
    {
    	MBCHAR * s = _fullpath(tempStr, FILE_SEP, _MAX_PATH);
	    Assert(s);
	    if (!s) return NULL;
    }

	strlcpy(m_desktopPath, tempStr, sizeof(m_desktopPath));
	return m_desktopPath;
#else
	return nullptr;
#endif
}

//----------------------------------------------------------------------------
//
// Name       : CivPaths::GetExtraDataPaths
//
// Description: Inspect the data include directory lookup paths
//
// Parameters : -
//
// Globals    : -
//
// Returns    : std::vector<MBCHAR const *> const & : the extra lookup paths
//
// Remark(s)  : -
//
//----------------------------------------------------------------------------
std::vector<std::string> const & CivPaths::GetExtraDataPaths() const
{
    return m_extraDataPaths;
}

//----------------------------------------------------------------------------
//
// Name       : CivPaths::InsertExtraDataPath
//
// Description: Insert a data include directory to the lookup path
//
// Parameters : path	: ctp2_data-style directory tree
//
// Globals    : -
//
// Returns    : -
//
// Remark(s)  : For lookup, the most recent path has the highest priority,
//              and any inserted paths have priority above the original
//              "..\..\ctp2_data" path.
//
//----------------------------------------------------------------------------
void CivPaths::InsertExtraDataPath(MBCHAR const * path)
{
	m_extraDataPaths.insert(m_extraDataPaths.begin(), path ? std::string(path) : std::string());
}

//----------------------------------------------------------------------------
//
// Name       : CivPaths::ResetExtraDataPaths
//
// Description: Clear the entire lookup path
//
// Parameters : -
//
// Globals    : -
//
// Returns    : -
//
// Remark(s)  : The original "..\..\ctp2_data" path will remain - it is not
//              stored in m_extraDataPaths, but in m_dataPath.
//
//----------------------------------------------------------------------------
void CivPaths::ResetExtraDataPaths()
{
    m_extraDataPaths.clear();
}
