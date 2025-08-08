// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "base_commandlink_button.h"
#include <QKeyEvent>
#include "ddlog.h"

using namespace DDLog;

BaseCommandLinkButton::BaseCommandLinkButton(const QString name, QWidget *parent)
    : Dtk::Widget::DCommandLinkButton(name, parent)
{
    qCDebug(app) << "BaseCommandLinkButton constructor";
    // Add the event filter.
    this->installEventFilter(this);
}

BaseCommandLinkButton::~BaseCommandLinkButton()
{
    // qCDebug(app) << "BaseCommandLinkButton destructor";
}

bool BaseCommandLinkButton::eventFilter(QObject *obj, QEvent *event)
{
    // qCDebug(app) << "BaseCommandLinkButton eventFilter";
    // Catch the keyboard press enter signal.
    if (obj == this) {
        if (event->type() == QEvent::KeyPress) {
            qCDebug(app) << "KeyPress event";
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Enter || keyEvent->key() == Qt::Key_Return) {
                qCDebug(app) << "Enter key pressed";
                emit clicked();
                return true;
            }
        }
    }
    return DCommandLinkButton::eventFilter(obj, event);
}
