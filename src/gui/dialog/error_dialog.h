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

#ifndef ERROR_DIALOG_H
#define ERROR_DIALOG_H

#include <DDialog>
#include <DLabel>

DWIDGET_USE_NAMESPACE

class ErrorDialog : public DDialog
{
    Q_OBJECT

public:
    static void show(QWidget *parent, const QString &errMsg, const QString &detail);

private:
    ErrorDialog(const QString &errMsg, const QString &detail, QWidget *parent = nullptr);
    void initUI();

private:
    QString  m_errMsg       {};
    QString  m_detail       {};
    DLabel  *m_detailLabel  {};
};

#endif // ERROR_DIALOG_H
