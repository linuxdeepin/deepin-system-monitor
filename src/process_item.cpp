#include "process_item.h"
#include "process_tools.h"
#include "utils.h"
#include <QDebug>

using namespace processTools;

ProcessItem::ProcessItem(QPixmap processIcon, QString processName, double processCpu, int processMemory, int processPid, QString processUser)
{
    iconPixmap = processIcon;
    name = processName;
    cpu = processCpu;
    pid = processPid;
    memory = processMemory;
    user = processUser;
    
    memoryString = Utils::convertSizeUnit(memory);

    iconSize = 24;
    
    padding = 10;
    
    networkStatus.sentBytes = 0;
    networkStatus.recvBytes = 0;
    networkStatus.sentKbs = 0;
    networkStatus.recvKbs = 0;
    
    diskStatus.readKbs = 0;
    diskStatus.writeKbs = 0;
}

bool ProcessItem::sameAs(ListItem *item) 
{
    return pid == ((static_cast<ProcessItem*>(item)))->pid;
}

void ProcessItem::drawBackground(QRect rect, QPainter *painter, int index, bool isSelect) 
{
    // Init draw path.
    QPainterPath path;
    path.addRect(QRectF(rect));

    // Draw selected effect.
    if (isSelect) {
        painter->setOpacity(0.8);
        painter->fillPath(path, QColor("#006BBA"));
    }
    // Draw background effect.
    else {
        // Use different opacity with item index.
        if (index % 2 == 0) {
            painter->setOpacity(0.1);
        } else {
            painter->setOpacity(0.2);
        }
        
        painter->fillPath(path, QColor("#000000"));
    }
}

void ProcessItem::drawForeground(QRect rect, QPainter *painter, int column, int, bool isSelect) 
{
    // Init opacity and font size.
    painter->setOpacity(1);
    
    // Set font color with selected status.
    if (isSelect) {
        painter->setPen(QPen(QColor("#ffffff")));
    } else {
        painter->setPen(QPen(QColor("#666666")));
    }

    // Draw icon and process name.
    if (column == 0) {
        Utils::setFontSize(*painter, 11);
        painter->drawPixmap(QRect(rect.x() + padding, rect.y() + (rect.height() - iconSize) / 2, iconSize, iconSize), iconPixmap);

        painter->drawText(QRect(rect.x() + iconSize + padding * 2, rect.y(), rect.width() - iconSize - padding * 3, rect.height()), Qt::AlignLeft | Qt::AlignVCenter, name);
    }
    // Draw CPU.
    else if (column == 1) {
        Utils::setFontSize(*painter, 9);
        painter->drawText(QRect(rect.x(), rect.y(), rect.width() - padding, rect.height()), Qt::AlignRight | Qt::AlignVCenter, QString("%1%").arg(QString::number(cpu, 'f', 1)));
    }
    // Draw memory.
    else if (column == 2) {
        Utils::setFontSize(*painter, 9);
        painter->drawText(QRect(rect.x(), rect.y(), rect.width() - padding, rect.height()), Qt::AlignRight | Qt::AlignVCenter, memoryString);
    }
    // Draw write.
    else if (column == 3) {
        if (diskStatus.writeKbs > 0) {
            Utils::setFontSize(*painter, 9);
            painter->drawText(QRect(rect.x(), rect.y(), rect.width() - padding, rect.height()), Qt::AlignRight | Qt::AlignVCenter, Utils::formatByteCount(diskStatus.writeKbs));
        }
    }
    // Draw read.
    else if (column == 4) {
        if (diskStatus.readKbs > 0) {
            Utils::setFontSize(*painter, 9);
            painter->drawText(QRect(rect.x(), rect.y(), rect.width() - padding, rect.height()), Qt::AlignRight | Qt::AlignVCenter, Utils::formatByteCount(diskStatus.readKbs));
        }
    }
    // Draw download.
    else if (column == 5) {
        if (networkStatus.recvKbs > 0) {
            Utils::setFontSize(*painter, 9);
            painter->drawText(QRect(rect.x(), rect.y(), rect.width() - padding, rect.height()), Qt::AlignRight | Qt::AlignVCenter, Utils::formatBandwidth(networkStatus.recvKbs));
        }
    }
    // Draw upload.
    else if (column == 6) {
        if (networkStatus.sentKbs > 0) {
            Utils::setFontSize(*painter, 9);
            painter->drawText(QRect(rect.x(), rect.y(), rect.width() - padding, rect.height()), Qt::AlignRight | Qt::AlignVCenter, Utils::formatBandwidth(networkStatus.sentKbs));
        }
    }
    // Draw pid.
    else if (column == 7) {
        Utils::setFontSize(*painter, 9);
        painter->drawText(QRect(rect.x(), rect.y(), rect.width() - padding, rect.height()), Qt::AlignRight | Qt::AlignVCenter, QString("%1").arg(pid));
    }
}

void ProcessItem::setNetworkStatus(NetworkStatus nStatus)
{
    networkStatus = nStatus;
}

void ProcessItem::setDiskStatus(DiskStatus dStatus)
{
    diskStatus = dStatus;
}

QString ProcessItem::getName() const 
{
    return name;
}

double ProcessItem::getCPU() const 
{
    return cpu;
}

int ProcessItem::getMemory() const 
{
    return memory;
}

