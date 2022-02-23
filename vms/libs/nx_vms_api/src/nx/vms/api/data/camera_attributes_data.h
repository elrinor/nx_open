// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <vector>

#include <QtCore/QString>

#include <nx/utils/latin1_array.h>
#include <nx/utils/uuid.h>
#include <nx/vms/api/types/motion_types.h>
#include <nx/vms/api/types/resource_types.h>

#include "check_resource_exists.h"
#include "data_macros.h"

namespace nx {
namespace vms {
namespace api {

struct NX_VMS_API ScheduleTaskData
{
    /**%apidoc[opt] Time of day when the backup starts (in seconds passed from 00:00:00). */
    int startTime = 0;

    /**%apidoc[opt] Time of day when the backup ends (in seconds passed from 00:00:00). */
    int endTime = 0;

    RecordingType recordingType = RecordingType::always;

    /**%apidoc[opt] Weekday for the recording task.
     *     %value 1 Monday
     *     %value 2 Tuesday
     *     %value 3 Wednesday
     *     %value 4 Thursday
     *     %value 5 Friday
     *     %value 6 Saturday
     *     %value 7 Sunday
     */
    qint8 dayOfWeek = 1;

    /**%apidoc[opt] Quality of the recording. */
    StreamQuality streamQuality = StreamQuality::undefined;

    /**%apidoc[opt] Frames per second. */
    int fps = 0;

    /**%apidoc[opt] Bitrate. */
    int bitrateKbps = 0;

    /**%apidoc[opt] Types of metadata, which should trigger recording in case of corresponding
     * `recordingType` values.
     */
    RecordingMetadataTypes metadataTypes;

    bool operator==(const ScheduleTaskData& other) const = default;
};
#define ScheduleTaskData_Fields \
    (startTime) \
    (endTime) \
    (recordingType) \
    (dayOfWeek) \
    (streamQuality) \
    (fps) \
    (bitrateKbps) \
    (metadataTypes)

//-------------------------------------------------------------------------------------------------

static constexpr int kDefaultMinArchiveDays = 1;
static constexpr int kDefaultMaxArchiveDays = 30;
static constexpr int kDefaultRecordBeforeMotionSec = 5;
static constexpr int kDefaultRecordAfterMotionSec = 5;

/**%apidoc Defines content to backup. It is the set of flags. */
NX_REFLECTION_ENUM_CLASS(BackupContentType,
    archive = 1 << 0,
    motion = 1 << 1,
    analytics = 1 << 2,
    bookmarks = 1 << 3
)

NX_REFLECTION_ENUM_CLASS(BackupPolicy,
    byDefault = -1,
    off = 0,
    on = 1
)

Q_DECLARE_FLAGS(BackupContentTypes, BackupContentType)
Q_DECLARE_OPERATORS_FOR_FLAGS(BackupContentTypes)

/**%apidoc Additional device attributes. */
struct NX_VMS_API CameraAttributesData
{
    QnUuid getIdForMerging() const { return cameraId; } //< See IdData::getIdForMerging().
    bool operator==(const CameraAttributesData& other) const = default;

    static DeprecatedFieldNames* getDeprecatedFieldNames();

    QnUuid cameraId;

    /**%apidoc[opt] Device name. */
    QString cameraName;

    /**%apidoc[opt] Name of the user-defined device group. */
    QString userDefinedGroupName;

    /**%apidoc[opt] Whether recording to the archive is enabled for the device. */
    bool scheduleEnabled = false;

    /**%apidoc[opt] Whether the license is used for the device. */
    bool licenseUsed = false; //< TODO: #sivanov Field is not used.

    /**%apidoc[opt] Type of motion detection method. */
    MotionType motionType = MotionType::default_;

    /**%apidoc[opt] List of motion detection areas and their sensitivity. The format is
     * proprietary and is likely to change in future API versions. Currently, this string defines
     * several rectangles separated with ":", each rectangle is described by 5 comma-separated
     * numbers: sensitivity, x and y (for left top corner), width, height.
     */
    QnLatin1Array motionMask;

