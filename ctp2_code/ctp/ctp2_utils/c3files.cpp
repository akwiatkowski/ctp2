//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  :
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
// WIN32
// - Generates windows specific code.
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Added some casts. (Aug 7th 2005 Martin G�hmann)
// - Removed unused local variables. (Sep 9th 2005 Martin G�hmann)
// - Improved CTP2 disk detection.
// - c3files_fopen can now ignore scenario paths. (9-Apr-2007 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ctp/ctp2_utils/c3files.h"

#include "ctp/ctp2_utils/appstrings.h"
#include "ctp/civ3_main.h"
#include "gs/fileio/CivPaths.h"
#include "ctp/ctp2_utils/c3errors.h"
#include "ui/aui_ctp2/c3ui.h"
#include "ctp/ctp2_utils/pointerlist.h"
#include "gs/database/profileDB.h"
#include "sound/soundmanager.h"
#include <memory>


#ifdef HAVE_STRING_H
#include <cstring>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef LINUX
#include <linux/fs.h>
#include <linux/iso_fs.h>
#include <errno.h>
#include <dirent.h>
#endif


FILE* c3files_fopen(C3DIR dirID, MBCHAR const * s1, MBCHAR const * s2, bool checkScenario)
{
	MBCHAR  s[_MAX_PATH];

	// C3DIR_DIRECT does not consult the CivPaths search list (FindFile just
	// copies the name through), so it stays usable in test harnesses that
	// never initialise g_civPaths — guard the deref instead of crashing.
	if (civpaths_Get() == nullptr) {
		if (dirID != C3DIR_DIRECT)
			return nullptr;
		return fopen(s1, s2);
	}

	return civpaths_Get()->FindFile(dirID, s1, s, false, true, checkScenario) ? fopen(s, s2) : nullptr;
}

FILE* c3files_freopen(const MBCHAR *s1, const MBCHAR *s2, FILE *file)
{
	return freopen(s1, s2, file);
}

sint32 c3files_fclose(FILE *file)
{
	return (sint32)fclose(file);
}

sint32 c3files_fscanf(FILE *file, const MBCHAR *s, ...)
{
	va_list		valist;
	sint32		val;

	va_start(valist, s);
	val = (sint32)fscanf(file, s, valist);
	va_end(valist);

	return val;
}

size_t c3files_fread(void *p, size_t i1, size_t i2, FILE *file)
{
	return fread(p, i1, i2, file);
}

sint32 c3files_fgetc(FILE *file)
{
	return (sint32)fgetc(file);
}

sint32 c3files_fgetpos(FILE *file, fpos_t *pos)
{
	return (sint32)fgetpos(file, pos);
}

MBCHAR* c3files_fgets(MBCHAR *s, sint32 i, FILE *file)
{
	return (MBCHAR *)fgets(s, i, file);
}


size_t c3files_fwrite(const void *p, size_t i1, size_t i2, FILE *file)
{
	return fwrite(p, i1, i2, file);
}


sint32 c3files_fprintf(FILE *file, const MBCHAR *s, ...)
{
	va_list     valist;
	sint32      val;

	va_start(valist, s);
	val = (sint32)vfprintf(file, s, valist);
	va_end(valist);

	return val;
}


sint32 c3files_fputc(sint32 i, FILE *file)
{
	return (sint32)fputc(i, file);
}


sint32 c3files_fputs(const MBCHAR *s, FILE *file)
{
	return (sint32)fputs(s, file);
}


sint32 c3files_fsetpos(FILE *file, const fpos_t *pos)
{
	return (sint32)fsetpos(file, pos);
}


sint32 c3files_fseek(FILE *file, sint32 i1, sint32 i2)
{
	return (sint32)fseek(file, i1, i2);
}


sint32 c3files_ftell(FILE *file)
{
	return (sint32)ftell(file);
}


sint32 c3files_feof(FILE *file)
{
	return (sint32)feof(file);
}


sint32 c3files_ferror(FILE *file)
{
	return (sint32)ferror(file);
}


void c3files_clearerr(FILE *file)
{
	clearerr(file);
}


sint32 c3files_fflush(FILE *file)
{
	return (sint32)fflush(file);
}


sint32 c3files_getfilesize(C3DIR dir, MBCHAR const *filename)
{
	sint32 filesize = 0;

	FILE *f = c3files_fopen(dir, filename, "rb");

	if ( !f )
		return -1;

	if (c3files_fseek(f, 0, SEEK_END) == 0) {
		filesize = c3files_ftell(f);
	} else {
		c3files_fclose(f);
		return -1;
	}

	c3files_fclose(f);

	return filesize;
}


