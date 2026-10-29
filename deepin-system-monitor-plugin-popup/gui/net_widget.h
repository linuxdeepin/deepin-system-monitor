// Copyright (C) 2011 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef NET_WIDGET_H
#define NET_WIDGET_H

#include <QWidget>
#include <QPainterPath>
#include <QIcon>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <DApplicationHelper>
DWIDGET_USE_NAMESPACE
#else
#include <DGuiApplicationHelper>
DGUI_USE_NAMESPACE
#endif

class NetWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NetWidget(QWidget *parent = nullptr);
    ~NetWidget();

public slots:
    void updateStatus();

protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *target, QEvent *event) override;

    //!
    //! \brief mouseDoubleClickEvent 鼠标压下事件，唤醒系统监视器主进程，并跳转到CPU详情界面
    //! \param event
    //!
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    void initConnection();
    void getPainterPathByData(QList<double> *listData, QPainterPath &path, qreal maxVlaue);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void changeTheme(DApplicationHelper::ColorType themeType);
#else
    void changeTheme(DGuiApplicationHelper::ColorType themeType);
#endif
    void changeFont(const QFont &font);

private:
    QList<double> *downloadSpeeds;
    QList<double> *uploadSpeeds;
    QPainterPath downloadPath;
    QPainterPath uploadPath;

    int m_width = 0;
    int pointsNumber = 25;
    int pointerRadius = 6;
    int renderMaxHeight = 17;
    QColor summaryColor;
    QColor textColor;
    QColor ltextColor;
    QFont m_sectionFont;
    QFont m_contentFont;
    QFont m_contentUnitFont;
    QFont m_subContentFont;

    QIcon m_icon {};

//    qreal m_recvBps {};
//    qreal m_sentBps {};
    QString m_netReceive;
    QString m_netTotalReceive;
    QString m_netSend;
    QString m_totalSend;

    QString m_netReceiveUnit;
    QString m_netTotalReceiveUnit;
    QString m_netSendUnit;
    QString m_totalSendUnit;

    bool m_isHover = false;
    int m_titleTrans = 0;
    int m_contentTrans = 0;
    int m_hoverTrans = 0;
};

#endif
