// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <functional>
#include <map>
#include <string>

#include <nx/kit/json.h>
#include <nx/sdk/i_string.h>
#include <nx/sdk/i_string_map.h>
#include <nx/sdk/result.h>

namespace nx {
namespace vms_server_plugins {
namespace analytics {
namespace stub {
namespace settings {

class ActiveSettingsBuilder
{
public:
    using ActiveSettingHandler = std::function<void(
        nx::kit::Json* /*inOutModel*/,
        std::map<std::string, std::string>* /*inOutValues*/)>;

    struct ActiveSettingKey
    {
        std::string activeSettingId;
        std::string activeSettingValue;

        bool operator<(const ActiveSettingKey& other) const;
    };

public:
    ActiveSettingsBuilder() = default;

    void addRule(
        const std::string& activeSettingId,
        const std::string& activeSettingValue,
        ActiveSettingHandler activeSettingHandler);

    void addDefaultRule(
        const std::string& activeSettingId,
        ActiveSettingHandler activeSettingHandler);

    void updateSettings(
        const std::string& activeSettingId,
        nx::kit::Json* inOutSettingsModel,
        std::map<std::string, std::string>* inOutSettingsValues);

private:
    std::map<ActiveSettingKey, ActiveSettingHandler> m_rules;
    std::map</*activeSettingId*/ std::string, ActiveSettingHandler> m_defaultRules;
};

} // namespace settings
} // namespace stub
} // namespace analytics
} // namespace vms_server_plugins
} // namespace nx
