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

void ProcessItem::drawForeground(QRect rect, QPainter *painter, int column, bool isSelect) 
{
    // Init opacity and font size.
    painter->setOpacity(1);
    Utils::setFontSize(*painter, 11);
    
    // Set font color with selected status.
    if (isSelect) {
        painter->setPen(QPen(QColor("#ffffff")));
    } else {
        painter->setPen(QPen(QColor("#666666")));
    }

    // Draw icon and process name.
    if (column == 0) {
        painter->drawPixmap(QRect(rect.x() + padding, rect.y() + (rect.height() - iconSize) / 2, iconSize, iconSize), iconPixmap);

        painter->drawText(QRect(rect.x() + iconSize + padding * 2, rect.y(), rect.width() - iconSize - padding * 3, rect.height()), Qt::AlignLeft | Qt::AlignVCenter, name);
    }
    // Draw CPU.
    else if (column == 1) {
        painter->drawText(QRect(rect.x(), rect.y(), rect.width() - padding, rect.height()), Qt::AlignRight | Qt::AlignVCenter, QString("%1%").arg(QString::number(cpu, 'f', 1)));
    }
    // Draw memory.
    else if (column == 2) {
        painter->drawText(QRect(rect.x(), rect.y(), rect.width() - padding, rect.height()), Qt::AlignRight | Qt::AlignVCenter, memoryString);
    }
    // Draw pid.
    else if (column == 3) {
        painter->drawText(QRect(rect.x(), rect.y(), rect.width() - padding, rect.height()), Qt::AlignRight | Qt::AlignVCenter, QString("%1").arg(pid));
    }
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
    