int ProcessItem::getPid() const 
{
    return pid;
}

NetworkStatus ProcessItem::getNetworkStatus() const
{
    return networkStatus;
}
    
DiskStatus ProcessItem::getDiskStatus() const
{
    return diskStatus;
}
    
bool ProcessItem::sortByName(const ListItem *item1, const ListItem *item2, bool descendingSort) 
{
    // Init.
    QString name1 = (static_cast<const ProcessItem*>(item1))->getName();
    QString name2 = (static_cast<const ProcessItem*>(item2))->getName();
    bool sortOrder;

    // Sort item with cpu if name is same.
    if (name1 == name2) {
        double cpu1 = static_cast<const ProcessItem*>(item1)->getCPU();
        double cpu2 = (static_cast<const ProcessItem*>(item2))->getCPU();
        
        sortOrder = cpu1 > cpu2;
    }
    // Otherwise sort by name.
    else {
        sortOrder = name1 > name2;
    }
    
    return descendingSort ? sortOrder : !sortOrder;
}

bool ProcessItem::sortByCPU(const ListItem *item1, const ListItem *item2, bool descendingSort) 
{
    // Init.
    double cpu1 = (static_cast<const ProcessItem*>(item1))->getCPU();
    double cpu2 = (static_cast<const ProcessItem*>(item2))->getCPU();
    bool sortOrder;
    
    // Sort item with memory if cpu is same.
    if (cpu1 == cpu2) {
        int memory1 = static_cast<const ProcessItem*>(item1)->getMemory();
        int memory2 = (static_cast<const ProcessItem*>(item2))->getMemory();
        
        sortOrder = memory1 > memory2;
    }
    // Otherwise sort by cpu.
    else {
        sortOrder = cpu1 > cpu2;
    }
    
    return descendingSort ? sortOrder : !sortOrder;
}

bool ProcessItem::sortByMemory(const ListItem *item1, const ListItem *item2, bool descendingSort) 
{
    // Init.
    int memory1 = (static_cast<const ProcessItem*>(item1))->getMemory();
    int memory2 = (static_cast<const ProcessItem*>(item2))->getMemory();
    bool sortOrder;
    
    // Sort item with cpu if memory is same.
    if (memory1 == memory2) {
        double cpu1 = static_cast<const ProcessItem*>(item1)->getCPU();
        double cpu2 = (static_cast<const ProcessItem*>(item2))->getCPU();
        
        sortOrder = cpu1 > cpu2;
    }
    // Otherwise sort by memory.
    else {
        sortOrder = memory1 > memory2;
    }
    
    return descendingSort ? sortOrder : !sortOrder;
}

bool ProcessItem::sortByPid(const ListItem *item1, const ListItem *item2, bool descendingSort) 
{
    bool sortOrder = (static_cast<const ProcessItem*>(item1))->getPid() > (static_cast<const ProcessItem*>(item2))->getPid();
    
    return descendingSort ? sortOrder : !sortOrder;
}


bool ProcessItem::sortByDownload(const ListItem *item1, const ListItem *item2, bool descendingSort)
{
    // Init.
    NetworkStatus status1 = (static_cast<const ProcessItem*>(item1))->getNetworkStatus();
    NetworkStatus status2 = (static_cast<const ProcessItem*>(item2))->getNetworkStatus();
    bool sortOrder;

    // Sort item with download bytes if download speed is same.
    if (status1.recvKbs == status2.recvKbs) {
        sortOrder = status1.recvBytes > status2.recvBytes;
    }
    // Otherwise sort by download speed.
    else {
        sortOrder = status1.recvKbs > status2.recvKbs;
    }
    
    return descendingSort ? sortOrder : !sortOrder;
}

bool ProcessItem::sortByUpload(const ListItem *item1, const ListItem *item2, bool descendingSort)
{
    // Init.
    NetworkStatus status1 = (static_cast<const ProcessItem*>(item1))->getNetworkStatus();
    NetworkStatus status2 = (static_cast<const ProcessItem*>(item2))->getNetworkStatus();
    bool sortOrder;

    // Sort item with upload bytes if upload speed is same.
    if (status1.sentKbs == status2.sentKbs) {
        sortOrder = status1.sentBytes > status2.sentBytes;
    }
    // Otherwise sort by upload speed.
    else {
        sortOrder = status1.sentKbs > status2.sentKbs;
    }
    
    return descendingSort ? sortOrder : !sortOrder;
}

bool ProcessItem::sortByWrite(const ListItem *item1, const ListItem *item2, bool descendingSort)
{
    // Init.
    DiskStatus status1 = (static_cast<const ProcessItem*>(item1))->getDiskStatus();
    DiskStatus status2 = (static_cast<const ProcessItem*>(item2))->getDiskStatus();
    bool sortOrder = status1.writeKbs > status2.writeKbs;
    
    return descendingSort ? sortOrder : !sortOrder;
}

bool ProcessItem::sortByRead(const ListItem *item1, const ListItem *item2, bool descendingSort)
{
    // Init.
    DiskStatus status1 = (static_cast<const ProcessItem*>(item1))->getDiskStatus();
    DiskStatus status2 = (static_cast<const ProcessItem*>(item2))->getDiskStatus();
    bool sortOrder = status1.readKbs > status2.readKbs;
    
    return descendingSort ? sortOrder : !sortOrder;
}
