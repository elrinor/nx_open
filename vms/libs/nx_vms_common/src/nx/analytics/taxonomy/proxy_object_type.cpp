// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "proxy_object_type.h"

#include <nx/analytics/taxonomy/utils.h>

#include "proxy_attribute.h"

namespace nx::analytics::taxonomy {

ProxyObjectType::ProxyObjectType(
    AbstractObjectType* proxiedObjectType,
    std::map<QString, AttributeSupportInfoTree> attributeSupportInfoTree)
    :
    AbstractObjectType(proxiedObjectType),
    m_proxiedObjectType(proxiedObjectType),
    m_attributeSupportInfoTree(std::move(attributeSupportInfoTree))
{
}

QString ProxyObjectType::id() const
{
    return m_proxiedObjectType->id();
}

QString ProxyObjectType::name() const
{
    return m_proxiedObjectType->name();
}

QString ProxyObjectType::icon() const
{
    return m_proxiedObjectType->icon();
}

AbstractObjectType* ProxyObjectType::base() const
{
    return m_proxiedObjectType->base();
}

std::vector<AbstractObjectType*> ProxyObjectType::derivedTypes() const
{
    return m_proxiedObjectType->derivedTypes();
}

std::vector<AbstractAttribute*> ProxyObjectType::baseAttributes() const
{
    return m_proxiedObjectType->baseAttributes();
}

std::vector<AbstractAttribute*> ProxyObjectType::ownAttributes() const
{
    return m_proxiedObjectType->ownAttributes();
}

std::vector<AbstractAttribute*> ProxyObjectType::supportedAttributes() const
{
    if (m_supportedAttributes)
        return *m_supportedAttributes;

    m_supportedAttributes = std::vector<AbstractAttribute*>();
    for (AbstractAttribute* attribute: this->attributes())
    {
        if (m_attributeSupportInfoTree.contains(attribute->name()))
        {
            auto proxyAttribute = new ProxyAttribute(
                attribute,
                m_attributeSupportInfoTree[attribute->name()]);

            m_supportedAttributes->push_back(proxyAttribute);
        }
    }

    return *m_supportedAttributes;
}

std::vector<AbstractAttribute*> ProxyObjectType::supportedOwnAttributes() const
{
    if (m_supportedOwnAttributes)
        return *m_supportedOwnAttributes;

    m_supportedOwnAttributes = std::vector<AbstractAttribute*>();
    for (AbstractAttribute* attribute: this->ownAttributes())
    {
        if (m_attributeSupportInfoTree.contains(attribute->name()))
        {
            auto proxyAttribute = new ProxyAttribute(
                attribute,
                m_attributeSupportInfoTree[attribute->name()]);

            m_supportedOwnAttributes->push_back(proxyAttribute);
        }
    }

    return *m_supportedOwnAttributes;
}

std::vector<AbstractAttribute*> ProxyObjectType::attributes() const
{
    return m_proxiedObjectType->attributes();
}

bool ProxyObjectType::hasEverBeenSupported() const
{
    return m_proxiedObjectType->hasEverBeenSupported();
}

bool ProxyObjectType::isPrivate() const
{
    return m_proxiedObjectType->isPrivate();
}

bool ProxyObjectType::isLiveOnly() const
{
    return m_proxiedObjectType->isLiveOnly();
}

bool ProxyObjectType::isNonIndexable() const
{
    return m_proxiedObjectType->isNonIndexable();
}

std::vector<AbstractScope*> ProxyObjectType::scopes() const
{
    return m_proxiedObjectType->scopes();
}

nx::vms::api::analytics::ObjectTypeDescriptor ProxyObjectType::serialize() const
{
    return m_proxiedObjectType->serialize();
}

} // namespace nx::analytics::taxonomy