uint8 *c3files_loadbinaryfile(C3DIR dir, MBCHAR const * filename, sint32 *size)
{
	if ( size ) *size = 0;

	uint32 filesize;

	FILE *f = c3files_fopen(dir, filename, "rb");

	if ( !f )
		return nullptr;

	if (c3files_fseek(f, 0, SEEK_END) == 0) {
		filesize = c3files_ftell(f);
	} else {
		fclose(f);
		return nullptr;
	}

	if (c3files_fseek(f, 0, SEEK_SET) != 0) {
		fclose(f);
		return nullptr;
	}

	// unique_ptr guards the early-return path; released into the raw
	// return value on success (callers still delete[] the buffer).
	std::unique_ptr<uint8[]> bits = std::make_unique<uint8[]>(filesize);

	Assert(bits != nullptr);
	if (!bits) {
		c3files_fclose(f);
		return nullptr;
	}

	if (c3files_fread( bits.get(), 1, filesize, f ) != filesize) {
		c3files_fclose(f);
		return nullptr;
	}

	c3files_fclose(f);

	if ( size ) *size = filesize;

	return bits.release();
}

bool c3files_PathIsValid(MBCHAR *path)
{
#if defined(_WIN32)
	struct _stat tmpstat;
	return !_stat(path, &tmpstat);
#else
	struct stat  tmpstat;
	return !stat(path, &tmpstat);
#endif
}

bool c3files_CreateDirectory(MBCHAR *path)
{
#if defined(_WIN32)
	return CreateDirectory(path, NULL) != FALSE; // BOOL to bool conversion
#else
	mode_t mask = 0777;
	return mkdir(path, mask) == 0;
#endif
}

void c3files_StripSpaces(MBCHAR * s)
{
    size_t  len     = strlen(s);
    size_t  copied  = 0;

    for (size_t i = 0; i < len; ++i)
    {
        if (s[i] == ' ')
        {
            // No action: skip this one
        }
        else
        {
            s[copied++] = s[i];
        }
    }

    if (0 == copied)
    {
        s[0] = '-';
        s[1] = '\0';
    }
    else
    {
        s[copied] = 0;
    }
}




bool c3files_getfilelist(C3SAVEDIR dirID, MBCHAR *ext, PointerList<MBCHAR> *list)
{
#ifdef _WIN32
	MBCHAR strbuf[256];
#endif
	MBCHAR path[_MAX_PATH];

	civpaths_Get()->GetSavePath(dirID, path);

#ifdef _WIN32
	if (ext) snprintf(strbuf, sizeof(strbuf), "*.%s", ext);
	else { strlcpy(strbuf, "*.*", sizeof(strbuf)); }

	strncat(path, strbuf, sizeof(path) - strlen(path) - 1);

	WIN32_FIND_DATA	fileData;
	HANDLE          lpFileList = FindFirstFile(path, &fileData);

	if (lpFileList ==  INVALID_HANDLE_VALUE) return false;

	std::unique_ptr<MBCHAR[]> lpFileName = std::make_unique<MBCHAR[]>(256);
	strlcpy(lpFileName.get(), fileData.cFileName, 256);
	list->AddTail(lpFileName.release());

	while (FindNextFile(lpFileList,&fileData))
	{
		lpFileName = std::make_unique<MBCHAR[]>(256);
		strlcpy(lpFileName.get(), fileData.cFileName, 256);
		list->AddTail(lpFileName.release());
	}

	FindClose(lpFileList);
#elif defined(LINUX)
        DIR *dir = opendir(path);
	if (!dir)
		return FALSE;
	struct dirent *dent = NULL;

	while ((dent = readdir(dir)))
	{
                char *p = strrchr(dent->d_name, '.');
		if (NULL == p) {
			continue;
		}
		if (1 == strlen(p)) {
			continue;
		}
                p++;
                if (ext != NULL && 0 != strcasecmp(p, ext)) {
			continue;
		}
		std::unique_ptr<MBCHAR[]> lpFileName = std::make_unique<MBCHAR[]>(NAME_MAX + 1);
		strlcpy(lpFileName.get(), dent->d_name, NAME_MAX + 1);
		list->AddTail(lpFileName.release());
	}

	closedir(dir);
#endif

	return true;
}

#ifdef _WIN32
bool c3files_getfilelist_ex(C3SAVEDIR dirID, MBCHAR *ext, PointerList<WIN32_FIND_DATA> *list)
{
	MBCHAR strbuf[256];
	MBCHAR path[_MAX_PATH];

	civpaths_Get()->GetSavePath(dirID, path);

	if (ext) snprintf(strbuf, sizeof(strbuf), "*.%s", ext);
	else { strlcpy(strbuf, "*.*", sizeof(strbuf)); }

	strncat(path, strbuf, sizeof(path) - strlen(path) - 1);

	// unique_ptr owns the scratch record until it is released into the
	// owning PointerList; the trailing allocation is freed automatically.
	std::unique_ptr<WIN32_FIND_DATA> lpFileData = std::make_unique<WIN32_FIND_DATA>();
	HANDLE              lpFileList  = FindFirstFile(path, lpFileData.get());

	if (lpFileList == INVALID_HANDLE_VALUE)
    {
        return false;
    }

	list->AddTail(lpFileData.release());

	lpFileData = std::make_unique<WIN32_FIND_DATA>();
	while (FindNextFile(lpFileList,lpFileData.get()))
	{
		list->AddTail(lpFileData.release());
		lpFileData = std::make_unique<WIN32_FIND_DATA>();
	}

	FindClose(lpFileList);

	return true;
}
#endif // _WIN32

bool c3files_HasLegalCD()
{
	return true;
}
