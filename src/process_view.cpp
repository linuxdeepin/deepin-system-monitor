#include "process_view.h"
#include <QTimer>

ProcessView::ProcessView()
{
    QTimer::singleShot(0, this, SLOT(setFocus()));
    
    rowHeight = 40;
    
    QList<int> widths;
    widths << -1 << 60 << 100 << 100;
    setColumnWidths(widths);
    
    QList<QString> titles;
    titles << "进程名" << "CPU" << "内存" << "PID";
    setTitles(titles, 36);
}
