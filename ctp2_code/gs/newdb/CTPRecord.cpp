//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
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
// - None
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Arrays can now be parsed sequentially, if their values are in sequence
//   without being separated of any other tokens. (Sep 3rd 2005 Martin G�hmann)
// - Repaired memory leaks
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "CTPRecord.h"

#include "gs/newdb/CTPDatabase.h"
#include "gs/newdb/DBLexer.h"
#include "gs/newdb/DBTokens.h"
#include "gs/database/StrDB.h"

#include <vector>

sint32 const CTPRecord::INDEX_INVALID;

// Vector-based primary form.
bool CTPRecord::ParseIntInArray(DBLexer *lex, std::vector<sint32> &array)
{
	if(lex->PeekAhead() != k_Token_Int) {
		DBERROR(("Expected integer"));
		return false;
	}
	do {
		lex->GetToken();
		array.push_back(atoi(lex->GetTokenText()));
	} while(lex->PeekAhead() == k_Token_Int);
	return true;
}

// Legacy T**+count adapter — copies into a vector, delegates, copies out.
// Generated record code still uses this form; safe internally via the vector.
bool CTPRecord::ParseIntInArray(DBLexer *lex, sint32 **array, sint32 *numElements)
{
	std::vector<sint32> tmp(*array, *array + *numElements);
	if(!ParseIntInArray(lex, tmp)) return false;
	delete [] *array;
	*array = new sint32[tmp.size()];
	std::copy(tmp.begin(), tmp.end(), *array);
	*numElements = static_cast<sint32>(tmp.size());
	return true;
}

bool CTPRecord::ParseFloatInArray(DBLexer *lex, std::vector<double> &array)
{
	if(lex->PeekAhead() != k_Token_Int && lex->PeekAhead() != k_Token_Float) {
		DBERROR(("Expected number"));
		return false;
	}
	do {
		lex->GetToken();
		array.push_back(atof(lex->GetTokenText()));
	} while(lex->PeekAhead() == k_Token_Int || lex->PeekAhead() == k_Token_Float);
	return true;
}

bool CTPRecord::ParseFloatInArray(DBLexer *lex, double **array, sint32 *numElements)
{
	std::vector<double> tmp(*array, *array + *numElements);
	if(!ParseFloatInArray(lex, tmp)) return false;
	delete [] *array;
	*array = new double[tmp.size()];
	std::copy(tmp.begin(), tmp.end(), *array);
	*numElements = static_cast<sint32>(tmp.size());
	return true;
}

bool CTPRecord::ParseFileInArray(DBLexer *lex, std::vector<char *> &array)
{
	if(lex->PeekAhead() != k_Token_String) {
		DBERROR(("Expected filename"));
		return false;
	}
	do {
		lex->GetToken();
		const char *value = lex->GetTokenText();
		char *owned = new char[strlen(value) + 1];
		strcpy(owned, value);
		array.push_back(owned);
	} while(lex->PeekAhead() == k_Token_String);
	return true;
}

bool CTPRecord::ParseFileInArray(DBLexer *lex, char ***array, sint32 *numElements)
{
	std::vector<char *> tmp(*array, *array + *numElements);
	if(!ParseFileInArray(lex, tmp)) return false;
	delete [] *array;
	*array = new char *[tmp.size()];
	std::copy(tmp.begin(), tmp.end(), *array);
	*numElements = static_cast<sint32>(tmp.size());
	return true;
}

bool CTPRecord::ParseStringIdInArray(DBLexer *lex, std::vector<sint32> &array)
{
	if(lex->PeekAhead() != k_Token_Name) {
		DBERROR(("Expected stringid"));
		return false;
	}
	do {
		lex->GetToken();
		const char *value = lex->GetTokenText();
		sint32 id;
		if(!stringdb_Get()->GetStringID(value, id)) {
			DBERROR(("%s not in string database", value));
			return false;
		}
		array.push_back(id);
	} while(lex->PeekAhead() == k_Token_Name);
	return true;
}

