#include "ctp/c3.h"
#include <objbase.h>
#include "gs/outcom/C3Rand.h"
#include "gs/utility/RandGen.h"
#include "civarchive.h"

STDMETHODIMP C3Rand::QueryInterface(REFIID riid, void **obj)
{
	*obj = nullptr;
	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) C3Rand::AddRef()
{
	return ++m_refCount;
}

STDMETHODIMP_(ULONG) C3Rand::Release()
{
	if (--m_refCount)
		return m_refCount;
	delete this;
	return 0;
}

C3Rand::C3Rand(BOOL ownGenerator)
{
	m_refCount = 0;
	m_ownGenerator = ownGenerator;
	if (m_ownGenerator) {
		m_rand = new RandomGenerator(*rand_ptr());
	} else {
		m_rand = rand_ptr();
	}
}

C3Rand::~C3Rand()
{
	if (m_ownGenerator) {
		delete m_rand;
		m_rand = nullptr;
	}
}

STDMETHODIMP_(sint32) C3Rand::Next(sint32 range)
{
	return m_rand->Next(range);
}

C3Rand::C3Rand(CivArchive &archive)
{
	m_rand = nullptr;
	Serialize(archive);
}

void C3Rand::Serialize(CivArchive &archive)
{
	if (archive.IsStoring()) {
		archive << static_cast<uint32>(m_refCount);
		archive.PutSINT32(m_ownGenerator);
		if (m_ownGenerator) {
			m_rand->Serialize(archive);
		}
	} else {
		uint32 l_refCount;
		archive >> l_refCount;
		m_refCount = l_refCount;
		m_ownGenerator = archive.GetSINT32();
		if (m_ownGenerator) {
			
				delete m_rand;
			m_rand = new RandomGenerator(archive);
		} else {
			m_rand = rand_ptr();
		}
	}
}
