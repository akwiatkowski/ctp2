#include "ctp/c3.h"
#include "gs/slic/QuickSlic.h"
#include <memory>

#include "gs/slic/SlicObject.h"
#include "gs/slic/SlicEngine.h"
#include "gs/slic/SlicSegment.h"




void QuickSlic(char const * id, sint32 recipient)
{
    if (slicengine_Get()->GetSegment(id) &&
        !slicengine_Get()->GetSegment(id)->HasBeenShown(recipient)
       )
    {
        auto so = std::make_unique<SlicObject>(id);
        so->AddRecipient(recipient);
        slicengine_Get()->Execute(std::move(so));
    }
}
