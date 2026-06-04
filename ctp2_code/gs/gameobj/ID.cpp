#include "ctp/c3.h"

#include "gs/gameobj/ID.h"
#include "robot/aibackdoor/civarchive.h"

uint32 ID_ID_GetVersion()
	{
	return (k_ID_VERSION_MAJOR<<16 | k_ID_VERSION_MINOR) ;
	}