bool CTPRecord::ParseStringIdInArray(DBLexer *lex, sint32 **array, sint32 *numElements)
{
	std::vector<sint32> tmp(*array, *array + *numElements);
	if(!ParseStringIdInArray(lex, tmp)) return false;
	delete [] *array;
	*array = new sint32[tmp.size()];
	std::copy(tmp.begin(), tmp.end(), *array);
	*numElements = static_cast<sint32>(tmp.size());
	return true;
}

bool CTPRecord::ParseIntInArray(DBLexer *lex, sint32 *array, sint32 *numElements, sint32 maxSize)
{
	if(lex->PeekAhead() != k_Token_Int) {
		DBERROR(("Expected integer"));
		return false;
	}

	do{
		lex->GetToken();
		sint32 value = atoi(lex->GetTokenText());
		if(*numElements >= maxSize) {
			DBERROR(("Too many entries"));
			return false;
		}

		array[*numElements] = value;
		*numElements += 1;
	}while(lex->PeekAhead() == k_Token_Int);
	return true;
}

bool CTPRecord::ParseFloatInArray(DBLexer *lex, double *array, sint32 *numElements, sint32 maxSize)
{
	if(lex->PeekAhead() != k_Token_Int && lex->PeekAhead() != k_Token_Float) {
		DBERROR(("Expected number"));
		return false;
	}

	do{
		lex->GetToken();
		double value = atof(lex->GetTokenText());
		if(*numElements >= maxSize) {
			DBERROR(("Too many entries"));
			return false;
		}

		array[*numElements] = value;
		*numElements += 1;
	}while(lex->PeekAhead() == k_Token_Int || lex->PeekAhead() == k_Token_Float);
	return true;
}

bool CTPRecord::ParseFileInArray(DBLexer *lex, char **array, sint32 *numElements, sint32 maxSize)
{
	if(lex->PeekAhead() != k_Token_String) {
		DBERROR(("Expected quoted string"));
		return false;
	}

	do{
		lex->GetToken();
		const char * value = lex->GetTokenText();

		if(*numElements >= maxSize) {
			DBERROR(("Too many entries"));
			return false;
		}
		// TODO(phase-2): ownership transfer out of function — needs separate strategy
		array[*numElements] = new char[strlen(value) + 1];
		strcpy(array[*numElements], value);
		*numElements += 1;
	}while(lex->PeekAhead() == k_Token_String);
	return true;
}

bool CTPRecord::ParseStringIdInArray(DBLexer *lex, sint32 *array, sint32 *numElements, sint32 maxSize)
{
	if(lex->PeekAhead() != k_Token_Name) {
		DBERROR(("Expected string id"));
		return false;
	}

	do{
		sint32 tok = lex->GetToken();
		const char * value = lex->GetTokenText();

		if(*numElements >= maxSize) {
			DBERROR(("Too many entries"));
			return false;
		}

		sint32 id;
		if(!stringdb_Get()->GetStringID(value, id)) {
			DBERROR(("%s not in string database. Token: %i, TokenName: %i, Next: %i", value, tok, k_Token_Name, lex->PeekAhead()));
			return false;
		}
		array[*numElements] = id;
		*numElements += 1;
	}while(lex->PeekAhead() == k_Token_Name);
	return true;
}

void CTPRecord::SetTextName(const char *text)
{
	Assert(text);
	if (text)
    {
	    m_name      = INDEX_INVALID;
	    m_textName  = text;
    }
}

const char *CTPRecord::GetIDText() const
{
	Assert(m_name >= 0);
    return (m_name >= 0) ? stringdb_Get()->GetIdStr(m_name) : "NO_ID";
}

const char *CTPRecord::GetNameText() const
{
    return (!m_textName.empty()) ? m_textName.c_str() : stringdb_Get()->GetNameStr(m_name);
}
