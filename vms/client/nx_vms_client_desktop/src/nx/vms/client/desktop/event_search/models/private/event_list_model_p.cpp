// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "event_list_model_p.h"

#include <QtGui/QDesktopServices>

#include <core/resource/camera_resource.h>
#include <nx/utils/log/assert.h>
#include <nx/vms/client/desktop/system_context.h>
#include <ui/workbench/workbench_access_controller.h>

namespace nx::vms::client::desktop {

EventListModel::Private::Private(EventListModel* q):
    base_type(),
    q(q)
{
}

EventListModel::Private::~Private()
{
}

int EventListModel::Private::count() const
{
    return m_events.count();
}

bool EventListModel::Private::addFront(const EventData& data)
{
    if (m_events.contains(data.id))
        return false;

    ScopedInsertRows insertRows(q, 0, 0);
    m_events.push_front(data.id, data);
    return true;
}

bool EventListModel::Private::addBack(const EventData& data)
{
    if (m_events.contains(data.id))
        return false;

    ScopedInsertRows insertRows(q,  m_events.size(), m_events.size());
    m_events.push_back(data.id, data);
    return true;
}

bool EventListModel::Private::removeEvent(const QnUuid& id)
{
    const auto index = m_events.index_of(id);
    if (index < 0)
        return false;

    ScopedRemoveRows removeRows(q,  index, index);
    m_events.removeAt(index);
    return true;
}

void EventListModel::Private::removeEvents(int first, int count)
{
    NX_ASSERT(first >= 0 && count >= 0 && first + count <= m_events.size(),
        "Rows are out of range");

    if (count == 0)
        return;

    ScopedRemoveRows removeRows(q,  first, first + count - 1);

    for (int i = 0; i < count; ++i)
        m_events.removeAt(first);
}

int EventListModel::Private::indexOf(const QnUuid& id) const
{
    return m_events.index_of(id);
}

void EventListModel::Private::clear()
{
    ScopedReset reset(q, !m_events.empty());
    m_events.clear();
}

bool EventListModel::Private::updateEvent(const EventData& data)
{
    const auto index = m_events.index_of(data.id);
    if (index < 0)
        return false;

    m_events[index] = data;

    const auto modelIndex = q->index(index);
    emit q->dataChanged(modelIndex, modelIndex);

    return true;
}

const EventListModel::EventData& EventListModel::Private::getEvent(int index) const
{
    if (index < 0 || index >= m_events.count())
    {
        NX_ASSERT(false, "Index is out of range");
        static EventData dummy;
        return dummy;
    }

    return m_events[index];
}

QnVirtualCameraResourcePtr EventListModel::Private::previewCamera(const EventData& event) const
{
    if (!event.previewCamera)
        return {};

    auto systemContext = SystemContext::fromResource(event.previewCamera);
    auto accessController = systemContext->accessController();
    const bool hasAccess =
        accessController->hasPermissions(event.previewCamera, Qn::ViewContentPermission)
        && accessController->hasGlobalPermission(GlobalPermission::viewArchive);
    return hasAccess ? event.previewCamera : QnVirtualCameraResourcePtr();
}

QnVirtualCameraResourceList EventListModel::Private::accessibleCameras(const EventData& event) const
{
    return event.cameras.filtered(
        [this](const QnVirtualCameraResourcePtr& camera) -> bool
        {
            auto systemContext = SystemContext::fromResource(camera);
            auto accessController = systemContext->accessController();
            return accessController->hasPermissions(camera, Qn::ViewContentPermission);
        });
}

} // namespace nx::vms::client::desktop
