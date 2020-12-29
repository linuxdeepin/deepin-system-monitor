/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      maojj <maojunjie@uniontech.com>
* Maintainer:  maojj <maojunjie@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "main_window.h"

#include "application.h"
#include "process_page_widget.h"
#include "system_service_page_widget.h"
#include "toolbar.h"
#include "common/common.h"
#include "settings.h"
#include "constant.h"
#include "common/perf.h"

#include <DApplicationHelper>
#include <DHiDPIHelper>
#include <DShadowLine>
#include <DStackedWidget>
#include <DTitlebar>
#include <DPalette>

#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QDebug>
#include <QTimer>
#include <QResizeEvent>

DGUI_USE_NAMESPACE

// constructor
MainWindow::MainWindow(QWidget *parent)
    : DMainWindow(parent)
{
    // get global setings instance
    m_settings = Settings::instance();
    if (m_settings)
        m_settings->init();

    // listen on loading status changed signal
    connect(this, &MainWindow::loadingStatusChanged, [ = ](bool loading) {
        if (loading) {
            // disable titlebar menu
            titlebar()->setMenuDisabled(true);
        } else {
            // enable titlebar menu
            titlebar()->setMenuDisabled(false);
        }
    });
}

// destructor
MainWindow::~MainWindow()
{
    PERF_PRINT_END("POINT-02");
}

// show shortcut help dialog
void MainWindow::displayShortcutHelpDialog()
{
    // calculate mainwindow central point
    QRect rect = window()->geometry();
    QPoint pos(rect.x() + rect.width() / 2, rect.y() + rect.height() / 2);

    QJsonObject shortcutObj;
    QJsonArray jsonGroups;

    // system section
    QJsonObject sysObj;
    sysObj.insert("groupName", DApplication::translate("Help.Shortcut.System", "System"));
    QJsonArray sysObjArr;

    // display shortcut shortcut help
    QJsonObject shortcutItem;
    shortcutItem.insert("name",
                        DApplication::translate("Help.Shortcut.System", "Display shortcuts"));
    shortcutItem.insert("value", "Ctrl+Shift+?");
    sysObjArr.append(shortcutItem);

    // display search shortcut help
    QJsonObject searchItem;
    searchItem.insert("name", DApplication::translate("Title.Bar.Search", "Search"));
    searchItem.insert("value", "Ctrl+F");
    sysObjArr.append(searchItem);

    sysObj.insert("groupItems", sysObjArr);
    jsonGroups.append(sysObj);

    // processes section
    QJsonObject procObj;
    procObj.insert("groupName", DApplication::translate("Title.Bar.Switch", "Processes"));
    QJsonArray procObjArr;

    // force end application shortcut help
    QJsonObject killAppItem;
    killAppItem.insert("name",
                       DApplication::translate("Title.Bar.Context.Menu", "Force end application"));
    killAppItem.insert("value", "Ctrl+Alt+K");
    procObjArr.append(killAppItem);

    // end process shortcut help
    QJsonObject endProcItem;
    endProcItem.insert("name",
                       DApplication::translate("Process.Table.Context.Menu", "End process"));
    endProcItem.insert("value", "Alt+E");
    procObjArr.append(endProcItem);
    // suspend process shortcut help
    QJsonObject pauseProcItem;
    pauseProcItem.insert("name",
                         DApplication::translate("Process.Table.Context.Menu", "Suspend process"));
    pauseProcItem.insert("value", "Alt+P");
    procObjArr.append(pauseProcItem);
    // resume process shortcut help
    QJsonObject resumeProcItem;
    resumeProcItem.insert("name",
                          DApplication::translate("Process.Table.Context.Menu", "Resume process"));
    resumeProcItem.insert("value", "Alt+C");
    procObjArr.append(resumeProcItem);
    // properties shortcut help
    QJsonObject propItem;
    propItem.insert("name", DApplication::translate("Process.Table.Context.Menu", "Properties"));
    propItem.insert("value", "Alt+Enter");
    procObjArr.append(propItem);
    // kill process shortcut help
    QJsonObject killProcItem;
    killProcItem.insert("name",
                        DApplication::translate("Process.Table.Context.Menu", "Kill process"));
    killProcItem.insert("value", "Alt+K");
    procObjArr.append(killProcItem);

    procObj.insert("groupItems", procObjArr);
    jsonGroups.append(procObj);

    // services section
    QJsonObject svcObj;
    svcObj.insert("groupName", DApplication::translate("Title.Bar.Switch", "Services"));
    QJsonArray svcObjArr;

    // start service shortcut help
    QJsonObject startSvcItem;
    startSvcItem.insert("name", DApplication::translate("Service.Table.Context.Menu", "Start"));
    startSvcItem.insert("value", "Alt+S");
    svcObjArr.append(startSvcItem);
    // stop service shortcut help
    QJsonObject stopSvcItem;
    stopSvcItem.insert("name", DApplication::translate("Service.Table.Context.Menu", "Stop"));
    stopSvcItem.insert("value", "Alt+T");
    svcObjArr.append(stopSvcItem);
    // restart service shortcut help
    QJsonObject restartSvcItem;
    restartSvcItem.insert("name", DApplication::translate("Service.Table.Context.Menu", "Restart"));
    restartSvcItem.insert("value", "Alt+R");
    svcObjArr.append(restartSvcItem);
    // refresh service shortcut help
    QJsonObject refreshSvcItem;
    refreshSvcItem.insert("name", DApplication::translate("Service.Table.Context.Menu", "Refresh"));
    refreshSvcItem.insert("value", "F5");
    svcObjArr.append(refreshSvcItem);

    svcObj.insert("groupItems", svcObjArr);
    jsonGroups.append(svcObj);

    shortcutObj.insert("shortcut", jsonGroups);

    QJsonDocument doc(shortcutObj);

    // fork new shortcut display process
    QProcess *shortcutViewProcess = new QProcess();
    QStringList shortcutString;
    // shortcut help json formatted text param
    QString param1 = "-j=" + QString(doc.toJson().data());
    // display position param
    QString param2 = "-p=" + QString::number(pos.x()) + "," + QString::number(pos.y());
    shortcutString << param1 << param2;

    // detach this process
    shortcutViewProcess->startDetached("deepin-shortcut-viewer", shortcutString);

    // delete process instance after successfully forked shortcut display process
    connect(shortcutViewProcess, SIGNAL(finished(int)), shortcutViewProcess, SLOT(deleteLater()));
}

