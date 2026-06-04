#include "ctp/c3.h"
#include "gs/slic/SlicConst.h"
#include "gs/slic/SlicEngine.h"
#include "gs/diplomacy/diplomacy_types.h"
#include "robot/aibackdoor/civarchive.h"

char *slic_const_test_names[] = {
	"Continue",
	"GetInput",
	"Stop",
};

void slicconst_Initialize()
{
	sint32 i;
	for(i = 0; i < SLIC_CONST_MAX; i++) {
		slicengine_Get()->AddConst(slic_const_test_names[i], i);
	}

	DiplomatTypes::InitializeSlicConsts();
}


