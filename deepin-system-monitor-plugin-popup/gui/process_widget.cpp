/*
* Copyright (C) 2011 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      Wang Yong <wangyong@deepin.com>
* Maintainer:  maojj <maojunjie@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "process_widget.h"
#include "../common/utils.h"
#include "common/datacommon.h"
#include "process_widget.h"

#include <DApplication>
#include <DApplicationHelper>
#include <DPalette>
#include <DStyleHelper>

#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <QBrush>
#include <QPaintEvent>
#include <QFontMetrics>
#include <QVBoxLayout>

DWIDGET_USE_NAMESPACE

using namespace Utils;

ProcessWidget::ProcessWidget(QWidget *parent)
    : QWidget(parent)
{
     auto *dAppHelper = DApplicationHelper::instance();
    connect(dAppHelper, &DApplicationHelper::themeTypeChanged, this, &ProcessWidget::changeTheme);
    changeTheme(dAppHelper->themeType());

    setFixedSize(280, 245);

    changeFont(DApplication::font());
    connect(dynamic_cast<QGuiApplication *>(DApplication::instance()), &DApplication::fontChanged,
            this, &ProcessWidget::changeFont);


    m_processTableView = new ProcessTableView;

    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    mainLayout->setMargin(10);
    mainLayout->addSpacing(18);
    mainLayout->addWidget(m_processTableView);

    setLayout(mainLayout);
    setFocusPolicy(Qt::NoFocus);
    installEventFilter(this);

    setAttribute(Qt::WA_TranslucentBackground);

}
ProcessWidget::~ProcessWidget() {}


void ProcessWidget::updateStatus(qreal cpuPercent, const QList<qreal> cPercents)
{
    totalCpuPercent = cpuPercent;

    for (int i = 0; i < cPercents.size(); i++) {
        QList<qreal> cpuPercent = cpuPercents[i];

        cpuPercent.append(cPercents[i]);

        if (cpuPercent.size() > pointsNumber) {
            cpuPercent.pop_front();
        }

        cpuPercents[i] = cpuPercent;
    }

    update();
}

void ProcessWidget::changeTheme(DApplicationHelper::ColorType themeType)
{
    switch (themeType) {
    case DApplicationHelper::LightType:
        m_titleTrans = Globals::TitleTransLight;
        m_contentTrans = Globals::contentTransLight;
        m_hoverTrans = Globals::hoverTransLight;
        m_icon = QIcon(QString(":/icons/icon_process_light.png"));
        break;
    case DApplicationHelper::DarkType:
        m_titleTrans = Globals::TitleTransDark;
        m_contentTrans = Globals::contentTransDark;
        m_hoverTrans = Globals::hoverTransDark;
//        m_icon = QIcon(iconPathFromQrc("dark/icon_network_light.svg"));
        break;
    default:
        break;
    }

    // init colors
    auto *dAppHelper = DApplicationHelper::instance();
    auto palette = dAppHelper->applicationPalette();
#ifndef THEME_FALLBACK_COLOR
    ltextColor = palette.color(DPalette::TextTitle);
#else
    ltextColor = palette.color(DPalette::Text);
#endif

    textColor = palette.color(DPalette::Text);

    textColor = palette.color(DPalette::Text);
    summaryColor = palette.color(DPalette::TextTips);

}

void ProcessWidget::paintEvent(QPaintEvent *e)
{
    setFixedWidth(280);
    QPainter painter;
    painter.begin(this);
    painter.setRenderHint(QPainter::Antialiasing);

    //裁剪绘制区域
    QPainterPath path;
    path.addRoundedRect(rect(), 8, 8);
    painter.setClipPath(path);
    if (m_isHover) {
        painter.fillRect(rect(), QBrush(QColor(255, 255, 255, m_hoverTrans)));
    } else {
        painter.fillRect(rect(), QBrush(QColor(255, 255, 255, 0)));
    }

    //标题栏背景
    QRect titleRect(rect().x(), rect().y(), 280, 36);
    painter.fillRect(titleRect, QBrush(QColor(255, 255, 255, m_titleTrans)));
    QRect contentRect(rect().x(), rect().y()+36, 280, 209);
    painter.fillRect(contentRect, QBrush(QColor(255, 255, 255,m_contentTrans)));

    //标题
    painter.setFont(m_sectionFont);
    QFontMetrics fmTitle = painter.fontMetrics();
    int widthTitleTxt = fmTitle.width("进程");
    int heightTitleTxt = fmTitle.descent()+fmTitle.ascent();
    QRect netTitleRect(titleRect.x(), titleRect.y(), widthTitleTxt, heightTitleTxt);
    painter.drawText(titleRect, Qt::AlignHCenter | Qt::AlignVCenter,"进程");

    //图标
    int iconSize = 20;
    QRect iconRect(titleRect.x()+titleRect.width()/2-netTitleRect.width() -5, titleRect.y() + qCeil((titleRect.height() - iconSize) / 2.) + 2,iconSize, iconSize);
    m_icon.paint(&painter, iconRect);

}

bool ProcessWidget::eventFilter(QObject *target, QEvent *event)
{
    if (target == this) {
        if (event->type() == QEvent::Enter) {
            m_isHover = true;
            update();
            return true;
        }
        if (event->type() == QEvent::Leave) {
            m_isHover = false;
            update();
            return true;
        }
    }
    return QWidget::eventFilter(target, event);
}

void ProcessWidget::changeFont(const QFont &font)
{
    m_sectionFont = font;
    m_sectionFont.setFamily("SourceHanSansSC");
    m_sectionFont.setWeight(QFont::DemiBold);
    m_sectionFont.setPointSizeF(m_sectionFont.pointSizeF());

    m_contentFont = font;
    m_contentFont.setFamily("HelveticaNeueLT");
    m_contentFont.setWeight(QFont::Normal);
    m_contentFont.setPointSizeF(m_contentFont.pointSizeF()+5);

    m_subContentFont = font;
    m_subContentFont.setFamily("SourceHanSansSC");
    m_subContentFont.setWeight(QFont::ExtraLight);
    m_subContentFont.setPointSizeF(m_subContentFont.pointSizeF()-2 );
}
