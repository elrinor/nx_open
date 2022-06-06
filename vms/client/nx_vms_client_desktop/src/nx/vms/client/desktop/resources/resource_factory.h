// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <core/resource/resource_factory.h>

namespace nx::vms::client::desktop {

class ResourceFactory: public QnResourceFactory
{
public:
    virtual QnResourcePtr createResource(
        const QnUuid& resourceTypeId,
        const QnResourceParams& params) override;
};

} // namespace nx::vms::client::desktop