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
    
    statusMonitor = new StatusMonitor();
    processManager = new ProcessManager();
    
    layout->addWidget(statusMonitor);
    layout->addWidget(processManager);
}
