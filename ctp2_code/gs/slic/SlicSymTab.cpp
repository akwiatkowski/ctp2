#include "ctp/c3.h"
#include "gs/slic/SlicSymTab.h"
#include "gs/slic/SlicEngine.h"

SlicSymTab::SlicSymTab(sint32 size) :
	StringHash<SlicNamedSymbol>(k_SLIC_SYM_TAB_HASH_SIZE)
{
	m_arraySize = m_numEntries = size;
	if(m_arraySize == 0)
		m_arraySize = 1;
	m_array = std::make_unique<SlicNamedSymbol *[]>(m_arraySize);   // value-init → all null
}

SlicSymTab::~SlicSymTab()
{
	// m_array is unique_ptr — frees itself
}

void SlicSymTab::PostSerialize()
{
	for(sint32 i = 0; i < m_numEntries; i++) {
		if(m_array[i]) {

		}
	}
}

void SlicSymTab::Add(SlicNamedSymbol *sym)
{
	StringHash<SlicNamedSymbol>::Add(sym);

	if(m_numEntries >= m_arraySize) {
		auto newArray = std::make_unique<SlicNamedSymbol *[]>(m_arraySize * 2);
		memcpy(newArray.get(), m_array.get(), m_arraySize * sizeof(SlicNamedSymbol *));
		m_array = std::move(newArray);
		m_arraySize *= 2;
	}
	sym->SetIndex(m_numEntries);
	m_array[m_numEntries] = sym;
	m_numEntries++;
}

void SlicSymTab::Add(sint32 index)
{
	// Dead entry point: the #if 0 body that consumed slicif's parallel
	// symbol table was disabled long ago; nothing may call this overload.
	Assert(FALSE);
}

const SlicNamedSymbol *SlicSymTab::Get(sint32 index) const
{
	Assert(index >= 0);
	Assert(index < m_numEntries);
	if(index < 0 || index >= m_numEntries)
		return nullptr;

	return m_array[index];
}

SlicNamedSymbol *SlicSymTab::Access(sint32 index)
{
	Assert(index >= 0);
	Assert(index < m_numEntries);
	if(index < 0 || index >= m_numEntries)
		return nullptr;

	return m_array[index];
}

void SlicSymTab::Resize(sint32 num)
{
	m_numEntries = num;
	if(num <= m_arraySize)
		return;
	auto newArray = std::make_unique<SlicNamedSymbol *[]>(num);
	memcpy(newArray.get(), m_array.get(), m_arraySize * sizeof (SlicNamedSymbol *));
	m_array = std::move(newArray);
	m_arraySize = num;
}
