// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "resource_tree_item_factory.h"

#include <api/global_settings.h>
#include <client/client_globals.h>
#include <common/common_module.h>
#include <core/resource/camera_resource.h>
#include <core/resource/layout_resource.h>
#include <core/resource/resource.h>
#include <core/resource/resource_display_info.h>
#include <core/resource/user_resource.h>
#include <core/resource_management/resource_pool.h>
#include <core/resource_management/user_roles_manager.h>
#include <nx/vms/api/data/user_role_data.h>
#include <nx/vms/client/desktop/resource_views/entity_item_model/item/generic_item/generic_item_builder.h>
#include <nx/vms/client/desktop/resource_views/entity_resource_tree/item/resource_item.h>
#include <nx/vms/client/desktop/resource_views/entity_resource_tree/item/showreel_item.h>
#include <nx/vms/client/desktop/resource_views/entity_resource_tree/item/videowall_matrix_item.h>
#include <nx/vms/client/desktop/resource_views/entity_resource_tree/item/videowall_screen_item.h>
#include <nx/vms/client/desktop/resource_views/entity_resource_tree/recorder_item_data_helper.h>
#include <nx/vms/client/desktop/resource_views/entity_resource_tree/resource_grouping/resource_grouping.h>
#include <nx/vms/client/desktop/style/resource_icon_cache.h>
#include <ui/help/help_topics.h>
#include <finders/systems_finder.h>

namespace {

using namespace nx::vms::client::desktop::entity_item_model;
using namespace nx::vms::client::desktop::entity_resource_tree;

//-------------------------------------------------------------------------------------------------
// Provider and invalidator pair factory functions for the header system name name generic item.
//-------------------------------------------------------------------------------------------------
GenericItem::DataProvider systemNameProvider(const QnGlobalSettings* globalSettings)
{
    return [globalSettings] { return globalSettings->systemName(); };
}

InvalidatorPtr systemNameInvalidator(const QnGlobalSettings* globalSettings)
{
    auto result = std::make_shared<Invalidator>();

    result->connections()->add(globalSettings->connect(
        globalSettings, &QnGlobalSettings::systemNameChanged, globalSettings,
        [invalidator = result.get()] { invalidator->invalidate(); }));

    return result;
}

//-------------------------------------------------------------------------------------------------
// Provider and invalidator pair factory functions for the header user name generic item.
//-------------------------------------------------------------------------------------------------
GenericItem::DataProvider userNameProvider(const QnUserResourcePtr& user)
{
    return [user] { return user->getName(); };
}

InvalidatorPtr resourceNameInvalidator(const QnResourcePtr& resource)
{
    auto result = std::make_shared<Invalidator>();

    result->connections()->add(resource->connect(
        resource.get(), &QnResource::nameChanged,
        [invalidator = result.get()] { invalidator->invalidate(); }));

    return result;
}

//-------------------------------------------------------------------------------------------------
// Provider and invalidator pair factory functions for a recorder group name generic item.
//-------------------------------------------------------------------------------------------------
GenericItem::DataProvider recorderNameProvider(
    const QString& groupId,
    const QSharedPointer<RecorderItemDataHelper>& recorderItemDataHelper)
{
    return [groupId, recorderItemDataHelper]
    {
        return recorderItemDataHelper->groupedDeviceName(groupId);
    };
}

InvalidatorPtr recorderNameInvalidator(
    const QString& groupId,
    const QSharedPointer<RecorderItemDataHelper>& recorderItemDataHelper)
{
    auto result = std::make_shared<Invalidator>();

    result->connections()->add(recorderItemDataHelper->connect(
        recorderItemDataHelper.get(), &RecorderItemDataHelper::groupedDeviceNameChanged,
        [groupId, invalidator = result.get()](const QString& changedGroupId)
        {
            if (groupId == changedGroupId)
                invalidator->invalidate();
        }));

    return result;
}

//-------------------------------------------------------------------------------------------------
// Provider and invalidator pair factory functions for an user role group name generic item.
//-------------------------------------------------------------------------------------------------
GenericItem::DataProvider userRoleNameProvider(QnUserRolesManager* rolesManager, const QnUuid& id)
{
    return [rolesManager, id]() { return rolesManager->userRoleName(id); };
}

InvalidatorPtr userRoleNameInvalidator(QnUserRolesManager* rolesManager, const QnUuid& roleId)
{
    auto result = std::make_shared<Invalidator>();

    result->connections()->add(rolesManager->connect(
        rolesManager, &QnUserRolesManager::userRoleAddedOrUpdated,
        [roleId, invalidator = result.get()](const nx::vms::api::UserRoleData& userRole)
        {
            if (roleId == userRole.id)
                invalidator->invalidate();
        }));

    return result;
}

} // namespace

