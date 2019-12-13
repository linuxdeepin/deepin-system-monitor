#ifndef PRIORITY_SLIDER_H
#define PRIORITY_SLIDER_H

#include <DSlider>

DWIDGET_USE_NAMESPACE

class PrioritySlider : public DSlider
{
    Q_OBJECT

public:
    PrioritySlider(Qt::Orientation orientation = Qt::Horizontal, QWidget *parent = nullptr);

Q_SIGNALS:
    void sizeChanged(const QSize &size);

protected:
    void resizeEvent(QResizeEvent *event) override;
};

#endif  // PRIORITY_SLIDER_H
