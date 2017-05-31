#include <DTitlebar>

#include "main_window.h"

MainWindow::MainWindow(DMainWindow *parent) : DMainWindow(parent)
{
    if (this->titlebar()) {
        toolbar = new Toolbar();
        this->titlebar()->setCustomWidget(toolbar, Qt::AlignVCenter, false);
        
        layoutWidget = new QWidget();
        layout = new QHBoxLayout(layoutWidget);

        this->setCentralWidget(layoutWidget);

        processManager = new ProcessManager();
        statusMonitor = new StatusMonitor();

        connect(processManager->onlyGuiButton, &DImageButton::clicked, statusMonitor, &StatusMonitor::switchToOnlyGui);
        connect(processManager->onlyMeButton, &DImageButton::clicked, statusMonitor, &StatusMonitor::switchToOnlyMe);
        connect(processManager->allProcessButton, &DImageButton::clicked, statusMonitor, &StatusMonitor::switchToAllProcess);

        connect(statusMonitor, &StatusMonitor::updateProcessStatus, processManager, &ProcessManager::updateStatus, Qt::QueuedConnection);
        connect(statusMonitor, &StatusMonitor::updateProcessNumber, processManager, &ProcessManager::updateProcessNumber, Qt::QueuedConnection);
        
        connect(toolbar, &Toolbar::search, processManager, &ProcessManager::handleSearch, Qt::QueuedConnection);

        statusMonitor->updateStatus();

        layout->addWidget(statusMonitor);
        layout->addWidget(processManager);
    }
}
