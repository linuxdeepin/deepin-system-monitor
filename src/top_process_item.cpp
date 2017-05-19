#include "top_process_item.h"
#include "process_tools.h"
#include "utils.h"
#include <QDebug>

using namespace processTools;

TopProcessItem::TopProcessItem(QPixmap processIcon, QString processName, int processMemory, int processPid)
{
    iconPixmap = processIcon;
    name = processName;
    pid = processPid;
    memory = processMemory;
    
    memoryString = Utils::convertSizeUnit(memory);

    iconSize = 24;
    
    padding = 5;
}

bool TopProcessItem::sameAs(ListItem *item) 
{
    return pid == ((static_cast<TopProcessItem*>(item)))->pid;
}

void TopProcessItem::drawBackground(QRect rect, QPainter *painter, int, bool isSelect) 
{
    // Init draw path.
    QPainterPath path;
    path.addRect(QRectF(rect));

    // Draw selected effect.
    if (isSelect) {
        painter->setOpacity(0.8);
        painter->fillPath(path, QColor("#006BBA"));
    }
}

void TopProcessItem::drawForeground(QRect rect, QPainter *painter, int column, int index, bool isSelect) 
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
    
    // Draw index.
    if (column == 0) {
        int radius = 16;
        painter->setPen(QPen(QColor("#2C1414")));
        painter->setBrush(QBrush(QColor("#2C1414")));
        painter->drawEllipse(QRect(rect.x() + padding - 2, rect.y() + (rect.height() - radius) / 2 + 1, radius, radius));
        
        Utils::setFontSize(*painter, 9);
        painter->setPen(QPen(QColor("#ffffff")));
        painter->drawText(QRect(rect.x() + padding + 2, rect.y(), rect.width() - padding * 2, rect.height()), Qt::AlignLeft | Qt::AlignVCenter, QString::number(index + 1));
    }
    // Draw icon and process name.
    else if (column == 1) {
        painter->drawPixmap(QRect(rect.x(), rect.y() + (rect.height() - iconSize) / 2, iconSize, iconSize), iconPixmap);

        painter->drawText(QRect(rect.x() + iconSize + padding, rect.y(), rect.width() - iconSize - padding * 2, rect.height()), Qt::AlignLeft | Qt::AlignVCenter, name);
    }
    // Draw memory.
    else if (column == 2) {
        painter->drawText(QRect(rect.x(), rect.y(), rect.width() - padding, rect.height()), Qt::AlignRight | Qt::AlignVCenter, memoryString);
    }
}

QString TopProcessItem::getName() const 
{
    return name;
}

int TopProcessItem::getMemory() const 
{
    return memory;
}

int TopProcessItem::getPid() const 
{
    return pid;
}

QPixmap TopProcessItem::getIcon() const
{
    return iconPixmap;
}

bool TopProcessItem::sortByMemory(const ListItem *item1, const ListItem *item2) 
{
    // Init.
    int memory1 = (static_cast<const TopProcessItem*>(item1))->getMemory();
    int memory2 = (static_cast<const TopProcessItem*>(item2))->getMemory();
    
    return memory1 > memory2;
}


