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

#ifndef SERVICE_NAME_SUB_INPUT_DIALOG_H
#define SERVICE_NAME_SUB_INPUT_DIALOG_H

#include <DDialog>
#include <DLineEdit>
#include <DWidget>

DWIDGET_USE_NAMESPACE

class ServiceNameSubInputDialog : public DDialog
{
    Q_OBJECT

public:
    ServiceNameSubInputDialog(DWidget *parent = nullptr);

    QString getServiceSubName() const { return m_name; }
    void setServiceSubName(const QString &name) { m_name = name; }

    int result() const { return m_result; }
    void setResult(int r) { m_result = r; }

public Q_SLOTS:
    void onButtonClicked(int index, const QString &text);

protected:
    void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;

private:
    QString m_name {};
    int m_result {0};
    DLineEdit *m_nameLineEdit;
};

#endif  // SERVICE_NAME_SUB_INPUT_DIALOG_H
