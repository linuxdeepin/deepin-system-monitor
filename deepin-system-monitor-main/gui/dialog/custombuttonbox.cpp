// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "custombuttonbox.h"
#include "ddlog.h"

#include <QFocusEvent>

using namespace DDLog;

CustomButtonBox::CustomButtonBox(QWidget *parent)
    : DButtonBox(parent)
{
    qCDebug(app) << "CustomButtonBox constructor";
}

void CustomButtonBox::focusInEvent(QFocusEvent *event)
{
    // qCDebug(app) << "CustomButtonBox focusInEvent, reason: " << event->reason();
    DButtonBox::focusInEvent(event);
    //窗口激活时，不设置Button焦点显示
    if (event->reason() != Qt::ActiveWindowFocusReason) {
        qCDebug(app) << "Setting focus on checked button";
        //设置当前选中想为焦点
        this->button(checkedId())->setFocus();
    }
}
