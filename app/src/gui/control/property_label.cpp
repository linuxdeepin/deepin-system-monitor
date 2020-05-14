#include <DApplicationHelper>
#include <DFontSizeManager>

#include "property_label.h"

PropertyLabel::PropertyLabel(QWidget *parent, int margin, int spacing)
    : DFrame(parent)
{
    setAutoFillBackground(true);
    setBackgroundRole(DPalette::ItemBackground);
    setFrameRounded(true);
    setLineWidth(0);
    setContentsMargins(0, 0, 0, 0);

    m_name = new DLabel();

    m_layout = new QHBoxLayout(this);
    m_layout->setMargin(margin);
    m_layout->setSpacing(spacing);
    m_layout->addWidget(m_name, 0, Qt::AlignLeft | Qt::AlignVCenter);

    DFontSizeManager::instance()->bind(m_name, DFontSizeManager::T6, QFont::Medium);
    auto pa = DApplicationHelper::instance()->palette(m_name);
    pa.setColor(DPalette::Text, pa.color(DPalette::TextTitle));
    m_name->setPalette(pa);
    m_name->adjustSize();
}

void PropertyLabel::addPropertyValueWidget(QWidget *widget, DPalette::ColorType ct)
{
    if (!widget)
        return;

    m_value = widget;
    m_layout->addWidget(m_value, 1, Qt::AlignRight | Qt::AlignVCenter);

    DFontSizeManager::instance()->bind(m_value, DFontSizeManager::T6, QFont::Medium);
    auto pa = DApplicationHelper::instance()->palette(m_value);
    pa.setColor(DPalette::Text, pa.color(ct));
    m_value->setPalette(pa);
}

void PropertyLabel::addPropertyValueWidget(QWidget *widget, QPalette::ColorRole cr)
{
    if (!widget)
        return;

    m_value = widget;
    m_layout->addWidget(m_value, 1, Qt::AlignRight | Qt::AlignVCenter);

    DFontSizeManager::instance()->bind(m_value, DFontSizeManager::T6, QFont::Medium);
    auto pa = DApplicationHelper::instance()->palette(m_value);
    pa.setColor(DPalette::Text, pa.color(cr));
    m_value->setPalette(pa);
}

void PropertyLabel::adjustWidth(int width)
{

}

QSize PropertyLabel::minimumSizeHint() const
{
    return {517, 36};
}

QSize PropertyLabel::sizeHint() const
{
    return {517, 36};
}
