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

#include "cpu_widget.h"
#include "common/datacommon.h"
#include "datadealsingleton.h"
#include "dbus/dbuscallmaininterface.h"

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
#include <QTimer>

const int cpuTxtWidth = 96;

DWIDGET_USE_NAMESPACE

CpuWidget::CpuWidget(QWidget *parent)
    : QWidget(parent)
{
    downloadSpeeds = new QList<double>();
    for (int i = 0; i <= pointsNumber; i++) {
        downloadSpeeds->append(0);
    }


    auto *dAppHelper = DApplicationHelper::instance();
    connect(dAppHelper, &DApplicationHelper::themeTypeChanged, this, &CpuWidget::changeTheme);
    changeTheme(dAppHelper->themeType());

    setFixedSize(280, 111);

    initConnection();
    this->installEventFilter(this);
}

CpuWidget::~CpuWidget() {
    delete downloadSpeeds;
}

void CpuWidget::initConnection()
{
    changeFont(DApplication::font());
    connect(dynamic_cast<QGuiApplication *>(DApplication::instance()), &DApplication::fontChanged,
            this, &CpuWidget::changeFont);

    connect(&DataDealSingleton::getInstance(), &DataDealSingleton::sigDataUpdate, this, &CpuWidget::updateStatus);
}

void CpuWidget::getPainterPathByData(QList<double> *listData, QPainterPath &path, qreal maxVlaue)
{
    qreal offsetX = 0;
    qreal distance = (this->width() - cpuTxtWidth-10) * 1.0 / pointsNumber;
    int dataCount = listData->size();

    if (maxVlaue < 0.000001)
        maxVlaue = 1;

    for (int i = 0;  i < dataCount - 1; i++) {
        //sp 为线段的起始点，ep 为线段的终点
        //c1，c2 为贝塞尔曲线的控制点
        QPointF sp = QPointF(offsetX, 20 * listData->at(i) / maxVlaue);;
        QPointF ep = QPointF(offsetX + distance, 20 * listData->at(i + 1) / maxVlaue);;

        offsetX += distance;

        QPointF c1 = QPointF((sp.x() + ep.x()) / 2.0, sp.y());
        QPointF c2 = QPointF((sp.x() + ep.x()) / 2.0, ep.y());
        path.cubicTo(c1, c2, ep);

//       qInfo()<<"sp,  x:"<<offsetX<< "Y:"<< 20 * listData->at(i) / maxVlaue;
    }
}

void CpuWidget::updateStatus()
{
    if (!DataDealSingleton::getInstance().readCpuPer(m_cpuPer))
        qInfo()<<"false: "<<m_cpuPer;

//    qInfo()<<"m_cpuPer: "<<m_cpuPer;

    // Init download path.
    downloadSpeeds->append(m_cpuPer);

    if (downloadSpeeds->size() > pointsNumber + 1) {
        downloadSpeeds->pop_front();
    }
    double downloadMaxHeight = *std::max_element(downloadSpeeds->begin(), downloadSpeeds->end()) * 1.1;

    QPainterPath tmpDownloadpath;
    getPainterPathByData(downloadSpeeds, tmpDownloadpath, downloadMaxHeight);
    downloadPath = tmpDownloadpath;

    update();
}

