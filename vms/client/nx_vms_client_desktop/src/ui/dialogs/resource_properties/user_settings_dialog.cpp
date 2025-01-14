// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "user_settings_dialog.h"
#include "ui_user_settings_dialog.h"

#include <QtWidgets/QMenu>

#include <core/resource/layout_resource.h>
#include <core/resource/user_resource.h>
#include <core/resource_access/resource_access_filter.h>
#include <core/resource_access/resource_access_manager.h>
#include <core/resource_access/shared_resources_manager.h>
#include <core/resource_management/resource_pool.h>
#include <core/resource_management/resources_changes_manager.h>
#include <core/resource_management/user_roles_manager.h>
#include <nx/branding.h>
#include <nx/utils/log/assert.h>
#include <nx/vms/client/core/common/utils/common_module_aware.h>
#include <nx/vms/client/core/ini.h>
#include <nx/vms/client/desktop/style/skin.h>
#include <nx/vms/client/desktop/ui/actions/action_manager.h>
#include <nx/vms/client/desktop/ui/common/color_theme.h>
#include <nx/vms/client/desktop/ui/dialogs/force_secure_auth_dialog.h>
#include <nx/vms/common/html/html.h>
#include <ui/common/palette.h>
#include <ui/help/help_handler.h>
#include <ui/help/help_topic_accessor.h>
#include <ui/help/help_topics.h>
#include <ui/models/resource_properties/user_settings_model.h>
#include <ui/widgets/properties/accessible_layouts_widget.h>
#include <ui/widgets/properties/accessible_media_widget.h>
#include <ui/widgets/properties/permissions_widget.h>
#include <ui/widgets/properties/user_profile_widget.h>
#include <ui/widgets/properties/user_settings_widget.h>
#include <ui/workbench/watchers/workbench_selection_watcher.h>
#include <ui/workbench/workbench_access_controller.h>
#include <ui/workbench/workbench_context.h>

using namespace nx::vms::client::desktop;
using namespace nx::vms::client::desktop::ui;

namespace {

// TODO: #akolesnikov #move to cdb api section
static const QString cloudAuthInfoPropertyName(lit("cloudUserAuthenticationInfo"));

bool resourcePassFilter(
    const QnResourcePtr& resource,
    const QnUserResourcePtr& currentUser,
    QnResourceAccessFilter::Filter filter)
{
    if (!currentUser)
        return false;

    if (!QnResourceAccessFilter::isShareable(filter, resource))
        return false;

    // Additionally filter layouts.
    if (filter == QnResourceAccessFilter::LayoutsFilter)
    {
        QnLayoutResourcePtr layout = resource.dynamicCast<QnLayoutResource>();
        NX_ASSERT(layout);
        if (!layout || !layout->resourcePool())
            return false;

        // Ignore "Preview Search" layouts.
        // TODO: #sivanov Do not add preview search layouts to the resource pool.
        if (layout->data().contains(Qn::LayoutSearchStateRole))
            return false;

        // Hide autogenerated layouts, e.g from videowall.
        if (layout->isServiceLayout())
            return false;

        // Hide unsaved layouts.
        if (layout->hasFlags(Qn::local))
            return false;

        // Show only shared or belonging to current user.
        return layout->isShared() || layout->getParentId() == currentUser->getId();
    }

    return true;
};

class PermissionsInfoTable: public nx::vms::client::core::CommonModuleAware
{
    Q_DECLARE_TR_FUNCTIONS(PermissionsInfoTable)

public:
    PermissionsInfoTable(const QnUserResourcePtr& currentUser):
        m_currentUser(currentUser)
    {}

    void addPermissionsRow(const QnResourceAccessSubject& subject)
    {
        auto permissions = resourceAccessManager()->globalPermissions(subject);
        addRow(makePermissionsRow(permissions));
    }

    void addPermissionsRow(QnPermissionsWidget* widget)
    {
        addRow(makePermissionsRow(widget->selectedPermissions()));
    }

