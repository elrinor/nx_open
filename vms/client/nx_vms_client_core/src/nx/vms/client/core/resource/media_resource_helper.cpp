// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "media_resource_helper.h"

#include <core/resource/camera_resource.h>
#include <core/resource/media_server_resource.h>
#include <core/resource/resource_property_key.h>
#include <core/resource_management/resource_pool.h>
#include <nx/fusion/model_functions.h>
#include <utils/common/connective.h>

namespace nx::vms::client::core {

namespace {

static constexpr bool kFisheyeDewarpingFeatureEnabled = true;

} // namespace

class MediaResourceHelper::Private: public Connective<QObject>
{
    MediaResourceHelper* const q = nullptr;

public:
    QnVirtualCameraResourcePtr camera;
    QnMediaServerResourcePtr server;

    Private(MediaResourceHelper* q);

    void handlePropertyChanged(const QnResourcePtr& resource, const QString& key);
    void updateServer();
    void handleResourceChanged();
};

MediaResourceHelper::MediaResourceHelper(QObject* parent):
    base_type(parent),
    d(new Private(this))
{
    connect(this, &ResourceHelper::resourceIdChanged, d, &Private::handleResourceChanged);
}

MediaResourceHelper::~MediaResourceHelper()
{
}

bool MediaResourceHelper::isVirtualCamera() const
{
    return d->camera && d->camera->flags().testFlag(Qn::virtual_camera);
}

bool MediaResourceHelper::audioSupported() const
{
    return d->camera && d->camera->isAudioSupported();
}

MediaPlayer::VideoQuality MediaResourceHelper::livePreviewVideoQuality() const
{
    if (!d->camera)
        return nx::media::Player::LowIframesOnlyVideoQuality;

    static const auto isLowNativeStream =
        [](const CameraMediaStreamInfo& info)
        {
            if (info.transcodingRequired)
                return false;

            static const QSize kMaximalSize(800, 600);
            const auto size = info.getResolution();
            return !size.isEmpty()
                && size.width() <= kMaximalSize.width()
                && size.height() < kMaximalSize.height();
        };

    if (isLowNativeStream(d->camera->streamInfo(QnSecurityCamResource::StreamIndex::secondary)))
        return nx::media::Player::LowVideoQuality;

    // Checks if primary stream is low quality one.
    return isLowNativeStream(d->camera->streamInfo(QnSecurityCamResource::StreamIndex::primary))
        ? nx::media::Player::HighVideoQuality
        : nx::media::Player::LowIframesOnlyVideoQuality;
}

bool MediaResourceHelper::analogCameraWithoutLicense() const
{
    return d->camera && d->camera->isDtsBased() && !d->camera->isScheduleEnabled();
}

QString MediaResourceHelper::serverName() const
{
    return d->server ? d->server->getName() : QString();
}

qreal MediaResourceHelper::customAspectRatio() const
{
    if (!d->camera)
        return 0.0;

    const auto customRatio = d->camera->customAspectRatio();
    return customRatio.isValid() ? customRatio.toFloat() : 0.0;
}

qreal MediaResourceHelper::aspectRatio() const
{
    if (!d->camera)
        return 0.0;

    const auto ratio = d->camera->aspectRatio();
    return ratio.isValid() ? ratio.toFloat() : 0.0;
}

int MediaResourceHelper::customRotation() const
{
    return d->camera ? d->camera->forcedRotation().value_or(0) : 0;
}

int MediaResourceHelper::channelCount() const
{
    return d->camera ? d->camera->getVideoLayout()->channelCount() : 0;
}

QSize MediaResourceHelper::layoutSize() const
{
    return d->camera ? d->camera->getVideoLayout()->size() : QSize(1, 1);
}

QPoint MediaResourceHelper::channelPosition(int channel) const
{
    return d->camera ? d->camera->getVideoLayout()->position(channel) : QPoint();
}

MediaDewarpingParams MediaResourceHelper::fisheyeParams() const
{
    return (d->camera && kFisheyeDewarpingFeatureEnabled)
        ? MediaDewarpingParams(d->camera->getDewarpingParams())
        : MediaDewarpingParams();
}

MediaResourceHelper::Private::Private(MediaResourceHelper* q):
    q(q)
{
}

void MediaResourceHelper::Private::handlePropertyChanged(
    const QnResourcePtr& resource, const QString& key)
{
    if (camera != resource)
        return;

    if (key == QnMediaResource::customAspectRatioKey())
    {
        emit q->customAspectRatioChanged();
        emit q->aspectRatioChanged();
    }
    else if (key == ResourcePropertyKey::kMediaStreams)
    {
        emit q->livePreviewVideoQualityChanged();
        emit q->aspectRatioChanged();
    }
}

void MediaResourceHelper::Private::updateServer()
{
    if (server)
    {
        server->disconnect(this);
        server.clear();
    }

    if (camera)
    {
        server = camera->getParentServer();

        if (server)
        {
            connect(server.data(), &QnMediaServerResource::nameChanged,
                q, &MediaResourceHelper::serverNameChanged);
        }
    }

    emit q->serverNameChanged();
}

void MediaResourceHelper::Private::handleResourceChanged()
{
    const auto currentResource = q->resource();
    const auto cameraResource = currentResource.dynamicCast<QnVirtualCameraResource>();
    if (currentResource && !cameraResource)
    {
        q->setResourceId(QnUuid()); //< We support only camera resources.
        return;
    }

    if (camera)
        camera->disconnect(this);

    updateServer();

    camera = cameraResource;

    if (camera)
    {
        connect(camera, &QnResource::propertyChanged, this, &Private::handlePropertyChanged);
        connect(camera, &QnResource::parentIdChanged, this, &Private::updateServer);

        connect(camera, &QnResource::videoLayoutChanged,
            q, &MediaResourceHelper::videoLayoutChanged);
        connect(camera, &QnResource::mediaDewarpingParamsChanged,
            q, &MediaResourceHelper::fisheyeParamsChanged);
        connect(camera, &QnVirtualCameraResource::scheduleEnabledChanged,
            q, &MediaResourceHelper::analogCameraWithoutLicenseChanged);
        connect(camera, &QnResource::rotationChanged,
            q, &MediaResourceHelper::customRotationChanged);

        updateServer();
    }

    emit q->customAspectRatioChanged();
    emit q->customRotationChanged();
    emit q->resourceStatusChanged();
    emit q->videoLayoutChanged();
    emit q->fisheyeParamsChanged();
    emit q->analogCameraWithoutLicenseChanged();
    emit q->virtualCameraChanged();
    emit q->audioSupportedChanged();
    emit q->livePreviewVideoQualityChanged();
}

} // namespace nx::vms::client::core