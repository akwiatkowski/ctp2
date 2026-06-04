#include "ctp/c3.h"
#include "gs/slic/SlicRecord.h"
#include "gs/slic/SlicEngine.h"
#include "gs/gameobj/MessagePool.h"
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

SlicRecord::~SlicRecord()
= default;

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