    void addResourceAccessRow(QnResourceAccessFilter::Filter filter,
            const QnResourceAccessSubject& subject, bool currentUserIsAdmin)
    {
        auto allResources = resourcePool()->getResources();
        auto accessibleResources = sharedResourcesManager()->sharedResources(subject);
        auto permissions = resourceAccessManager()->globalPermissions(subject);

        int count = 0;
        int total = currentUserIsAdmin ? 0 : -1;

        if (filter == QnResourceAccessFilter::MediaFilter &&
            permissions.testFlag(GlobalPermission::accessAllMedia))
        {
            addRow(makeResourceAccessRow(filter, true, count, total));
            return;
        }

        for (const auto& resource: allResources)
        {
            if (resourcePassFilter(resource, m_currentUser, filter))
            {
                if (currentUserIsAdmin)
                    ++total;

                if (accessibleResources.contains(resource->getId()))
                    ++count;
            }
        }

        addRow(makeResourceAccessRow(filter, false, count, total));
    };

    void addResourceAccessRow(const QnAccessibleLayoutsWidget* layoutsWidget)
    {
        addRow(makeResourceAccessRow(
            QnResourceAccessFilter::LayoutsFilter,
            /*all*/ false,
            layoutsWidget->checkedLayouts().size(),
            layoutsWidget->layoutsCount()));
    }

    void addResourceAccessRow(const QnAccessibleMediaWidget* mediaResourcesWidget)
    {
        addRow(makeResourceAccessRow(
            QnResourceAccessFilter::MediaFilter,
            mediaResourcesWidget->allCamerasItemChecked(),
            mediaResourcesWidget->checkedResources().size(),
            mediaResourcesWidget->resourcesCount()));
    }

    QString makeTable() const
    {
        using namespace nx::vms::common;

        QString result;
        {
            html::Tag tableTag("table", result, html::Tag::NoBreaks);
            for (const auto& row: m_rows)
            {
                html::Tag rowTag("tr", result, html::Tag::NoBreaks);
                result.append(row);
            }
        }
        return result;
    }

private:
    void addRow(const QString& row)
    {
        m_rows.push_back(row);
    }

    QString categoryName(QnResourceAccessFilter::Filter value) const
    {
        switch (value)
        {
            case QnResourceAccessFilter::MediaFilter:
                return tr("Cameras & Resources");

            case QnResourceAccessFilter::LayoutsFilter:
                return tr("Shared Layouts");
            default:
                break;
        }

        return QString();
    }

    QString makeGenericCountRow(const QString& name, const QString& count) const
    {
        static const QString kHtmlRowTemplate =
            lit("<td><b>%1</b></td><td width=\"16\"/><td>%2</td>");
        return kHtmlRowTemplate.arg(count).arg(name.toHtmlEscaped());
    }

    QString makeGenericCountRow(const QString& name, int count) const
    {
        return makeGenericCountRow(name, QString::number(count));
    }

    QString makeGenericCountOfTotalRow(const QString& name, int count, int total) const
    {
        static const QString kHtmlRowTemplate =
            lit("<td><b>%1</b> / %2</td><td width=\"16\"/><td>%3</td>");
        return kHtmlRowTemplate.arg(count).arg(total).arg(name);
    }

    QString makeResourceAccessRow(QnResourceAccessFilter::Filter filter, bool all,
        int count, int total) const
    {
        QString name = categoryName(filter);
        if (all)
        {
            return makeGenericCountRow(
                name,
                tr("All",
                    "This will be a part of \"All Cameras & Resources\" or \"All Shared Layouts\""
                ));
        }

        if (filter == QnResourceAccessFilter::LayoutsFilter || total < 0)
            return makeGenericCountRow(name, count);

        return makeGenericCountOfTotalRow(name, count, total);
    };

