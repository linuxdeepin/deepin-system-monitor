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
#include "dbus/dbuscallmaininterface.h"
#include "datadealsingleton.h"

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
#include <QProcess>

DWIDGET_USE_NAMESPACE

//1080p下，面板总高度
#define STANDARD_TOTAL_HEIGHT     972
//1080p下，面板高度-进程框高度
#define STANDARD_PROCESS_OTHER_HEIGHT     687

using namespace Utils;

ProcessWidget::ProcessWidget(QWidget *parent)
    : QWidget(parent)
{
     auto *dAppHelper = DApplicationHelper::instance();
    connect(dAppHelper, &DApplicationHelper::themeTypeChanged, this, &ProcessWidget::changeTheme);
    changeTheme(dAppHelper->themeType());

    setFixedSize(280, m_processWidgetHeight);

    changeFont(DApplication::font());
    connect(dynamic_cast<QGuiApplication *>(DApplication::instance()), &DApplication::fontChanged,
            this, &ProcessWidget::changeFont);
    changeFont(DApplication::font());
    connect(dynamic_cast<QGuiApplication *>(DApplication::instance()), &DApplication::fontChanged,
            this, &ProcessWidget::changeFont);

    m_processTableView = new ProcessTableView(this);

    QHBoxLayout *headerLayout = new QHBoxLayout(this);
    DLabel *headerLabel = new DLabel(this);
    headerLabel->setLayout(headerLayout);
    headerLabel->setFixedHeight(30);
    headerLabel->setContentsMargins(10, 0, 10, 0);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    mainLayout->setMargin(0);
    mainLayout->addSpacing(35);
    mainLayout->addWidget(headerLabel);
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
        m_icon = QIcon(QString(":/icons/deepin/builtin/light/icon_list.png"));
        break;
    case DApplicationHelper::DarkType:
        m_titleTrans = Globals::TitleTransDark;
        m_contentTrans = Globals::contentTransDark;
        m_hoverTrans = Globals::hoverTransDark;
        m_icon = QIcon(QString(":/icons/deepin/builtin/dark/icon_list.png"));
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

void ProcessWidget::autoHeight(int height)
{
    int standardHeight = STANDARD_TOTAL_HEIGHT;
    if (height > standardHeight)
    {
        m_processWidgetHeight = height - STANDARD_PROCESS_OTHER_HEIGHT;
        setFixedSize(280, m_processWidgetHeight);
        update();
    }
}

void ProcessWidget::paintEvent(QPaintEvent *e)
{
    QPainter painter;
    painter.begin(this);
    painter.setPen(textColor);
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
    QRect contentRect(rect().x(), rect().y()+36, 280, m_processWidgetHeight - 36);
    painter.fillRect(contentRect, QBrush(QColor(255, 255, 255,m_contentTrans)));

    //标题
    painter.setFont(m_sectionFont);
    QFontMetrics fmTitle = painter.fontMetrics();
    int widthTitleTxt = fmTitle.width(tr("Processes"));
    int heightTitleTxt = fmTitle.descent()+fmTitle.ascent();
    QRect netTitleRect(titleRect.x(), titleRect.y(), widthTitleTxt, heightTitleTxt);
    painter.drawText(titleRect, Qt::AlignHCenter | Qt::AlignVCenter, tr("Processes"));

    //图标
    int iconSize = 20;
    QRect iconRect(titleRect.x()+(titleRect.width()-widthTitleTxt)/2-iconSize, titleRect.y() + qCeil((titleRect.height() - iconSize) / 2.) + 2,iconSize, iconSize);
    m_icon.paint(&painter, iconRect);

    //表头
    painter.setOpacity(0.6);
    QFontMetrics fmSubContent(m_subContentFont);
    QString strName = tr("Name");
    painter.setFont(m_subContentFont);
    QRect NameRect(contentRect.x() + 25, contentRect.y() + 10,
                   fmSubContent.size(Qt::TextSingleLine, strName).width(), fmSubContent.height());
    painter.drawText(NameRect, Qt::AlignLeft | Qt::AlignVCenter, strName);

    QString strCPU = tr("CPU");
    QRect CpuRect(contentRect.x() + contentRect.width()-20 -fmSubContent.size(Qt::TextSingleLine, strCPU).width(),
                  contentRect.y() + 10,
                  fmSubContent.size(Qt::TextSingleLine, strCPU).width(),
                  fmSubContent.height());
    painter.drawText(CpuRect, Qt::AlignLeft | Qt::AlignVCenter, strCPU);
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

void ProcessWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if (event->type() == QEvent::MouseButtonDblClick) {
            DataDealSingleton::getInstance().sendJumpWidgetMessage("MSG_PROCESS");
        }
    }
    return QWidget::mousePressEvent(event);
}

void ProcessWidget::changeFont(const QFont &font)
{
    m_sectionFont = font;
    m_sectionFont.setWeight(QFont::DemiBold);
    m_sectionFont.setPointSizeF(m_sectionFont.pointSizeF());

    m_contentFont = font;
    m_contentFont.setWeight(QFont::Normal);
    m_contentFont.setPointSizeF(Globals::ContentFont);

    m_subContentFont = font;
    m_subContentFont.setWeight(QFont::ExtraLight);
    m_subContentFont.setPointSizeF(Globals::subContentFont+1);
}
