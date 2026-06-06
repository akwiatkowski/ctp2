#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef _NET_ARRAY_H_
#define _NET_ARRAY_H_

#include <memory>

class NetArray
{
public:
	NetArray(sint32 initSize = 0) :
		m_size(initSize)
	{
		if(initSize != 0) {
			m_array = std::make_unique<void*[]>(initSize);
		}
	};

	~NetArray() = default;

	sint32 Add(void* ptr);
	void Set(void* ptr, sint32 idx);
	void* Get(sint32 idx);
	sint32 GetSize() { return m_size; };
private:
	std::unique_ptr<void*[]> m_array;
	sint32 m_size;
};

inline sint32
NetArray::Add(void* ptr)
{
	if(m_size > 0) {
		auto old_array = std::move(m_array);
		m_array = std::make_unique<void*[]>(++m_size);
		memcpy(m_array.get(), old_array.get(), sizeof(void*) * (m_size - 1));
		m_array[m_size - 1] = ptr;
		return m_size - 1;
	} else {
		m_size = 1;
		m_array = std::make_unique<void*[]>(m_size);
		m_array[m_size - 1] = ptr;
		return m_size;
	}
}

inline void
NetArray::Set(void* ptr, sint32 idx)
{
	m_array[idx] = ptr;
}

inline void*
NetArray::Get(sint32 idx)
{
	return m_array[idx];
}

#endif