    QString makePermissionsRow(GlobalPermissions rawPermissions) const
    {
        int count = 0;
        int total = 0;

        const auto checkFlag =
            [&count, &total, rawPermissions](GlobalPermission flag)
            {
                ++total;
                if (rawPermissions.testFlag(flag))
                    ++count;
            };

        // TODO: #sivanov Think where to store flags set to avoid duplication.
        checkFlag(GlobalPermission::editCameras);
        checkFlag(GlobalPermission::controlVideowall);
        checkFlag(GlobalPermission::viewLogs);
        checkFlag(GlobalPermission::viewArchive);
        checkFlag(GlobalPermission::exportArchive);
        checkFlag(GlobalPermission::viewBookmarks);
        checkFlag(GlobalPermission::manageBookmarks);
        checkFlag(GlobalPermission::userInput);

        return makeGenericCountOfTotalRow(tr("Permissions"), count, total);
    }

private:
    QnUserResourcePtr m_currentUser;
    QStringList m_rows;
};

QnLayoutResourceList layoutsToShare(const QnResourcePool* resourcePool,
    const QnUuidSet& accessibleResources)
{
    if (!NX_ASSERT(resourcePool))
        return {};

    return resourcePool->getResourcesByIds(accessibleResources).filtered<QnLayoutResource>(
        [](const QnLayoutResourcePtr& layout)
        {
            return !layout->isFile() && !layout->isShared();
        });
}

bool isCustomUser(const QnUserResourcePtr& user)
{
    if (!NX_ASSERT(user))
        return false;

    return user->userRole() == Qn::UserRole::customPermissions;
}

}; // namespace

