//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Base DB Template class
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
// __TILETOOL__
// - Probably supposed to generate the tool for creating the *.til files.
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Implemented GovernmentsModified subclass (allowing cdb files including
//   a GovernmentsModified record to produce parsers capable of reading and
//   storing subrecords for Government types.)
//   See http://apolyton.net/forums/showthread.php?s=&threadid=107916 for
//   more details  _____ by MrBaggins Jan-04
//
//   * Added m_modifiedList member variable (array of pointers to PointerList
//     of a new class GovernmentModifiedRecordNode, which has (only) 2 member
//     variables, to store which government goes with which subrecord.  The
//     array subscript is synonymous with the actual record (identical
//     m_record index.)  Please note that its a pointer to an array, not a
//     direct array.
//
//   * Added m_modifiedRecords member variable (array of pointers to template
//     class.)  This is the container for the subrecords.  The Add member
//     function determines if the templated class has
//     m_hasGovernmentsModified set to yes, (via the Generic accessor created
//     via ctpdb.exe, and ultimately from RecordDescription.cpp,) and if the
//     parsed record contains a number (more than 0) of valid
//     governments in its GovernmentsModified properties.  If so it adds
//     the templated class into the m_modifiedRecords member variable, rather
//     than m_records.  It also creates a list of links, by inserting each
//     government index, and the m_modifiedRecords index at the head of the
//     m_modifiedList pointer list (at the subscript of the appropriate
//     m_records.)
//
//   * m_numModifiedRecords, and m_allocatedModifiedSize are auxilary member
//     variables to aid in insertion and dynamic growth (of m_modifiedRecords)
//
//   * Included memory destructors and dynamic growth code for new member
//     variables
//
//   * Added overloaded public accessor member functions, which take the
//     subscript of the m_records requested, and the government to search for
//     if a government is found in the pointer list, then the applicable
//     m_modifiedRecords object is returned, otherwise the regular
//     m_records object is returned (as per normal.)
//
// - Repaired memory leaks.
// - Removed some completely unused code.
// - Modernised some code: e.g. implemented the modified records list as a
//   std::vector, so we don't have to do the memory management ourselves.
// - Prevented crash in Parse when m_numrecords is 0.
// - Added the new civilisation database. (Aug 20th 2005 Martin G�hmann)
// - Added Serialize method for datachecks. (Aug 23rd 2005 Martin G�hmann)
// - Records can now be also parsed as quoted string. (Aug 26th 2005 Martin G�hmann)
// - The new databases can now be ordered alphabethical like the old ones. (Aug 26th 2005 Martin G�hmann)
// - Added the new risk database. (Aug 29th 2005 Martin G�hmann)
// - Parser for struct ADVANCE_CHANCES of DiffDB.txt can now be generated. (Jan 3rd 2006 Martin G�hmann)
// - If database records have no name a default name is generated. e.g.
//   DIFFICULTY_5 for the sixth entry in the DifficultyDB. (Jan 3rd 2006 Martin G�hman)
// - Added new pollution database. (July 15th 2006 Martin G�hmann)
// - Added new global warming database. (July 15th 2006 Martin G�hmann)
// - Added new map icon database. (3-Mar-2007 Martin G�hmann)
// - Added new map database. (27-Mar-2007 Martin G�hmann)
// - Added new concept database. (31-Mar-2007 Martin G�hmann)
// - Added new const database. (5-Aug-2007 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/newdb/CTPDatabase.h"
#include "gs/newdb/DBLexer.h"
#include "gs/newdb/DBTokens.h"
#include "gs/database/StrDB.h"
#include "ctp/ctp2_utils/pointerlist.h"

#include "GovernmentRecord.h"
#include <memory>

#define k_INITIAL_DB_SIZE 10
#define k_GROW_DB_STEP 10

template <class T> CTPDatabase<T>::CTPDatabase()
    // Initializer order must match the declaration order in CTPDatabase.h
    // (m_records, m_numRecords, m_allocatedSize, ...) or -Wreorder-ctor fires
:    m_numRecords        (0),
    m_allocatedSize     (k_INITIAL_DB_SIZE),
    m_modifiedRecords   (),
    m_indexToAlpha      (nullptr),
    m_alphaToIndex      (nullptr)
{
	m_records       = std::make_unique<T *[]>(m_allocatedSize).release();
	m_modifiedList  = std::make_unique<PointerList<GovernmentModifiedRecordNode> *[]>(m_allocatedSize).release();
}

template <class T> CTPDatabase<T>::~CTPDatabase()
{
	if (m_records)
	{
		for (sint32 i = 0; i < m_numRecords; i++)
		{
			std::unique_ptr<T>{m_records[i]};
		}
		std::unique_ptr<T *[]>{m_records};
	}

	std::unique_ptr<sint32[]>{m_indexToAlpha};
	std::unique_ptr<sint32[]>{m_alphaToIndex};

	for (size_t j = 0; j < m_modifiedRecords.size(); ++j)
	{
		std::unique_ptr<T>{m_modifiedRecords[j]};
	}
	m_modifiedRecords.clear();

	if (m_modifiedList)
	{
		for (sint32 k = 0; k < m_numRecords; k++)
		{
			if (m_modifiedList[k])
			{
				m_modifiedList[k]->DeleteAll();
				std::unique_ptr<PointerList<GovernmentModifiedRecordNode>>{m_modifiedList[k]};
			}
		}
		std::unique_ptr<PointerList<GovernmentModifiedRecordNode> *[]>{m_modifiedList};
	}
}

//----------------------------------------------------------------------------
//
// Name       : CTPDatabase<T>::Serialize
//
// Description: Store/Load CTPDatabase<T>
//
// Parameters : CivArchive &archive       :
//
// Globals    : -
//
// Returns    : -
//
// Remark(s)  : Does not Serialize the government modified stuff.
//              Fortunately these database serialize methods are
//              not thought for loading, just for database check.
//              But this is on the TODO list.
//
//----------------------------------------------------------------------------
/// Access a specific entry of the database
/// \param  index       Database index
/// \param  govIndex    Government index
/// \remarks When \a govIndex is not found in the government specific overrides,
///          the generic entry is returned.
template <class T> T * CTPDatabase<T>::Access(sint32 index, sint32 govIndex)
{
	// Check validity of index
	T * nonSpecific = Access(index);
	if (!nonSpecific) return nullptr;

	// Check for any government specific overrides
	for
	(
	    PointerList<GovernmentModifiedRecordNode>::Walker   walk =
	        PointerList<GovernmentModifiedRecordNode>::Walker(m_modifiedList[index]);
	    walk.IsValid();
	    walk.Next()
	)
	{
		if (govIndex == walk.GetObj()->m_governmentModified)
		{
			return m_modifiedRecords[walk.GetObj()->m_modifiedRecord];
		}
	}

	return nonSpecific;
}

template <class T> const T * CTPDatabase<T>::Get(sint32 index,sint32 govIndex)
{
	return const_cast<const T *>(Access(index, govIndex));
}

template <class T> void CTPDatabase<T>::Grow()
{
	PointerList<GovernmentModifiedRecordNode> **oldList = m_modifiedList;
	m_modifiedList = std::make_unique<PointerList<GovernmentModifiedRecordNode> *[]>(m_allocatedSize + k_GROW_DB_STEP).release();
	memcpy(m_modifiedList, oldList, m_allocatedSize * sizeof(PointerList<GovernmentModifiedRecordNode> *));
	std::unique_ptr<PointerList<GovernmentModifiedRecordNode> *[]>{oldList};

	T **oldRecords = m_records;
	m_records = std::make_unique<T *[]>(m_allocatedSize + k_GROW_DB_STEP).release();
	memcpy(m_records, oldRecords, m_allocatedSize * sizeof(T *));
	std::unique_ptr<T *[]>{oldRecords};
	m_allocatedSize += k_GROW_DB_STEP;
}

template <class T> void CTPDatabase<T>::Add(T *obj)
{
	if (obj->GetHasGovernmentsModified() &&
	    (obj->GenericGetNumGovernmentsModified() > 0)
	   )
	{
		sint32 numberGovernmentRecords;
		if (g_theGovernmentDB)
		{
			numberGovernmentRecords=g_theGovernmentDB->NumRecords();
		}
		else
		{
			numberGovernmentRecords=0;
			DPRINTF(k_DBG_FIX, ("GovMod- No Government Records \n"));
		}

		sint32 validIndex = 0;
		for (sint32 j = 0; j < obj->GenericGetNumGovernmentsModified(); j++)
		{
			if ((obj->GenericGetGovernmentsModifiedIndex(j) >= 0) &&
				(obj->GenericGetGovernmentsModifiedIndex(j) < numberGovernmentRecords)
			   )
			{
				validIndex++;
			}
		}

		sint32 mainRecord=FindRecordNameIndex(obj->GetIDText());
		if ((mainRecord >= 0) && (validIndex > 0))
		{
			// Add the new object to the list of modified records.
			sint32 const	newIndex	= m_modifiedRecords.size();
			obj->SetIndex(newIndex);
			m_modifiedRecords.push_back(obj);

			// Add references to the modified list
			if (!m_modifiedList[mainRecord])
			{
				m_modifiedList[mainRecord] = std::make_unique<PointerList<GovernmentModifiedRecordNode>>().release();
			}

			for (sint32 i = 0; i < obj->GenericGetNumGovernmentsModified(); i++)
			{
				if ((obj->GenericGetGovernmentsModifiedIndex(i) >= 0) &&
					(obj->GenericGetGovernmentsModifiedIndex(i) < numberGovernmentRecords)
				   )
				{
					DPRINTF(k_DBG_FIX, ("GovMod- Adding modified record %s, Gov Index %d \n",obj->GetIDText(),obj->GenericGetGovernmentsModifiedIndex(i)));
					m_modifiedList[mainRecord]->AddHead
						(std::make_unique<GovernmentModifiedRecordNode>
							(obj->GenericGetGovernmentsModifiedIndex(i), newIndex).release()
						);
				}
			}
		}
		else
		{
			DPRINTF(k_DBG_FIX, ("GovMod- No main record, or no valid GovernmentsModified %s \n",obj->GetIDText()));
		}
	}
	else
	{
		if (m_numRecords >= m_allocatedSize)
			Grow();
		Assert(m_numRecords < m_allocatedSize);
		obj->SetIndex(m_numRecords);
		m_records[m_numRecords] = obj;

		m_modifiedList[m_numRecords] = std::make_unique<PointerList<GovernmentModifiedRecordNode>>().release();
		m_modifiedList[m_numRecords]->AddHead(std::make_unique<GovernmentModifiedRecordNode>().release());
		m_numRecords++;
	}
}

template <class T> T *CTPDatabase<T>::Access(sint32 index)
{
	Assert(index >= 0);
	Assert(index < m_numRecords);
	if((index < 0) || (index >= m_numRecords))
	{
		DPRINTF(k_DBG_GAMESTATE, ("CTPDatabase::Access: index: %i, numRecords: %i\n", index, m_numRecords));
		return nullptr;
	}

	return m_records[index];
}

template <class T> sint32 CTPDatabase<T>::GetName(sint32 index)
{
	Assert(index >= 0);
	Assert(index < m_numRecords);
	if((index < 0) || (index >= m_numRecords))
		return 0;

	return m_records[index]->m_name;
}

template <class T> const char *CTPDatabase<T>::GetNameStr(sint32 index)
{
	Assert(index >= 0);
	Assert(index < m_numRecords);
	if((index < 0) || (index >= m_numRecords))
		return nullptr;

	return stringdb_Get()->GetNameStr(m_records[index]->m_name);
}

//----------------------------------------------------------------------------
//
// Name       : CTPDatabase<T>::Parse
//
// Description: Parses the data from text files into the data structures.
//
// Parameters : DBLexer *lex: The lexer used to parse the data.
//
// Globals    : -
//
// Returns    : 1 if the database was parsed successfully otherwise 0.
//
// Remark(s)  : -
//
//----------------------------------------------------------------------------
template <class T> bool CTPDatabase<T>::Parse(DBLexer *lex)
{
	bool isOk = true;

	while (!lex->EndOfInput())
	{
		auto obj = std::make_unique<T>();

		if (obj->Parse(lex, m_numRecords))
		{
			Add(obj.release());
		}
		else
		{
			isOk = false;
		}
	}

	std::unique_ptr<sint32[]>{m_indexToAlpha};
	std::unique_ptr<sint32[]>{m_alphaToIndex};
	m_indexToAlpha = (m_numRecords > 0) ? std::make_unique<sint32[]>(m_numRecords).release() : nullptr;
	m_alphaToIndex = (m_numRecords > 0) ? std::make_unique<sint32[]>(m_numRecords).release() : nullptr;

	memset(m_indexToAlpha, 0, sizeof(sint32) * m_numRecords);
	memset(m_alphaToIndex, 0, sizeof(sint32) * m_numRecords);

	// A merge sort algorithm is of course better, but the complexity
	// is the same as for the old databases even the constant is the same.
	for (sint32 i = 0; i < m_numRecords; ++i)
	{
		const MBCHAR *str = m_records[i]->GetNameText();
		sint32 a;
		for (a = 0; a < i; ++a)
		{
			if(_stricoll(str, m_records[m_alphaToIndex[a]]->GetNameText()) < 0)
			{
				memmove(
					m_alphaToIndex + a + 1,
					m_alphaToIndex + a,
					(i - a) * sizeof(sint32));

				for(sint32 j = 0; j < i; ++j)
					if(m_indexToAlpha[j] >= a)
						++m_indexToAlpha[j];

				break;

			}
		}
		m_alphaToIndex[a] = i;
		m_indexToAlpha[i] = a;
	}

	return isOk;
}

template <class T> bool CTPDatabase<T>::Parse(const C3DIR & c3dir, const char *filename)
{
	DBLexer lex = DBLexer(c3dir, filename);
	return Parse(&lex);
}

template <class T> bool CTPDatabase<T>::GetRecordFromLexer(DBLexer * lex, sint32 & index)
{
	sint32 tok = lex->GetToken();
	if(tok != k_Token_Name) {
		if(tok == k_Token_Int) {
			index = atoi(lex->GetTokenText());
			return true;
		}
		else if(tok != k_Token_String){
			DBERROR(("Expected record name1"));
			return false;
		}
	}

	sint32 strId;
	if(!stringdb_Get()->GetStringID(lex->GetTokenText(), strId)) {

		sint32 i;
		for(i = 0; i < m_numRecords; i++) {
			if(!stricmp(m_records[i]->GetNameText(), lex->GetTokenText())) {
				index = i;
				return true;
			}
		}

		stringdb_Get()->InsertStr(lex->GetTokenText(), lex->GetTokenText());
		if(stringdb_Get()->GetStringID(lex->GetTokenText(), strId)) {
			index = strId | 0x80000000;
			return true;
		} else {
			return false;
		}
	}

	if(GetNamedItem(strId, index)) {
		return true;
	} else {
		index = strId | 0x80000000;
		return true;
	}
}

template <class T> bool CTPDatabase<T>::GetCurrentRecordFromLexer(DBLexer *lex, sint32 &index)
{
	sint32 tok = lex->GetCurrentToken();
	if(tok != k_Token_Name) {
		if(tok == k_Token_Int) {
			index = atoi(lex->GetTokenText());
			return true;
		}
		else if(tok != k_Token_String){
			DBERROR(("Expected record name1"));
			return false;
		}
	}

	sint32 strId;
	if(!stringdb_Get()->GetStringID(lex->GetTokenText(), strId)) {

		sint32 i;
		for(i = 0; i < m_numRecords; i++) {
			if(!stricmp(m_records[i]->GetNameText(), lex->GetTokenText())) {
				index = i;
				return true;
			}
		}

		stringdb_Get()->InsertStr(lex->GetTokenText(), lex->GetTokenText());
		if(stringdb_Get()->GetStringID(lex->GetTokenText(), strId)) {
			index = strId | 0x80000000;
			return true;
		} else {
			return false;
		}
	}

	if(GetNamedItem(strId, index)) {
		return true;
	} else {
		index = strId | 0x80000000;
		return true;
	}
}

// Vector-based primary form for unbounded grow.
template <class T> bool CTPDatabase<T>::ParseRecordInArray(DBLexer *lex, std::vector<sint32> &array)
{
	sint32 tok = lex->GetToken();
	if(tok != k_Token_Name) {
		DBERROR(("Expected record name"));
		return false;
	}

	sint32 i;
	for(i = 0; i < m_numRecords; i++) {
		if(((m_records[i]->m_name >= 0) &&
			(!strcmp(stringdb_Get()->GetIdStr(m_records[i]->m_name), lex->GetTokenText()))) ||
		   ((m_records[i]->m_name < 0) &&
			(!strcmp(m_records[i]->GetNameText(), lex->GetTokenText())))) {
				array.push_back(i);
				return true;
			}
	}

	sint32 strId;
	if(!stringdb_Get()->GetStringID(lex->GetTokenText(), strId)) {
		stringdb_Get()->InsertStr(lex->GetTokenText(), lex->GetTokenText());
	}

	if(stringdb_Get()->GetStringID(lex->GetTokenText(), strId)) {
		array.push_back(strId | 0x80000000);
		return true;
	} else {
		return false;
	}
}

// Legacy T**+count adapter — copies into vector, delegates, copies out.
template <class T> bool CTPDatabase<T>::ParseRecordInArray(DBLexer *lex, sint32 **array, sint32 *numElements)
{
	std::vector<sint32> tmp(*array, *array + *numElements);
	if(!ParseRecordInArray(lex, tmp)) return false;
	std::unique_ptr<sint32[]>{*array};
	*array = std::make_unique<sint32[]>(tmp.size()).release();
	std::copy(tmp.begin(), tmp.end(), *array);
	*numElements = static_cast<sint32>(tmp.size());
	return true;
}

template <class T> bool CTPDatabase<T>::ParseRecordInArray(DBLexer *lex, sint32 *array, sint32 *numElements, sint32 maxSize)
{
	sint32 tok = lex->GetToken();
	if(tok != k_Token_Name) {
		DBERROR(("Expected record name3"));
		return false;
	}

	if(*numElements >= maxSize) {
		DBERROR(("too many entries"));
		return false;
	}

	sint32 i;
	for(i = 0; i < m_numRecords; i++) {
		if(((m_records[i]->m_name >= 0) &&
			(!strcmp(stringdb_Get()->GetIdStr(m_records[i]->m_name), lex->GetTokenText()))) ||
		   ((m_records[i]->m_name < 0) &&
			(!strcmp(m_records[i]->GetNameText(), lex->GetTokenText())))) {
			array[*numElements] = i;
			*numElements += 1;
			return true;
		}
	}

	sint32 strId;
	if(!stringdb_Get()->GetStringID(lex->GetTokenText(), strId)) {
		stringdb_Get()->InsertStr(lex->GetTokenText(), lex->GetTokenText());
	}

	if(stringdb_Get()->GetStringID(lex->GetTokenText(), strId)) {
		array[*numElements] = (strId | 0x80000000);
		*numElements += 1;
		return true;
	} else {
		return false;
	}
}

template <class T> bool CTPDatabase<T>::GetNamedItem(sint32 name, sint32 &index)
{
	sint32 i;
	for(i = 0; i < m_numRecords; i++) {
		if(name == m_records[i]->GetName()) {
			index = i;
			return true;
		}
	}
	return false;
}

template <class T> bool CTPDatabase<T>::GetNamedItem(const char *name, sint32 &index)
{
	sint32 strId;
	if(stringdb_Get()->GetStringID(name, strId)) {
		return GetNamedItem(strId, index);
	}

	sint32 i;
	for(i = 0; i < m_numRecords; i++) {
		if(!stricmp(name, m_records[i]->GetNameText())) {
			index = i;
			return true;
		}
	}
	return false;
}

template <class T> bool CTPDatabase<T>::GetNamedItemID(sint32 index, sint32 &name)
{
	if(index < 0)
		return false;

	if(index >= m_numRecords)
		return true;

	name = m_records[index]->GetName();
	return true;
}

template <class T> bool CTPDatabase<T>::ResolveReferences()
{
	bool success = true;
	sint32 i;
	for(i = 0; i < m_numRecords; i++) {
		m_records[i]->ResolveDBReferences();
	}

	return success;
}

template <class T> sint32 CTPDatabase<T>::FindTypeIndex(const char *str) const
{
	sint32 i;
	for(i = 0; i < m_numRecords; i++) {
		if(stricmp(m_records[i]->GetNameText(), str) == 0) {
			return i;
		}
	}
	return CTPRecord::INDEX_INVALID;
}

template <class T> sint32 CTPDatabase<T>::FindRecordNameIndex(const char *str) const
{
	sint32 i;
	for(i = 0; i < m_numRecords; i++) {
		if(stricmp(m_records[i]->GetIDText(), str) == 0) {
			return i;
		}
	}
	return CTPRecord::INDEX_INVALID;
}

#include "IconRecord.h" // 0
template class CTPDatabase<IconRecord>;

#include "SoundRecord.h" // 1
template class CTPDatabase<SoundRecord>;

#include "TerrainRecord.h" // 2
template class CTPDatabase<TerrainRecord>;

#include "ResourceRecord.h" // 3
template class CTPDatabase<ResourceRecord>;

#include "AgeRecord.h" // 4
template class CTPDatabase<AgeRecord>;

#include "AdvanceRecord.h" // 5
template class CTPDatabase<AdvanceRecord>;

#include "AdvanceBranchRecord.h" // 6
template class CTPDatabase<AdvanceBranchRecord>;

#include "FeatRecord.h" // 7
template class CTPDatabase<FeatRecord>;

#include "WonderRecord.h" // 8
template class CTPDatabase<WonderRecord>;

#include "WonderMovieRecord.h" // 9
template class CTPDatabase<WonderMovieRecord>;

#include "BuildingRecord.h" // 10
template class CTPDatabase<BuildingRecord>;

#ifndef __TILETOOL__

#include "UnitRecord.h" // 11
template class CTPDatabase<UnitRecord>;

#include "SpriteRecord.h" // 12
template class CTPDatabase<SpriteRecord>;

#include "GovernmentRecord.h" // 13
template class CTPDatabase<GovernmentRecord>;

#include "SpecialAttackInfoRecord.h" // 14
template class CTPDatabase<SpecialAttackInfoRecord>;

#include "SpecialEffectRecord.h" // 15
template class CTPDatabase<SpecialEffectRecord>;

#include "TerrainImprovementRecord.h" // 26
template class CTPDatabase<TerrainImprovementRecord>;

#include "OrderRecord.h" // 17
template class CTPDatabase<OrderRecord>;

#include "GoalRecord.h" // 18
template class CTPDatabase<GoalRecord>;

#include "UnitBuildListRecord.h" // 19
template class CTPDatabase<UnitBuildListRecord>;

#include "BuildingBuildListRecord.h" // 20
template class CTPDatabase<BuildingBuildListRecord>;

#include "WonderBuildListRecord.h" // 21
template class CTPDatabase<WonderBuildListRecord>;

#include "ImprovementListRecord.h" // 22
template class CTPDatabase<ImprovementListRecord>;

#include "StrategyRecord.h" // 23
template class CTPDatabase<StrategyRecord>;

#include "BuildListSequenceRecord.h" // 24
template class CTPDatabase<BuildListSequenceRecord>;

#include "DiplomacyRecord.h" // 25
template class CTPDatabase<DiplomacyRecord>;

#include "AdvanceListRecord.h" // 26
template class CTPDatabase<AdvanceListRecord>;

#include "CitySizeRecord.h" // 27
template class CTPDatabase<CitySizeRecord>;

#include "PopRecord.h" // 28
template class CTPDatabase<PopRecord>;

#include "DiplomacyProposalRecord.h" // 29
template class CTPDatabase<DiplomacyProposalRecord>;

#include "DiplomacyThreatRecord.h" // 30
template class CTPDatabase<DiplomacyThreatRecord>;

#include "PersonalityRecord.h" // 31
template class CTPDatabase<PersonalityRecord>;

#include "EndGameObjectRecord.h" // 32
template class CTPDatabase<EndGameObjectRecord>;

#include "CityStyleRecord.h" // 33
template class CTPDatabase<CityStyleRecord>;

#include "AgeCityStyleRecord.h" // 34
template class CTPDatabase<AgeCityStyleRecord>;

#include "CivilisationRecord.h" // 35
template class CTPDatabase<CivilisationRecord>;

#include "RiskRecord.h" // 36
template class CTPDatabase<RiskRecord>;

#include "DifficultyRecord.h" // 37
template class CTPDatabase<DifficultyRecord>;

#include "PollutionRecord.h" // 38
template class CTPDatabase<PollutionRecord>;

#include "GlobalWarmingRecord.h" // 39
template class CTPDatabase<GlobalWarmingRecord>;

#include "MapIconRecord.h" // 40
template class CTPDatabase<MapIconRecord>;

#include "MapRecord.h" // 41
template class CTPDatabase<MapRecord>;

#include "ConceptRecord.h" // 42
template class CTPDatabase<ConceptRecord>;

#include "ConstRecord.h" // 43
template class CTPDatabase<ConstRecord>;
#endif // __TILETOOL__
