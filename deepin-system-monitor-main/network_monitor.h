// Copyright (C) 2011 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef NETWORKMONITOR_H
#define NETWORKMONITOR_H

#include <QIcon>
#include <QWidget>
#include <QPainterPath>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <DApplicationHelper>
DWIDGET_USE_NAMESPACE
#else
#include <DGuiApplicationHelper>
DGUI_USE_NAMESPACE
#endif

class NetworkMonitor : public QWidget
{
    Q_OBJECT

public:
    explicit NetworkMonitor(QWidget *parent = nullptr);
    ~NetworkMonitor();

signals:
    void clicked(QString msgCode);

public slots:
    void updateStatus();

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;

    //!
    //! \brief mouseMoveEvent 目前的策略是屏蔽鼠标移动拖拽的响应操作
    //! \param event
    //!
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

private:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void changeTheme(DApplicationHelper::ColorType themeType);
#else
    void changeTheme(DGuiApplicationHelper::ColorType themeType);
#endif
    void changeFont(const QFont &font);
    void getPainterPathByData(QList<double> *listData, QPainterPath &path, qreal maxVlaue);

private:
    QIcon m_icon;

    QList<double> *downloadSpeeds;
    QList<double> *uploadSpeeds;
    QPainterPath downloadPath;
    QPainterPath uploadPath;

    QColor downloadColor = "#55D500";
    QColor recvStartColor {"#FF1212"};
    QColor recvEndColor {"#E0FF00"};
    QColor sentStartColor {"#2357FF"};
    QColor sentEndColor {"#00A6E3"};
    QColor uploadColor = "#C362FF";
    QColor m_recvIndicatorColor {"#E14300"};
    QColor m_sentIndicatorColor {"#004EEF"};
    QColor summaryColor;
    QColor textColor;
    QColor ltextColor;
    QColor m_frameColor;

    QFont m_sectionFont;
    QFont m_contentFont;
    QFont m_subContentFont;

    int renderMaxHeight = 20;

    qreal m_recvBps {};
    qreal m_sentBps {};
    qulonglong m_totalRecvBytes {};
    qulonglong m_totalSentBytes {};
};

#endif
