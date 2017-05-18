#include <DTitlebar>

#include "main_window.h"
#include "toolbar.h"

MainWindow::MainWindow(DMainWindow *parent) : DMainWindow(parent)
{
    if (this->titleBar()) {
        Toolbar *toolbar = new Toolbar();
        this->titleBar()->setCustomWidget(toolbar, Qt::AlignVCenter, false);
    }
    
    layoutWidget = new QWidget();
    layout = new QHBoxLayout(layoutWidget);
    
    this->setCentralWidget(layoutWidget);
    
    processManager = new ProcessManager();
    statusMonitor = new StatusMonitor();
    
    connect(statusMonitor, &StatusMonitor::updateProcessStatus, processManager, &ProcessManager::updateStatus, Qt::QueuedConnection);
    
    statusMonitor->updateStatus();
    
    layout->addWidget(statusMonitor);
    layout->addWidget(processManager);
}
