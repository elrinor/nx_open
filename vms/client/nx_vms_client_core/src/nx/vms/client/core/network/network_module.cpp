// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "network_module.h"

#include <memory>

#include <QtCore/QStandardPaths>

#include <common/common_module.h>
#include <nx/utils/log/assert.h>
#include <nx/utils/thread/mutex.h>

#include "certificate_storage.h"
#include "certificate_verifier.h"
#include "remote_connection.h"
#include "remote_connection_factory.h"
#include "remote_session.h"
#include "remote_session_timeout_watcher.h"
#include "server_certificate_watcher.h"

namespace nx::vms::client::core {

struct NetworkModule::Private
{
    /** Storage for certificates which were actually used for connection. */
    std::unique_ptr<CertificateStorage> connectionCertificatesStorage;

    /** Storage for server auto-generated certificates. */
    std::unique_ptr<CertificateStorage> autoGeneratedCertificatesStorage;

    std::unique_ptr<CertificateVerifier> certificateVerifier;
    std::unique_ptr<RemoteConnectionFactory> connectionFactory;
    std::unique_ptr<ServerCertificateWatcher> serverCertificateWatcher;
    std::shared_ptr<RemoteSession> session;
    std::unique_ptr<RemoteSessionTimeoutWatcher> sessionTimeoutWatcher;

    mutable nx::Mutex mutex;
};

NetworkModule::NetworkModule(
    QnCommonModule* commonModule,
    nx::vms::api::PeerType peerType,
    Qn::SerializationFormat serializationFormat,
    ServerCertificateValidationLevel certificateValidationLevel)
    :
    d(new Private)
{
    NX_CRITICAL(commonModule, "Initialization order error");

    const QString rootCertificatesPath =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/certificates";

    d->connectionCertificatesStorage = std::make_unique<CertificateStorage>(
        rootCertificatesPath + "/connection", certificateValidationLevel);
    d->autoGeneratedCertificatesStorage = std::make_unique<CertificateStorage>(
        rootCertificatesPath + "/autogenerated", certificateValidationLevel);

    d->certificateVerifier = std::make_unique<CertificateVerifier>(
        d->connectionCertificatesStorage.get(),
        d->autoGeneratedCertificatesStorage.get(),
        commonModule);
    commonModule->setCertificateVerifier(d->certificateVerifier.get());

    d->connectionFactory = std::make_unique<RemoteConnectionFactory>(
        commonModule,
        d->certificateVerifier.get(),
        peerType,
        serializationFormat);

    d->serverCertificateWatcher = std::make_unique<ServerCertificateWatcher>(
        commonModule,
        d->certificateVerifier.get());

    d->sessionTimeoutWatcher = std::make_unique<RemoteSessionTimeoutWatcher>(
        commonModule->globalSettings());
}

NetworkModule::~NetworkModule()
{
    // Stop running session before network module is completely destroyed.
    NX_MUTEX_LOCKER lock(&d->mutex);
    d->session.reset();
}

CertificateVerifier* NetworkModule::certificateVerifier() const
{
    return d->certificateVerifier.get();
}

std::shared_ptr<RemoteSession> NetworkModule::session() const
{
    NX_MUTEX_LOCKER lock(&d->mutex);
    return d->session;
}

void NetworkModule::setSession(std::shared_ptr<RemoteSession> session)
{
    // This is necessary to prolong the life of the old session, otherwise a deadlock happen.
    // This is due to the connectionClose signal in the messageProcessor, which is emitted
    // by ~RemoteSession.
    auto tmpSession = d->session;
    d->certificateVerifier->setSession(session);
    d->sessionTimeoutWatcher->sessionStopped();
    {
        NX_MUTEX_LOCKER lock(&d->mutex);
        d->session = session;
    }
    if (session)
        d->sessionTimeoutWatcher->sessionStarted(session);

    emit remoteIdChanged(currentServerId());
}

QnUuid NetworkModule::currentServerId() const
{
    NX_MUTEX_LOCKER lock(&d->mutex);
    if (d->session && d->session->connection())
        return d->session->connection()->moduleInformation().id;

    return QnUuid();
}

bool NetworkModule::isConnected() const
{
    NX_MUTEX_LOCKER lock(&d->mutex);
    if (!d->session)
        return false;

    return NX_ASSERT(d->session->connection());
}

RemoteConnectionFactory* NetworkModule::connectionFactory() const
{
    return d->connectionFactory.get();
}

void NetworkModule::reinitializeCertificateStorage(
    ServerCertificateValidationLevel certificateValidationLevel)
{
    d->connectionCertificatesStorage->reinitialize(certificateValidationLevel);
    d->autoGeneratedCertificatesStorage->reinitialize(certificateValidationLevel);
}

RemoteSessionTimeoutWatcher* NetworkModule::sessionTimeoutWatcher() const
{
    return d->sessionTimeoutWatcher.get();
}

} // namespace nx::vms::client::core