// initialize ui components
void MainWindow::initUI()
{
    // initialize loading status flag
    m_loading = true;
    // trigger loading status changed signal for once
    Q_EMIT loadingStatusChanged(m_loading);

    // default window size
    int width = Constant::WINDOW_MIN_WIDTH;
    int height = Constant::WINDOW_MIN_HEIGHT;

    // if settings has window width saved, restore with saved width
    if (m_settings->getOption(kSettingKeyWindowWidth).isValid()) {
        width = m_settings->getOption(kSettingKeyWindowWidth).toInt();
    }

    // if settings has window height saved, restore with saved height
    if (m_settings->getOption(kSettingKeyWindowHeight).isValid()) {
        height = m_settings->getOption(kSettingKeyWindowHeight).toInt();
    }

    // resize main window
    resize(width, height);

    // set titlebar icon
    titlebar()->setIcon(QIcon::fromTheme("deepin-system-monitor"));
    // new toolbar instance to add custom widgets
    m_toolbar = new Toolbar(this);
    // set toolbar focus policy to only accept tab focus
    m_toolbar->setFocusPolicy(Qt::TabFocus);
    titlebar()->setCustomWidget(m_toolbar);
    // disable toolbar menu right after initialization, only when loading status changed to finished then we enable it
    titlebar()->setMenuDisabled(true);

    // custom menu to hold custom menu items
    DMenu *menu = new DMenu(this);
    titlebar()->setMenu(menu);

    // adjust toolbar tab order
    setTabOrder(titlebar(), m_toolbar);

    // kill process menu item
    m_killAction = new QAction(
        DApplication::translate("Title.Bar.Context.Menu", "Force end application"), menu);
    // control + alt + k
    m_killAction->setShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_K));
    // emit process kill requested signal if kill process menu item triggered
    connect(m_killAction, &QAction::triggered, this, [ = ]() { Q_EMIT killProcessPerformed(); });

    // display mode menu item
    m_modeMenu = new DMenu(DApplication::translate("Title.Bar.Context.Menu", "View"), menu);
    QActionGroup *modeGroup = new QActionGroup(m_modeMenu);
    // set group items auto exclusive
    modeGroup->setExclusive(true);
    // expand mode sub menu item
    auto *expandModeAction =
        new QAction(DApplication::translate("Title.Bar.Context.Menu", "Expand"), modeGroup);
    expandModeAction->setCheckable(true);
    // compact mode sub menu item
    auto *compactModeAction =
        new QAction(DApplication::translate("Title.Bar.Context.Menu", "Compact"), modeGroup);
    compactModeAction->setCheckable(true);
    // add compact & expand sub menus into display mode menu as a group
    m_modeMenu->addAction(expandModeAction);
    m_modeMenu->addAction(compactModeAction);

    // load display mode backup setting
    QVariant vmode = m_settings->getOption(kSettingKeyDisplayMode);
    if (vmode.isValid()) {
        bool ok = false;
        int mode = vmode.toInt(&ok);
        if (ok) {
            // if backup settings has display mode saved, use saved setting
            if (mode == kDisplayModeExpand) {
                expandModeAction->setChecked(true);
            } else if (mode == kDisplayModeCompact) {
                compactModeAction->setChecked(true);
            }
        } else {
            // if no backup settings saved or setting load failed, show expand mode by default
            expandModeAction->setChecked(true);
            m_settings->setOption(kSettingKeyDisplayMode, kDisplayModeExpand);
            Q_EMIT displayModeChanged(kDisplayModeExpand);
        }
    }

    // emit display mode changed signal if ether expand or compact menu item triggered
    connect(expandModeAction, &QAction::triggered, this, [ = ]() {
        m_settings->setOption(kSettingKeyDisplayMode, kDisplayModeExpand);
        Q_EMIT displayModeChanged(kDisplayModeExpand);
    });
    connect(compactModeAction, &QAction::triggered, this, [ = ]() {
        m_settings->setOption(kSettingKeyDisplayMode, kDisplayModeCompact);
        Q_EMIT displayModeChanged(kDisplayModeCompact);
    });

    menu->addAction(m_killAction);
    menu->addSeparator();
    menu->addMenu(m_modeMenu);

    // listen on theme changed signal
    DApplicationHelper *dAppHelper = DApplicationHelper::instance();
    connect(dAppHelper, &DApplicationHelper::themeTypeChanged, this,
    [ = ]() { m_settings->setOption(kSettingKeyThemeType, DApplicationHelper::LightType); });

    // stacked widget instance to hold process & service pages
    m_pages = new DStackedWidget(this);
    setCentralWidget(m_pages);
    setContentsMargins(0, 0, 0, 0);

    // shadow widget instance under titlebar
    m_tbShadow = new DShadowLine(m_pages);
    m_tbShadow->setFixedWidth(m_pages->width());
    m_tbShadow->setFixedHeight(10);
    m_tbShadow->move(0, 0);
    m_tbShadow->show();

    // change application highlight color to themed highlight color
    auto pa = DApplicationHelper::instance()->applicationPalette();
    QBrush hlBrush = pa.color(DPalette::Active, DPalette::Highlight);
    pa.setColor(DPalette::Highlight, hlBrush.color());

    // process page widget
    m_procPage = new ProcessPageWidget(m_pages);
    // service page widget
    m_svcPage = new SystemServicePageWidget(m_pages);
    m_pages->setContentsMargins(0, 0, 0, 0);
    m_pages->addWidget(m_procPage);
    m_pages->addWidget(m_svcPage);

    // raise shadow widget in case being corvered by other children widgets
    m_tbShadow->raise();

    // finally emit loading status changed signal to finished state
    m_loading = false;
    Q_EMIT loadingStatusChanged(m_loading);

    // install event filter for main window widget
    installEventFilter(this);
}

