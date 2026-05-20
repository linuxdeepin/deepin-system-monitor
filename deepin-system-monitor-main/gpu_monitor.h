// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef GPU_MONITOR_H
#define GPU_MONITOR_H

#include <QIcon>
#include <QWidget>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <DApplicationHelper>
DWIDGET_USE_NAMESPACE
#else
#include <DGuiApplicationHelper>
DGUI_USE_NAMESPACE
#endif

class QPropertyAnimation;
class GPUInfoModel;

/**
 * @brief GPU monitor widget for the expand view (ring-based utilization display)
 */
class GpuMonitor : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal progress READ progress WRITE setProgress)

public:
    explicit GpuMonitor(QWidget *parent = nullptr);
    ~GpuMonitor();

signals:
    void clicked(QString msgCode);

public slots:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void changeTheme(DApplicationHelper::ColorType themeType);
#else
    void changeTheme(DGuiApplicationHelper::ColorType themeType);
#endif
    void updateStatus();
    void onAnimationFinished();

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *ev) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

    qreal progress() const { return m_progress; }
    void setProgress(qreal p) { m_progress = p; }

private:
    void changeFont(const QFont &font);

private:
    QIcon m_icon;

    QColor ringBackgroundColor;
    QColor ringForegroundColor {"#AA00FF"};
    QColor textColor;
    QColor numberColor;
    QColor summaryColor;
    QColor memoryColor {"#AA00FF"};

    double ringBackgroundOpacity = 0.1;
    double ringForegroundOpacity = 1;
    int outsideRingRadius = 90;
    int insideRingRadius = 82;
    int ringWidth = 8;
    int ringCenterPointerX;
    int ringCenterPointerY = 86;
    int pointerRadius = 3;

    QFont m_titleFont;
    QFont m_contentFont;
    QFont m_subContentFont;
    QFont m_percentFont;

    qreal m_progress {};
    qreal m_lastGpuPercent = 0.;
    QPropertyAnimation *m_animation {};

    GPUInfoModel *m_gpuInfoModel {};

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    DApplicationHelper::ColorType m_themeType;
#else
    DGuiApplicationHelper::ColorType m_themeType;
#endif
};

#endif // GPU_MONITOR_H
