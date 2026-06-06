#ifdef HAVE_PRAGMA_ONCE
#pragma once
#endif
#ifndef __BIT_ARRAY_H__
#define __BIT_ARRAY_H__

#include <memory>

#define k_BITS_PER_ENTRY 8
class BitArray
{
private:
	std::unique_ptr<uint8[]> m_array;
	sint32 m_numBits;

public:
	BitArray(sint32 numBits) {
		m_numBits = numBits;
		m_array = std::make_unique<uint8[]>((m_numBits + (k_BITS_PER_ENTRY - 1))/k_BITS_PER_ENTRY);
	}
	~BitArray() = default;

	void Clear() {
		memset(m_array.get(), 0, (m_numBits + (k_BITS_PER_ENTRY - 1)) / k_BITS_PER_ENTRY);
	}

	void SetBit(sint32 bit) {
		Assert(bit >= 0);
		Assert(bit < m_numBits);
		if(bit < 0 || bit >= m_numBits)
			return;

		m_array[bit / k_BITS_PER_ENTRY] |= 1 << (bit % k_BITS_PER_ENTRY);
	}

	void ClearBit(sint32 bit) {
		Assert(bit >= 0);
		Assert(bit < m_numBits);
		if(bit < 0 || bit >= m_numBits)
			return;

		m_array[bit / k_BITS_PER_ENTRY] &= ~(1 << (bit % k_BITS_PER_ENTRY));
	}

	bool Bit(sint32 bit) {
		Assert(bit >= 0);
		Assert(bit < m_numBits);
		if(bit < 0 || bit >= m_numBits)
			return false;

		return (m_array[bit / k_BITS_PER_ENTRY] & (1 << (bit % k_BITS_PER_ENTRY))) != 0;
	}
};

#endif