// initialize connections
void MainWindow::initConnections()
{
    // listen on process button triggered signal
    connect(m_toolbar, &Toolbar::procTabButtonClicked, this, [=]() {
        PERF_PRINT_BEGIN("POINT-05", QString("switch(%1->%2)").arg(DApplication::translate("Title.Bar.Switch", "Services")).arg(DApplication::translate("Title.Bar.Switch", "Processes")));
        // clear search text input widget
        m_toolbar->clearSearchText();
        // swap stacked widget to show process page
        m_pages->setCurrentIndex(m_pages->indexOf(m_procPage));
        // raise & show shadow widget incase corvered by other widgets
        m_tbShadow->raise();
        m_tbShadow->show();
        PERF_PRINT_END("POINT-05");
    });
    // listen on service button triggered signal
    connect(m_toolbar, &Toolbar::serviceTabButtonClicked, this, [=]() {
        PERF_PRINT_BEGIN("POINT-05", QString("switch(%1->%2)").arg(DApplication::translate("Title.Bar.Switch", "Processes")).arg(DApplication::translate("Title.Bar.Switch", "Services")));
        // clear search text input widget
        m_toolbar->clearSearchText();
        // swap stacked widget to show service page
        m_pages->setCurrentIndex(m_pages->indexOf(m_svcPage));
        // raise & show shadow widget incase corvered by other widgets
        m_tbShadow->raise();
        m_tbShadow->show();
        PERF_PRINT_END("POINT-05");
    });

    // listen on background task state changed signal
    connect(gApp, &Application::backgroundTaskStateChanged, this, [=](Application::TaskState state) {
        if (state == Application::kTaskStarted) {
            // save last focused widget inside main window
            m_focusedWidget = gApp->mainWindow()->focusWidget();
            // disable main window (and any children widgets inside) if background process ongoing
            setEnabled(false);
        } else if (state == Application::kTaskFinished) {
            // reenable main window after background process finished
            setEnabled(true);
            // restore focus to last focused widget if any
            if (m_focusedWidget && m_focusedWidget->isEnabled()) {
                m_focusedWidget->setFocus();
            }
        }
    });
}

