//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Game event
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
// - Improved slic event debugging. (7-Nov-2007 Martin G�hmann)
// - An event is not executed if its arguments became invalid during
//   code execution. (7-Nov-2007 Martin G�hmann)
// - Corrected delete operator use in destructor.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "gs/events/GameEvent.h"

#include "gs/events/GameEventArgList.h"
#include "gs/events/GameEventManager.h"
#include "ctp/crash_handler.h"   // gevmanager_Get()

#include "gs/slic/SlicEngine.h"
#include "gs/slic/SlicObject.h"
#include "gs/slic/SlicFrame.h"
#include "gs/slic/SlicSegment.h"

GameEvent::GameEvent
(
    GAME_EVENT          type,
    GameEventArgList *  args,
    sint32              serial,
    GAME_EVENT          addedDuring
)
:
	m_type              (type),
	m_argList           (args),
	m_resumeIndex       (-1),   // Handling not started yet
	m_addedDuring       (addedDuring),
	m_serial            (serial),
	m_line              (-1)   // Line invalid
{
	/// @todo Improve code style
	// Instead of accessing global variables (which may or may not exist),
	// better pass the data as arguments.

	if(slicengine_Get()->GetContext())
	{
		m_line = slicengine_Get()->GetContext()->GetFrame()->GetCurrentLine();

		const char * file = slicengine_Get()->GetContext()->GetFrame()->GetSlicSegment()->GetFilename();
		const char * name = slicengine_Get()->GetContext()->GetFrame()->GetSlicSegment()->GetName();

		m_file        = file;
		m_contextName = name;
	}
	else if(gevmanager_Get()->GetHeadEvent()
	&&      gevmanager_Get()->GetHeadEvent()->GetLine() >= 0
	){
		GameEvent * event = gevmanager_Get()->GetHeadEvent();

		m_line = event->GetLine();

		const char * file = event->GetFile();
		const char * name = event->GetContextName();

		m_file        = file;
		m_contextName = name;
	}
}

GameEvent::~GameEvent() = default;

GAME_EVENT_ERR GameEvent::Process()
{
	// Feed the crash reporter's "what was the engine doing" ring.
	crash_handler::NoteEvent(GameEventManager::GetEventName(m_type));

	/// @todo Check whether there are valid situations when the data became invalid.
	// When a unit is killed or consumed in the main execution phase of the event,
	// its ID may have become invalid when reaching the 'post' execution phase.
	if(m_argList->TestArgs(m_type, this))
	{
		return gevmanager_Get()->ActivateHook
		            (m_type, m_argList.get(), m_resumeIndex, m_resumeIndex);
	}
	else
	{
		return GEV_ERR_ArgsInvalid;
	}
}
