//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Slic eyepoint message
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
// - AOM facilitation: set player[0] to the recipient.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/slic/SlicEyePoint.h"

#include <vector>
#include "gs/gameobj/message.h"
#include "gs/core/tiledmap_observer.h"
#include "gs/core/player_view.h"
#include "gs/gameobj/UnitPool.h"
#include "gs/slic/SlicEngine.h"
#include "gs/slic/SlicObject.h"
#include "gs/slic/SlicSegment.h"
#include "gs/core/render_observer.h"

#ifdef _BAD_EYE
PointerList<SlicEyePoint> s_deletedEyepoints;
#endif

SlicEyePoint::SlicEyePoint()
:   m_point     (),
    m_message   (new Message()),
    m_data      (0),
    m_unit      (),
    m_recipient (PLAYER_INDEX_INVALID),
    m_segment   (nullptr),
    m_type      (EYE_POINT_TYPE_NOTHING)
{
}

SlicEyePoint::SlicEyePoint(const MapPoint &point, const MBCHAR *name,
						   sint32 data, EYE_POINT_TYPE type,
						   const Unit &unit,
						   sint32 recipient,
						   SlicSegment *segment)
:   m_point     (point),
    m_name      (name ? name : ""),
    m_message   (new Message()),
    m_data      (data),
    m_type      (type),
    m_unit      (unit),
    m_recipient (recipient),
    m_segment   (segment)
{
}

SlicEyePoint::SlicEyePoint(SlicEyePoint *copy)
:   m_point     (copy->m_point),
    m_name      (copy->m_name),
    m_message   (new Message(*copy->m_message)),
    m_data      (copy->m_data),
    m_type      (copy->m_type),
    m_unit      (copy->m_unit),
    m_recipient (copy->m_recipient),
    m_segment   (copy->m_segment)
{
}

SlicEyePoint::~SlicEyePoint()
{
#ifdef _BAD_EYE
	s_deletedEyepoints.AddTail(this);
#endif

	
		delete m_message;
}

void SlicEyePoint::SetMessage(const Message &message)
{
#ifdef _BAD_EYE
	Assert(!s_deletedEyepoints.Find(this));
#endif

	*m_message = message;
}

Message SlicEyePoint::GetMessage() const
{
	return *m_message;
}

void SlicEyePoint::Callback()
{
	SlicObject * obj = nullptr;
	if (m_segment)
    {
		obj = new SlicObject(m_segment);
		obj->AddRecipient(m_recipient);
        obj->AddPlayer(m_recipient);
	}

	switch(m_type) {
		case EYE_POINT_TYPE_UNIT:
		case EYE_POINT_TYPE_CITY:
		case EYE_POINT_TYPE_GENERIC:
			if(m_unit.m_id != (0)) {
				if(unitpool_Get()->IsValid(m_unit) &&
				   m_unit.GetOwner() == player_view::VisiblePlayer()) {
					player_view::SetSelectUnit(m_unit);
					render_observer::AddCenterMap(m_point);
					if(obj)
						obj->AddUnit(m_unit);
				}
			} else {
				render_observer::AddCenterMap(m_point);
				if(obj)
					obj->AddLocation(m_point);
			}
			tiledmap_observer::Refresh();
			tiledmap_observer::InvalidateMap();
			break;
		case EYE_POINT_TYPE_ADVANCE:
			Assert(*m_message != Message());
			if(*m_message != Message()) {
				m_message->SetSelectedAdvance(m_data);
			}
			if(obj)
				obj->AddAdvance(m_data);
			break;
        case EYE_POINT_TYPE_NOTHING:
            break;
		default:
			Assert(FALSE);
	}

	if(obj) {
		Message oldMsg = slicengine_Get()->GetEyepointMessage();
		slicengine_Get()->SetEyepointMessage(*m_message);
		slicengine_Get()->Execute(obj);

		slicengine_Get()->SetEyepointMessage(oldMsg);
	}
}
