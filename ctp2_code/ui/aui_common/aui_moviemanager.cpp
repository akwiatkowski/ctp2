#include "ctp/c3.h"
#include "ui/aui_common/aui_moviemanager.h"


aui_MovieManager::aui_MovieManager(bool init)
:
    aui_Base		(),
    m_movieResource     (init ? std::make_unique<aui_Resource<aui_Movie>>() : nullptr)
{
}

aui_MovieManager::~aui_MovieManager() = default;
