#define MAKE_EVENT_DESCRIPTIONS 1

#include "gs/events/GameEventDescription.h"

GameEventDescription & event_description(sint32 type)
{
    return s_eventDescriptions[type];
}
