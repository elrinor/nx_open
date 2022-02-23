// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <atomic>
#include <chrono>

#include <QtCore/QDateTime>
#include <QtCore/QObject>

#include <nx/utils/singleton.h>
#include <nx/utils/impl_ptr.h>

namespace ec2 {

class AbstractTimeNotificationManager;
using AbstractTimeNotificationManagerPtr = std::shared_ptr<AbstractTimeNotificationManager>;

} // namespace ec2

namespace nx::vms::common {

class AbstractTimeSyncManager;
using AbstractTimeSyncManagerPtr = std::shared_ptr<AbstractTimeSyncManager>;

} // namespace nx::vms::common

/**
 * Time provider that is synchronized with Server.
 */
class NX_VMS_COMMON_API QnSyncTime final:
    public QObject,
    public Singleton<QnSyncTime>
{
    Q_OBJECT;

public:
    QnSyncTime(QObject *parent = NULL);
    ~QnSyncTime();

    void setTimeSyncManager(nx::vms::common::AbstractTimeSyncManagerPtr timeSyncManager);
    void setTimeNotificationManager(ec2::AbstractTimeNotificationManagerPtr timeNotificationManager);

    qint64 currentMSecsSinceEpoch() const;
    qint64 currentUSecsSinceEpoch() const;
    std::chrono::microseconds currentTimePoint();
    QDateTime currentDateTime() const;

signals:
    /**
     * Emitted whenever time on Server changes. Deprecated. Use AbstractTimeSyncManager instead.
     */
    void timeChanged();

private:
    struct Private;
    nx::utils::ImplPtr<Private> d;
};

#define qnSyncTime (QnSyncTime::instance())