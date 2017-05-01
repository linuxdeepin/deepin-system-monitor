#include "process_manager.h"
#include "QVBoxLayout"

ProcessManager::ProcessManager(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    processView = new ProcessView();
    
    layout->addWidget(processView);
}