    /**%apidoc[opt] List of scheduleTask objects which define the device recording schedule. */
    std::vector<ScheduleTaskData> scheduleTasks;

    /**%apidoc[opt] Whether audio is enabled on the device. */
    bool audioEnabled = false;

    /**%apidoc[opt] Whether dual streaming is enabled if it is supported by device. */
    bool disableDualStreaming = false; //< TODO: #sivanov Double negation.

    /**%apidoc[opt] Whether server manages the device (changes resolution, FPS, create profiles,
     * etc).
     */
    bool controlEnabled = true;

    /**%apidoc[opt] Image dewarping parameters. The format is proprietary and is likely to change
     * in future API versions.
     */
    QnLatin1Array dewarpingParams;

    /**%apidoc[opt] Minimum number of days to keep the archive for. If the value is less than or
     * equal to zero, it is not used.
     */
    int minArchiveDays = -kDefaultMinArchiveDays; //< Negative means 'auto'.

    /**%apidoc[opt] Maximum number of days to keep the archive for. If the value is less than or
     * equal zero, it is not used.
     */
    int maxArchiveDays = -kDefaultMaxArchiveDays; //< Negative means 'auto'.

    /**%apidoc[opt] Unique id of a server which has the highest priority of hosting the device for
     * failover (if the current server fails).
     */
    QnUuid preferredServerId;

    /**%apidoc[opt] Priority for the device to be moved to another server for failover (if the
     * current server fails).
     */
    FailoverPriority failoverPriority = FailoverPriority::medium;

    /**%apidoc[opt] Combination (via "|") of the flags defining backup options. */
    CameraBackupQuality backupQuality = CameraBackup_Default;

    /**%apidoc[opt] Logical id of the device, set by user. */
    QString logicalId;

    /**%apidoc[opt] The number of seconds before a motion event to record the video for. */
    int recordBeforeMotionSec = kDefaultRecordBeforeMotionSec;
    /**%apidoc[opt] The number of seconds after a motion event to record the video for. */
    int recordAfterMotionSec = kDefaultRecordAfterMotionSec;
    BackupContentTypes backupContentType = BackupContentType::archive; //< What to backup content wise.
    BackupPolicy backupPolicy = BackupPolicy::byDefault;

    /**%apidoc[readonly] */
    QString dataAccessId;

    /** Used by ...Model::toDbTypes() and transaction-description-modify checkers. */
    CheckResourceExists checkResourceExists = CheckResourceExists::yes; /**<%apidoc[unused] */
};
#define CameraAttributesData_Fields_Short \
    (userDefinedGroupName) \
    (scheduleEnabled) \
    (licenseUsed) \
    (motionType) \
    (motionMask) \
    (scheduleTasks) \
    (audioEnabled) \
    (disableDualStreaming)\
    (controlEnabled) \
    (dewarpingParams) \
    (minArchiveDays) \
    (maxArchiveDays) \
    (preferredServerId) \
    (failoverPriority) \
    (backupQuality) \
    (logicalId) \
    (recordBeforeMotionSec) \
    (recordAfterMotionSec) \
    (backupContentType) \
    (backupPolicy) \
    (dataAccessId)

#define CameraAttributesData_Fields (cameraId)(cameraName) CameraAttributesData_Fields_Short

NX_VMS_API_DECLARE_STRUCT(ScheduleTaskData)
NX_VMS_API_DECLARE_STRUCT_AND_LIST(CameraAttributesData)

} // namespace api
} // namespace vms
} // namespace nx

Q_DECLARE_METATYPE(nx::vms::api::ScheduleTaskData)
Q_DECLARE_METATYPE(nx::vms::api::BackupContentType)
Q_DECLARE_METATYPE(nx::vms::api::CameraAttributesData)
Q_DECLARE_METATYPE(nx::vms::api::CameraAttributesDataList)
