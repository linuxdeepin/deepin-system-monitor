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
#include "xwin_kill_preview_background_widget.h"

#include "ui_common.h"

#include <DFontSizeManager>
#include <DApplicationHelper>
#include <DPalette>
#include <DApplication>
#include <DStyle>

#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QPaintEvent>
#include <QDebug>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QHideEvent>
#include <QRegion>

DWIDGET_USE_NAMESPACE

static const QSize kDefaultIconSize {50, 50};

XWinKillPreviewBackgroundWidget::XWinKillPreviewBackgroundWidget(QPixmap &background, QWidget *parent)
    : QWidget(parent)
    , m_background(background)
{
    initUI();
    initConnection();
}

void XWinKillPreviewBackgroundWidget::updateSelection(const QRegion &region)
{
    // coordinate translate
    m_selRegion = region.translated(-geometry().x(), -geometry().y());
    update();
}

void XWinKillPreviewBackgroundWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    QPainterPath path;

    painter.setOpacity(1);
    painter.drawPixmap(rect(), m_background);

    painter.setOpacity(0.25);
    path.addRegion(m_selRegion);
    painter.fillPath(path, Qt::red);

    auto palette = DApplicationHelper::instance()->applicationPalette();
    auto *style = dynamic_cast<DStyle *>(this->style());
    auto radius = style->pixelMetric(DStyle::PM_FrameRadius);
    auto margin = style->pixelMetric(DStyle::PM_ContentsMargins);
    auto background = palette.color(DPalette::Current, DPalette::Background);
    auto foreground = palette.color(DPalette::Current, DPalette::Text);

    QSize minSize(std::max(m_textSize.width(), kDefaultIconSize.width()) + margin * 4,
                  margin * 6 + m_textSize.height() + kDefaultIconSize.height());
    QRect tooltipRect {(rect().width() - minSize.width()) / 2,
                       (rect().height() - minSize.height()) / 2,
                       minSize.width(),
                       minSize.height()};

    painter.setRenderHint(QPainter::Antialiasing);
    painter.setOpacity(.8);
    QPainterPath bgpath;
    bgpath.addRoundedRect(tooltipRect, radius, radius);
    painter.fillPath(bgpath, background);

    painter.setOpacity(1);

    // draw icon
    QRect iconRect(tooltipRect.x() + (tooltipRect.width() - kDefaultIconSize.width()) / 2,
                   tooltipRect.y() + margin * 2,
                   kDefaultIconSize.width(),
                   kDefaultIconSize.height());
    m_icon.paint(&painter, iconRect, Qt::AlignHCenter | Qt::AlignVCenter);

    // draw tooltip text
    QPen pen(foreground);
    painter.setPen(pen);
    painter.setFont(m_font);
    painter.setOpacity(.8);
    QRect textRect(tooltipRect.x() + (tooltipRect.width() - m_textSize.width()) / 2,
                   tooltipRect.y() + (tooltipRect.height() - m_textSize.height()) - margin * 2,
                   m_textSize.width(),
                   m_textSize.height());
    painter.drawText(textRect, Qt::AlignHCenter | Qt::AlignVCenter, m_text);
}

void XWinKillPreviewBackgroundWidget::initUI()
{
    Qt::WindowFlags flags {};
    flags |= Qt::Window;
    flags |= Qt::WindowStaysOnTopHint;
    flags |= Qt::FramelessWindowHint;
    flags |= Qt::BypassWindowManagerHint;
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
    m_textSize = fm.size(Qt::TextSingleLine, m_text);
}

void XWinKillPreviewBackgroundWidget::initConnection()
{

}
