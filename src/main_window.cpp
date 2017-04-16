#include <DTitlebar>

#include "main_window.h"
#include "toolbar.h"

MainWindow::MainWindow(DMainWindow *parent) : DMainWindow(parent)
{
    if (this->titleBar()) {
        Toolbar *toolbar = new Toolbar();
        this->titleBar()->setCustomWidget(toolbar, Qt::AlignVCenter, false);
    }
}
