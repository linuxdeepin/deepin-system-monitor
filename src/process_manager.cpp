#include "QVBoxLayout"
#include "list_view.h"
#include "process_item.h"
#include "process_manager.h"
#include "process_tools.h"
#include <QDebug>
#include <QList>
#include <proc/sysinfo.h>
#include <signal.h>

using namespace processTools;

ProcessManager::ProcessManager(QWidget *parent) : QWidget(parent)
{
    // Init widget.
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    processView = new ProcessView();
    layout->addWidget(processView);

    // Set sort algorithms.
    QList<SortAlgorithm> *alorithms = new QList<SortAlgorithm>();
    alorithms->append(&ProcessItem::sortByName);
    alorithms->append(&ProcessItem::sortByCPU);
    alorithms->append(&ProcessItem::sortByMemory);
    alorithms->append(&ProcessItem::sortByWrite);
    alorithms->append(&ProcessItem::sortByRead);
    alorithms->append(&ProcessItem::sortByDownload);
    alorithms->append(&ProcessItem::sortByUpload);
    alorithms->append(&ProcessItem::sortByPid);
    processView->setColumnSortingAlgorithms(alorithms, 1, true);
    
    actionItems = new QList<ListItem*>();
    
    rightMenu = new QMenu();
    killAction = new QAction("结束进程", this);
    connect(killAction, &QAction::triggered, this, &ProcessManager::killProcesses);
    pauseAction = new QAction("暂停进程", this);
    connect(pauseAction, &QAction::triggered, this, &ProcessManager::stopProcesses);
    resumeAction = new QAction("继续进程", this);
    connect(resumeAction, &QAction::triggered, this, &ProcessManager::resumeProcesses);
    rightMenu->addAction(killAction);
    rightMenu->addAction(pauseAction);
    rightMenu->addAction(resumeAction);
    
    connect(processView, &ProcessView::rightClickItems, this, &ProcessManager::popupMenu, Qt::QueuedConnection);
}

ProcessManager::~ProcessManager()
{
    delete processView;
}

void ProcessManager::updateStatus(QList<ListItem*> items)
{
    processView->refreshItems(items);
}

void ProcessManager::popupMenu(QPoint pos, QList<ListItem*> items)
{
    actionItems->append(items);
    rightMenu->exec(this->mapToGlobal(pos));
}

void ProcessManager::killProcesses()
{
    for (ListItem *item : *actionItems) {
        ProcessItem *processItem = static_cast<ProcessItem*>(item);
        if (kill(processItem->getPid(), SIGTERM) != 0) {
            qDebug() << QString("Kill process %1 failed, permission denied.").arg(processItem->getPid());
        }
    }
    
    actionItems->clear();
}

void ProcessManager::stopProcesses()
{
    for (ListItem *item : *actionItems) {
        ProcessItem *processItem = static_cast<ProcessItem*>(item);
        if (kill(processItem->getPid(), SIGSTOP) != 0) {
            qDebug() << QString("Stop process %1 failed, permission denied.").arg(processItem->getPid());
        }
    }
    
    actionItems->clear();
}

void ProcessManager::resumeProcesses()
{
    for (ListItem *item : *actionItems) {
        ProcessItem *processItem = static_cast<ProcessItem*>(item);
        if (kill(processItem->getPid(), SIGCONT) != 0) {
            qDebug() << QString("Resume process %1 failed, permission denied.").arg(processItem->getPid());
        }
    }
    
    actionItems->clear();
}

