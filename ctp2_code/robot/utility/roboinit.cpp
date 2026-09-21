//----------------------------------------------------------------------------
//
// Project      : Call To Power 2
// File type    : C++ source
// Description  : Robot initialization and cleanup
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
// - Cleaned up code.
//
//----------------------------------------------------------------------------

#include "ctp/c3.h"
#include "robot/utility/RoboInit.h"

#include <memory>

#include "robot/pathing/UnitAstar.h"
#include "ai/ctpai.h"
#include "ai/diplomacy/AgreementMatrix.h"
#include "ai/mapanalysis/mapanalysis.h"

std::unique_ptr<UnitAstar> g_theUnitAstar;

void roboinit_Initalize()
{
	Astar_Init();

    g_theUnitAstar = std::make_unique<UnitAstar>();
}

void roboinit_Cleanup()
{
    g_theUnitAstar.reset();

	Astar_Cleanup();
}
