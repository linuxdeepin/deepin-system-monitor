#include <DTitlebar>
#include <QStyleFactory>
#include <QApplication>
#include <QDesktopWidget>
#include <QDebug>
#include <signal.h>

#include "main_window.h"
#include <iostream>
using namespace std;

MainWindow::MainWindow(DMainWindow *parent) : DMainWindow(parent)
{
    installEventFilter(this);   // add event filter

    if (this->titlebar()) {
        toolbar = new Toolbar();

        menu = new QMenu();
        menu->setStyle(QStyleFactory::create("dlight"));
        killAction = new QAction("结束应用程序", this);
        connect(killAction, &QAction::triggered, this, &MainWindow::showWindowKiller);
        menu->addAction(killAction);
        menu->addSeparator();

        this->titlebar()->setCustomWidget(toolbar, Qt::AlignVCenter, false);
        this->titlebar()->setMenu(menu);

        layoutWidget = new QWidget();
        layout = new QHBoxLayout(layoutWidget);

        this->setCentralWidget(layoutWidget);

        processManager = new ProcessManager();
        statusMonitor = new StatusMonitor();

        connect(processManager, &ProcessManager::activeTab, this, &MainWindow::switchTab);

        connect(statusMonitor, &StatusMonitor::updateProcessStatus, processManager, &ProcessManager::updateStatus, Qt::QueuedConnection);
        connect(statusMonitor, &StatusMonitor::updateProcessNumber, processManager, &ProcessManager::updateProcessNumber, Qt::QueuedConnection);

        connect(toolbar, &Toolbar::search, processManager, &ProcessManager::handleSearch, Qt::QueuedConnection);

        statusMonitor->updateStatus();

        layout->addWidget(statusMonitor);
        layout->addWidget(processManager);

        killPid = -1;

        killProcessDialog = new DDialog(QString("结束进程"), QString("结束应用会有丢失数据的风险\n您确定要结束选中的应用吗？"), this);
        killProcessDialog->setIcon(QIcon(Utils::getQrcPath("deepin-system-monitor.svg")));
        killProcessDialog->addButton(QString("取消"), false, DDialog::ButtonNormal);
        killProcessDialog->addButton(QString("结束进程"), true, DDialog::ButtonNormal);
        connect(killProcessDialog, &DDialog::buttonClicked, this, &MainWindow::dialogButtonClicked);

        killer = NULL;
    }
}

bool MainWindow::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange) {
        QRect rect = QApplication::desktop()->screenGeometry();
        
        // Just change status monitor width when screen width is more than 1024.
        if (rect.width() * 0.2 > 300) {
            if (windowState() == Qt::WindowMaximized) {
                statusMonitor->setFixedWidth(rect.width() * 0.2);
            } else {
                statusMonitor->setFixedWidth(300);
            }
        }
        
    }

    return false;
}

void MainWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    QPainterPath path;
    path.addRect(QRectF(rect()));
    painter.setOpacity(1);
    painter.fillPath(path, QColor("#0E0E0E"));
}

void MainWindow::createWindowKiller()
{
    killer = new InteractiveKill();
    killer->setFocus();
    connect(killer, &InteractiveKill::killWindow, this, &MainWindow::popupKillConfirmDialog, Qt::QueuedConnection);
}

void MainWindow::dialogButtonClicked(int index, QString)
{
    if (index == 1) {
        if (killPid != -1) {
            if (kill(killPid, SIGTERM) != 0) {
                cout << "Kill failed." << endl;
            }

            killPid = -1;
        }
    }
}

void MainWindow::popupKillConfirmDialog(int pid)
{
    killer->close();
    
    killPid = pid;
    killProcessDialog->show();
}

void MainWindow::showWindowKiller()
{
    QTimer::singleShot(200, this, SLOT(createWindowKiller()));
}

void MainWindow::switchTab(int index)
{
    if (index == 0) {
        statusMonitor->switchToOnlyGui();
    } else if (index == 1) {
        statusMonitor->switchToOnlyMe();
    } else {
        statusMonitor->switchToAllProcess();
    }
}
