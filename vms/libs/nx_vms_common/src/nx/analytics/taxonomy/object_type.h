// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <nx/analytics/taxonomy/abstract_object_type.h>
#include <nx/analytics/taxonomy/utils.h>

#include <nx/vms/api/analytics/descriptors.h>

namespace nx::analytics::taxonomy {

struct InternalState;
class ErrorHandler;

template<typename Descriptor, typename AbstractResolvedType, typename ResolvedType>
class BaseObjectEventTypeImpl;

class ObjectType: public AbstractObjectType
{
public:
    ObjectType(
        nx::vms::api::analytics::ObjectTypeDescriptor objectTypeDescriptor,
        QObject* parent = nullptr);

    virtual QString id() const override;

    virtual QString name() const override;

    virtual QString icon() const override;

    virtual AbstractObjectType* base() const override;

    virtual std::vector<AbstractObjectType*> derivedTypes() const override;

    virtual std::vector<AbstractAttribute*> baseAttributes() const override;

    virtual std::vector<AbstractAttribute*> ownAttributes() const override;

    virtual std::vector<AbstractAttribute*> attributes() const override;

    virtual std::vector<AbstractAttribute*> supportedAttributes() const override;

    virtual std::vector<AbstractAttribute*> supportedOwnAttributes() const override;

    virtual bool hasEverBeenSupported() const override;

    virtual bool isPrivate() const override;

    virtual bool isNonIndexable() const override;

    virtual bool isLiveOnly() const override;

    virtual std::vector<AbstractScope*> scopes() const override;

    virtual nx::vms::api::analytics::ObjectTypeDescriptor serialize() const override;

    void addDerivedType(AbstractObjectType* derivedObjectType);

    void resolvePrivateness(bool hasPublicDescendants);

    void resolve(InternalState* inOutInternalState, ErrorHandler* errorHandler);

    void resolveSupportedAttributes(InternalState* inOutInternalState, ErrorHandler* errorHandler);

    void resolvePrivateness();

private:
    void resolveScopes(InternalState* inOutInternalState, ErrorHandler* errorHandler);

private:
    std::shared_ptr<BaseObjectEventTypeImpl<
        nx::vms::api::analytics::ObjectTypeDescriptor,
        AbstractObjectType,
        ObjectType>> m_impl;
};

} // namespace nx::analytics::taxonomy
