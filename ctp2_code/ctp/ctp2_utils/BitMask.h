#ifndef BIT_MASK_H__
#define BIT_MASK_H__


#include <cstring>
#include <vector>
#include <nlohmann/json.hpp>

#define SIZE_IN_BYTES(bitSize) ((bitSize + 7) / 8)

class BitMask {
  private:
	uint8* m_bytes;
	sint32 m_sizeInBits;

  public:
	BitMask(sint32 bitSize) {
		m_bytes = new uint8[SIZE_IN_BYTES(bitSize)];
		m_sizeInBits = bitSize;

		memset(m_bytes, 0, SIZE_IN_BYTES(m_sizeInBits) * sizeof(uint8));
	}

	~BitMask() {
		delete [] m_bytes;
	}

	void SetBit(sint32 bit) {
		Assert(bit >= 0);
		Assert(bit < m_sizeInBits);
		if(bit < 0 || bit >= m_sizeInBits)
			return;

		sint32 byte = bit / 8;
		m_bytes[byte] |= (1 << (bit % 8));
	}

	void ClearBit(sint32 bit) {
		Assert(bit >= 0);
		Assert(bit < m_sizeInBits);
		if(bit < 0 || bit >= m_sizeInBits)
			return;

		sint32 byte = bit / 8;

		m_bytes[byte] &= ~(1 << (bit % 8));
	}

	bool GetBit(sint32 bit) {
		Assert(bit >= 0);
		Assert(bit < m_sizeInBits);
		if(bit < 0 || bit >= m_sizeInBits)
			return false;

		sint32 byte = bit / 8;

		return (m_bytes[byte] & (1 << (bit % 8))) != 0;
	}

	// JSON bridge — stores size + byte payload (matches Serialize).
	friend void to_json(nlohmann::json &j, BitMask const &b) {
		j = nlohmann::json{
			{"size_in_bits", b.m_sizeInBits},
			{"bytes",        std::vector<uint8>(b.m_bytes, b.m_bytes + SIZE_IN_BYTES(b.m_sizeInBits))},
		};
	}

	friend void from_json(nlohmann::json const &j, BitMask &b) {
		j.at("size_in_bits").get_to(b.m_sizeInBits);
		std::vector<uint8> bytes;
		j.at("bytes").get_to(bytes);
		delete[] b.m_bytes;
		b.m_bytes = new uint8[SIZE_IN_BYTES(b.m_sizeInBits)];
		std::memcpy(b.m_bytes, bytes.data(), bytes.size());
	}

	bool AllBitsSet() {
		sint32 i;
		for(i = 0; i < SIZE_IN_BYTES(m_sizeInBits) - 1; i++) {
			if(m_bytes[i] != 0xff)
				return false;
		}

		if(!(m_sizeInBits % 8))
			return m_bytes[SIZE_IN_BYTES(m_sizeInBits) - 1] == 0xff;

		if(m_bytes[SIZE_IN_BYTES(m_sizeInBits) - 1] == (0xff >> (8 - (m_sizeInBits % 8))))
			return true;
		return false;
	}

};

#endif
