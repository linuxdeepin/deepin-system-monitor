// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef COMPACTGPUMONITOR_H
#define COMPACTGPUMONITOR_H

#include <QWidget>
#include <QPainterPath>

class GPUInfoModel;

/**
 * @brief Compact GPU monitor widget for the compact view (bar chart style)
 */
class CompactGpuMonitor : public QWidget
{
    Q_OBJECT

public:
    explicit CompactGpuMonitor(QWidget *parent = nullptr);
    ~CompactGpuMonitor();

signals:
    void clicked(QString msgCode);

public slots:
    void updateStatus();

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

private:
    void changeFont(const QFont &font);
    void getPainterPathByData(QList<double> *listData, QPainterPath &path, qreal maxValue);

private:
    QList<qreal> *gpuPercents;
    qreal m_gpuPercent {};

    int m_bulletSize = 6;

    QColor m_gpuColor {"#AA00FF"};

    QPainterPath gpuPath;

    int renderMaxHeight = 80;
    int pointsNumber = 25;

    QFont m_tagFont;
    QFont m_statFont;

    GPUInfoModel *m_gpuInfoModel {};
};

#endif // COMPACTGPUMONITOR_H
