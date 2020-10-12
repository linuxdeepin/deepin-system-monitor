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

#include "process_page_widget.h"

#include "application.h"
#include "main_window.h"
#include "kill_process_confirm_dialog.h"
#include "xwin_kill_preview_widget.h"
#include "monitor_compact_view.h"
#include "monitor_expand_view.h"
#include "process/system_monitor.h"
#include "process_table_view.h"
#include "process/stats_collector.h"
#include "constant.h"
#include "settings.h"
#include "ui_common.h"
#include "utils.h"

#include <DApplication>
#include <DApplicationHelper>
#include <DFontSizeManager>
#include <DLabel>
#include <DStackedWidget>

#include <QDebug>
#include <QDesktopWidget>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QTimer>
#include <QPainterPath>

DWIDGET_USE_NAMESPACE

// process context summary text
static const char *kProcSummaryTemplateText =
    QT_TRANSLATE_NOOP("Process.Summary", "(%1 applications and %2 processes are running)");

// application context text
static const char *appText = QT_TRANSLATE_NOOP("Process.Show.Mode", "Applications");
// my process context mode text
static const char *myProcText = QT_TRANSLATE_NOOP("Process.Show.Mode", "My processes");
// all process context mode text
static const char *allProcText = QT_TRANSLATE_NOOP("Process.Show.Mode", "All processes");

// constructor
ProcessPageWidget::ProcessPageWidget(DWidget *parent)
    : DFrame(parent)
{
    // initialize global settings
    m_settings = Settings::instance();
    if (m_settings) {
        m_settings->init();
    }

    // initialize ui components
    initUI();
    initConnections();
}

// destructor
ProcessPageWidget::~ProcessPageWidget() {}

