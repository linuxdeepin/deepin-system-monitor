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
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "service_name_sub_input_dialog.h"

#include <DApplication>

#include <QDebug>
#include <QMessageBox>

#include "utils.h"

// constructor
ServiceNameSubInputDialog::ServiceNameSubInputDialog(DWidget *parent)
    : DDialog(parent)
{
    // set dialog icon
    setIcon(QIcon::fromTheme("dialog-question"));

    addSpacing(10);

    // service sub name text editor
    m_nameLineEdit = new DLineEdit(this);
    Q_ASSERT(m_nameLineEdit);
    addContent(m_nameLineEdit);

    // add ok & cancel button
    addButton(DApplication::translate("Service.Instance.Name.Dialog", "OK"), true);
    addButton(DApplication::translate("Service.Instance.Name.Dialog", "Cancel"));

    // connect button clicked signal
    connect(this,
            &ServiceNameSubInputDialog::buttonClicked,
            this,
            &ServiceNameSubInputDialog::onButtonClicked);
}

// button click event handler
void ServiceNameSubInputDialog::onButtonClicked(int index, const QString &)
{
    if (index == 0) {
        m_name = m_nameLineEdit->text();
        setResult(QMessageBox::Ok);
    } else {
        setResult(QMessageBox::Cancel);
    }
}

// show event handler
void ServiceNameSubInputDialog::showEvent(QShowEvent *)
{
    // focus on text editor when shown
    m_nameLineEdit->lineEdit()->setFocus();
}
