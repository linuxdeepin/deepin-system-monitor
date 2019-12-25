#include <DApplicationHelper>
#include <DPalette>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>

#include "priority_tip.h"

PriorityTip::PriorityTip(const QString &text, QWidget *parent)
    : DTipLabel(text, parent)
{
}

void PriorityTip::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    auto *dAppHelper = DApplicationHelper::instance();
    auto palette = dAppHelper->applicationPalette();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPainterPath path;
    path.addRoundedRect(rect(), m_radius, m_radius);
    painter.fillPath(path, palette.color(DPalette::Active, DPalette::LightLively));

    QPen pen(palette.color(DPalette::Active, DPalette::TextLively));
    painter.setPen(pen);
    painter.drawText(rect(), int(alignment()), text());
}
