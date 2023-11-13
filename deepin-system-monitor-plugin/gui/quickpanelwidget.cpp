// SPDX-FileCopyrightText: 2019 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "quickpanelwidget.h"

#include <QVBoxLayout>

#include <DFontSizeManager>
#include <DGuiApplicationHelper>
#include <DPalette>


QuickPanelWidget::QuickPanelWidget(QWidget* parent)
    :QWidget(parent)
    , m_icon(new CommonIconButton(this))
    , m_description(new DLabel(this))
{
    initUI();
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &QuickPanelWidget::refreshBg);
}

QuickPanelWidget::~QuickPanelWidget()
{

}

void QuickPanelWidget::initUI()
{
    m_icon->setFixedSize(QSize(24, 24));

    m_description->setElideMode(Qt::ElideRight);
    DFontSizeManager::instance()->bind(m_description, DFontSizeManager::T10);

    auto layout = new QVBoxLayout;
    layout->setMargin(10);
    layout->setSpacing(0);
    layout->addStretch(1);
    layout->addWidget(m_icon, 0, Qt::AlignHCenter);
    layout->addSpacing(15);
    layout->addWidget(m_description, 0, Qt::AlignHCenter);
    layout->addStretch(1);
    
    setLayout(layout);
}

void QuickPanelWidget::setIcon(const QIcon &icon)
{
    m_icon->setIcon(icon, Qt::black, Qt::white);
}

void QuickPanelWidget::setDescription(const QString &description)
{
    m_description->setText(description);
    m_description->setToolTip(description);
}

void QuickPanelWidget::setWidgetState(WidgetState state)
{
    if (m_icon)
        m_icon->setActiveState(WS_ACTIVE == state);

    refreshBg();
}

void QuickPanelWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (underMouse()) {
        Q_EMIT clicked();
    }
}

void QuickPanelWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::RenderHint::Antialiasing);
    if (m_icon->activeState() && DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType) {
        painter.setBrush(QColor(255, 255, 255, 255 * 0.8));
    } else {
        painter.setBrush(Qt::transparent);
    }
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect(), 8, 8);

    QWidget::paintEvent(event);
}

void QuickPanelWidget::refreshBg()
{
    QString plugIcon = DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType? "dsm_pluginicon_dark" : "dsm_pluginicon_light";
    setIcon(QIcon::fromTheme(plugIcon));

    m_description->setForegroundRole(m_icon->activeState() && DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType ? QPalette::Highlight : QPalette::NoRole);
    update();
}

