/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      maojj <maojunjie@uniontech.com>
* Maintainer:  maojj <maojunjie@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "xwin_kill_preview_tooltip_widget.h"

#include "gui/ui_common.h"

#include <DFontSizeManager>
#include <DApplicationHelper>
#include <DPalette>
#include <DApplication>
#include <DStyle>

#include <QPainter>
#include <QPaintEvent>
#include <QPainterPath>
#include <QPen>
#include <QFont>
#include <QFontMetrics>
#include <QDebug>

DWIDGET_USE_NAMESPACE

static const QSize kDefaultIconSize {50, 50};

XWinKillPreviewTooltipWidget::XWinKillPreviewTooltipWidget(QWidget *parent) : QWidget(parent)
{
    initUI();
}

void XWinKillPreviewTooltipWidget::paintEvent(QPaintEvent *)
{
    auto palette = DApplicationHelper::instance()->applicationPalette();
    auto *style = dynamic_cast<DStyle *>(this->style());
    auto radius = style->pixelMetric(DStyle::PM_FrameRadius);
    auto margin = style->pixelMetric(DStyle::PM_ContentsMargins);
    auto background = palette.color(DPalette::Current, DPalette::Background);
    auto foreground = palette.color(DPalette::Current, DPalette::Text);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setOpacity(.8);
    QPainterPath path;
    path.addRoundedRect(rect(), radius, radius);
    painter.fillPath(path, background);

    painter.setOpacity(1);

    // draw icon
    QRect iconRect((width() - kDefaultIconSize.width()) / 2,
                   margin * 2,
                   kDefaultIconSize.width(),
                   kDefaultIconSize.height());
    m_icon.paint(&painter, iconRect, Qt::AlignHCenter | Qt::AlignVCenter);

    // draw tooltip text
    QPen pen(foreground);
    painter.setPen(pen);
    painter.setFont(m_font);
    painter.setOpacity(.8);
    QRect textRect((width() - m_textSize.width()) / 2,
                   height() - m_textSize.height() - margin * 2,
                   m_textSize.width(),
                   m_textSize.height());
    painter.drawText(textRect, Qt::AlignHCenter | Qt::AlignVCenter, m_text);
}

void XWinKillPreviewTooltipWidget::initUI()
{
    Qt::WindowFlags flags {};
    flags |= Qt::ToolTip;
    flags |= Qt::WindowStaysOnTopHint;
    setWindowFlags(flags);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_TranslucentBackground);

    m_icon = QIcon::fromTheme(iconPathFromQrc("kill.svg"));
    m_text = QApplication::translate("Process.Choose.Window.Dialog",
                                     "Click the application you want to end");

    // calc preferred size for this widget
    m_font = this->font();
    m_font.setPixelSize(DFontSizeManager::instance()->fontPixelSize(DFontSizeManager::T6));
    QFontMetrics fm(m_font);
    auto *style = dynamic_cast<DStyle *>(this->style());
    auto margin = style->pixelMetric(DStyle::PM_ContentsMargins);
    m_textSize = fm.size(Qt::TextSingleLine, m_text);
    QSize minSize(std::max(m_textSize.width(), kDefaultIconSize.width()) + margin * 4,
                  margin * 6 + m_textSize.height() + kDefaultIconSize.height());
    setMinimumSize(minSize);
}
