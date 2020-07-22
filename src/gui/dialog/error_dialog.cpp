/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      maojj <maojunjie@uniontech.com>
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

#include "error_dialog.h"

#include "gui/ui_common.h"

#include <DApplicationHelper>
#include <DFontSizeManager>

#include <QApplication>
#include <QMap>
#include <QDebug>

void ErrorDialog::show(QWidget *parent, const QString &errMsg, const QString &detail)
{
    auto *dlg = new ErrorDialog(errMsg, detail, parent);
    dlg->exec();
}

void ErrorDialog::initUI()
{
    setIcon(QIcon::fromTheme("dialog-warning"));
    setAttribute(Qt::WA_DeleteOnClose);

    m_detailLabel = new DLabel(this);
    auto pa = DApplicationHelper::instance()->palette(m_detailLabel);
    auto color = pa.color(DPalette::TextTips);
    pa.setColor(DPalette::Foreground, color);
    m_detailLabel->setPalette(pa);
    DFontSizeManager::instance()->bind(m_detailLabel, DFontSizeManager::T6, QFont::Medium);
    m_detailLabel->setWordWrap(true);

    setMessage(m_errMsg);
    m_detailLabel->setText(m_detail);

    addSpacing(5);

    addContent(m_detailLabel, Qt::AlignHCenter);
    addButton(QApplication::translate("Error.Dialog", "OK"), true);

    setMaximumWidth(720);
}

ErrorDialog::ErrorDialog(const QString &errMsg, const QString &detail, QWidget *parent)
    : DDialog(parent), m_errMsg(errMsg), m_detail(detail)
{
    initUI();
}
