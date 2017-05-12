#include "process_view.h"
#include <QTimer>

ProcessView::ProcessView()
{
    // Set row height.
    setRowHeight(40);
    
    // Set column widths.
    QList<int> widths;
    widths << -1 << 60 << 100 << 60;
    setColumnWidths(widths);
    
    // Set column titles.
    QList<QString> titles;
    titles << "进程名" << "CPU" << "内存" << "PID";
    setColumnTitles(titles, 36);
    
    // Focus keyboard when create.
    QTimer::singleShot(0, this, SLOT(setFocus()));
}
