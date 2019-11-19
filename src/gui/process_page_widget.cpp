/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2018 Deepin, Inc.
 *               2011 ~ 2018 Wang Yong
 *
 * Author:     Wang Yong <wangyong@deepin.com>
 * Maintainer: Wang Yong <wangyong@deepin.com>
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

#include <signal.h>
#include <iostream>

#include <DApplication>
#include <DApplicationHelper>
#include <DHiDPIHelper>
#include <DStackedWidget>
#include <DTitlebar>
#include <QDebug>
#include <QDesktopWidget>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeyEvent>
#include <QMessageBox>
#include <QStyleFactory>
#include <QVBoxLayout>

#include "constant.h"
#include "kill_process_confirm_dialog.h"
#include "main_window.h"
#include "monitor_compact_view.h"
#include "monitor_expand_view.h"
#include "process/system_monitor.h"
#include "process_page_widget.h"
#include "process_table_view.h"
#include "ui_common.h"
#include "utils.h"

using namespace std;

DWIDGET_USE_NAMESPACE

static const QString kProcSummaryTemplateText = {
    QT_TRANSLATE_NOOP("Process.Summary", "(%1 applications and %2 processes are running)")};

static const QString appText = DApplication::translate("Process.Show.Mode", "Applications");
static const QString myProcText = DApplication::translate("Process.Show.Mode", "My processes");
static const QString allProcText = DApplication::translate("Process.Show.Mode", "All processes");

ProcessPageWidget::ProcessPageWidget(DWidget *parent)
    : DFrame(parent)
{
    m_settings = Settings::instance();
    if (m_settings) {
        m_settings->init();
    }

    initUI();
    initConnections();
}

ProcessPageWidget::~ProcessPageWidget() {}

void ProcessPageWidget::initUI()
{
    DStyle *style = dynamic_cast<DStyle *>(DApplication::style());
    DApplicationHelper *dAppHelper = DApplicationHelper::instance();
    DPalette palette = dAppHelper->applicationPalette();
    QStyleOption option;
    option.initFrom(this);
    int margin = style->pixelMetric(DStyle::PM_ContentsMargins, &option);

    auto *tw = new QWidget(this);
    auto *cw = new QWidget(this);

    // left =====> stackview
    m_views = new DStackedWidget(this);
    m_views->setAutoFillBackground(false);
    m_views->setContentsMargins(0, 0, 0, 0);
    m_views->setAutoFillBackground(false);
    m_compactView = new MonitorCompactView(m_views);
    m_expandView = new MonitorExpandView(m_views);
    m_views->addWidget(m_compactView);
    m_views->addWidget(m_expandView);
    m_views->setFixedWidth(280);
    // TODO: fix size problem
    //    adjustStatusBarWidth();

    // right ====> tab button + process table
    auto *contentlayout = new QVBoxLayout(cw);
    contentlayout->setSpacing(margin);
    contentlayout->setContentsMargins(0, 0, 0, 0);

    auto *toolsLayout = new QHBoxLayout(tw);
    toolsLayout->setSpacing(margin);
    toolsLayout->setContentsMargins(0, 0, 0, 0);

    m_procViewMode = new DLabel(tw);
    m_procViewMode->setFixedHeight(24);
    m_procViewMode->setText(appText);  // default text
    m_procViewMode->adjustSize();
    QFont fn = m_procViewMode->font();
    fn.setWeight(QFont::Medium);
    fn.setPointSize(fn.pointSize() - 1);
    m_procViewMode->setFont(fn);
    m_procViewMode->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    m_procViewModeSummary = new DLabel(tw);
    m_procViewMode->setFixedHeight(24);
    fn = m_procViewModeSummary->font();
    fn.setWeight(QFont::Medium);
    fn.setPointSize(fn.pointSize() - 1);
    m_procViewModeSummary->setFont(fn);
    m_procViewMode->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    auto pa = DApplicationHelper::instance()->palette(m_procViewModeSummary);
    palette.setColor(DPalette::Text, palette.color(DPalette::TextTips));
    m_procViewModeSummary->setPalette(palette);

    auto *modeButtonGroup = new DButtonBox(tw);
    modeButtonGroup->setFixedWidth(26 * 3);
    modeButtonGroup->setFixedHeight(24);
    m_appButton = new DButtonBoxButton(appText, modeButtonGroup);
    m_appButton->setCheckable(true);
    m_appButton->setChecked(true);
    m_myProcButton = new DButtonBoxButton(myProcText, modeButtonGroup);
    m_myProcButton->setCheckable(true);
    m_allProcButton = new DButtonBoxButton(allProcText, modeButtonGroup);
    m_allProcButton->setCheckable(true);
    QList<DButtonBoxButton *> list;
    list << m_appButton << m_myProcButton << m_allProcButton;
    modeButtonGroup->setButtonList(list, true);

    toolsLayout->addWidget(m_procViewMode, 0, Qt::AlignLeft);
    toolsLayout->addWidget(m_procViewModeSummary, 0, Qt::AlignLeft);
    toolsLayout->addStretch();
    toolsLayout->addWidget(modeButtonGroup, 0, Qt::AlignRight);

    tw->setLayout(toolsLayout);

    m_procTable = new ProcessTableView(cw);
    contentlayout->addWidget(tw);
    contentlayout->addWidget(m_procTable, 1);
    cw->setLayout(contentlayout);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(margin, margin, margin, margin);
    layout->setSpacing(margin);
    layout->addWidget(m_views);
    layout->addWidget(cw);
    setLayout(layout);

    setAutoFillBackground(false);

    QVariant vindex = m_settings->getOption(kSettingKeyProcessTabIndex);
    int index = 0;
    if (vindex.isValid())
        index = vindex.toInt();
    switch (index) {
        case SystemMonitor::OnlyMe:
            m_procTable->switchDisplayMode(SystemMonitor::OnlyMe);
            break;
        case SystemMonitor::AllProcess:
            m_procTable->switchDisplayMode(SystemMonitor::AllProcess);
            break;
        default:
            m_procTable->switchDisplayMode(SystemMonitor::OnlyGUI);
    }
}