QnUserSettingsDialog::QnUserSettingsDialog(QWidget* parent):
    base_type(parent),
    ui(new Ui::UserSettingsDialog()),
    m_model(new QnUserSettingsModel(this)),
    m_user(),
    m_profilePage(new QnUserProfileWidget(m_model, this)),
    m_settingsPage(new QnUserSettingsWidget(m_model, this)),
    m_permissionsPage(new QnPermissionsWidget(m_model, this)),
    m_camerasPage(new QnAccessibleMediaWidget(m_model, this)),
    m_layoutsPage(new QnAccessibleLayoutsWidget(m_model, this)),
    m_userEnabledButton(new QPushButton(tr("Enabled"), this)),
    m_digestMenuButton(new QPushButton(this)),
    m_digestMenu(new QMenu(this))
{
    NX_ASSERT(parent);

    ui->setupUi(this);

    addPage(ProfilePage, m_profilePage, tr("User Information"));
    addPage(SettingsPage, m_settingsPage, tr("User Information"));
    addPage(PermissionsPage, m_permissionsPage, tr("Permissions"));
    addPage(CamerasPage, m_camerasPage, tr("Cameras && Resources"));
    addPage(LayoutsPage, m_layoutsPage, tr("Layouts"));

    connect(resourceAccessManager(), &QnResourceAccessManager::permissionsChanged, this,
        [this](const QnResourceAccessSubject& subject)
        {
            if (m_user && subject.user() == m_user)
            {
                updatePermissions();
                updateButtonBox();
            }
        });

    connect(m_settingsPage, &QnAbstractPreferencesWidget::hasChangesChanged,
        this, &QnUserSettingsDialog::updatePermissions);
    connect(m_permissionsPage, &QnAbstractPreferencesWidget::hasChangesChanged,
        this, &QnUserSettingsDialog::updatePermissions);
    connect(m_camerasPage, &QnAbstractPreferencesWidget::hasChangesChanged,
        this, &QnUserSettingsDialog::updatePermissions);
    connect(m_layoutsPage, &QnAbstractPreferencesWidget::hasChangesChanged,
        this, &QnUserSettingsDialog::updatePermissions);

    connect(m_settingsPage, &QnUserSettingsWidget::userTypeChanged, this,
        [this](bool isCloud)
        {
            // We have to recreate user resource to change user type.
            QnUserResourcePtr newUser(new QnUserResource(isCloud
                ? nx::vms::api::UserType::cloud
                : nx::vms::api::UserType::local));
            newUser->setFlags(m_user->flags());
            newUser->setIdUnsafe(m_user->getId());
            newUser->setRawPermissions(m_user->getRawPermissions());
            m_user = newUser;
            m_model->setUser(m_user);
            if (!isCloud)
                m_model->setDigestAuthorizationEnabled(false);
        });

    auto selectionWatcher = new QnWorkbenchSelectionWatcher(this);
    connect(selectionWatcher, &QnWorkbenchSelectionWatcher::selectionChanged, this,
        [this](const QnResourceList& resources)
        {
            if (isHidden())
                return;

            // Do not automatically switch if we are creating a new user.
            if (m_model->mode() == QnUserSettingsModel::Mode::NewUser)
                return;

            auto users = resources.filtered<QnUserResource>();
            if (!users.isEmpty())
                setUser(users.first());
        });

    connect(resourcePool(), &QnResourcePool::resourceRemoved, this,
        [this](const QnResourcePtr& resource)
        {
            if (resource != m_user)
            {
                // TODO: #vkutin #sivanov Check if permissions change is correctly handled through
                // a chain of signals.
                return;
            }
            setUser(QnUserResourcePtr());
            tryClose(true);
        });

    connect(m_model,
        &QnUserSettingsModel::digestSupportChanged,
        this,
        [this]
        {
            if (m_model->digestSupport() == QnUserResource::DigestSupport::disable
                && m_model->mode() != QnUserSettingsModel::NewUser
                && accessController()->hasPermissions(m_user, Qn::WritePasswordPermission)
                && !ForceSecureAuthDialog::isConfirmed(this))
            {
                m_model->setDigestAuthorizationEnabled(!m_model->digestAuthorizationEnabled());
            }
        });

    ui->buttonBox->addButton(m_userEnabledButton, QDialogButtonBox::HelpRole);
    ui->buttonBox->addButton(m_digestMenuButton, QDialogButtonBox::HelpRole);
    connect(m_userEnabledButton, &QPushButton::clicked, this, &QnUserSettingsDialog::updateButtonBox);

    m_userEnabledButton->setFlat(true);
    m_userEnabledButton->setCheckable(true);
    m_userEnabledButton->setVisible(false);
    setHelpTopic(m_userEnabledButton, Qn::UserSettings_DisableUser_Help);

    m_digestMenuButton->setIcon(qnSkin->icon("misc/extra_settings.svg"));
    m_digestMenuButton->setFlat(true);
    m_digestMenuButton->setVisible(false);

    connect(m_digestMenuButton, &QPushButton::clicked, this,
        [this]
        {
            m_digestMenu->exec(
                m_digestMenuButton->mapToGlobal(QPoint(0, m_digestMenuButton->height())));
        });

    auto digestAction = m_digestMenu->addAction(tr("Allow digest authentication for this user"));
    connect(digestAction, &QAction::triggered, this,
        [this]
        {
            m_model->setDigestAuthorizationEnabled(true);
            updateControlsVisibility();
        });

    const auto textColor = nx::vms::client::desktop::colorTheme()->color("light4");
    const auto highlightColor = nx::vms::client::desktop::colorTheme()->color("light1");
    ui->warningLabel->setStyleSheet(QString("QLabel {color: %1;}").arg(textColor.name()));

    ui->warningLink->setText(nx::vms::common::html::localLink(tr("Learn More")));
    setPaletteColor(ui->warningLink, QPalette::Link, textColor);
    // LinkHoverProcessor class maintains link hover color using LinkVisited palette entry.
    setPaletteColor(ui->warningLink, QPalette::LinkVisited, highlightColor);

    connect(ui->warningLink, &QLabel::linkActivated, this,
        [](){ QnHelpHandler::openHelpTopic(Qn::HelpTopic::SessionAndDigestAuth_Help); });

    ui->forceSecureAuthButton->setIcon(qnSkin->icon("misc/shield.svg"));

    connect(ui->forceSecureAuthButton, &QPushButton::clicked, this,
        [this]
        {
            m_model->setDigestAuthorizationEnabled(false);
            updateControlsVisibility();
        });

    setPaletteColor(ui->warningBanner,
        QPalette::Window, nx::vms::client::desktop::colorTheme()->color("dialog.alertBar"));
    ui->warningBanner->setAutoFillBackground(true);

    connect(this,
        &QnGenericTabbedDialog::dialogClosed,
        this,
        [this] { setUser(QnUserResourcePtr()); });

    ui->alertBar->setRetainSpaceWhenNotDisplayed(false);

    auto okButton = ui->buttonBox->button(QDialogButtonBox::Ok);
    auto applyButton = ui->buttonBox->button(QDialogButtonBox::Apply);

    updatePermissions();
}

