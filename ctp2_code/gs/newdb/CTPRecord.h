//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : Database Record template class
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
// HAVE_PRAGMA_ONCE
// HAVE_STATIC_CONST_INIT_DECL_BUG
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - None
//
//----------------------------------------------------------------------------

#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __CTP_RECORD_H__
#define __CTP_RECORD_H__

class CTPRecord;

#include <vector>
#include "os/include/ctp2_inttypes.h"      // sint32
#include "gs/database/dbtypes.h"            // StringId
class DBLexer;

class CTPRecord
{
protected:
	sint32 m_index;
	std::string m_textName;

public:
	StringId m_name;

#if defined(HAVE_STATIC_CONST_INIT_DECL_BUG)
    enum { INDEX_INVALID = -1 }; // Compiler bug workaround
#else
    static sint32 const INDEX_INVALID   = -1;
#endif

	CTPRecord()
    :
        m_index     (INDEX_INVALID),
        m_name      (INDEX_INVALID) // StringID is an integer
    { };
	virtual ~CTPRecord() = default;

	sint32 GetIndex() const { return m_index; }
	void SetIndex(sint32 index) { m_index = index; }

	StringId GetName() const { return m_name; }
	const char *GetIDText() const;
	virtual const char *GetNameText() const;
	void SetTextName(const char *text);

	// Vector-based API — preferred for dynamically-grown arrays.
	// Caller passes a vector; we append parsed values.  No manual new[]/delete[].
	bool ParseIntInArray(DBLexer *lex, std::vector<sint32> &array);
	bool ParseFloatInArray(DBLexer *lex, std::vector<double> &array);
	bool ParseFileInArray(DBLexer *lex, std::vector<char *> &array);
	bool ParseStringIdInArray(DBLexer *lex, std::vector<sint32> &array);

	// Legacy T** + count API — thin wrappers over the vector form.
	// Kept for generated record code that hasn't migrated yet.
	bool ParseIntInArray(DBLexer *lex, sint32 **array, sint32 *numElements);
	bool ParseFloatInArray(DBLexer *lex, double **array, sint32 *numElements);
	bool ParseFileInArray(DBLexer *lex, char ***array, sint32 *numElements);
	bool ParseStringIdInArray(DBLexer *lex, sint32 **array, sint32 *numElements);

	bool ParseIntInArray(DBLexer *lex, sint32 *array, sint32 *numElements, sint32 maxSize);
	bool ParseFloatInArray(DBLexer *lex, double *array, sint32 *numElements, sint32 maxSize);
	bool ParseFileInArray(DBLexer *lex, char **array, sint32 *numElements, sint32 maxSize);
	bool ParseStringIdInArray(DBLexer *lex, sint32 *array, sint32 *numElements, sint32 maxSize);
};

#endif
