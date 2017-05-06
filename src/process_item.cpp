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
    
    qDebug() << name << user << cpu << pid << memory;
}

void ProcessItem::render(int column, QRect rect, QPainter &painter) {
    Utils::setFontSize(painter, 11);
    painter.setOpacity(1);
    painter.setPen(QPen(QColor("#666666")));
    
    if (column == 0) {
        
    } else if (column == 1) {
        painter.drawText(rect, Qt::AlignLeft, name);
    } else if (column == 2) {
        painter.drawText(rect, Qt::AlignLeft, QString("%1%").arg(cpu));
    } else if (column == 3) {
        painter.drawText(rect, Qt::AlignLeft, QString("%1").arg(memory));
    } else if (column == 4) {
        painter.drawText(rect, Qt::AlignLeft, QString("%1").arg(pid));
    }
}

void ProcessItem::renderSelection(QRect rect, QPainter &painter) {

}
