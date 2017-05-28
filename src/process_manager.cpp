#include "QVBoxLayout"
#include "list_view.h"
#include "process_item.h"
#include "process_manager.h"
#include <QDebug>
#include <QList>
#include <proc/sysinfo.h>
#include <signal.h>

using namespace Utils;

ProcessManager::ProcessManager(QWidget *parent) : QWidget(parent)
{
    // Init widget.
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    QWidget *topWidget = new QWidget();
    topWidget->setFixedHeight(48);
    QHBoxLayout *topLayout = new QHBoxLayout(topWidget);
    processView = new ProcessView();
    layout->addWidget(topWidget);
    layout->addWidget(processView);
    
    statusLabel = new QLabel("我还没有想好这里应该放什么状态，也许你可以告诉我。;)");
    onlyGuiButton = new DImageButton(
        Utils::getQrcPath("only_gui_normal.png"),
        Utils::getQrcPath("only_gui_hover.png"),
        Utils::getQrcPath("only_gui_press.png")
        );
    onlyMeButton = new DImageButton(
        Utils::getQrcPath("only_me_normal.png"),
        Utils::getQrcPath("only_me_hover.png"),
        Utils::getQrcPath("only_me_press.png")
        );
    allProcessButton = new DImageButton(
        Utils::getQrcPath("all_process_normal.png"),
        Utils::getQrcPath("all_process_hover.png"),
        Utils::getQrcPath("all_process_press.png")
        );
    topLayout->addWidget(statusLabel);
    topLayout->addStretch();
    topLayout->addWidget(onlyGuiButton);
    topLayout->addWidget(onlyMeButton);
    topLayout->addWidget(allProcessButton);
    
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
    processView->setSearchAlgorithm(&ProcessItem::search);
    
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

void ProcessManager::handleSearch(QString searchContent)
{
    processView->search(searchContent);
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

