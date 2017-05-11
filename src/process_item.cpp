#include "process_item.h"
#include "process_tools.h"
#include <QDebug>
#include "utils.h"

using namespace processTools;

ProcessItem::ProcessItem(proc_t *p, QMap<QString, QPixmap> *processIconCache)
{
    process = p;

    name = getProcessName(process);
    user = process->euser;
    cpu = process->pcpu;
    pid = process->tid;
    memory = (process->resident - process->share) * sysconf(_SC_PAGESIZE);
    memoryString = Utils::convertSizeUnit(memory);

    iconSize = 24;
    iconPixmap = getProcessIconFromName(name, processIconCache);
    padding = 10;
}

void ProcessItem::render(QRect rect, QPainter *painter, int column, bool isSelect) {
    painter->setOpacity(1);
    Utils::setFontSize(*painter, 11);
    if (isSelect) {
        painter->setPen(QPen(QColor("#ffffff")));
    } else {
        painter->setPen(QPen(QColor("#666666")));
    }

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
        painter->drawText(QRect(rect.x(), rect.y(), rect.width() - padding, rect.height()), Qt::AlignRight | Qt::AlignVCenter, memoryString);
    } else if (column == 3) {
        painter->drawText(QRect(rect.x(), rect.y(), rect.width() - padding, rect.height()), Qt::AlignRight | Qt::AlignVCenter, QString("%1").arg(pid));
    }
}

void ProcessItem::renderBackground(QRect rect, QPainter *painter, int index, bool isSelect) {
    QPainterPath path;
    path.addRect(QRectF(rect));

    if (isSelect) {
        painter->setOpacity(0.8);
        painter->fillPath(path, QColor("#006BBA"));
    } else {
        if (index % 2 == 0) {
            painter->setOpacity(0.1);
        } else {
            painter->setOpacity(0.2);
        }
        painter->fillPath(path, QColor("#000000"));
    }
}

bool ProcessItem::hasSameContent(ListItem *item) {
    return pid == ((static_cast<ProcessItem*>(item)))->pid;
}
