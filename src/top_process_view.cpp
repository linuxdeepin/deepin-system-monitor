#include "top_process_view.h"

TopProcessView::TopProcessView()
{
    // Set row height.
    setRowHeight(36);
    
    // Set column widths.
    QList<int> widths;
    widths << 30 << -1 << 80;
    setColumnWidths(widths);
    
    setFixedHeight(108);
}
