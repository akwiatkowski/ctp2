#include "ctp/c3.h"
#include "gs/slic/SlicRecord.h"
#include "gs/slic/SlicEngine.h"
#include "gs/gameobj/MessagePool.h"
#include "robot/aibackdoor/civarchive.h"
#include "gs/gameobj/Player.h"
#include "robot/aibackdoor/dynarr.h"
#include "gs/slic/SlicSegment.h"
#include "gs/slic/slicif.h"
#include <vector>

SlicRecord::SlicRecord(sint32 owner, MBCHAR *title, MBCHAR *text,
					   SlicSegment *segment)
{
	m_owner = owner;
	m_title = title ? title : "";
	m_text = text ? text : "";
	m_segment = segment;
}

SlicRecord::SlicRecord(CivArchive &archive)
{
	Serialize(archive);
}

SlicRecord::~SlicRecord()
{
}

void SlicRecord::Serialize(CivArchive &archive)
{
	sint32 l;
	if(archive.IsStoring()) {
		archive << m_owner;
		l = static_cast<sint32>(m_title.size()) + 1;
		archive << l;
		archive.Store((uint8*)m_title.c_str(), l * sizeof(MBCHAR));

		l = static_cast<sint32>(m_text.size()) + 1;
		archive << l;
		archive.Store((uint8*)m_text.c_str(), l * sizeof(MBCHAR));

		l = strlen(m_segment->GetName()) + 1;
		archive << l;
		archive.Store((uint8*)m_segment->GetName(), l);
	} else {
		archive >> m_owner;
		archive >> l;
		if(l < 0) {
			m_title = "";
		} else {
			std::vector<MBCHAR> buf(l);
			archive.Load((uint8*)buf.data(), l * sizeof(MBCHAR));
			m_title.assign(buf.data());
		}

		archive >> l;
		if(l < 0) {
			m_text = "";
		} else {
			std::vector<MBCHAR> buf(l);
			archive.Load((uint8*)buf.data(), l * sizeof(MBCHAR));
			m_text.assign(buf.data());
		}

		archive >> l;
		char segmentName[k_MAX_SLIC_STRING];
		archive.Load((uint8*)segmentName, l);
		m_segment = slicengine_Get()->GetSegment(segmentName);
	}
}

void SlicRecord::Reconstitute()
{
	sint32 i;
	for(i = player_Get(m_owner)->m_messages->Num() - 1; i >= 0; i--) {
		if(player_Get(m_owner)->m_messages->Access(i).GetClass() ==
		   k_HACK_RECONSTITUTED_CLASS) {
			player_Get(m_owner)->m_messages->Access(i).Kill();
		}
	}

	Message msg = messagepool_Get()->Recreate(m_owner, const_cast<MBCHAR*>(m_text.c_str()), const_cast<MBCHAR*>(m_title.c_str()));
	Assert(messagepool_Get()->IsValid(msg));
	if(messagepool_Get()->IsValid(msg)) {
		msg.SetClass(k_HACK_RECONSTITUTED_CLASS);
		msg.Show();
	}
}
