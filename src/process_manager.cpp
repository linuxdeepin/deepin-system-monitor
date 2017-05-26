#include "QVBoxLayout"
#include "list_view.h"
#include "process_item.h"
#include "process_manager.h"
#include "process_tools.h"
#include <QDebug>
#include <QList>
#include <proc/sysinfo.h>

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
    alorithms->append(&ProcessItem::sortByDownload);
    alorithms->append(&ProcessItem::sortByUpload);
    alorithms->append(&ProcessItem::sortByDownload);
    alorithms->append(&ProcessItem::sortByUpload);
    alorithms->append(&ProcessItem::sortByPid);
    processView->setColumnSortingAlgorithms(alorithms, 1, true);
}

ProcessManager::~ProcessManager()
{
    delete processView;
}

void ProcessManager::updateStatus(QList<ListItem*> items)
{
    processView->refreshItems(items);
}

