// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "priority_slider.h"
#include "ddlog.h"

#include <QResizeEvent>

using namespace DDLog;

// constructor
PrioritySlider::PrioritySlider(Qt::Orientation orientation, QWidget *parent)
    : DSlider(orientation, parent)
{
    qCDebug(app) << "PrioritySlider constructor";
}

// resize event handler
void PrioritySlider::resizeEvent(QResizeEvent *event)
{
    // qCDebug(app) << "PrioritySlider resizeEvent";
    DSlider::resizeEvent(event);
    // emit size changed signal
    Q_EMIT sizeChanged(event->size());
}
