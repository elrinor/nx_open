// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <map>
#include <memory>

#include <QObject>

#include "basic_event.h"

namespace nx::vms::rules {

class EventField;

/**
 * Event filters are used to filter events received by VMS rules engine
 * on the basis of event type and field values. Events that passed the filter
 * are proceed by the engine and lead to execution of actions from the same rule.
 */
class NX_VMS_RULES_API /*FieldBased*/EventFilter: public QObject
{
    Q_OBJECT

public:
    EventFilter(const QnUuid& id, const QString& eventType);
    virtual ~EventFilter();

    QnUuid id() const;
    QString eventType() const;

    /**
     * Get all configurable Filter properties in a form of flattened dictionary,
     * where each key has name in a format "field_name/field_property_name".
     */
    std::map<QString, QVariant> flatData() const;

    /**
     * Update single Filter property.
     * @path Path of the property, should has "field_name/field_property_name" format.
     * @value Value that should be assigned to the given property.
     * @return True on success, false if such field does not exist.
     */
    bool updateData(const QString& path, const QVariant& value);

    /**
     * Update several Filter properties at once.
     * Does nothing if any of the keys is invalid (e.g. such field does not exist).
     * @data Dictionary of values.
     * @return True on success, false otherwise.
     */
    bool updateFlatData(const std::map<QString, QVariant>& data);

    // Takes ownership.
    void addField(const QString& name, std::unique_ptr<EventField> field);

    const QHash<QString, EventField*> fields() const;

    bool match(const EventPtr& event) const;

    void connectSignals();

signals:
    void stateChanged();

private:
    void updateState();

private:
    QnUuid m_id;
    QString m_eventType;
    std::map<QString, std::unique_ptr<EventField>> m_fields;
    bool m_updateInProgress = false;
};

} // namespace nx::vms::rules