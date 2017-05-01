#include "process_view.h"
#include <QTimer>

ProcessView::ProcessView()
{
    QTimer::singleShot(0, this, SLOT(setFocus()));
}
