// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <nx/vms/rules/basic_event.h>

#include "../data_macros.h"

namespace nx::vms::rules {

class NX_VMS_RULES_API DebugEvent: public BasicEvent
{
    Q_OBJECT
    Q_CLASSINFO("type", "nx.events.debug")

    //Q_PROPERTY(QString action READ action)
    //Q_PROPERTY(qint64 value READ value)

public:
    DebugEvent(const QString &action, qint64 value):
        m_action(action),
        m_value(value)
    {
    }

    FIELD(QString, action, setAction)
    FIELD(qint64, value, setValue)

    static FilterManifest filterManifest();

//    QString action() const
//    {
//        return m_action;
//    }
//
//    qint64 value() const
//    {
//        return m_value;
//    }
//
//private:
//    QString m_action;
//    qint64 m_value;
};

} // namespace nx::vms::rules