// resize event handler
void MainWindow::resizeEvent(QResizeEvent *event)
{
    // propogate resize event to base handler first
    DMainWindow::resizeEvent(event);

    // resize shadow widget
    if (m_tbShadow)
        m_tbShadow->setFixedWidth(event->size().width());

    // save new size to backup settings instace
    m_settings->setOption(kSettingKeyWindowWidth, event->size().width());
    m_settings->setOption(kSettingKeyWindowHeight, event->size().height());
}

// close event handler
void MainWindow::closeEvent(QCloseEvent *event)
{
    PERF_PRINT_BEGIN("POINT-02", "");
    // save new size to backup settings instace
    m_settings->setOption(kSettingKeyWindowWidth, width());
    m_settings->setOption(kSettingKeyWindowHeight, height());
    // flush backup settings
    m_settings->flush();

    // propogate close event to base event handler
    DMainWindow::closeEvent(event);
}

// event filter handler
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    // handle key press events
    if (event->type() == QEvent::KeyPress) {
        auto *kev = dynamic_cast<QKeyEvent *>(event);
        if (kev->matches(QKeySequence::Quit)) {
            // ESC pressed
            DApplication::quit();
            return true;
        } else if (kev->matches(QKeySequence::Find)) {
            // CTRL + F pressed
            toolbar()->focusInput();
            return true;
        } else if ((kev->modifiers() == (Qt::ControlModifier | Qt::AltModifier)) && kev->key() == Qt::Key_K) {
            // CTRL + ALT + K pressed
            Q_EMIT killProcessPerformed();
            return true;
        } else if ((kev->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier)) && kev->key() == Qt::Key_Question) {
            // CTRL + SHIFT + ? pressed
            displayShortcutHelpDialog();
            return true;
        } else {
            return false;
        }
    } else {
        // propogate other events to base handler
        return DMainWindow::eventFilter(obj, event);
    }
}

// show event handler
void MainWindow::showEvent(QShowEvent *event)
{
    // propogate show event to base handler first
    DMainWindow::showEvent(event);

    std::call_once(oflag, []() {
        auto *msev = new MonitorStartEvent();
        gApp->postEvent(gApp, msev);

        PERF_PRINT_END("POINT-01");
    });
}
