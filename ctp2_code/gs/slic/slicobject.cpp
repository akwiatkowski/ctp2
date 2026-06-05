//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Slic objects
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
// _BAD_BUTTON
// - Tracts a list of deleted buttons.
//
//----------------------------------------------------------------------------
//
// Modifications from the original Activision code:
//
// - The finish method just deletes the MessageData from the asscicated
//   SlicFrame as the SlicFrame may be used later. (Sep. 24th 2006 Martin G�hmann)
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/slic/SlicObject.h"

#include "gs/slic/SlicEngine.h"
#include "gs/slic/SlicSegment.h"
#include "robot/aibackdoor/dynarr.h"
#include "gs/utility/UnitDynArr.h"
#include "gs/world/MapPoint.h"
#include "gs/gameobj/Unit.h"
#include "gs/gameobj/Civilisation.h"
#include "gs/gameobj/Advances.h"
#include "gs/slic/SlicFrame.h"
#include "gs/gameobj/MessageData.h"
#include "gs/gameobj/MessagePool.h"
#include "gs/gameobj/Player.h"
#include "gs/slic/SlicButton.h"
#include "net/general/network.h"
#include "ctp/civapp.h"
#include "gs/core/game_observer.h"          // gameobservers_Get()
#include "gs/gameobj/TradeBids.h"
#include "gs/utility/stringutils.h"
#include "gs/utility/TurnCnt.h"            // turn_Get()
#include "gs/utility/Globals.h"
#include <vector>


namespace
{
    sint32 const            INDEX_INVALID   = -1;
}

#ifdef _PLAYTEST
sint32 g_robotMessages = FALSE;
#endif

sint32 g_hackInstantMessages = 0;

SlicObject::SlicObject()
:
    SlicContext             (),
    m_refCount              (0),
    m_id                    (),
	m_segment               (nullptr),
	m_frame                 (nullptr),
	m_seconds               (1),
	m_recipientList         (nullptr),
	m_numRecipients         (0),
	m_request               (nullptr),
	m_defaultAdvanceSet     (FALSE),
	m_defaultAdvance        (INDEX_INVALID),
	m_aborted               (FALSE),
	m_instantMessage        (FALSE),
	m_index                 (INDEX_INVALID),
	m_class                 (k_NON_TUTORIAL_MESSAGE_CLASS),
	m_dontSave              (FALSE),
    m_closeDisabled         (FALSE),
	m_isDiplomaticResponse  (FALSE),
	m_useDirector           (FALSE),
	m_result                (0),
	m_argList               (nullptr)
{
}

SlicObject::SlicObject(char const * id)
:
    SlicContext             (),
    m_refCount              (0),
    m_id                    (id ? id : ""),
	m_segment               (nullptr),
	m_frame                 (nullptr),
	m_seconds               (1),
	m_recipientList         (nullptr),
	m_numRecipients         (0),
	m_request               (new ID),
	m_defaultAdvanceSet     (FALSE),
	m_defaultAdvance        (INDEX_INVALID),
	m_aborted               (FALSE),
	m_instantMessage        (FALSE),
	m_index                 (INDEX_INVALID),
	m_class                 (k_NON_TUTORIAL_HELP_CLASS),
	m_dontSave              (FALSE),
    m_closeDisabled         (FALSE),
	m_isDiplomaticResponse  (FALSE),
	m_useDirector           (FALSE),
	m_result                (0),
	m_argList               (nullptr)
{
	m_segment   = slicengine_Get()->GetSegment(m_id.c_str());
	m_frame     = new SlicFrame(m_segment);

    if (m_segment && !m_segment->IsHelp())
    {
		m_class = k_NON_TUTORIAL_MESSAGE_CLASS;
	}
}