// initialize ui components
void ProcessPageWidget::initUI()
{
    // global DStyle instance
    auto *style = dynamic_cast<DStyle *>(DApplication::style());
    // global app helper instance
    auto *dAppHelper = DApplicationHelper::instance();
    // global palette
    auto palette = dAppHelper->applicationPalette();

    // initialize style option
    QStyleOption option;
    option.initFrom(this);

    // content margin
    int margin = style->pixelMetric(DStyle::PM_ContentsMargins, &option);

    // tool area background
    auto *tw = new QWidget(this);
    // content area background
    auto *cw = new QWidget(this);

    // left =====> stackview
    m_views = new DStackedWidget(this);
    m_views->setAutoFillBackground(false);
    m_views->setContentsMargins(0, 0, 0, 0);
    m_views->setAutoFillBackground(false);
    // compact view instance
    m_compactView = new MonitorCompactView(m_views);
    // expand view instance
    m_expandView = new MonitorExpandView(m_views);
    m_views->addWidget(m_compactView);
    m_views->addWidget(m_expandView);
    m_views->setFixedWidth(300);

    // restore previous backupped display mode if any
    auto *settings = Settings::instance();
    if (settings) {
        auto mode = settings->getOption(kSettingKeyDisplayMode);
        if (mode.isValid()) {
            if (qvariant_cast<int>(mode) == kDisplayModeCompact) {
                m_views->setCurrentIndex(0);
            } else {
                m_views->setCurrentIndex(1);
            }
        }
    }

    // right ====> tab button + process table
    auto *contentlayout = new QVBoxLayout(cw);
    contentlayout->setSpacing(margin);
    contentlayout->setContentsMargins(0, 0, 0, 0);

    // tools area layout
    auto *toolsLayout = new QHBoxLayout(tw);
    toolsLayout->setSpacing(margin);
    toolsLayout->setContentsMargins(0, 0, 0, 0);

    // process context instance
    m_procViewMode = new DLabel(tw);
    m_procViewMode->setText(DApplication::translate("Process.Show.Mode", appText));  // default text
    DFontSizeManager::instance()->bind(m_procViewMode, DFontSizeManager::T7, QFont::Medium);
    // text aligment
    m_procViewMode->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    // change text color to text title style
    auto pam = DApplicationHelper::instance()->palette(m_procViewMode);
    palette.setColor(DPalette::Text, palette.color(DPalette::TextTitle));
    m_procViewMode->setPalette(palette);

    // process context summary instance
    m_procViewModeSummary = new DLabel(tw);
    DFontSizeManager::instance()->bind(m_procViewModeSummary, DFontSizeManager::T7, QFont::Medium);
    // text aligment & elide mode
    m_procViewModeSummary->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_procViewModeSummary->setElideMode(Qt::ElideRight);
    // change text color to text tips style
    auto pa = DApplicationHelper::instance()->palette(m_procViewModeSummary);
    palette.setColor(DPalette::Text, palette.color(DPalette::TextTips));
    m_procViewModeSummary->setPalette(palette);

    // change icon type when theme changed
    connect(dAppHelper, &DApplicationHelper::themeTypeChanged, this,
            &ProcessPageWidget::changeIconTheme);

    auto *modeButtonGroup = new DButtonBox(tw);
    modeButtonGroup->setFixedWidth(30 * 3);
    modeButtonGroup->setFixedHeight(26);

    // show application mode button
    m_appButton = new DButtonBoxButton(QIcon(), {}, modeButtonGroup);
    m_appButton->setIconSize(QSize(26, 24));
    m_appButton->setCheckable(true);
    m_appButton->setFocusPolicy(Qt::TabFocus);
    m_appButton->setToolTip(DApplication::translate("Process.Show.Mode", appText));
    m_appButton->setAccessibleName(m_appButton->toolTip());

    // show my process mode button
    m_myProcButton = new DButtonBoxButton(QIcon(), {}, modeButtonGroup);
    m_myProcButton->setIconSize(QSize(26, 24));
    m_myProcButton->setCheckable(true);
    m_myProcButton->setFocusPolicy(Qt::TabFocus);
    m_myProcButton->setToolTip(DApplication::translate("Process.Show.Mode", myProcText));
    m_myProcButton->setAccessibleName(m_myProcButton->toolTip());

    // show all process mode button
    m_allProcButton = new DButtonBoxButton(QIcon(), {}, modeButtonGroup);
    m_allProcButton->setIconSize(QSize(26, 24));
    m_allProcButton->setCheckable(true);
    m_allProcButton->setFocusPolicy(Qt::TabFocus);
    m_allProcButton->setToolTip(DApplication::translate("Process.Show.Mode", allProcText));
    m_allProcButton->setAccessibleName(m_allProcButton->toolTip());

    // install event filters to handle left/right direction key press
    m_appButton->installEventFilter(this);
    m_myProcButton->installEventFilter(this);
    m_allProcButton->installEventFilter(this);

    // change icon type based on current theme when initialized
    changeIconTheme(dAppHelper->themeType());

    QList<DButtonBoxButton *> list;
    list << m_appButton << m_myProcButton << m_allProcButton;
    modeButtonGroup->setButtonList(list, true);

    // add widgets to tools layout
    toolsLayout->addWidget(m_procViewMode, 0, Qt::AlignLeft | Qt::AlignVCenter);
    toolsLayout->addWidget(m_procViewModeSummary, 0, Qt::AlignLeft | Qt::AlignVCenter);
    toolsLayout->addStretch();
    toolsLayout->addWidget(modeButtonGroup, 0, Qt::AlignRight);

    tw->setLayout(toolsLayout);

    // process table view instance
    m_procTable = new ProcessTableView(cw);
    contentlayout->addWidget(tw);
    contentlayout->addWidget(m_procTable, 1);
    cw->setLayout(contentlayout);

    // fill left side bar & main content area
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(margin, margin, margin, margin);
    layout->setSpacing(margin);
    layout->addWidget(m_views);
    layout->addWidget(cw);
    setLayout(layout);

    setAutoFillBackground(false);

    // restore previous saved process view mode if any previous settings found
    QVariant vindex = m_settings->getOption(kSettingKeyProcessTabIndex);
    int index = 0;
    if (vindex.isValid())
        index = vindex.toInt();
    switch (index) {
    case SystemMonitor::OnlyMe: {
        // show my process view
        m_myProcButton->setChecked(true);
        m_procTable->switchDisplayMode(SystemMonitor::OnlyMe);
        m_procViewMode->setText(
            DApplication::translate("Process.Show.Mode", myProcText));  // default text
    } break;
    case SystemMonitor::AllProcess: {
        // show all process view
        m_allProcButton->setChecked(true);
        m_procTable->switchDisplayMode(SystemMonitor::AllProcess);
        m_procViewMode->setText(
            DApplication::translate("Process.Show.Mode", allProcText));  // default text
    } break;
    default: {
        // show my application view by default
        m_appButton->setChecked(true);
        m_procTable->switchDisplayMode(SystemMonitor::OnlyGUI);
        m_procViewMode->setText(
            DApplication::translate("Process.Show.Mode", appText));  // default text
    }
    }
}

