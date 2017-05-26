#include "process_view.h"
#include <QTimer>

ProcessView::ProcessView()
{
    // Set row height.
    setRowHeight(40);
    
    // Set column widths.
    QList<int> widths;
    widths << -1 << 50 << 70 << 70 << 70 << 70 << 70 << 50;
    setColumnWidths(widths);
    
    // Set column titles.
    QList<QString> titles;
    titles << "进程名" << "CPU" << "内存" << "磁盘写入" << "磁盘读取" << "下载" << "上传" << "PID";
    setColumnTitles(titles, 36);
    
    // Focus keyboard when create.
    QTimer::singleShot(0, this, SLOT(setFocus()));
}
