// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "systemmonitorplugin.h"

#include "systemmonitorapplet.h"

#include <QQuickView>
#include <QQmlContext>
#include <QQmlEngine>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCoreApplication>
#include <QMouseEvent>
#include <QIcon>
#include <DGuiApplicationHelper>
#include <DStandardPaths>

DGUI_USE_NAMESPACE
DCORE_USE_NAMESPACE

#define SYSTEM_MONITOR_KEY "system-monitor"
#define STATE_KEY "enable"

// ── Inline QuickPanelWidget ─────────────────────────────────────────

class SystemMonitorPlugin::QuickPanelWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QuickPanelWidget(SystemMonitorApplet *applet, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_applet(applet)
    {
        setFixedHeight(Dock::QUICK_ITEM_HEIGHT);
        setAttribute(Qt::WA_TranslucentBackground);

        auto *mainLayout = new QHBoxLayout(this);
        mainLayout->setContentsMargins(10, 8, 10, 8);
        mainLayout->setSpacing(8);

        // Icon area
        m_iconLabel = new QLabel(this);
        m_iconLabel->setFixedSize(Dock::QUICK_PANEL_ICON_SIZE);
        m_iconLabel->setPixmap(QIcon::fromTheme(QStringLiteral("deepin-system-monitor"),
                                                   QIcon::fromTheme(QStringLiteral("utilities-system-monitor")))
                                   .pixmap(Dock::QUICK_PANEL_ICON_SIZE));
        mainLayout->addWidget(m_iconLabel);

        // Text area
        auto *textLayout = new QVBoxLayout;
        textLayout->setSpacing(1);

        m_titleLabel = new QLabel(pluginDisplayName_static(), this);
        m_titleLabel->setObjectName("QuickPanelTitle");
        QFont titleFont;
        titleFont.setWeight(QFont::Medium);
        m_titleLabel->setFont(titleFont);

        m_statsLabel = new QLabel(this);
        m_statsLabel->setObjectName("QuickPanelStats");

        textLayout->addWidget(m_titleLabel);
        textLayout->addWidget(m_statsLabel);
        mainLayout->addLayout(textLayout, 1);

        connect(m_applet, &SystemMonitorApplet::systemStatsChanged, this, [this] {
            updateStatsText();
        });

        updateStatsText();
        updateThemeColors();

        // Follow system theme changes
        connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, [this] {
            updateThemeColors();
        });
    }

    static QString pluginDisplayName_static()
    {
        return QCoreApplication::translate("Plugin.DisplayName", "System Monitor");
    }

    bool isDarkTheme() const
    {
        return DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType;
    }

    void updateThemeColors()
    {
        const bool dark = isDarkTheme();
        const QColor titleColor = dark ? QColor(255, 255, 255, 245) : QColor(0, 0, 0, 235);
        const QColor statsColor = dark ? QColor(255, 255, 255, 173) : QColor(0, 0, 0, 148);

        m_titleLabel->setPalette(QPalette(titleColor));
        m_titleLabel->setAttribute(Qt::WA_TranslucentBackground, false);
        m_statsLabel->setPalette(QPalette(statsColor));
        m_statsLabel->setAttribute(Qt::WA_TranslucentBackground, false);
    }

    void updateStatsText()
    {
        m_statsLabel->setText(
            QStringLiteral("CPU %1%  |  MEM %2%")
                .arg(m_applet->cpuUsage())
                .arg(m_applet->memoryUsage()));
    }

Q_SIGNALS:
    void clicked();

protected:
    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton)
            Q_EMIT clicked();
        QWidget::mouseReleaseEvent(event);
    }

private:
    SystemMonitorApplet *m_applet = nullptr;
    QLabel *m_iconLabel = nullptr;
    QLabel *m_titleLabel = nullptr;
    QLabel *m_statsLabel = nullptr;
};

// ── SystemMonitorPlugin ────────────────────────────────────────────────

SystemMonitorPlugin::SystemMonitorPlugin(QObject *parent)
    : QObject(parent)
{
}

SystemMonitorPlugin::~SystemMonitorPlugin()
{
    if (m_cardView) {
        delete m_cardView;
        m_cardView = nullptr;
    }
}

const QString SystemMonitorPlugin::pluginName() const
{
    return QStringLiteral("system-monitor");
}

const QString SystemMonitorPlugin::pluginDisplayName() const
{
    return QCoreApplication::translate("Plugin.DisplayName", "System Monitor");
}

