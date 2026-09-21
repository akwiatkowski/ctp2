//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  :
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
// Note: For the blocks with _MSC_VER preprocessor directives, the following
//       is implied: the (_MSC_VER) preprocessor directive lines, and the blocks
//       that are inactive for _MSC_VER value 1200 are modified Apolyton code.
//       The blocks that are inactiThe blocks that are active for _MSC_VER value
//       1200 are the original Activision code.
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Reimplemented containers as vectors, to make it less error prone.
// - Load default strings if they are missing in the database so that mods
//   also have a full set of strings. (Jan 30th 2006 Martin G�hmann)
//
//----------------------------------------------------------------------------

#if defined(HAVE_PRAGMA_ONCE)
#pragma once
#endif

#ifndef __STRING_DB_H__
#define __STRING_DB_H__ 1

//----------------------------------------------------------------------------
// Library dependencies
//----------------------------------------------------------------------------

#include <deque>        // std::deque
#include <vector>       // std::vector

//----------------------------------------------------------------------------
// Export overview
//----------------------------------------------------------------------------

class	StringDB;

// App-singleton accessor pair.  Storage is file-static in
// gs/utility/gameinit.cpp; outside callers must go through these.
StringDB * stringdb_Get();
void       stringdb_Set(StringDB *p);

// i18n format lookup with hardcoded fallback: returns the string-db template
// for `key`, or `fallback` when the db has no such entry (missing translation,
// early boot). format_arg(2) marks the RESULT as a printf format string so
// callers can pass it directly to snprintf-family calls without
// -Wformat-nonliteral (maximum-hardening builds) — while the compiler still
// checks the call's arguments against the fallback literal at each site.
// Mirrors the gettext _() idiom; the fallback literal must be defined inline
// at the call site for the attribute to apply.
MBCHAR const * stringdb_FormatOr(MBCHAR const * key, MBCHAR const * fallback) __attribute__((format_arg(2)));

//----------------------------------------------------------------------------
// Project dependencies
//----------------------------------------------------------------------------

#include "ctp/c3types.h"	// MBCHAR
#include "gs/database/dbtypes.h"	// StringId
#include "gs/database/StrRec.h"		// StringRecord
#include "gs/fileio/Token.h"		// Token

//----------------------------------------------------------------------------
// Class declarations
//----------------------------------------------------------------------------

class StringDB
{
public:
	StringDB();
    virtual ~StringDB();

	// Modifiers
	bool					InsertStr
	(
		MBCHAR const *			add_id,
		MBCHAR const *			new_text
	);
	bool					Parse(MBCHAR * filename);

	// Accessors
	MBCHAR *				GetIdStr
	(
		StringId const &		index
	) const;
	// NOTE: GetNameStr results are printf-style i18n templates, but the KEY
	// argument is not a literal format — so no format_arg here (it would
	// check the key, not the template). Callers that feed the result into
	// snprintf-family functions must use stringdb_FormatOr() below, whose
	// inline fallback literal is what the compiler checks.
	MBCHAR const *			GetNameStr(StringId const & n) const;
	MBCHAR const *			GetNameStr(MBCHAR const * s) const;
	bool					GetStringID
	(
		MBCHAR const *			str_id,
		StringId &				index
	) const;
	bool					GetText
	(
		MBCHAR const *			get_id,
		MBCHAR **				new_text
	) const;
	// Localized-text lookup with inline hardcoded fallback, for printf-style
	// use: format_arg(3) (member fn: this=1, get_id=2, fallback=3) marks the
	// RESULT as a format string so it can be passed straight to
	// vsnprintf/snprintf without -Wformat-nonliteral. Fallback must be a
	// call-site literal for the attribute to apply.
	MBCHAR const *			GetTextOr
	(
		MBCHAR const *			get_id,
		MBCHAR const *			fallback
	) const __attribute__((format_arg(3)));

	void Export(MBCHAR * file);

private:
	// Owning store for all records; deque keeps pointers stable across
	// insertions, so m_head/m_all can stay non-owning raw pointers.
	std::deque<StringRecord>	m_records;
	std::vector<StringRecord *>	m_all;	// a flattened list version of m_head
	std::vector<StringRecord *> m_head;
		// hash vector of B-trees of lexicographically ordered strings

	bool					AddStrNode
	(
		StringRecord * &		ptr,
		MBCHAR const *			add_id,
		MBCHAR const *			new_text,
		StringRecord * &		newPtr
	);
	void					AssignIndex(StringRecord * & ptr);
	void					Btree2Array();
	StringRecord * &		GetHead(MBCHAR const * id);
	StringRecord const * const &
							GetHead(MBCHAR const * id) const;
	bool					GetIndexNode
	(
		StringRecord const *	ptr,
		MBCHAR const *			str_id,
		StringId &				index
	) const;
	bool					GetStrNode
	(
		StringRecord const *	ptr,
		MBCHAR const *			add_id,
		MBCHAR **				new_text
	) const;

	bool					ParseAStringEntry            (Token *strToken);
	bool					ParseAStringEntryNoDuplicates(Token *strToken);
};

#endif	// Multiple include guard
