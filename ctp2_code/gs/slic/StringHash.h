//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ header
// Description  : String hash table handling.
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
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Repaired memory leak or illegal access when loading a file with a
//   different table size.
// - Repaired crash when computing the key of a zero-length string.
//
//----------------------------------------------------------------------------

#ifndef __STRING_HASH_H__
#define __STRING_HASH_H__
#include <memory>
#include <vector>

template <class T> class StringHashNode {
public:

	std::unique_ptr<T> m_obj;
	std::unique_ptr<StringHashNode<T>> m_next;

	StringHashNode(const char *string, T *obj)
    :   m_obj   (obj),
        m_next  (nullptr)
    { ; };

	// m_obj/m_next are unique_ptr — chain frees itself
};

template <class T> class StringHash {
protected:
	std::vector<std::unique_ptr<StringHashNode<T>>> m_table;
	sint32 m_table_size;

public:
	StringHash(sint32 table_size);
	virtual ~StringHash();

	uint16 Key(const char *str);

	T * operator [] (const char *str) {
		return Access(str);
	}

	const T *Get(const char *str);
	T *Access(const char *str);
	virtual void Add(const char *str, T *obj);
	virtual void Add(T *obj);
	T *Del(const char *str);
	void Clear();

	// Iterate every entry in the hash, in bucket-then-chain order.
	// Used by the JSON save bridge in gs/fileio/json_save.cpp to
	// linearise StringHash<T> contents without exposing m_table.
	template <class Visitor>
	void ForEach(Visitor const &visit) const
	{
		if (m_table.empty()) return;
		for (sint32 i = 0; i < m_table_size; ++i)
		{
			for (StringHashNode<T> *node = m_table[i].get(); node; node = node->m_next.get())
				visit(node->m_obj.get());
		}
	}
};

template <class T> StringHash<T>::StringHash(sint32 table_size)
{
	// Assert validity of % and cast to uint16.
	Assert(table_size > 0);
	Assert(table_size < 0x10000);
	m_table.resize(table_size);   // value-init → all null
	m_table_size = table_size;

}

template <class T> StringHash<T>::~StringHash()
{
	// m_table is vector<unique_ptr> — nodes and chains free themselves
}

template <class T> void StringHash<T>::Clear()
{
	for(auto & head : m_table) {
		head.reset();
	}
}

template <class T> uint16 StringHash<T>::Key(const char *str)
{
	uint16			key = 0;
	size_t const	len	= strlen(str);

	for (size_t i = 0; (i + 1) < len; ++i)
	{
		key = key + static_cast<uint16>((tolower(str[i]) << 8 | tolower(str[i + 1])) + i);
	}

	if (len > 0)
	{
		key = key + static_cast<uint16>((tolower(str[len - 1]) << 8) + (len - 1));
	}

	return static_cast<uint16>(key % m_table_size);
}

template <class T> const T *StringHash<T>::Get(const char *str)
{
	uint16 index = Key(str);
	StringHashNode<T> *node = m_table[index].get();
	while(node) {
		if(stricmp(node->m_obj->GetName(), str) == 0)
			return node->m_obj.get();
		node = node->m_next.get();
	}
	return nullptr;
}

template <class T> T *StringHash<T>::Access(const char *str)
{
	uint16 index = Key(str);
	StringHashNode<T> *node = m_table[index].get();
	while(node) {
		if(stricmp(node->m_obj->GetName(), str) == 0)
			return node->m_obj.get();
		node = node->m_next.get();
	}
	return nullptr;
}

template <class T> void StringHash<T>::Add(const char *str, T *obj)
{
	auto node = std::make_unique<StringHashNode<T>>(str, obj);
	uint16 index = Key(str);
	node->m_next = std::move(m_table[index]);
	m_table[index] = std::move(node);
}

template <class T> void StringHash<T>::Add(T *obj)
{
	Add(obj->GetName(), obj);
}

template <class T> T *StringHash<T>::Del(const char *str)
{
	uint16 index = Key(str);
	StringHashNode<T> *node = m_table[index].get();
	StringHashNode<T> *last = nullptr;
	while(node) {
		if(stricmp(node->m_obj->GetName(), str) == 0) {
			// Unlink: splice node's tail into the predecessor slot
			if(last) {
				last->m_next = std::move(node->m_next);
			} else {
				m_table[index] = std::move(node->m_next);
			}
			T *obj = node->m_obj.release();
			// node is now unlinked; unique_ptr frees it
			std::unique_ptr<StringHashNode<T>>{node};
			return obj;
		}
		last = node;
		node = node->m_next.get();
	}
	return nullptr;
}

#endif