void SystemMonitorPlugin::init(PluginProxyInterface *proxyInter)
{
    m_proxyInter = proxyInter;

    // Load translations via DTK helper (searches standard data dirs)
    const QString trName = QStringLiteral("deepin-system-monitor-dde-shell-plugin");
    QStringList trDirs;
    const auto dataDirs = DStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    for (const auto &dir : dataDirs) {
        trDirs << QDir(dir).filePath(QStringLiteral("deepin-system-monitor-dde-shell-plugin/translations"));
    }
    DGuiApplicationHelper::loadTranslator(trName, trDirs, { QLocale::system() });

    // Create applet (backend data model)
    m_applet.reset(new SystemMonitorApplet(this));

    // Create quick panel widget
    m_quickPanelWidget.reset(new QuickPanelWidget(m_applet.data()));
    connect(m_quickPanelWidget.data(), &QuickPanelWidget::clicked, m_applet.data(), &SystemMonitorApplet::openSystemMonitor);

    refreshPluginItemsVisible();
}

QWidget *SystemMonitorPlugin::itemWidget(const QString &itemKey)
{
    if (itemKey == Dock::QUICK_ITEM_KEY) {
        return m_quickPanelWidget.data();
    }
    return nullptr;
}

Dock::PluginFlags SystemMonitorPlugin::flags() const
{
    return Dock::Type_Quick | Dock::Quick_Panel_Single | Dock::Attribute_HasCard;
}

bool SystemMonitorPlugin::pluginIsDisable()
{
    return !m_proxyInter->getValue(this, STATE_KEY, true).toBool();
}

void SystemMonitorPlugin::pluginStateSwitched()
{
    m_proxyInter->saveValue(this, STATE_KEY, pluginIsDisable());
    refreshPluginItemsVisible();
}

int SystemMonitorPlugin::itemSortKey(const QString &itemKey)
{
    const QString key = QString("pos_%1_%2").arg(itemKey).arg(Dock::Efficient);
    return m_proxyInter->getValue(this, key, -1).toInt();
}

void SystemMonitorPlugin::setSortKey(const QString &itemKey, const int order)
{
    const QString key = QString("pos_%1_%2").arg(itemKey).arg(Dock::Efficient);
    m_proxyInter->saveValue(this, key, order);
}

const QString SystemMonitorPlugin::itemContextMenu(const QString &itemKey)
{
    Q_UNUSED(itemKey)

    QJsonObject menuObj;
    QJsonArray items;

    QJsonObject openItem;
    openItem["itemId"] = "open";
    openItem["itemText"] = QCoreApplication::translate("Plugin.OpenSystemMonitor", "Open");
    openItem["isActive"] = true;
    items.append(openItem);

    menuObj["items"] = items;
    return QJsonDocument(menuObj).toJson();
}

void SystemMonitorPlugin::invokedMenuItem(const QString &itemKey, const QString &menuId, const bool checked)
{
    Q_UNUSED(itemKey)
    Q_UNUSED(checked)

    if (menuId == QLatin1String("open")) {
        m_applet->openSystemMonitor();
    }
}

void SystemMonitorPlugin::refreshIcon(const QString &itemKey)
{
    Q_UNUSED(itemKey)
}

QString SystemMonitorPlugin::cardItemKey() const
{
    return QStringLiteral("system-monitor-card");
}

QWindow *SystemMonitorPlugin::cardWindow() const
{
    if (m_cardView) {
        return m_cardView;
    }

    auto view = new QQuickView;
    view->setColor(Qt::transparent);
    view->setResizeMode(QQuickView::SizeRootObjectToView);
    view->rootContext()->setContextProperty(QStringLiteral("systemMonitorApplet"), m_applet.data());
    view->setSource(QUrl(QStringLiteral("qrc:/systemmonitor/SystemMonitorCard.qml")));
    if (view->status() == QQuickView::Error) {
        delete view;
        return nullptr;
    }

    // Parent to this plugin so the window is destroyed before unload
    QQmlEngine::setObjectOwnership(view, QQmlEngine::CppOwnership);
    connect(this, &QObject::destroyed, view, [view](QObject*){ delete view; });

    m_cardView = view;
    return m_cardView;
}

void SystemMonitorPlugin::refreshPluginItemsVisible()
{
    if (!m_proxyInter) {
        return;
    }

    if (pluginIsDisable()) {
        m_proxyInter->itemRemoved(this, SYSTEM_MONITOR_KEY);
        return;
    }

    m_proxyInter->itemAdded(this, SYSTEM_MONITOR_KEY);
}

// Required for Q_OBJECT in cpp file
#include "systemmonitorplugin.moc"
