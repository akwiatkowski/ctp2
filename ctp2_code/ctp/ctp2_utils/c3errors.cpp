//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source file
// Description  : Civilisation 3 error dialogs
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
// _DEBUG
// - Generate debug version
//
// _BFR_
// - Force CD checking when set (build final release).
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Added to the database error dialog the possibility to close the program.
//   This dialog is also used for slic errors and therefore also very useful.
//   (Aug 26th 2005 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "ctp/ctp2_utils/c3errors.h"

#include "ui/aui_common/aui_ui.h"
#include "gs/database/StrDB.h"      // stringdb_Get()

#include <cstdlib>
#include <vector>


void c3errors_FatalDialog(const char* module, const char* fmt, ...)
{
	va_list		list;
	char str[_MAX_PATH];

    va_start(list, fmt);
	vsnprintf(str, sizeof(str), fmt, list);
	str[sizeof(str) - 1] = '\0';
	va_end(list);

	// "%s": str is an already-formatted message; passing it as the format
	// would re-interpret any % in the caller's arguments.
	c3errors_ErrorDialog(module, "%s", str);

	Assert(FALSE);

	Report("Fatal error.  Aborting.\n");

#ifdef WIN32
#ifndef _DEBUG
#ifndef _BFR_
	abort();
#endif
#endif
#endif

	exit(-1);
}

void c3errors_FatalDialogFromDB(const char *module, const char *err, ...)
{
	// Titles are plain text (no format processing) — direct lookups are fine.
	MBCHAR *    dbTitle;
	if (!stringdb_Get()->GetText(module, &dbTitle))
		dbTitle = const_cast<MBCHAR *>((const MBCHAR *)module);

	// The error BODY is a printf-style template from the db: GetTextOr's
	// format_arg attribute lets the template flow into vsnprintf verbatim,
	// with the call-site literal below as the missing-key fallback.
	va_list		list;
	MBCHAR	    str[_MAX_PATH];

	va_start(list, err) ;
	vsnprintf(str, sizeof(str),
		stringdb_Get()->GetTextOr(err, "%s"), list) ;
	str[sizeof(str) - 1] = '\0';
	va_end(list) ;

	MessageBox(nullptr, str, dbTitle, MB_OK | MB_ICONEXCLAMATION);

	Assert(FALSE);

	Report("Fatal error.  Aborting.\n");

#if defined(WIN32)
#ifndef _DEBUG
#ifndef _BFR_
	abort();
#endif
#endif
#endif

	exit(-1) ;
}

void c3errors_ErrorDialogFromDB(const char *module, const char *err, ...)
{
    // Titles are plain text (no format processing) — direct lookups are fine.
    MBCHAR *    dbTitle;
	if (!stringdb_Get()->GetText(module, &dbTitle))
		dbTitle = const_cast<MBCHAR *>((const MBCHAR *)module);

	// The error BODY is a printf-style template from the db: GetTextOr's
	// format_arg attribute lets it flow into vsnprintf verbatim, with the
	// call-site literal as the missing-key fallback.
	va_list		list;
	MBCHAR	    str[_MAX_PATH];
	va_start(list, err);
	vsnprintf(str, sizeof(str),
		stringdb_Get()->GetTextOr(err, "%s"), list);
	str[sizeof(str) - 1] = '\0';
	va_end(list);

	MessageBox(nullptr, str, dbTitle, MB_OK | MB_ICONEXCLAMATION) ;
}

extern BOOL g_smokeTest;
#include "gs/utility/Globals.h"   // is_headless()

void c3errors_ErrorDialog(const char* module, const char* fmt, ...)
{
	// In smoke test or headless mode, skip modal dialogs and just log the error
	if (g_smokeTest || is_headless()) {
		va_list list;
		va_start(list, fmt);
		char buf[1024];
		vsnprintf(buf, sizeof(buf), fmt, list);
		va_end(list);
		fprintf(stderr, "[SMOKE-ERROR] %s: %s\n", module ? module : "CTP 2", buf);
		return;
	}

	LPTSTR			szTitle;
	LPCTSTR			szTitleText = "%s Error";

    LPCTSTR  szTmp = (module) ? (LPCTSTR) module : (LPCTSTR) "CTP 2";

	size_t const titleChars = lstrlen(szTmp) + lstrlen(szTitleText) +
	                          lstrlen(fmt) + 33000;
#if defined(WIN32)
	// vector<TCHAR> replaces LocalAlloc/LocalFree — freed on every path
	std::vector<TCHAR> szTitleBuf(titleChars);
	szTitle = szTitleBuf.data();

   wsprintf(szTitle, szTitleText, szTmp);
#else
   std::vector<char> szTitleBuf(titleChars);
   szTitle = szTitleBuf.data();

   // (szTitleText inlined: the local hid the literal from -Wformat checking;
   // "%s Error" is the only value it ever holds.)
   snprintf(szTitle, titleChars, "%s Error", szTmp);
#endif

	LPTSTR  szFmtTmp    = szTitle + lstrlen(szTitle) + 2;

	va_list list;
	va_start(list, fmt);
	vsnprintf(szFmtTmp, titleChars - (lstrlen(szTitle) + 2), fmt, list);
	char Tmp[2000];
	snprintf(Tmp, sizeof(Tmp), "%s\n\nContinue?", szFmtTmp);
	va_end(list);

	DPRINTF(k_DBG_FIX, ("Error: %s, %s\n", szTitle, szFmtTmp));

	// TODO: Make it work with LPTSTR szFmtTmp if it is worth the efforts at all.
//	MessageBox(NULL, szFmtTmp, szTitle, MB_OK | MB_ICONEXCLAMATION);
	sint32 result = MessageBox(nullptr, Tmp, szTitle, MB_YESNO | MB_ICONEXCLAMATION);

	// szTitleBuf frees itself on both branches

#ifndef _DEBUG
	extern bool g_autoAltTab;
	if(g_autoAltTab && aui_ui_Get()) {
		aui_ui_Get()->AltTabIn();
	}
#endif

	if (result == IDNO) {
		exit(1);
	}

	}
