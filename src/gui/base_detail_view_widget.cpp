/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     leiyu <leiyu@uniontech.com>
*
* Maintainer: leiyu <leiyu@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "base_detail_view_widget.h"
#include "detail_view_stacked_widget.h"

#include <QPainter>
#include <DApplication>
#include <DApplicationHelper>

BaseDetailViewWidget::BaseDetailViewWidget(QWidget *parent) : QWidget(parent)
{
    m_centralLayout = new QVBoxLayout(this);
    m_centralLayout->setContentsMargins(16, 10, 16, 16);
    m_centralLayout->setSpacing(16);

    DetailViewStackedWidget *stackViewWidget = dynamic_cast<DetailViewStackedWidget *>(parent);
    m_detailButton = new DCommandLinkButton(tr("Hide details"), this);
    m_detailButton->setFont(m_contentFont);
    connect(m_detailButton, &DCommandLinkButton::clicked, stackViewWidget, &DetailViewStackedWidget::onSwitchProcessPage);

    m_arrowButton = new DIconButton(DStyle::SP_ReduceElement, this);
    m_arrowButton->setIconSize(QSize(10, 10));
    m_arrowButton->setFixedSize(20, 20);
    connect(m_arrowButton, &DIconButton::clicked, [ = ]() {
        stackViewWidget->onShowPerformMenu(m_arrowButton->mapToGlobal(QPoint(0, m_arrowButton->height() + 6)));
    });

    fontChanged(DApplication::font());
    connect(dynamic_cast<QGuiApplication *>(DApplication::instance()), &DApplication::fontChanged,
            this, &BaseDetailViewWidget::fontChanged);
}

void BaseDetailViewWidget::fontChanged(const QFont &font)
{
    m_titleFont = font;
    m_titleFont.setPointSize(m_titleFont.pointSize() + 12);

    m_contentFont = font;
    m_contentFont.setPointSize(m_contentFont.pointSize() - 1);

    updateWidgetGrometry();
}

int BaseDetailViewWidget::titleHeight()
{
    return QFontMetrics(m_titleFont).height();
}

void BaseDetailViewWidget::setTitle(const QString &text)
{
    m_titleText = text;
    update();
}

QString BaseDetailViewWidget::title()
{
    return m_titleText;
}

void BaseDetailViewWidget::setDetail(const QString &text)
{
    m_detailText = text;
    update();
}

QString BaseDetailViewWidget::detail()
{
    return m_detailText;
}

void BaseDetailViewWidget::updateWidgetGrometry()
{
    QFontMetrics titleFont(m_titleFont);
    const QMargins &margin = m_centralLayout->contentsMargins();
    m_arrowButton->setGeometry(margin.left() + titleFont.width(m_titleText) + 6, 10 + titleFont.height() / 2 - m_arrowButton->height() / 2, m_arrowButton->width(), m_arrowButton->height());

    const QSize &detailtextSize =  QSize(m_detailButton->fontMetrics().width(m_detailButton->text()), m_detailButton->fontMetrics().height());
    m_detailButton->setGeometry(this->width() - detailtextSize.width() - margin.right(), 10 + titleFont.height() / 2 - detailtextSize.height() / 2, detailtextSize.width(), detailtextSize.height());
}

void BaseDetailViewWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateWidgetGrometry();
}

void BaseDetailViewWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    auto *dAppHelper = DApplicationHelper::instance();
    auto palette = dAppHelper->applicationPalette();

    painter.setPen(Qt::NoPen);
    painter.setBrush(palette.color(DPalette::Base));
    painter.drawRoundRect(this->rect(), 2, 2);

    painter.setFont(m_titleFont);
    painter.setPen(palette.color(DPalette::Text));
    QRect titleRect(m_centralLayout->contentsMargins().left(), 10, painter.fontMetrics().width(m_titleText), painter.fontMetrics().height());
    painter.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, m_titleText);
    m_centralLayout->setContentsMargins(16, titleRect.height() + 10, 16, 16);

    if (!m_detailText.isEmpty()) {
        painter.setFont(m_contentFont);
        painter.setPen(palette.color(DPalette::TextTips));
        QRect detailRect(m_arrowButton->width() + titleRect.right() + 12, titleRect.y(), painter.fontMetrics().width(m_detailText), titleRect.height());
        painter.drawText(detailRect, Qt::AlignLeft | Qt::AlignVCenter, m_detailText);
    }
}
