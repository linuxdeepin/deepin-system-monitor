/*
 * Copyright (C) 2018 ~ 2025 Deepin Technology Co., Ltd.
 *
 * Author:     fanpengcheng <fanpengcheng_cm@deepin.com>
 *
 * Maintainer: fanpengcheng <fanpengcheng_cm@deepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "mainwindow.h"
#include "common/datacommon.h"
//#include "itemwidget.h"
#include "dbus/dbusayatanainterface.h"

#include <DApplication>
#include <DApplicationHelper>
#include <DFontSizeManager>
#include <DGuiApplicationHelper>
#include <DPalette>
#include <DStyleHelper>

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QScrollBar>
#include <QScreen>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QDesktopWidget>
#include <QApplication>
#include <QPalette>
#include <QTimer>

#define DOCK_TOP        0
#define DOCK_RIGHT      1
#define DOCK_BOTTOM     2
#define DOCK_LEFT       3

#define MONITOR_SERVICE "com.deepin.api.XEventMonitor"
#define SCREEN_HEIGHT_MAX 1080

MainWindow::MainWindow(QWidget *parent)
    : DBlurEffectWidget(parent)
    , m_displayInter(new DBusDisplay("com.deepin.daemon.Display", "/com/deepin/daemon/Display", QDBusConnection::sessionBus(), this))
    , m_daemonDockInter(new DBusDaemonDock("com.deepin.dde.daemon.Dock", "/com/deepin/dde/daemon/Dock", QDBusConnection::sessionBus(), this))
    , m_dockInter(new DBusDockInterface)
    , m_systemMonitorDbusAdaptor(new SystemMonitorDBusAdaptor)
    , m_regionMonitor(nullptr)
    , m_xAni(new QPropertyAnimation(this, "x"))
    , m_widthAni(new QPropertyAnimation(this, "width"))
    , m_aniGroup(new QSequentialAnimationGroup(this))
    , m_wmHelper(DWindowManagerHelper::instance())
    , m_trickTimer(new QTimer(this))
{
    m_trickTimer->setInterval(300);
    m_trickTimer->setSingleShot(true);
    initDBus();
    initUI();
    initAni();
    initConnect();

    geometryChanged();
    CompositeChanged();
    registerMonitor();
    installEventFilter(this);

    DBusAyatanaInterface::getInstance();
}

MainWindow::~MainWindow()
{

}

void MainWindow::Toggle()
{
    if (m_hasComposite) {
        if (m_aniGroup->state() == QAbstractAnimation::Running)
            return;
        if (isVisible()) {
            hideAni();
        } else {
            showAni();
        }
    } else {
        if (isVisible()) {
            hideAni();
        } else {
            showAni();
        }
    }
}

void MainWindow::geometryChanged()
{
    adjustPosition();

    //init animation by 'm_rect'
    m_xAni->setStartValue(m_rect.x());
    m_xAni->setEndValue(m_rect.x() + Globals::WindowMargin);

    m_widthAni->setStartValue(m_rect.width());
    m_widthAni->setEndValue(0);
}

void MainWindow::showAni()
{
    if (m_trickTimer->isActive()) {
        return;
    }

    m_trickTimer->start();
    if (!m_hasComposite) {
        if (m_daemonDockInter->position() == DOCK_RIGHT) {
            if (m_daemonDockInter->displayMode() == 0) {
                setGeometry(getDisplayScreen().x() + getDisplayScreen().width() - m_rect.width() - m_dockInter->geometry().width() - Globals::WindowMargin - 2*Globals::DockMargin, m_rect.y(), m_rect.width(), m_rect.height());
            } else {
                setGeometry(getDisplayScreen().x() + getDisplayScreen().width() - m_rect.width() - m_dockInter->geometry().width() - Globals::WindowMargin, m_rect.y(), m_rect.width(), m_rect.height());
            }
        } else {
            setGeometry(getDisplayScreen().x() + getDisplayScreen().width() - m_rect.width() - Globals::WindowMargin, m_rect.y(), m_rect.width(), m_rect.height());
        }

        setFixedWidth(m_rect.width());
        show();
        activateWindow();

        return;
    }
    setFixedWidth(0);

    show();
    activateWindow();
    m_aniGroup->setDirection(QAbstractAnimation::Backward);
    m_aniGroup->start();
}

void MainWindow::hideAni()
{
    if (m_trickTimer->isActive()) {
        return;
    }

    m_trickTimer->start();

    if (!m_hasComposite) {
        hide();
        return;
    }

    m_aniGroup->setDirection(QAbstractAnimation::Forward);
    m_aniGroup->start();

    QTimer::singleShot(m_aniGroup->duration(), this, [ = ] {setVisible(false);});
}

void MainWindow::startLoader()
{
    QProcess *process = new QProcess(this);
    process->startDetached("deepin-system-monitor-plugin-popup");
    process->waitForStarted();
    process->waitForFinished();
    process->deleteLater();
    process = Q_NULLPTR;
}

void MainWindow::Show()
{
    if (m_trickTimer->isActive()) {
        return;
    }

    m_trickTimer->start();

    if (m_aniGroup->state() == QAbstractAnimation::Running)
        return;

    if (!isVisible()) {
        showAni();
    }
}

void MainWindow::Hide()
{
    if (m_trickTimer->isActive()) {
        return;
    }

    m_trickTimer->start();

    if (m_aniGroup->state() == QAbstractAnimation::Running)
        return;

    if (isVisible()) {
        hideAni();
    }
}

void MainWindow::setX(int x)
{
    move(int(std::round(qreal(x) / qApp->primaryScreen()->devicePixelRatio())) , m_rect.y());
}

void MainWindow::CompositeChanged()
{
    m_hasComposite = m_wmHelper->hasComposite();
}

void MainWindow::registerMonitor()
{
    if (m_regionMonitor) {
        delete m_regionMonitor;
        m_regionMonitor = nullptr;
    }
    m_regionMonitor = new DRegionMonitor(this);
    m_regionMonitor->registerRegion(QRegion(QRect()));
    connect(m_regionMonitor, &DRegionMonitor::buttonPress, this, [ = ](const QPoint &p, const int flag) {
        Q_UNUSED(flag);
        if (!geometry().contains(p))
            if (!isHidden()) {
                hideAni();
            }
    });
}

void MainWindow::initUI()
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool  | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedWidth(Globals::WindowWidth);

    m_cpuMonitor = new CpuWidget(this);
    m_netWidget = new NetWidget(this);
    m_diskWidget = new DiskWidget(this);
    m_memoryWidget = new MemoryWidget(this);
    m_processWidget = new ProcessWidget(this);

    QWidget *parentWidget = new QWidget(this);
    QVBoxLayout *vBoxLayout = new QVBoxLayout(parentWidget);
    vBoxLayout->setContentsMargins(10, 0, 10, 0);

    vBoxLayout->addWidget(m_cpuMonitor);
    vBoxLayout->addSpacing(5);
    vBoxLayout->addWidget(m_netWidget);
    vBoxLayout->addSpacing(5);
    vBoxLayout->addWidget(m_memoryWidget);
    vBoxLayout->addSpacing(5);
    vBoxLayout->addWidget(m_diskWidget);
    vBoxLayout->addSpacing(5);
    vBoxLayout->addWidget(m_processWidget);

    m_scrollArea = new DScrollArea(this);
    m_scrollArea->horizontalScrollBar()->setEnabled(false);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    auto pal = qApp->palette();
    pal.setColor(QPalette::Background, QColor(0,0,0,0));
    m_scrollArea->setPalette(pal);

    m_scrollArea->setWidget(parentWidget);
    m_scrollArea->setWindowFlags(Qt::FramelessWindowHint);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setWidgetResizable(true);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_scrollArea);
    setLayout(mainLayout);

    setFocusPolicy(Qt::NoFocus);
}

void MainWindow::initAni()
{
    m_xAni->setEasingCurve(QEasingCurve::Linear);
    m_xAni->setDuration(Globals::AnimationTime / 2);

    m_widthAni->setEasingCurve(QEasingCurve::Linear);
    m_widthAni->setDuration(Globals::AnimationTime);

    m_aniGroup->addAnimation(m_xAni);
    m_aniGroup->addAnimation(m_widthAni);

    auto *dAppHelper = DApplicationHelper::instance();
    connect(dAppHelper, &DApplicationHelper::themeTypeChanged, this, &MainWindow::changeTheme);
    changeTheme(dAppHelper->themeType());
}

void MainWindow::initConnect()
{
    connect(m_displayInter, &DBusDisplay::PrimaryRectChanged, this, &MainWindow::geometryChanged, Qt::QueuedConnection);
    connect(m_dockInter, &DBusDockInterface::geometryChanged, this, &MainWindow::geometryChanged, Qt::UniqueConnection);
    connect(this, SIGNAL(signal_geometry(int)), m_processWidget, SLOT(autoHeight(int)));

    connect(m_systemMonitorDbusAdaptor, &SystemMonitorDBusAdaptor::sigSendShowOrHideSystemMonitorPluginPopupWidget,
            this, &MainWindow::slotShowOrHideSystemMonitorPluginPopupWidget);


    connect(m_wmHelper, &DWindowManagerHelper::hasCompositeChanged, this, &MainWindow::CompositeChanged, Qt::QueuedConnection);

    connect(m_widthAni, &QVariantAnimation::valueChanged, this, [ = ](const QVariant &value) {
        int width = value.toInt();

        move(int(std::round(qreal(m_rect.x() + m_rect.width()  + Globals::WindowMargin - width)) / qApp->primaryScreen()->devicePixelRatio()), m_rect.y());
    });

    QDBusServiceWatcher *m_watcher = new QDBusServiceWatcher(MONITOR_SERVICE, QDBusConnection::sessionBus());
    connect(m_watcher, &QDBusServiceWatcher::serviceRegistered, this, [ = ](const QString &service){
        if (MONITOR_SERVICE != service)
            return;
        registerMonitor();
    });

    connect(m_watcher, &QDBusServiceWatcher::serviceUnregistered, this, [ = ](const QString &service){
        if (MONITOR_SERVICE != service)
            return;
        disconnect(m_regionMonitor);
    });

    // 设置窗口透明度调节
    connect(m_daemonDockInter, &DBusDaemonDock::OpacityChanged, this, [=] () {
        this->setMaskAlpha(static_cast<quint8>(m_daemonDockInter->opacity() * 255));
    });

    connect(DBusAyatanaInterface::getInstance(), &DBusAyatanaInterface::sigSendCloseWidget, this, [=]() { QTimer::singleShot(0, this, &MainWindow::hideAni); });
}

void MainWindow::changeTheme(DApplicationHelper::ColorType themeType)
{
    // disable auto fill frame background
    QPalette palette;
    setAutoFillBackground(true);

    switch (themeType) {
    case DApplicationHelper::LightType:
        palette.setColor(QPalette::Background,QColor(210,210,210,75));
        m_scrollArea->verticalScrollBar()->setStyleSheet("QScrollBar::handle:vertical:hover{background-color:rgb(50,50,50);}");
        break;
    case DApplicationHelper::DarkType:
        setAutoFillBackground(false);
        //Dark主题的透明度为204
        palette.setColor(QPalette::Background,QColor(25,25,25,204));
        m_scrollArea->verticalScrollBar()->setStyleSheet("QScrollBar::handle:vertical:hover{background-color:rgb(200,200,200);}");
        break;
    default:
        break;
    }

    setPalette(palette);
    setBackgroundRole(DPalette::Window);
}

void MainWindow::adjustPosition()
{
    // 屏幕尺寸
    QRect rect = getDisplayScreen();
    qreal scale = qApp->primaryScreen()->devicePixelRatio();
    rect.setWidth(Globals::WindowWidth+10);

    int dockHeight = 0;
    int displayHeight = int(std::round(qreal(rect.height()) / scale));
    rect.setHeight(displayHeight);

    QRect dockRect = m_dockInter->geometry();
    dockRect.setWidth(int(std::round(qreal(dockRect.width()) / scale)));
    dockRect.setHeight(int(std::round(qreal(dockRect.height()) / scale)));

    // 初始化弹出框位置
    switch (m_daemonDockInter->position()) {
    case DOCK_TOP:
        rect.moveTop(dockRect.height());
        rect.setHeight(rect.height() - dockRect.height());
        rect.moveLeft(getDisplayScreen().x() + getDisplayScreen().width() - int(std::round(qreal(rect.width()) * scale)));
        dockHeight = int(std::round(qreal(dockRect.height()) / scale));
        break;
    case DOCK_BOTTOM:
        rect.setHeight(rect.height() - dockRect.height());
        rect.moveLeft(getDisplayScreen().x() + getDisplayScreen().width() - int(std::round(qreal(rect.width()) * scale)));
        dockHeight = int(std::round(qreal(dockRect.height()) / scale));

        break;
    case DOCK_LEFT:
        rect.moveLeft(getDisplayScreen().x() + getDisplayScreen().width() - rect.width());
        dockHeight = 0;
        break;
    case DOCK_RIGHT:
        if (m_daemonDockInter->displayMode() == 0) {
            rect.moveLeft(getDisplayScreen().x() + getDisplayScreen().width() - (rect.width() + dockRect.width()) - Globals::WindowMargin);
        } else {
            rect.moveLeft(getDisplayScreen().x() + getDisplayScreen().width() - (rect.width() + dockRect.width()));
        }
        dockHeight = 0;
        break;
    default:
        break;
    }

    int scrollHeight = displayHeight - dockHeight - Globals::WindowMargin*2 - 20;
    emit signal_geometry(scrollHeight);

    m_scrollArea->setFixedHeight(scrollHeight);
    m_scrollArea->horizontalScrollBar()->setEnabled(false);

    auto pal = qApp->palette();
    pal.setColor(QPalette::Background, QColor(0,0,0,0));
    m_scrollArea->setPalette(pal);
    m_scrollArea->setWindowFlags(Qt::FramelessWindowHint);
    m_scrollArea->setFrameShape(QFrame::NoFrame);


    // 右上下部分预留的间隙- dockRect.height()
    rect -= QMargins(0, Globals::WindowMargin, Globals::WindowMargin, Globals::WindowMargin);

    // 针对时尚模式的特殊处理
    // 只有任务栏显示的时候, 才额外偏移
    if(m_daemonDockInter->displayMode() == 0 && dockRect.width() * dockRect.height() > 0) {
        switch (m_daemonDockInter->position()) {
        case DOCK_TOP:
            rect -= QMargins(0, int(std::round(qreal(Globals::WindowMargin)) * scale), 0, 0);
            break;
        case DOCK_BOTTOM:
            rect -= QMargins(0, 0, 0, int(std::round(qreal(Globals::WindowMargin)) * scale));
            break;
        default:
            break;
        }
    }

    setGeometry(rect);
    m_rect = rect;
    setFixedSize(rect.size());
}

QRect MainWindow::getDisplayScreen()
{
    QRect dockRect = m_dockInter->geometry();
    for (const auto& monitorPath : m_displayInter->monitors()) {
        DisplayMonitor monitor("com.deepin.daemon.Display", monitorPath.path(), QDBusConnection::sessionBus());
        QRect screenRect(monitor.x(), monitor.y(), monitor.width(), monitor.height());
        if (screenRect.contains(dockRect))
            return screenRect;
    }
    return m_displayInter->primaryRect();
}

bool MainWindow::initDBus()
{
    // 申请一个总线连接
    if (!QDBusConnection::sessionBus().isConnected()) {
        return false;
    }

    // 在总线连接上挂载服务，这样其他进程才能请求该服务
    if (!QDBusConnection::sessionBus().registerService(Globals::SERVICE_NAME)) {
        return false;
    }

    // 在挂载的服务上注册一个执行服务的对象
    if (!QDBusConnection::sessionBus().registerObject(Globals::SERVICE_PATH, m_systemMonitorDbusAdaptor, QDBusConnection::ExportAllSlots |
                                                     QDBusConnection::ExportAllSignals)) {
        qInfo() << QDBusConnection::sessionBus().lastError();
        return false;
    }

    return true;
}

bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
    // 设置滚动区域显示策略
    if (event->type() == QEvent::MouseMove) {
        QMouseEvent *mouseEvent = dynamic_cast<QMouseEvent *>(event);
        if (m_last_time_move == 0) {
            m_last_time_move = mouseEvent->pos().y();
        }

        int distance = m_last_time_move - mouseEvent->pos().y();
        m_scrollArea->verticalScrollBar()->setValue(m_scrollArea->verticalScrollBar()->value() + distance);
        m_last_time_move = mouseEvent->pos().y();
    } else if (event->type() == QEvent::MouseButtonRelease) {
        m_last_time_move = 0;
    }
    return DBlurEffectWidget::eventFilter(object, event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    //禁止窗口被拖动
    Q_UNUSED(event);
    return;
}

void MainWindow::slotShowOrHideSystemMonitorPluginPopupWidget()
{
    QTimer::singleShot(0, this, &MainWindow::Toggle);
}