namespace nx::vms::client::desktop {
namespace entity_resource_tree {

using NodeType = ResourceTree::NodeType;
using IconCache = QnResourceIconCache;

using namespace entity_item_model;

ResourceTreeItemFactory::ResourceTreeItemFactory(const QnCommonModule* commonModule):
    base_type(),
    m_commonModule(commonModule)
{
}

AbstractItemPtr ResourceTreeItemFactory::createCurrentSystemItem() const
{
    const auto nameProvider = systemNameProvider(globalSettings());
    const auto nameInvalidator = systemNameInvalidator(globalSettings());

    return GenericItemBuilder()
        .withRole(Qt::DisplayRole, nameProvider, nameInvalidator)
        .withRole(Qn::ResourceIconKeyRole, static_cast<int>(IconCache::CurrentSystem))
        .withRole(Qn::NodeTypeRole, QVariant::fromValue(NodeType::currentSystem))
        .withFlags({Qt::ItemIsEnabled, Qt::ItemIsSelectable, Qt::ItemNeverHasChildren});
}

AbstractItemPtr ResourceTreeItemFactory::createCurrentUserItem(const QnUserResourcePtr& user) const
{
    const auto nameProvider = userNameProvider(user);
    const auto nameInvalidator = resourceNameInvalidator(user);

    return GenericItemBuilder()
        .withRole(Qt::DisplayRole, nameProvider, nameInvalidator)
        .withRole(Qn::ResourceRole, QVariant::fromValue(user.staticCast<QnResource>()))
        .withRole(Qn::ResourceIconKeyRole, static_cast<int>(IconCache::User))
        .withRole(Qn::NodeTypeRole, QVariant::fromValue(NodeType::currentUser))
        .withRole(Qn::ExtraInfoRole, QnResourceDisplayInfo(user).extraInfo())
        .withFlags({Qt::ItemIsEnabled, Qt::ItemIsSelectable, Qt::ItemNeverHasChildren});
}

AbstractItemPtr ResourceTreeItemFactory::createSpacerItem() const
{
    return GenericItemBuilder()
        .withRole(Qn::NodeTypeRole, QVariant::fromValue(NodeType::spacer))
        .withFlags({Qt::ItemNeverHasChildren});
}

AbstractItemPtr ResourceTreeItemFactory::createSeparatorItem() const
{
    return GenericItemBuilder()
        .withRole(Qn::NodeTypeRole, QVariant::fromValue(NodeType::separator))
        .withFlags({Qt::ItemNeverHasChildren});
}

AbstractItemPtr ResourceTreeItemFactory::createServersItem() const
{
    return GenericItemBuilder()
        .withRole(Qt::DisplayRole, tr("Servers"))
        .withRole(Qn::ResourceIconKeyRole, static_cast<int>(IconCache::Servers))
        .withRole(Qn::NodeTypeRole, QVariant::fromValue(NodeType::servers))
        .withRole(Qn::HelpTopicIdRole, static_cast<int>(Qn::MainWindow_Tree_Servers_Help));
}

AbstractItemPtr ResourceTreeItemFactory::createCamerasAndDevicesItem(Qt::ItemFlags itemFlags) const
{
    return GenericItemBuilder()
        .withRole(Qt::DisplayRole, tr("Cameras & Devices"))
        .withRole(Qn::ResourceIconKeyRole, static_cast<int>(IconCache::Cameras))
        .withRole(Qn::NodeTypeRole, QVariant::fromValue(NodeType::camerasAndDevices))
        .withFlags(itemFlags);
}

AbstractItemPtr ResourceTreeItemFactory::createLayoutsItem() const
{
    return GenericItemBuilder()
        .withRole(Qt::DisplayRole, tr("Layouts"))
        .withRole(Qn::ResourceIconKeyRole, static_cast<int>(IconCache::Layouts))
        .withRole(Qn::NodeTypeRole, QVariant::fromValue(NodeType::layouts))
        .withRole(Qn::HelpTopicIdRole, static_cast<int>(Qn::MainWindow_Tree_Layouts_Help));
}

AbstractItemPtr ResourceTreeItemFactory::createShowreelsItem() const
{
    return GenericItemBuilder()
        .withRole(Qt::DisplayRole, tr("Showreels"))
        .withRole(Qn::ResourceIconKeyRole, static_cast<int>(IconCache::LayoutTours))
        .withRole(Qn::NodeTypeRole, QVariant::fromValue(NodeType::layoutTours))
        .withRole(Qn::HelpTopicIdRole, static_cast<int>(Qn::Showreel_Help));
}

AbstractItemPtr ResourceTreeItemFactory::createWebPagesItem(Qt::ItemFlags flags) const
{
    return GenericItemBuilder()
        .withRole(Qt::DisplayRole, tr("Web Pages"))
        .withRole(Qn::ResourceIconKeyRole, static_cast<int>(IconCache::WebPages))
        .withRole(Qn::NodeTypeRole, QVariant::fromValue(NodeType::webPages))
        .withRole(Qn::HelpTopicIdRole, static_cast<int>(Qn::MainWindow_Tree_WebPage_Help))
        .withFlags(flags);
}

AbstractItemPtr ResourceTreeItemFactory::createUsersItem() const
{
    return GenericItemBuilder()
        .withRole(Qt::DisplayRole, tr("Users"))
        .withRole(Qn::ResourceIconKeyRole, static_cast<int>(IconCache::Users))
        .withRole(Qn::NodeTypeRole, QVariant::fromValue(NodeType::users))
        .withRole(Qn::HelpTopicIdRole, static_cast<int>(Qn::MainWindow_Tree_Users_Help));
}

AbstractItemPtr ResourceTreeItemFactory::createOtherSystemsItem() const
{
    return GenericItemBuilder()
        .withRole(Qt::DisplayRole, tr("Other Systems"))
        .withRole(Qn::ResourceIconKeyRole, static_cast<int>(IconCache::OtherSystems))
        .withRole(Qn::NodeTypeRole, QVariant::fromValue(NodeType::otherSystems))
        .withRole(Qn::HelpTopicIdRole, static_cast<int>(Qn::OtherSystems_Help));
}

AbstractItemPtr ResourceTreeItemFactory::createLocalFilesItem() const
{
    return GenericItemBuilder()
        .withRole(Qt::DisplayRole, tr("Local Files"))
        .withRole(Qn::ResourceIconKeyRole, static_cast<int>(IconCache::LocalResources))
        .withRole(Qn::NodeTypeRole, QVariant::fromValue(NodeType::localResources))
        .withRole(Qn::HelpTopicIdRole, static_cast<int>(Qn::MediaFolders_Help));
}

AbstractItemPtr ResourceTreeItemFactory::createAllCamerasAndResourcesItem() const
{
    return GenericItemBuilder()
        .withRole(Qt::DisplayRole, tr("All Cameras & Resources"))
        .withRole(Qn::ResourceIconKeyRole, static_cast<int>(IconCache::Cameras))
        .withRole(Qn::NodeTypeRole, QVariant::fromValue(NodeType::allCamerasAccess))
        .withFlags({Qt::ItemNeverHasChildren});
}

AbstractItemPtr ResourceTreeItemFactory::createAllSharedLayoutsItem() const
{
    return GenericItemBuilder()
        .withRole(Qt::DisplayRole, tr("All Shared Layouts"))
        .withRole(Qn::ResourceIconKeyRole, static_cast<int>(IconCache::SharedLayouts))
        .withRole(Qn::NodeTypeRole, QVariant::fromValue(NodeType::allLayoutsAccess))
        .withFlags({Qt::ItemNeverHasChildren});
}

AbstractItemPtr ResourceTreeItemFactory::createSharedResourcesItem() const
{
    return GenericItemBuilder()
        .withRole(Qt::DisplayRole, tr("Cameras & Resources"))
        .withRole(Qn::ResourceIconKeyRole, static_cast<int>(IconCache::Cameras))
        .withRole(Qn::NodeTypeRole, QVariant::fromValue(NodeType::sharedResources));
}

AbstractItemPtr ResourceTreeItemFactory::createSharedLayoutsItem(bool useRegularAppearance) const
{
    const auto displayText = useRegularAppearance
        ? tr("Layouts")
        : tr("Shared Layouts");

   const auto iconKey = useRegularAppearance
       ? static_cast<int>(IconCache::Layouts)
       : static_cast<int>(IconCache::SharedLayouts);

    return GenericItemBuilder()
        .withRole(Qt::DisplayRole, displayText)
        .withRole(Qn::ResourceIconKeyRole, iconKey)
        .withRole(Qn::NodeTypeRole, QVariant::fromValue(NodeType::sharedLayouts))
        .withRole(Qn::HelpTopicIdRole, static_cast<int>(Qn::MainWindow_Tree_Layouts_Help));
}

AbstractItemPtr ResourceTreeItemFactory::createResourceItem(const QnResourcePtr& resource)
{
    if (!m_resourceItemCache.contains(resource))
    {
        const auto sharedItemOrigin =
            std::make_shared<SharedItemOrigin>(std::make_unique<ResourceItem>(resource));

        sharedItemOrigin->setSharedInstanceCountObserver(
            [this, resource](int count)
            {
                if (count == 0)
                    m_resourceItemCache.remove(resource);
            });

        m_resourceItemCache.insert(resource, sharedItemOrigin);
    }
    return m_resourceItemCache.value(resource)->createSharedInstance();
}

AbstractItemPtr ResourceTreeItemFactory::createResourceGroupItem(
    const QString& compositeGroupId,
    const Qt::ItemFlags itemFlags)
{
    const auto displayText = resource_grouping::getResourceTreeDisplayText(compositeGroupId);

    return GenericItemBuilder()
        .withRole(Qt::DisplayRole, displayText)
        .withRole(Qt::ToolTipRole, displayText)
        .withRole(Qt::EditRole, displayText)
        .withRole(Qn::ResourceTreeCustomGroupIdRole, compositeGroupId)
        .withRole(Qn::ResourceIconKeyRole, static_cast<int>(IconCache::LocalResources))
        .withRole(Qn::NodeTypeRole, QVariant::fromValue(NodeType::customResourceGroup))
        .withFlags(itemFlags);
}

AbstractItemPtr ResourceTreeItemFactory::createRecorderItem(
    const QString& cameraGroupId,
    const QSharedPointer<RecorderItemDataHelper>& recorderItemDataHelper,
    Qt::ItemFlags itemFlags)
{
    const auto iconKey = recorderItemDataHelper->isMultisensorCamera(cameraGroupId)
        ? IconCache::MultisensorCamera
        : IconCache::Recorder;
    const auto nameProvider = recorderNameProvider(cameraGroupId, recorderItemDataHelper);
    const auto nameInvalidator = recorderNameInvalidator(cameraGroupId, recorderItemDataHelper);

    return GenericItemBuilder()
        .withRole(Qt::DisplayRole, nameProvider, nameInvalidator)
        .withRole(Qt::ToolTipRole, nameProvider, nameInvalidator)
        .withRole(Qt::EditRole, nameProvider, nameInvalidator)
        .withRole(Qn::ResourceIconKeyRole, static_cast<int>(iconKey))
        .withRole(Qn::NodeTypeRole, QVariant::fromValue(NodeType::recorder))
        .withRole(Qn::HelpTopicIdRole, static_cast<int>(Qn::MainWindow_Tree_Recorder_Help))
        .withRole(Qn::CameraGroupIdRole, cameraGroupId)
        .withFlags(itemFlags);
}

AbstractItemPtr ResourceTreeItemFactory::createOtherSystemItem(const QString& systemId)
{
    return GenericItemBuilder()
        .withRole(Qt::DisplayRole, systemId)
        .withRole(Qn::ResourceIconKeyRole, static_cast<int>(IconCache::OtherSystem))
        .withRole(Qn::NodeTypeRole, QVariant::fromValue(NodeType::localSystem))
        .withRole(Qn::HelpTopicIdRole, static_cast<int>(Qn::OtherSystems_Help));
}

AbstractItemPtr ResourceTreeItemFactory::createCloudSystemItem(const QString& systemId)
{
    const auto systemDescription = qnSystemsFinder->getSystem(systemId);

    if (!NX_ASSERT(systemDescription && systemDescription->isCloudSystem()))
        return {};

    return GenericItemBuilder()
        .withRole(Qt::DisplayRole, systemDescription->name())
        .withRole(Qn::ResourceIconKeyRole, static_cast<int>(IconCache::CloudSystem))
        .withRole(Qn::CloudSystemIdRole, systemId)
        .withRole(Qn::NodeTypeRole, QVariant::fromValue(NodeType::cloudSystem))
        .withRole(Qn::HelpTopicIdRole, static_cast<int>(Qn::OtherSystems_Help));
}

AbstractItemPtr ResourceTreeItemFactory::createCloudCameraItem(const QString& id)
{
    return GenericItemBuilder()
        .withRole(Qt::DisplayRole, id)
        .withRole(Qn::NodeTypeRole, QVariant::fromValue(NodeType::sharedLayout))
        .withRole(Qn::ResourceIconKeyRole, static_cast<int>(IconCache::Camera));
}

AbstractItemPtr ResourceTreeItemFactory::createUserRoleItem(const QnUuid& roleUuid)
{
    const auto userRolesManager = m_commonModule->userRolesManager();
    const auto nameProvider = userRoleNameProvider(userRolesManager, roleUuid);
    const auto nameInvalidator = userRoleNameInvalidator(userRolesManager, roleUuid);

    return GenericItemBuilder()
        .withRole(Qt::DisplayRole, nameProvider, nameInvalidator)
        .withRole(Qn::ResourceIconKeyRole, static_cast<int>(IconCache::Users))
        .withRole(Qn::NodeTypeRole, QVariant::fromValue(NodeType::role))
        .withRole(Qn::UuidRole, QVariant::fromValue(roleUuid))
        .withFlags({Qt::ItemIsEnabled, Qt::ItemIsSelectable, Qt::ItemIsDropEnabled});
}

AbstractItemPtr ResourceTreeItemFactory::createShowreelItem(const QnUuid& showreelId)
{
    return std::make_unique<ShowreelItem>(layoutTourManager(), showreelId);
}

AbstractItemPtr ResourceTreeItemFactory::createVideoWallScreenItem(
    const QnVideoWallResourcePtr& viedeoWall,
    const QnUuid& screenUuid)
{
    return std::make_unique<VideoWallScreenItem>(viedeoWall, screenUuid);
}

AbstractItemPtr ResourceTreeItemFactory::createVideoWallMatrixItem(
    const QnVideoWallResourcePtr& viedeoWall,
    const QnUuid& matrixUuid)
{
    return std::make_unique<VideoWallMatrixItem>(viedeoWall, matrixUuid);
}

QnResourcePool* ResourceTreeItemFactory::resourcePool() const
{
    return m_commonModule->resourcePool();
}

QnGlobalSettings* ResourceTreeItemFactory::globalSettings() const
{
    return m_commonModule->globalSettings();
}

QnLayoutTourManager* ResourceTreeItemFactory::layoutTourManager() const
{
    return m_commonModule->layoutTourManager();
}

} // namespace entity_resource_tree
} // namespace nx::vms::client::desktop