SlicObject::SlicObject(SlicSegment *segment)
:
    SlicContext             (),
    m_refCount              (0),
    m_id                    ((segment && segment->GetName()) ? segment->GetName() : ""),
	m_segment               (segment),
	m_frame                 (new SlicFrame(segment)),
	m_seconds               (1),
	m_recipientList         (nullptr),
	m_numRecipients         (0),
	m_request               (new ID),
	m_defaultAdvanceSet     (FALSE),
	m_defaultAdvance        (INDEX_INVALID),
	m_aborted               (FALSE),
	m_instantMessage        (FALSE),
	m_index                 (INDEX_INVALID),
	m_class                 (k_NON_TUTORIAL_HELP_CLASS),
	m_dontSave              (FALSE),
    m_closeDisabled         (FALSE),
	m_isDiplomaticResponse  (FALSE),
	m_useDirector           (FALSE),
	m_result                (0),
	m_argList               (nullptr)
{
	if (m_segment && !m_segment->IsHelp())
    {
		m_class = k_NON_TUTORIAL_MESSAGE_CLASS;
	}
}

SlicObject::SlicObject(SlicSegment * segment, SlicObject * copy)
:
    SlicContext             (copy),
    m_refCount              (0),
    m_id                    ((segment && segment->GetName()) ? segment->GetName() : ""),
	m_segment               (segment),
	m_frame                 (new SlicFrame(segment)),
	m_seconds               (1),
	m_recipientList         (nullptr),
	m_numRecipients         (0),
	m_request               (nullptr),
    m_defaultAdvanceSet     (copy->m_defaultAdvanceSet),
    m_defaultAdvance        (copy->m_defaultAdvance),
	m_aborted               (FALSE),
	m_instantMessage        (copy->m_instantMessage),
	m_index                 (INDEX_INVALID),
	m_class                 (copy->m_class),
    m_dontSave              (copy->m_dontSave),
    m_closeDisabled         (copy->m_closeDisabled),
    m_isDiplomaticResponse  (copy->m_isDiplomaticResponse),
    m_useDirector           (copy->m_useDirector),
	m_result                (0),
	m_argList               (nullptr)
{
	m_request               = new ID(*copy->m_request);
}

SlicObject::SlicObject(char const * id, SlicContext *copy)
:
    SlicContext             (copy),
    m_refCount              (0),
    m_id                    (id ? id : ""),
	m_segment               (nullptr),
	m_frame                 (nullptr),
	m_seconds               (1),
	m_recipientList         (nullptr),
	m_numRecipients         (0),
	m_request               (new ID),
	m_defaultAdvanceSet     (FALSE),
	m_defaultAdvance        (INDEX_INVALID),
	m_aborted               (FALSE),
	m_instantMessage        (FALSE),
	m_index                 (INDEX_INVALID),
	m_class                 (k_NON_TUTORIAL_HELP_CLASS),
	m_dontSave              (FALSE),
    m_closeDisabled         (FALSE),
	m_isDiplomaticResponse  (FALSE),
	m_useDirector           (FALSE),
	m_result                (0),
	m_argList               (nullptr)
{
	m_segment   = slicengine_Get()->GetSegment(m_id.c_str());
	m_frame     = new SlicFrame(m_segment);
	if (m_segment && !m_segment->IsHelp())
    {
		m_class = k_NON_TUTORIAL_MESSAGE_CLASS;
	}
}

SlicObject::~SlicObject()
{
	delete [] m_recipientList;
	delete m_frame;
	delete m_request;
}

sint32 SlicObject::AddRef()
{
	Assert(m_refCount >= 0);
	return ++m_refCount;
}

sint32 SlicObject::Release()
{
	--m_refCount;
	if(m_refCount <= 0) {
		Assert(m_refCount == 0);
		if(m_refCount == 0)
			delete this;
		return 0;
	}
	return m_refCount;
}

bool SlicObject::IsValid() const
{
	return m_segment != nullptr;
}

void SlicObject::AddRecipient(const PLAYER_INDEX recip)
{
	m_recipientList = Expand(m_recipientList, m_numRecipients);
	m_recipientList[m_numRecipients++] = recip;
}

void SlicObject::AddAllRecipients()
{
	for(sint32 i = 0; i < k_MAX_PLAYERS; i++) {
		if(player_Get(i) && (!player_Get(i)->m_isDead)) {
			m_recipientList = Expand(m_recipientList, m_numRecipients);
			m_recipientList[m_numRecipients++] = i;
		}
	}
}