QnUserSettingsDialog::~QnUserSettingsDialog()
{
}

QnUserResourcePtr QnUserSettingsDialog::user() const
{
    return m_user;
}

void QnUserSettingsDialog::updatePermissions()
{
    updateControlsVisibility();

    const auto currentUser = context()->user();
    if (!currentUser)
        return;

    PermissionsInfoTable helper(currentUser);
    if (isPageVisible(ProfilePage))
    {
        Qn::UserRole role = m_user->userRole();
        QString permissionsText;

        if (role == Qn::UserRole::customUserRole || role == Qn::UserRole::customPermissions)
        {
            QnResourceAccessSubject subject(m_user);
            helper.addPermissionsRow(subject);
            helper.addResourceAccessRow(QnResourceAccessFilter::MediaFilter, subject, false);
            helper.addResourceAccessRow(QnResourceAccessFilter::LayoutsFilter, subject, false);
            permissionsText = helper.makeTable();
        }
        else
        {
            permissionsText = QnPredefinedUserRoles::description(role);
        }

        m_profilePage->updatePermissionsLabel(permissionsText);
    }
    else
    {
        Qn::UserRole roleType = m_settingsPage->selectedRole();
        QString permissionsText;

        if (roleType == Qn::UserRole::customUserRole)
        {
            // Handle custom user role.
            QnUuid roleId = m_settingsPage->selectedUserRoleId();
            QnResourceAccessSubject subject(userRolesManager()->userRole(roleId));

            helper.addPermissionsRow(subject);
            helper.addResourceAccessRow(QnResourceAccessFilter::MediaFilter, subject, true);
            helper.addResourceAccessRow(QnResourceAccessFilter::LayoutsFilter, subject, true);
            permissionsText = helper.makeTable();
        }
        else if (roleType == Qn::UserRole::customPermissions)
        {
            helper.addPermissionsRow(m_permissionsPage);
            helper.addResourceAccessRow(m_camerasPage);
            helper.addResourceAccessRow(m_layoutsPage);
            permissionsText = helper.makeTable();
        }
        else
        {
            permissionsText = QnPredefinedUserRoles::description(roleType);
        }

        m_settingsPage->updatePermissionsLabel(permissionsText);
    }

    m_model->updatePermissions();
}

void QnUserSettingsDialog::setUser(const QnUserResourcePtr &user)
{
    if (m_user == user)
        return;

    if (!tryToApplyOrDiscardChanges())
        return;

    if (m_user)
        m_user->disconnect(this);

    m_user = user;
    m_model->setUser(user);

    if (m_model->mode() == QnUserSettingsModel::NewUser)
        m_model->setDigestAuthorizationEnabled(false);

    if (m_user)
    {
        connect(m_user.get(), &QnResource::propertyChanged, this,
            [this](const QnResourcePtr& resource, const QString& propertyName)
            {
                if (resource == m_user && propertyName == cloudAuthInfoPropertyName)
                    forcedUpdate();
            });
        connect(m_user.get(), &QnUserResource::digestChanged, this,
            [this](const auto& /*user*/) { QnUserSettingsDialog::updateButtonBox(); });
    }

    // Hide Apply button if cannot apply changes.
    bool applyButtonVisible = m_model->mode() == QnUserSettingsModel::OwnProfile
                           || m_model->mode() == QnUserSettingsModel::OtherSettings;
    ui->buttonBox->button(QDialogButtonBox::Apply)->setVisible(applyButtonVisible);

    // Hide Cancel button if we cannot edit user.
    bool cancelButtonVisible = m_model->mode() != QnUserSettingsModel::OtherProfile
                            && m_model->mode() != QnUserSettingsModel::Invalid;
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setVisible(cancelButtonVisible);

    forcedUpdate();
}

