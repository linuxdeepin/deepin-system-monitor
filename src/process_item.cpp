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
    memory = (process->resident - process->share) * sysconf(_SC_PAGESIZE);
    
    iconSize = 24;
    icon = QIcon::fromTheme("application-x-executable");
    iconPixmap = icon.pixmap(iconSize, iconSize);
}

void ProcessItem::render(int column, QRect rect, QPainter *painter) {
    Utils::setFontSize(*painter, 11);
    painter->setOpacity(1);
    painter->setPen(QPen(QColor("#666666")));
    
    if (column == 0) {
        painter->drawPixmap(QRect(rect.x() + (rect.width() - iconSize) / 2, 
                                  rect.y() + (rect.height() - iconSize) / 2, 
                                  iconSize,
                                  iconSize), 
                            iconPixmap);
    } else if (column == 1) {
        painter->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, name);
    } else if (column == 2) {
        painter->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, QString("%1%").arg(cpu));
    } else if (column == 3) {
        painter->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, QString("%1").arg(memory));
    } else if (column == 4) {
        painter->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, QString("%1").arg(pid));
    }
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
