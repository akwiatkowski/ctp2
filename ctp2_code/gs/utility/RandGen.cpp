#include "ctp/c3.h"
#include "gs/utility/RandGen.h"
#include "gs/fileio/gamefile.h"

RandomGenerator::RandomGenerator(sint32 seed)

{
	Initialize(seed);
}

RandomGenerator::RandomGenerator(RandomGenerator &copyme)
{
	m_start_seed = copyme.m_start_seed;
	memcpy(m_buffer, copyme.m_buffer, sizeof(m_buffer));
	m_firstp = &(m_buffer[copyme.m_firstp - copyme.m_buffer]);
	m_secondp = &(m_buffer[copyme.m_secondp - copyme.m_buffer]);
	m_endp = &(m_buffer[56]);
	m_callCount = copyme.m_callCount;
}

void RandomGenerator::Initialize(sint32 seed)

{
	sint32 i;
	sint32 j;
	sint32 mj;
	sint32 mk;

    m_start_seed = seed;
    m_buffer[0] = 0;
	mj = k_RAND_MSEED - seed;
    mj = mj & 0x7fffffff;
	mj %= k_RAND_MBIG;
	m_buffer[55] = mj;
	mk = 1;

	for (i=1; i<=54; i++) {
		j = (21 * i) % 55;
		m_buffer[j]=mk;
		mk = mj-mk;
		if (mk < 0)
			mk += k_RAND_MBIG;

		mj = m_buffer[j];
	}

	for (i=0; i<4; i++) {
		for (j=1; j<=54; j++) {
			m_buffer[j] -= m_buffer[1 + ((j+30)%55)];
			if (m_buffer[j] < 0)
				m_buffer[j] += k_RAND_MBIG;
		}
	}

    m_firstp = &(m_buffer[0]);
    m_secondp = &(m_buffer[31]);
    m_endp = &(m_buffer[56]);

	m_callCount = 0;
}

sint32 RandomGenerator::Next()
{
   if (++m_firstp == m_endp)
      m_firstp = (m_buffer + 1);

   if (++m_secondp == m_endp)
      m_secondp = (m_buffer + 1);

   *m_firstp -= *m_secondp;

   if (*m_firstp < 0)
       *m_firstp += k_RAND_MBIG;

   m_callCount++;

   return *m_firstp;
}
