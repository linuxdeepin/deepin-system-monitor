#include "process_view.h"
#include <QTimer>

ProcessView::ProcessView()
{
    QTimer::singleShot(0, this, SLOT(setFocus()));
    
    rowHeight = 40;
    
    QList<int> widths;
    widths << 40 << 300 << 100 << 100 << 100;
    setColumnWidths(widths);
}