void QnUserSettingsDialog::loadDataToUi()
{
    ui->alertBar->setText(QString());

    base_type::loadDataToUi();

    if (!m_user)
        return;

    bool userIsEnabled = m_user->isEnabled();
    m_userEnabledButton->setChecked(userIsEnabled);

    if (m_user->userType() == nx::vms::api::UserType::cloud
        && m_model->mode() == QnUserSettingsModel::OtherSettings)
    {
        const auto auth = m_user->getProperty(cloudAuthInfoPropertyName);
        if (auth.isEmpty())
        {
            ui->alertBar->setText(tr("This user has not yet signed up for %1",
                "%1 is the cloud name (like Nx Cloud)").arg(nx::branding::cloudName()));
            return;
        }
    }
    if (!userIsEnabled)
        ui->alertBar->setText(tr("User is disabled"));
}

void QnUserSettingsDialog::forcedUpdate()
{
    m_model->updateMode();
    Qn::updateGuarded(this, [this]() { base_type::forcedUpdate(); });
    updatePermissions();
    updateButtonBox();
}

QDialogButtonBox::StandardButton QnUserSettingsDialog::showConfirmationDialog()
{
    NX_ASSERT(m_user, "User must exist here");

    if (m_model->mode() != QnUserSettingsModel::OwnProfile
        && m_model->mode() != QnUserSettingsModel::OtherSettings)
    {
        return QDialogButtonBox::Cancel;
    }

    if (!canApplyChanges())
        return QDialogButtonBox::Cancel;

    return QnMessageBox::question(this,
        tr("Apply changes before switching to another user?"), QString(),
        QDialogButtonBox::Apply | QDialogButtonBox::Discard | QDialogButtonBox::Cancel,
        QDialogButtonBox::Apply);
}

void QnUserSettingsDialog::retranslateUi()
{
    base_type::retranslateUi();
    if (!m_user)
        return;

    if (m_model->mode() == QnUserSettingsModel::NewUser)
    {
        setWindowTitle(tr("New User..."));
        setHelpTopic(this, Qn::NewUser_Help);
    }
    else
    {
        const bool readOnly =
            !accessController()->hasPermissions(m_user, Qn::WritePermission | Qn::SavePermission);
        setWindowTitle(readOnly
            ? tr("User Settings - %1 (readonly)").arg(m_user->getName())
            : tr("User Settings - %1").arg(m_user->getName()));

        setHelpTopic(this, Qn::UserSettings_Help);
    }
}

