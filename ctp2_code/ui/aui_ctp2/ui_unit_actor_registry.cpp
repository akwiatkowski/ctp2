// ui/aui_ctp2/ui_unit_actor_registry.cpp
// See ui_unit_actor_registry.h for design notes.

#include "ctp/c3.h"
#include "ui/aui_ctp2/ui_unit_actor_registry.h"

static UIUnitActorRegistry g_uiUnitActorRegistry;

UIUnitActorRegistry & uiunitactorregistry_Get()
{
    return g_uiUnitActorRegistry;
}

void UIUnitActorRegistry::Insert(Unit unit, UnitActorPtr actor)
{
    if (!actor) {
        return;
    }
    m_actors[unit.m_id] = std::move(actor);
}

void UIUnitActorRegistry::Remove(Unit unit)
{
    m_actors.erase(unit.m_id);
}

UnitActorPtr UIUnitActorRegistry::Get(Unit unit) const
{
    auto const it = m_actors.find(unit.m_id);
    return (it != m_actors.end()) ? it->second : nullptr;
}
