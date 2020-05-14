#include <QResizeEvent>

#include "priority_slider.h"

PrioritySlider::PrioritySlider(Qt::Orientation orientation, QWidget *parent)
    : DSlider(orientation, parent)
{
}

void PrioritySlider::resizeEvent(QResizeEvent *event)
{
    DSlider::resizeEvent(event);
    Q_EMIT sizeChanged(event->size());
}
