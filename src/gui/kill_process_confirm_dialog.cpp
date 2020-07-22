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

#include "kill_process_confirm_dialog.h"

#include "utils.h"

#include <DApplication>
#include <DHiDPIHelper>
#include <DWidget>

#include <QMessageBox>

KillProcessConfirmDialog::KillProcessConfirmDialog(QWidget *parent)
    : DDialog(parent)
{
    setIcon(QIcon::fromTheme("dialog-warning"));

    connect(this, &KillProcessConfirmDialog::buttonClicked, this,
            &KillProcessConfirmDialog::onButtonClicked);
}

void KillProcessConfirmDialog::onButtonClicked(int index, const QString &text)
{
    Q_UNUSED(text);

    if (index == 1) {
        setResult(QMessageBox::Ok);
    } else {
        setResult(QMessageBox::Cancel);
    }
}
