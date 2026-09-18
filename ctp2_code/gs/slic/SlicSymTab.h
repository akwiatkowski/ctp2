#ifndef __SLIC_SYM_TAB_H__
#define __SLIC_SYM_TAB_H__

#include "gs/slic/StringHash.h"
#include "gs/slic/SlicSymbol.h"
#include "gs/slic/SlicNamedSymbol.h"
#include <nlohmann/json.hpp>

#define k_SLIC_SYM_TAB_HASH_SIZE 256

class SlicSymTab : public StringHash<SlicNamedSymbol>
{
private:

	sint32 m_arraySize;
	sint32 m_numEntries;


	SlicNamedSymbol **m_array;

public:
	SlicSymTab(sint32 size);
	~SlicSymTab() override;
	void PostSerialize();

	void Add(SlicNamedSymbol *sym) override;
	// Keep the base overloads visible; Add(sint32) below would hide them.
	using StringHash<SlicNamedSymbol>::Add;
	void Add(sint32 index);
	const SlicNamedSymbol *Get(sint32 index) const;
	SlicNamedSymbol *Access(sint32 index);

	const sint32 GetSize() { return m_arraySize; }
	const sint32 GetNumEntries() { return m_numEntries; }

	void GrowBy(sint32 num);
	void Resize(sint32 num);

	friend void to_json(nlohmann::json &j, SlicSymTab const &t);
	friend void from_json(nlohmann::json const &j, SlicSymTab &t);
};

#endif