void ProcessPageWidget::initConnections()
{
    auto *mainWindow = gApp->mainWindow();
    // kill application signal triggered, show application kill preview widget
    connect(mainWindow, &MainWindow::killProcessPerformed, this,
            &ProcessPageWidget::showWindowKiller);
    // switch display mode signal triggered, switch performance display mode
    connect(mainWindow, &MainWindow::displayModeChanged, this,
            &ProcessPageWidget::switchDisplayMode);

    // show my application when my application button toggled
    connect(m_appButton, &DButtonBoxButton::toggled, this, [=](bool) {
        m_procViewMode->setText(DApplication::translate("Process.Show.Mode", appText));
        m_procViewMode->adjustSize();
        m_procTable->switchDisplayMode(SystemMonitor::OnlyGUI);
        m_settings->setOption(kSettingKeyProcessTabIndex, SystemMonitor::OnlyGUI);
    });
    // show my process when my process button toggled
    connect(m_myProcButton, &DButtonBoxButton::toggled, this, [=](bool) {
        m_procViewMode->setText(DApplication::translate("Process.Show.Mode", myProcText));
        m_procViewMode->adjustSize();
        m_procTable->switchDisplayMode(SystemMonitor::OnlyMe);
        m_settings->setOption(kSettingKeyProcessTabIndex, SystemMonitor::OnlyMe);
    });
    // show all application when all application button toggled
    connect(m_allProcButton, &DButtonBoxButton::toggled, this, [=](bool) {
        m_procViewMode->setText(DApplication::translate("Process.Show.Mode", allProcText));
        m_procViewMode->adjustSize();
        m_procTable->switchDisplayMode(SystemMonitor::AllProcess);
        m_settings->setOption(kSettingKeyProcessTabIndex, SystemMonitor::AllProcess);
    });

    auto *smo = SystemMonitor::instance();
    Q_ASSERT(smo != nullptr);
    // update process summary text when process summary info updated background
    connect(smo->jobInstance(), &StatsCollector::processSummaryUpdated,
            this, &ProcessPageWidget::updateProcessSummary);

    auto *dAppHelper = DApplicationHelper::instance();
    // change text color dynamically on theme type change, if not do this way, text color wont synchronize with theme type
    connect(dAppHelper, &DApplicationHelper::themeTypeChanged, this, [ = ]() {
        if (m_procViewMode) {
            auto palette = DApplicationHelper::instance()->applicationPalette();
            palette.setColor(DPalette::Text, palette.color(DPalette::TextTitle));
            m_procViewMode->setPalette(palette);
        }
        if (m_procViewModeSummary) {
            auto palette = DApplicationHelper::instance()->applicationPalette();
            palette.setColor(DPalette::Text, palette.color(DPalette::TextTips));
            m_procViewModeSummary->setPalette(palette);
        }
    });
}

// event filter
bool ProcessPageWidget::eventFilter(QObject *obj, QEvent *event)
{
    // filter key press events for process show mode buttons, move focus ether way
    if (event->type() == QEvent::KeyPress) {
        if (obj == m_appButton) {
            auto *kev = dynamic_cast<QKeyEvent *>(event);
            if (kev->key() == Qt::Key_Right) {
                m_myProcButton->setFocus();
                return true;
            }
        } else if (obj == m_myProcButton) {
            auto *kev = dynamic_cast<QKeyEvent *>(event);
            if (kev->key() == Qt::Key_Right) {
                m_allProcButton->setFocus();
                return true;
            } else if (kev->key() == Qt::Key_Left) {
                m_appButton->setFocus();
                return true;
            }
        } else if (obj == m_allProcButton) {
            auto *kev = dynamic_cast<QKeyEvent *>(event);
            if (kev->key() == Qt::Key_Left) {
                m_myProcButton->setFocus();
                return true;
            }
        }
    }

    return DFrame::eventFilter(obj, event);
}

// paint event handler
void ProcessPageWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QPainterPath path;
    path.addRect(QRectF(rect()));
    painter.setOpacity(1);

    DApplicationHelper *dAppHelper = DApplicationHelper::instance();
    DPalette palette = dAppHelper->applicationPalette();
    QColor bgColor = palette.color(DPalette::Background);

    // paint frame backgroud with current themed background color
    painter.fillPath(path, bgColor);
}

// create application kill preview widget
void ProcessPageWidget::createWindowKiller()
{
    // application kill preview instace
    m_xwkillPreview = new XWinKillPreviewWidget();
    // handle left mouse button click event inside the preview widget, popup process kill confirm dialog
    connect(m_xwkillPreview,
            &XWinKillPreviewWidget::windowClicked,
            this,
            &ProcessPageWidget::popupKillConfirmDialog);
}

// update process context summary text
void ProcessPageWidget::updateProcessSummary(int napps, int nprocs)
{
    QString buf = DApplication::translate("Process.Summary", kProcSummaryTemplateText);
    m_procViewModeSummary->setText(buf.arg(napps).arg(nprocs));
}