void ProcessPageWidget::initConnections()
{
    MainWindow *mainWindow = MainWindow::instance();
    connect(mainWindow, &MainWindow::killProcessPerformed, this,
            &ProcessPageWidget::showWindowKiller);
    connect(mainWindow, &MainWindow::displayModeChanged, this,
            &ProcessPageWidget::switchDisplayMode);

    connect(m_appButton, &DButtonBoxButton::clicked, this, [=]() {
        m_procViewMode->setText(appText);
        m_procViewMode->adjustSize();
        m_procTable->switchDisplayMode(SystemMonitor::OnlyGUI);
        m_settings->setOption(kSettingKeyProcessTabIndex, SystemMonitor::OnlyGUI);
    });
    connect(m_myProcButton, &DButtonBoxButton::clicked, this, [=]() {
        m_procViewMode->setText(myProcText);
        m_procViewMode->adjustSize();
        m_procTable->switchDisplayMode(SystemMonitor::OnlyMe);
        m_settings->setOption(kSettingKeyProcessTabIndex, SystemMonitor::OnlyMe);
    });
    connect(m_allProcButton, &DButtonBoxButton::clicked, this, [=]() {
        m_procViewMode->setText(allProcText);
        m_procViewMode->adjustSize();
        m_procTable->switchDisplayMode(SystemMonitor::AllProcess);
        m_settings->setOption(kSettingKeyProcessTabIndex, SystemMonitor::AllProcess);
    });

    auto *sysmon = SystemMonitor::instance();
    if (sysmon) {
        connect(sysmon, &SystemMonitor::processSummaryUpdated, this,
                &ProcessPageWidget::updateProcessSummary);
    }
}

void ProcessPageWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QPainterPath path;
    path.addRect(QRectF(rect()));
    painter.setOpacity(1);

    DApplicationHelper *dAppHelper = DApplicationHelper::instance();
    DPalette palette = dAppHelper->applicationPalette();
    QColor bgColor = palette.color(DPalette::Background);

    painter.fillPath(path, bgColor);
}

void ProcessPageWidget::createWindowKiller()
{
    auto *killer = new InteractiveKill(this);
    killer->setFocus();
    connect(killer, &InteractiveKill::killWindow, [=](int pid) {
        // TODO: popup kill
        //        popupKillConfirmDialog(pid);
        killer->deleteLater();
    });
}

void ProcessPageWidget::updateProcessSummary(int napps, int nprocs)
{
    m_procViewModeSummary->setText(kProcSummaryTemplateText.arg(napps).arg(nprocs));
}

void ProcessPageWidget::popupKillConfirmDialog(pid_t pid)
{
    QString title = DApplication::translate("Kill.Process.Dialog", "End process");
    QString description = DApplication::translate("Kill.Process.Dialog",
                                                  "Force nding this process may cause data "
                                                  "loss.\nAre you sure you want to continue?");

    KillProcessConfirmDialog dialog(this);
    dialog.setTitle(title);
    dialog.setMessage(description);
    dialog.addButton(DApplication::translate("Kill.Process.Dialog", "Cancel"), false);
    dialog.addButton(DApplication::translate("Kill.Process.Dialog", "Force end"), true);
    dialog.exec();
    if (dialog.result() == QMessageBox::Ok) {
        auto *sysmon = SystemMonitor::instance();
        if (sysmon) {
            sysmon->killProcess(qvariant_cast<pid_t>(pid));
        }
    } else {
        // restore window
        MainWindow *mainWindow = MainWindow::instance();
        mainWindow->showNormal();
    }
}

void ProcessPageWidget::showWindowKiller()
{
    // Minimize window before show killer window.
    MainWindow *mainWindow = MainWindow::instance();
    mainWindow->showMinimized();

    QTimer::singleShot(200, this, SLOT(createWindowKiller()));
}

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
    QRect rect = QApplication::desktop()->screenGeometry();

    // Just change status monitor width when screen width is more than 1024.
    int statusBarMaxWidth = Utils::getStatusBarMaxWidth();
    if (rect.width() * 0.2 > statusBarMaxWidth) {
        if (windowState() == Qt::WindowMaximized) {
            m_views->setFixedWidth(static_cast<int>(rect.width() * 0.2));
        } else {
            m_views->setFixedWidth(statusBarMaxWidth);
        }
    }
}
