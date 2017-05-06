#include "process_item.h"
#include "process_tools.h"
#include <QDebug>
#include "utils.h"

using namespace processTools;

ProcessItem::ProcessItem(proc_t *p)
{
    process = p;

    name = getProcessName(process);
    user = process->euser;
    cpu = process->pcpu;
    pid = process->tid;
    memory = Utils::convertSizeUnit((process->resident - process->share) * sysconf(_SC_PAGESIZE));

    iconSize = 24;
    icon = getProcessIconFromName(name);
    iconPixmap = icon.pixmap(iconSize, iconSize);
    padding = 10;
}

void ProcessItem::render(int column, QRect rect, QPainter *painter) {
    painter->setOpacity(1);
    Utils::setFontSize(*painter, 11);
    painter->setPen(QPen(QColor("#666666")));

    if (column == 0) {
        painter->drawPixmap(QRect(rect.x() + padding,
                                  rect.y() + (rect.height() - iconSize) / 2,
                                  iconSize,
                                  iconSize),
                            iconPixmap);
        
        painter->drawText(QRect(rect.x() + iconSize + padding * 2, rect.y(), rect.width() - iconSize - padding * 3, rect.height()), Qt::AlignLeft | Qt::AlignVCenter, name);
    } else if (column == 1) {
        painter->drawText(QRect(rect.x(), rect.y(), rect.width() - padding, rect.height()), Qt::AlignRight | Qt::AlignVCenter, QString("%1%").arg(cpu));
    } else if (column == 2) {
        painter->drawText(QRect(rect.x(), rect.y(), rect.width() - padding, rect.height()), Qt::AlignRight | Qt::AlignVCenter, memory);
    } else if (column == 3) {
        painter->drawText(QRect(rect.x(), rect.y(), rect.width() - padding, rect.height()), Qt::AlignRight | Qt::AlignVCenter, QString("%1").arg(pid));
    }
    
    qDebug() << name;
}

void ProcessItem::renderBackground(int index, QRect rect, QPainter *painter) {
    QPainterPath path;
    path.addRect(QRectF(rect));

    if (index % 2 == 0) {
        painter->setOpacity(0.1);
    } else {
        painter->setOpacity(0.2);
    }
    painter->fillPath(path, QColor("#000000"));
}

void ProcessItem::renderSelection(QRect rect, QPainter *painter) {

}