void CpuWidget::changeTheme(DApplicationHelper::ColorType themeType)
{
    switch (themeType) {
    case DApplicationHelper::LightType:
        m_titleTrans = Globals::TitleTransLight;
        m_contentTrans = Globals::contentTransLight;
        m_hoverTrans = Globals::hoverTransLight;
        m_icon = QIcon(QString(":/icons/icon_network_light.png"));
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
    summaryColor = palette.color(DPalette::TextTips);
}

void CpuWidget::paintEvent(QPaintEvent *e)
{
    QPainter painter;
    painter.begin(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QFontMetrics fmContent(m_TitleFont);
    QFontMetrics fmSubContent(m_cpuFont);
    QFontMetrics fmTextContent(m_textFont);

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
    QRect contentRect(rect().x(), rect().y()+36, 280, 75);
    painter.fillRect(contentRect, QBrush(QColor(255, 255, 255,m_contentTrans)));

    //图标
    m_icon = QIcon(QString(":/icons/icon_cpu_light.png"));
    int iconSize = 20;
    QRect iconRect(titleRect.x()+97, titleRect.y() + qCeil((titleRect.height() - iconSize) / 2.) + 2,iconSize, iconSize);
    m_icon.paint(&painter, iconRect);

    //标题
    painter.setFont(m_TitleFont);
    QFontMetrics fmTitle = painter.fontMetrics();
    int widthTitle = fmTitle.width("处理器");
    int heightTitle = fmTitle.descent()+fmTitle.ascent();
    QRect cpuTitleRect(titleRect.x(), titleRect.y(), widthTitle, heightTitle);
    painter.drawText(titleRect, Qt::AlignHCenter | Qt::AlignVCenter,
                     fmContent.elidedText("处理器", Qt::ElideRight, cpuTitleRect.width()));

    //cpu使用率
//    painter.setPen(ltextColor);
    painter.setFont(m_cpuFont);
    QFontMetrics fmCpu = painter.fontMetrics();
    QString cpuStatText = QString::number(m_cpuPer, 'f', 1);
    int widthCpu = fmCpu.width(cpuStatText);
    int heightCpu = fmCpu.descent()+fmCpu.ascent();
    QRect cpuUsageRect(contentRect.x()+27, contentRect.y()+13, widthCpu, heightCpu);
    painter.drawText(cpuUsageRect, Qt::AlignLeft | Qt::AlignTop, cpuStatText);
    //cpu使用率百分号
    painter.setFont(m_TitleFont);
    QFontMetrics fmCpuper = painter.fontMetrics();
    int widthCpuper = fmCpuper.width("%");
    int heightCpuper = fmCpuper.descent()+fmCpuper.ascent();
    QRect cpuperUsageRect(contentRect.x()+27+widthCpu, contentRect.y()+13+ heightCpu/4, widthCpuper, heightCpuper);
    painter.drawText(cpuperUsageRect, Qt::AlignLeft | Qt::AlignTop, "%");

    //总使用率 文本
//    painter.setPen(summaryColor);
    painter.setFont(m_textFont);
    QFontMetrics fmText = painter.fontMetrics();
    QRect cpuTextRect(cpuUsageRect.x(), cpuUsageRect.y()+cpuUsageRect.height(),
                      fmContent.size(Qt::TextSingleLine, tr("总使用率")).width(), fmTextContent.height());
    painter.drawText(cpuTextRect, Qt::AlignLeft | Qt::AlignTop,tr("总使用率"));

    //分隔符
    int sepheight = 50;
    int sepwidth =(contentRect.width() - 92-10)/2;
    int rightMax = contentRect.x()+contentRect.width()-10;
    painter.setFont(m_cpuFont);
    QRect separatorRect1(contentRect.x()+cpuTxtWidth, contentRect.y()+10, 1, sepheight);
    painter.fillRect(separatorRect1, QBrush(QColor(0, 0, 0,20)));
    QRect separatorRect2(contentRect.x()+cpuTxtWidth+sepwidth, contentRect.y()+10, 1, sepheight);
    painter.fillRect(separatorRect2, QBrush(QColor(0, 0, 0,20)));
    QRect separatorRect3(rightMax, contentRect.y()+10, 1, sepheight);
    painter.fillRect(separatorRect3, QBrush(QColor(0, 0, 0,20)));

    //走势图
    QPainterPath framePath;
    QRect chartRect(separatorRect1.x(), separatorRect1.y(), contentRect.width()-cpuTxtWidth-10, sepheight*2);
//    framePath.addRect(chartRect);

    QColor Color {"#004EEF"};
    QBrush recvBrush(Color);
    qreal networkCurveWidth = 1.2;

    QPainterPath clip;
    clip.addRect(chartRect);
    painter.setClipPath(clip);
    painter.setRenderHint(QPainter::Antialiasing, true);//反锯齿


    painter.translate(chartRect.x() + 2, chartRect.y() + chartRect.height() / 2 - 2);
    painter.scale(1, -1);
    painter.setPen(QPen(recvBrush, networkCurveWidth));
    painter.setRenderHint(QPainter::Antialiasing);
    painter.drawPath(downloadPath);
}

bool CpuWidget::eventFilter(QObject *target, QEvent *event)
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

void CpuWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        //1.先唤醒主进程
        bool rt = QProcess::startDetached(Globals::DEEPIN_SYSTEM_MONITOR_PATH);
        if (true == rt) {
            //2.跳转DBUS
            QTimer::singleShot(500, this, [=]() {
                DbusCallMainInterface::getInstance()->jumpWidget(QString("MSG_CPU"));
            });
        }
    }
    return QWidget::mousePressEvent(event);
}

void CpuWidget::changeFont(const QFont &font)
{
    m_TitleFont = font;
    m_TitleFont.setWeight(QFont::DemiBold);
    m_TitleFont.setPointSizeF(m_TitleFont.pointSizeF());

    m_cpuFont = font;
    m_cpuFont.setWeight(QFont::Normal);
    m_cpuFont.setPointSizeF(m_cpuFont.pointSizeF()+8);

    m_textFont = font;
    m_textFont.setWeight(QFont::ExtraLight);
    m_textFont.setPointSizeF(m_textFont.pointSizeF() - 2);
}