void SlicObject::AddAllRecipientsBut(PLAYER_INDEX loser1, PLAYER_INDEX loser2)
{
	for(sint32 i = 0; i < k_MAX_PLAYERS; i++) {
		if(player_Get(i) && (!player_Get(i)->m_isDead) &&
           (i != loser1) && (i != loser2)) {
			m_recipientList = Expand(m_recipientList, m_numRecipients);
			m_recipientList[m_numRecipients++] = i;
		}
	}
}

sint32 SlicObject::GetRecipient(const PLAYER_INDEX recip) const
{
	if(!m_recipientList)
		return -1;
	if(recip >= m_numRecipients)
		return -1;
	return m_recipientList[recip];
}

void SlicObject::Execute()
{
	Assert(!slicengine_Get()->AtBreak());
	if(slicengine_Get()->AtBreak())
		return;

	if(!m_frame) {
		m_frame = new SlicFrame(m_segment);
	}

	if(m_segment->GetType() == SLIC_OBJECT_MESSAGEBOX) {
		m_frame->ClearMessageData();
	} else {
		m_segment->AddSpecials(this);
	}

	if (m_frame->GetMessageData())
    {
		m_frame->GetMessageData()->SetMsgText(m_segment->GetName());
	}

	m_frame->Run();

	if (!slicengine_Get()->AtBreak())
    {
	    Finish();
	}
}

//----------------------------------------------------------------------------
//
// Name       : SlicObject::Finish
//
// Description: Displays any messagebox defined by this SlicObject and then
//              deletes the original MessageData from the associated SlicFrame.
//              The SlicFrame is not deleted as it may be used for error
//              messages later.
//
// Parameters : -
//
// Globals    : Unknown
//
// Returns    : -
//
// Remark(s)  : This gets the MessageData object from m_frame, finishes
//              initializing its fields and uses it to construct a
//              Message object which is then added to messagepool_Get().
//
//              It appears to do this once for each recipient, and it seems
//              to use the same MessageData object every time, so a great
//              deal of the work could be superfluous, and could be moved
//              out of the for loop.
//
//----------------------------------------------------------------------------
void SlicObject::Finish()
{
	if(m_segment->GetType() == SLIC_OBJECT_MESSAGEBOX) {

		if (m_numRecipients == 0 && !civapp_Get()->IsGameLoaded()) {

			MessageData *messageData = m_frame->GetMessageData();

			if(m_segment->IsHelp()) {
				messageData->SetIsHelpBox();
			}

			Message newMessage(messagepool_Get()->NewKey(k_BIT_GAME_OBJ_TYPE_MESSAGE));
			MessageData * newData = new MessageData(newMessage, messageData);
			newData->SetOwner(0);
			newData->SetSlicSegment(m_segment);
			messagepool_Get()->Insert(newData);

			if(newMessage.IsAlertBox()) {
				gameobservers_Get()->NotifyRequestModalMessage(newMessage);
			} else {
				if (gameobservers_Get()) gameobservers_Get()->NotifyMessageShow(newMessage);
			}
		} else {
			for(sint32 i = 0; i < m_numRecipients; i++) {
				m_segment->SetLastShown(m_recipientList[i], turn_Get()->GetRound());

				if(slicengine_Get()->IsMessageClassDisabled(m_class))
					continue;

				if(m_aborted)
					continue;

				if(!network_Get().IsActive() ||
				   network_Get().IsLocalPlayer(m_recipientList[i]) ||
				   (m_request && m_request->m_id != 0)) {

					if(!player_Get(m_recipientList[i]))
						continue;

					if(player_Get(m_recipientList[i])->IsRobot()
					&& *m_request == ID()
#ifdef _PLAYTEST
					&& !g_robotMessages
#endif
					){
						continue;
					}

					MessageData *messageData = m_frame->GetMessageData();

					messageData->SetSlicSegment(m_segment);

					if(m_segment->IsHelp()) {
						messageData->SetIsHelpBox();
					}

					if(m_segment->IsAlert()) {
						messageData->SetIsAlertBox();
					}

					if(m_instantMessage) {
						messageData->SetIsInstantMessage();
					}

					if(m_closeDisabled) {
						messageData->DisableClose(TRUE);
					}

					messageData->SetClass(m_class);

					if(m_isDiplomaticResponse) {
						messageData->SetIsDiplomaticResponse();
					}

					if(true || m_useDirector) {
						messageData->SetUseDirector();
					}

					if(m_segment->GetFilenum() == k_TUTORIAL_FILE &&
					   !m_dontSave) {
						slicengine_Get()->AddTutorialRecord(m_recipientList[i],
														messageData->GetTitle(),
														messageData->GetMsgText(),
														m_segment);
					}
					Message realMessage = messagepool_Get()->
						Create(m_recipientList[i], m_frame->GetMessageData());

					if(GetNumTradeBids() > 0) {
						tradebids_Get()->SetMessage(GetTradeBid(0), realMessage);
					}

					if(m_defaultAdvanceSet) {
						realMessage.SetSelectedAdvance(m_defaultAdvance);
					}

					if(m_segment->IsAlert()) {

					}
				}
			}
		}
	}
	m_frame->DeleteMessageData();
}