void QnUserSettingsDialog::applyChanges()
{
    auto mode = m_model->mode();
    const bool isDigestModeChanged =
        m_model->digestSupport() != QnUserResource::DigestSupport::keep;

    if ((mode == QnUserSettingsModel::Invalid || mode == QnUserSettingsModel::OtherProfile)
        && !isDigestModeChanged)
    {
        return;
    }

    // TODO: #sivanov What to rollback if current password changes cannot be saved?

    using CustomUserAccessibleResources = std::optional<QnUuidSet>;
    using CustomUserAccessibleResourcesPtr = std::shared_ptr<CustomUserAccessibleResources>;

    CustomUserAccessibleResourcesPtr customUserResources =
        std::make_shared<CustomUserAccessibleResources>(std::nullopt);

    QnResourcesChangesManager::UserChangesFunction applyChangesFunction =
        [this, customUserResources](const QnUserResourcePtr& /*user*/)
        {
            // Here accessible resources will also be filled to model.
            applyChangesInternal();
            if (m_user->getId().isNull())
                m_user->fillIdUnsafe();

            if (isCustomUser(m_user))
                *customUserResources = m_model->accessibleResources();
        };

    // Handle new user creating.
    const auto actionManager = QPointer<action::Manager>(menu());
    QnResourcesChangesManager::UserCallbackFunction callbackFunction =
        [actionManager, mode, customUserResources](bool success, const QnUserResourcePtr& user)
        {
            if (!success || !actionManager)
                return;

            // Cannot capture the resource directly because real resource pointer may differ if
            // the transaction is received before the request callback.
            if (!NX_ASSERT(user))
                return;

            if (isCustomUser(user))
            {
                const auto resourcePool = user->resourcePool();
                const auto layouts = layoutsToShare(resourcePool, **customUserResources);
                for (const auto& layout: layouts)
                {
                    actionManager->trigger(action::ShareLayoutAction,
                        action::Parameters(layout).withArgument(Qn::UserResourceRole, user));
                }
                qnResourcesChangesManager->saveAccessibleResources(user, **customUserResources);
            }

            if (mode == QnUserSettingsModel::NewUser)
                actionManager->trigger(action::SelectNewItemAction, user);
        };

    qnResourcesChangesManager->saveUser(
        m_user, m_model->digestSupport(), applyChangesFunction, callbackFunction);

    if (!isCustomUser(m_user) && (m_model->mode() != QnUserSettingsModel::NewUser))
        qnResourcesChangesManager->saveAccessibleResources(m_user, QSet<QnUuid>());

    // We may fill password field to change current user password.
    m_user->resetPassword();

    // New User was created, clear dialog.
    if (m_model->mode() == QnUserSettingsModel::NewUser)
        setUser(QnUserResourcePtr());

    forcedUpdate();
}

void QnUserSettingsDialog::applyChangesInternal()
{
    base_type::applyChanges();

    if (accessController()->hasPermissions(m_user, Qn::WriteAccessRightsPermission))
        m_user->setEnabled(m_userEnabledButton->isChecked());
}

bool QnUserSettingsDialog::hasChanges() const
{
    if (base_type::hasChanges())
        return true;

    if (m_model->digestSupport() != QnUserResource::DigestSupport::keep)
        return true;

    if (m_user && accessController()->hasPermissions(m_user, Qn::WriteAccessRightsPermission))
        return (m_user->isEnabled() != m_userEnabledButton->isChecked());

    return false;
}

void QnUserSettingsDialog::updateControlsVisibility()
{
    auto mode = m_model->mode();

    // We are displaying profile for ourself and for users, that we cannot edit.
    bool profilePageVisible = mode == QnUserSettingsModel::OwnProfile
        || mode == QnUserSettingsModel::OtherProfile;

    bool settingsPageVisible = mode == QnUserSettingsModel::NewUser
        || mode == QnUserSettingsModel::OtherSettings;

    bool customAccessRights = settingsPageVisible
        && m_settingsPage->selectedRole() == Qn::UserRole::customPermissions;

    setPageVisible(ProfilePage,     profilePageVisible);

    setPageVisible(SettingsPage,    settingsPageVisible);
    setPageVisible(PermissionsPage, customAccessRights);
    setPageVisible(CamerasPage,     customAccessRights);
    setPageVisible(LayoutsPage,     customAccessRights);

    const bool canEditUser =
        m_user && accessController()->hasPermissions(m_user, Qn::WriteAccessRightsPermission);

    m_userEnabledButton->setVisible(settingsPageVisible && canEditUser);

    const bool digestEnabled = m_model->digestAuthorizationEnabled();
    const bool canChangeDigestMode =
        m_user && accessController()->hasPermissions(m_user, Qn::WriteDigestPermission);

    const bool canEnableDigest = canChangeDigestMode
        && accessController()->hasPermissions(m_user, Qn::WritePasswordPermission);

    ui->warningBanner->setVisible(digestEnabled);

    ui->forceSecureAuthButton->setVisible(canChangeDigestMode);
    m_digestMenuButton->setVisible(canEnableDigest && !digestEnabled);

    // Buttons state takes into account pages visibility, so we must recalculate it.
    updateButtonBox();
}
