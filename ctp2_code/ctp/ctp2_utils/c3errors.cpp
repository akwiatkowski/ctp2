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


void c3errors_FatalDialog(const char* module, const char* fmt, ...)
{
	va_list		list;
	char str[_MAX_PATH];

    va_start(list, fmt);
	vsnprintf(str, sizeof(str), fmt, list);
	str[sizeof(str) - 1] = '\0';
	va_end(list);

	c3errors_ErrorDialog(module, str);

	Assert(FALSE);

	Report("Fatal error.  Aborting.\n");

#ifdef WIN32
#ifndef _DEBUG
#ifndef _BFR_
	sint32 *s = 0;
	*s = 0;
#endif
#endif
#endif

	exit(-1);
}

void c3errors_FatalDialogFromDB(const char *module, const char *err, ...)
{
	MBCHAR *    dbTitle;
	if (!stringdb_Get()->GetText(module, &dbTitle))
		c3errors_FatalDialog("string db", "%s missing from string db", module);

	MBCHAR *    dbError;
	if (!stringdb_Get()->GetText(err, &dbError))
		c3errors_FatalDialog("string db", "%s missing from string db", err) ;

	va_list		list;
	MBCHAR	    str[_MAX_PATH];

	// TODO: I've changed the second argument in the following from dbError (which made no sense)
	//   into err.  I think that this is what was originally intended, but since the feature this
	//   code implements is never actually used anywhere, I expect it makes little difference.
	//   nevertheless, that this works should be checked at some point.  The same applies to the
	//   next function (c3errors_ErrorDialogFromDB) - JJB
	va_start(list, err) ;
	vsnprintf(str, sizeof(str), dbError, list) ;
	str[sizeof(str) - 1] = '\0';
	va_end(list) ;

	MessageBox(nullptr, str, dbTitle, MB_OK | MB_ICONEXCLAMATION);

	Assert(FALSE);

	Report("Fatal error.  Aborting.\n");

#if defined(WIN32)
#ifndef _DEBUG
#ifndef _BFR_
	sint32 *s = 0;
	*s = 0;
#endif
#endif
#endif

	exit(-1) ;
}

void c3errors_ErrorDialogFromDB(const char *module, const char *err, ...)
{
    MBCHAR *    dbTitle;
	if (!stringdb_Get()->GetText(module, &dbTitle))
		c3errors_FatalDialog("string db", "%s missing from string db", module) ;

    MBCHAR *    dbError;
	if (!stringdb_Get()->GetText(err, &dbError))
		c3errors_FatalDialog("string db", "%s missing from string db", err) ;

	va_list		list;
	MBCHAR	    str[_MAX_PATH];
	va_start(list, err);
	vsnprintf(str, sizeof(str), dbError, list);
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

#if defined(WIN32)
	if ((szTitle = (LPTSTR)LocalAlloc(LMEM_FIXED, (lstrlen(szTmp) +
			lstrlen(szTitleText) + lstrlen(fmt) + 33000)*sizeof(TCHAR))) == NULL)
		return;

   wsprintf(szTitle, szTitleText, szTmp);
#else
   if ((szTitle = (LPTSTR)malloc((lstrlen(szTmp) + lstrlen(szTitleText) +
                                  lstrlen(fmt) + 33000
                                 )*sizeof(TCHAR)
                                )
       ) == nullptr)
      return;

   sprintf(szTitle, szTitleText, szTmp);
#endif

	LPTSTR  szFmtTmp    = szTitle + lstrlen(szTitle) + 2;

	va_list list;
	va_start(list, fmt);
	vsprintf(szFmtTmp, fmt, list);
	char Tmp[2000];
	snprintf(Tmp, sizeof(Tmp), "%s\n\nContinue?", szFmtTmp);
	va_end(list);

	DPRINTF(k_DBG_FIX, ("Error: %s, %s\n", szTitle, szFmtTmp));

	// TODO: Make it work with LPTSTR szFmtTmp if it is worth the efforts at all.
//	MessageBox(NULL, szFmtTmp, szTitle, MB_OK | MB_ICONEXCLAMATION);
	sint32 result = MessageBox(nullptr, Tmp, szTitle, MB_YESNO | MB_ICONEXCLAMATION);

#if defined(WIN32)
	LocalFree(szTitle);
#else
   free(szTitle);
#endif

#ifndef _DEBUG
	extern bool g_autoAltTab;
	if(g_autoAltTab && aui_ui_Get()) {
		aui_ui_Get()->AltTabIn();
	}
#endif

	if (result == IDNO) {
		exit(1);
	}

	return;
}