#ifdef _DEBUG
void SlicObject::Dump()
{
	DPRINTF(k_DBG_INFO, ("SlicObject: ID '%s'\n", m_id.c_str()));

	SlicContext::Dump();
}
#endif

void SlicObject::SetMessageText(const MBCHAR *text)
{
	Assert(m_frame);
	if(!m_frame)
		return;

	static MBCHAR interpretedText[k_MAX_TEXT_LEN];
	stringutils_Interpret(text, *this, interpretedText, k_MAX_TEXT_LEN);

	m_frame->GetMessageData()->SetMsgText(interpretedText);
}

void SlicObject::SetMessageCaption(const MBCHAR *text)
{
	m_frame->GetMessageData()->SetMsgCaption(text);
}

void SlicObject::SetMessageTitle(const MBCHAR *text)
{
	Assert(m_frame);
	if(!m_frame)
		return;

	static MBCHAR interpretedText[k_MAX_TEXT_LEN];

	stringutils_HackColor(FALSE);
	stringutils_Interpret(text, *this, interpretedText, k_MAX_TEXT_LEN);
	stringutils_HackColor(TRUE);

	m_frame->GetMessageData()->SetTitle(interpretedText);
}

void SlicObject::SetMessageType(MESSAGE_TYPE type,
								MESSAGE_TYPE selectedType)
{
	m_frame->GetMessageData()->SetMsgType(type);
	m_frame->GetMessageData()->SetSelectedMsgType(selectedType);
}

void SlicObject::SetMessageDuration(sint32 duration)
{
	m_frame->GetMessageData()->SetDuration(duration, turn_Get() ? turn_Get()->GetRound() : 0);
}











void SlicObject::AddButton(SlicButton *button)
{
	Assert(m_frame && m_frame->GetMessageData());
	if (m_frame && m_frame->GetMessageData())
    {
    	m_frame->GetMessageData()->AddButton(button);
    }
}

void SlicObject::AddEyePoint(SlicEyePoint *eyepoint)
{
	Assert(m_frame && m_frame->GetMessageData());
	if (m_frame && m_frame->GetMessageData())
    {
	    m_frame->GetMessageData()->AddEyePoint(eyepoint);
    }
}

void SlicObject::SetDefaultAdvance(sint32 adv)
{
	m_defaultAdvanceSet = TRUE;
	m_defaultAdvance = adv;
}

bool SlicObject::ConcernsPlayer(PLAYER_INDEX player) const
{
	for (sint32 i = 0; i < m_numRecipients; i++)
    {
		if (m_recipientList[i] == player)
			return true;
	}

	return SlicContext::ConcernsPlayer(player);
}

void SlicObject::SetFrame(SlicFrame *frame)
{
	delete m_frame;
	m_frame = frame;
}

void SlicObject::Continue()
{
	m_frame->Run();

	if (!slicengine_Get()->AtBreak())
    {
    	Finish();
    }
}
