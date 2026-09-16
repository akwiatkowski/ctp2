//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : The old database template class. (Should be replaced)
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
// - None
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - Removed refferences to the old civilisation database. (Aug 20th 2005 Martin G�hmann)
// - Removed old endgame, risk and installation databases. (Aug 29th 2005 Martin G�hmann)
// - Removed old pollution and global warming databases. (July 15th 2006 Martin G�hmann)
// - Removed old map database. (24-Mar2007 Martin G�hmann)
//
// @ToDo: Check whether this file can be removed savely
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"

#include "gs/database/DB.h"
#include "EndGameRecord.h"

#include "gs/database/StrDB.h"


template <class T> Database<T>::Database()

{
	m_nRec = m_max_nRec = 0;
}

template <class T> Database<T>::~Database() = default;

template <class T> void  Database<T>::SetSize(const sint32 n)

{
	Assert (0 < n);
	auto oldrec = std::move(m_rec);
	auto oldalpha = std::move(m_indexToAlpha);
	auto oldindex = std::move(m_alphaToIndex);
	sint32 oldmax = m_max_nRec;

	m_max_nRec = n;
	m_rec = std::make_unique<T[]>(n);
	m_indexToAlpha = std::make_unique<sint32[]>(n);
	m_alphaToIndex = std::make_unique<sint32[]>(n);
	memset(m_indexToAlpha.get(), 0, sizeof(sint32) * n);
	memset(m_alphaToIndex.get(), 0, sizeof(sint32) * n);

	if(oldrec) {
		// Number of records to carry over into the grown array.
		sint32 const carry = std::min(oldmax, m_max_nRec);
		// Move per element, not memcpy: record types are polymorphic and hold
		// owning members (e.g. EndGameRecord::m_requiredForStage is a
		// unique_ptr). The old byte copy duplicated those pointers and left
		// the originals in oldrec, whose destructor then freed them —
		// dangling pointers in the grown array (-Wdynamic-class-memaccess
		// pointed at the vtable half of the same problem). Move assignment
		// transfers ownership and leaves oldrec's members null.
		for (sint32 i = 0; i < carry; ++i) {
			m_rec[i] = std::move(oldrec[i]);
		}
		std::copy(oldalpha.get(), oldalpha.get() + carry, m_indexToAlpha.get());
		std::copy(oldindex.get(), oldindex.get() + carry, m_alphaToIndex.get());
	}
}

template <class T> void  Database<T>::SetSizeAll(const sint32 n)

{
	Assert (0 < n);
	m_max_nRec = n;
	m_rec = std::make_unique<T[]>(n);
	m_indexToAlpha = std::make_unique<sint32[]>(n);
	m_alphaToIndex = std::make_unique<sint32[]>(n);
	memset(m_indexToAlpha.get(), 0, sizeof(sint32) * n);
	memset(m_alphaToIndex.get(), 0, sizeof(sint32) * n);
	m_nRec = n;
	Assert(m_rec.get());
	Assert(m_indexToAlpha.get());
	Assert(m_alphaToIndex.get());
}

template <class T> const T* Database<T>::Get(const sint32 i) const

{
	Assert(0<=i);
	Assert(i<m_nRec);
	if(i < 0 || i >= m_nRec)
		return nullptr;
	return &(m_rec[i]);
}

template <class T> T* Database<T>::Access(const sint32 i)

{
	Assert(0<=i);
	Assert(i<m_nRec);
	if(i < 0 || i >= m_nRec)
		return nullptr;
	return &(m_rec[i]);
}

template <class T> void Database<T>::AddRec(const StringId sid, sint32 &i)

{
	Assert (m_nRec < m_max_nRec);

	m_rec[m_nRec].SetName(sid);
	{
		const MBCHAR *str = stringdb_Get()->GetNameStr( sid );
		sint32 a;
		for (a = 0; a < m_nRec; ++a )
		{
			if ( _stricoll( str, stringdb_Get()->GetNameStr(
				m_rec[ m_alphaToIndex[ a ] ].GetName() ) ) < 0 )
			{

				memmove(
					m_alphaToIndex.get() + a + 1,
					m_alphaToIndex.get() + a,
					( m_nRec - a ) * sizeof(sint32) );

				for ( sint32 j = 0; j < m_nRec; ++j )
					if ( m_indexToAlpha[ j ] >= a )
						++m_indexToAlpha[ j ];

				break;
			}
		}

		m_alphaToIndex[ a ] = m_nRec;
		m_indexToAlpha[ m_nRec ] = a;
	}

	i = m_nRec;
	m_nRec++;
}

template <class T> void Database<T>::SetEnabling (const sint32 i, const sint32 e)

{
	Assert(0 <= i);
	Assert(i < m_nRec);

	m_rec[i].SetEnabling(e);
}

template <class T> sint32 Database<T>::GetEnabling (const sint32 i) const
{
	Assert(0 <= i);
	Assert(i < m_nRec);

	return m_rec[i].GetEnabling();
}

template <class T> sint32 Database<T>::GetObsolete (const sint32 i, sint32 index) const
{
	 Assert(0 <= i);
	 Assert(i < m_nRec);

	 return m_rec[i].GetObsolete(index);
}

template <class T> void Database<T>::SetObsolete (const sint32 i, const sint32 o, sint32 index)

{
	Assert(0 <= i);
	Assert(i < m_nRec);

	m_rec[i].SetObsolete(o, index);
}

template <class T> sint32 Database<T>::GetNamedItem (const StringId id, sint32 &index) const

{
	sint32 i;

	for (i=0; i<m_nRec; i++) {
		if (m_rec[i].m_name == id) {
			index = i;
			return TRUE;
		}
	}
	return FALSE;
}

template <class T> sint32 Database<T>::GetNamedItemID
(
	sint32 index,
	StringId &id
) const

{
	if ((index < 0) || (index >= m_nRec))
		return FALSE;

	id = m_rec[index].m_name;

	return TRUE;
}












template class Database<EndGameRecord>;
