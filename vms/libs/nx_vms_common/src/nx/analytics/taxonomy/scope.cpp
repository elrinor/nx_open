// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "scope.h"

#include <nx/analytics/taxonomy/engine.h>
#include <nx/analytics/taxonomy/group.h>

namespace nx::analytics::taxonomy {

Scope::Scope(QObject* parent):
    AbstractScope(parent)
{
}

AbstractEngine* Scope::engine() const
{
    return m_engine;
}

AbstractGroup* Scope::group() const
{
    return m_group;
}

bool Scope::isEmpty() const
{
    return m_engine == nullptr && m_group == nullptr;
}

void Scope::setEngine(Engine* engine)
{
    m_engine = engine;
}

void Scope::setGroup(Group* group)
{
    m_group = group;
}

} // namespace nx::analytics::taxonomy