// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "monitorpluginbuttonwidget.h"

// Qt
#include <QSvgRenderer>
#include <QPainter>
#include <QMouseEvent>
#include <QApplication>
#include <QIcon>
#include <QPainterPath>

//Dtk
#include <DGuiApplicationHelper>
#include <DStyle>
#include "dde-dock/constants.h"

DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

MonitorPluginButtonWidget::MonitorPluginButtonWidget(QWidget *parent)
    : QWidget(parent)
    , m_hover(false)
    , m_pressed(false)
{
    setMouseTracking(true);
    setMinimumSize(PLUGIN_BACKGROUND_MIN_SIZE, PLUGIN_BACKGROUND_MIN_SIZE);

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, [ = ] {
        update();
    });
}

void MonitorPluginButtonWidget::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e)

    QString iconName = "status-system-monitor";
    QString fallbackIconName = "dsm_pluginicon_light";
    QPixmap pixmap;
    int iconSize = PLUGIN_ICON_MAX_SIZE;

    QPainter painter(this);
    if (std::min(width(), height()) > PLUGIN_BACKGROUND_MIN_SIZE) {
        QColor color;
        if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType) {
            color = Qt::black;
            painter.setOpacity(0.5);

            if (m_hover) {
                painter.setOpacity(0.6);
            }

            if (m_pressed) {
                painter.setOpacity(0.3);
            }
        } else {
            color = Qt::white;
            painter.setOpacity(0.1);

            if (m_hover) {
                painter.setOpacity(0.2);
            }

            if (m_pressed) {
                painter.setOpacity(0.05);
            }
        }

        painter.setRenderHint(QPainter::Antialiasing, true);

        DStyleHelper dstyle(style());
        const int radius = dstyle.pixelMetric(DStyle::PM_FrameRadius);

        QPainterPath path;

        int minSize = std::min(width(), height());
        QRect rc(0, 0, minSize, minSize);
        rc.moveTo(rect().center() - rc.center());

        path.addRoundedRect(rc, radius, radius);
        painter.fillPath(path, color);
    } else if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType) {
        // 最小尺寸时，不画背景，采用深色图标
        iconName = "status-system-monitor-dark";
        fallbackIconName = "dsm_pluginicon_dark";
    }

    painter.setOpacity(1);
    painter.setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing);

    // Use Qt's icon system to load icons directly instead of custom loadSvg function
    // Reasons:
    // 1. loadSvg function manually handles DPI scaling which causes duplicate scaling issues
    // 2. Qt's icon system has built-in DPI handling mechanism, no manual intervention needed
    // 3. Using QIcon::pixmap directly ensures correct icon display across different DPI settings
    QIcon fallbackIcon = QIcon::fromTheme(fallbackIconName);
    QIcon icon = QIcon::fromTheme(iconName, fallbackIcon);
    if (!icon.isNull()) {
        pixmap = icon.pixmap(QSize(iconSize, iconSize));
    } else {
        QString localIcon = QString(":/icons/deepin/builtin/actions/%1_20px.svg").arg(fallbackIconName);
        QSvgRenderer renderer(localIcon);
        pixmap = QPixmap(iconSize, iconSize);
        pixmap.fill(Qt::transparent);
        QPainter iconPainter(&pixmap);
        renderer.render(&iconPainter);
    }

    // Calculate the centered position of the icon in the window
    // Use integer coordinates to avoid drawing position offset caused by floating point precision issues
    int x = (width() - iconSize) / 2;
    int y = (height() - iconSize) / 2;
    painter.drawPixmap(x, y, pixmap);
}

void MonitorPluginButtonWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

//    const Dock::Position position = qApp->property(PROP_POSITION).value<Dock::Position>();
//    // 保持横纵比
//    if (position == Dock::Bottom || position == Dock::Top) {
//        setMinimumWidth(qBound(PLUGIN_BACKGROUND_MIN_SIZE, height(), PLUGIN_BACKGROUND_MAX_SIZE));
//    } else {
//        setMaximumHeight(width());
//        setMaximumWidth(QWIDGETSIZE_MAX);
//    }
//    update();
}

void MonitorPluginButtonWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (containCursorPos()) {
        m_hover = true;
    } else {
        m_hover = false;
    }

    QWidget::mouseMoveEvent(event);
}

void MonitorPluginButtonWidget::mousePressEvent(QMouseEvent *event)
{
    if (containCursorPos()) {
        m_pressed = true;
    } else {
        m_pressed = false;
    }

    update();

    QWidget::mousePressEvent(event);
}

void MonitorPluginButtonWidget::mouseReleaseEvent(QMouseEvent *event)
{
    m_pressed = false;
    m_hover = false;
    update();

    QWidget::mouseReleaseEvent(event);
}

void MonitorPluginButtonWidget::leaveEvent(QEvent *event)
{
    m_hover = false;
    m_pressed = false;
    update();

    QWidget::leaveEvent(event);
}

bool MonitorPluginButtonWidget::containCursorPos()
{
    QPoint cursorPos = this->mapFromGlobal(QCursor::pos());
    QRect rect(this->rect());

    int iconSize = qMin(rect.width(), rect.height());
    int w = (rect.width() - iconSize) / 2;
    int h = (rect.height() - iconSize) / 2;

    rect = rect.adjusted(w, h, -w, -h);

    return rect.contains(cursorPos);
}




































