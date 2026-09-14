#pragma once
#ifndef __PQUEUE_H__
#define __PQUEUE_H__

#include <algorithm>
#include <memory>

template <class T> class PQueue {
private:
	std::unique_ptr<T[]> m_array;
	sint32 m_nElements;
	sint32 m_maxSize;

	void Grow() {
		auto oldArray = std::move(m_array);
		m_array = std::make_unique<T[]>(m_maxSize * 2);
		std::move(oldArray.get(), oldArray.get() + m_maxSize, m_array.get());
		m_maxSize *= 2;
	}

	void Shift(sint32 index) {
		if(m_nElements >= m_maxSize)
			Grow();

		if(m_nElements - index > 0) {
			std::move_backward(&m_array[index], &m_array[m_nElements],
							   &m_array[m_nElements + 1]);
		}
		m_nElements++;
	}

public:
	PQueue(sint32 startSize) {
		m_maxSize = startSize;
		m_nElements = 0;
		m_array = std::make_unique<T[]>(m_maxSize);
	}
	~PQueue() = default;

	void Insert(const T &obj);
	bool RemoveTop(T &obj);
	sint32 Num() { return m_nElements; }
	void Clear() { m_nElements = 0; }
};

template <class T> void PQueue<T>::Insert(const T &obj)
{
	if(m_nElements >= m_maxSize) {
		Grow();
	}
	if(m_nElements == 0) {
		m_array[0] = obj;
		m_nElements++;
		return;
	}

	sint32 index = m_nElements >> 1;
	sint32 step = m_nElements >> 2;
	if(step < 1)
		step = 1;

	while(true) {
		if(obj.m_value > m_array[index].m_value) {
			if(index == 0 || obj.m_value <= m_array[index - 1].m_value) {
				Shift(index);
				m_array[index] = obj;
				return;
			}
			index -= step;
		} else if(obj.m_value < m_array[index].m_value) {
			if(index >= m_nElements - 1 ||
			   obj.m_value >= m_array[index + 1].m_value) {
				Shift(index + 1);
				m_array[index + 1] = obj;
				return;
			}
			index += step;
		} else {
			Shift(index);
			m_array[index] = obj;
			return;
		}
		if(step > 1)
			step >>= 1;
	}
}

template <class T> bool PQueue<T>::RemoveTop(T &obj)
{
	if(m_nElements <= 0)
		return FALSE;

	obj = m_array[m_nElements - 1];
	m_nElements--;
	return true;
}

#endif
