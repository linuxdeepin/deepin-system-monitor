#ifndef PRIORITY_TIP_H
#define PRIORITY_TIP_H

#include <DTipLabel>

DWIDGET_USE_NAMESPACE

class PriorityTip : public DTipLabel
{
    Q_OBJECT

public:
    PriorityTip(const QString &text = {}, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_radius {8};
};

#endif  // PRIORITY_TIP_H