// change icon theme when theme changed
void ProcessPageWidget::changeIconTheme(DGuiApplicationHelper::ColorType themeType)
{
    QIcon appIcon;
    QIcon myProcIcon;
    QIcon allProcIcon;

    if (themeType == DApplicationHelper::LightType) {
        // light theme icon
        appIcon.addFile(iconPathFromQrc("light/app_normal.svg"), {}, QIcon::Normal, QIcon::Off);
        appIcon.addFile(iconPathFromQrc("light/app_highlight.svg"), {}, QIcon::Normal, QIcon::On);

        myProcIcon.addFile(iconPathFromQrc("light/me_normal.svg"), {}, QIcon::Normal, QIcon::Off);
        myProcIcon.addFile(iconPathFromQrc("light/me_highlight.svg"), {}, QIcon::Normal, QIcon::On);

        allProcIcon.addFile(iconPathFromQrc("light/all_normal.svg"), {}, QIcon::Normal, QIcon::Off);
        allProcIcon.addFile(iconPathFromQrc("light/all_highlight.svg"), {}, QIcon::Normal, QIcon::On);
    } else if (themeType == DApplicationHelper::DarkType) {
        // dark theme icon
        appIcon.addFile(iconPathFromQrc("dark/app_normal_dark.svg"), {}, QIcon::Normal, QIcon::Off);
        appIcon.addFile(iconPathFromQrc("dark/app_highlight.svg"), {}, QIcon::Normal, QIcon::On);

        myProcIcon.addFile(iconPathFromQrc("dark/me_normal_dark.svg"), {}, QIcon::Normal, QIcon::Off);
        myProcIcon.addFile(iconPathFromQrc("dark/me_highlight.svg"), {}, QIcon::Normal, QIcon::On);

        allProcIcon.addFile(iconPathFromQrc("dark/all_normal_dark.svg"), {}, QIcon::Normal, QIcon::Off);
        allProcIcon.addFile(iconPathFromQrc("dark/all_highlight.svg"), {}, QIcon::Normal, QIcon::On);
    }

    m_appButton->setIcon(appIcon);
    m_appButton->setIconSize(QSize(26, 24));

    m_myProcButton->setIcon(myProcIcon);
    m_myProcButton->setIconSize(QSize(26, 24));

    m_allProcButton->setIcon(allProcIcon);
    m_allProcButton->setIconSize(QSize(26, 24));
}

// popup application kill confirm dialog
void ProcessPageWidget::popupKillConfirmDialog(pid_t pid)
{
    QString title = DApplication::translate("Kill.Process.Dialog", "End process");
    QString description = DApplication::translate("Kill.Process.Dialog",
                                                  "Force ending this application may cause data "
                                                  "loss.\nAre you sure you want to continue?");

    KillProcessConfirmDialog dialog;
    // always stay on top
    dialog.setWindowFlag(Qt::WindowStaysOnTopHint);
    dialog.setMessage(description);
    dialog.addButton(DApplication::translate("Kill.Process.Dialog", "Cancel"), false);
    dialog.addButton(DApplication::translate("Kill.Process.Dialog", "Force End"), true,
                     DDialog::ButtonWarning);
    dialog.exec();
    if (dialog.result() == QMessageBox::Ok) {
        auto *sysmon = SystemMonitor::instance();
        if (sysmon) {
            sysmon->killProcess(qvariant_cast<pid_t>(pid));
        }
    }
}

// show application kill preview widget
void ProcessPageWidget::showWindowKiller()
{
    auto *mw = gApp->mainWindow();
    // minimize main window before show application kill preview widget
    if (mw)
        mw->showMinimized();

    // use a timer to slightly delay preview widget shown to fix glitched screen snapshot pixmap on low end platforms
    QTimer::singleShot(500, this, SLOT(createWindowKiller()));
}

// switch performance view display mode
void ProcessPageWidget::switchDisplayMode(DisplayMode mode)
{
    switch (mode) {
    case kDisplayModeExpand: {
        m_views->setCurrentIndex(1);
    } break;
    case kDisplayModeCompact: {
        m_views->setCurrentIndex(0);
    } break;
    }
}

void ProcessPageWidget::adjustStatusBarWidth()
{
//    QRect rect = QApplication::desktop()->screenGeometry();

//    // Just change status monitor width when screen width is more than 1024.
//    int statusBarMaxWidth = Utils::getStatusBarMaxWidth();
//    if (rect.width() * 0.2 > statusBarMaxWidth) {
//        if (windowState() == Qt::WindowMaximized) {
//            m_views->setFixedWidth(int(rect.width() * 0.2));
//        } else {
//            m_views->setFixedWidth(statusBarMaxWidth);
//        }
//    }
// set fixed width temporary, might add splitter between sidebar & content area in a late update
m_views->setFixedWidth(300);
}
