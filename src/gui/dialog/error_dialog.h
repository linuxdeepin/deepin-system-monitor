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

#ifndef ERROR_DIALOG_H
#define ERROR_DIALOG_H

#include <DDialog>
#include <DLabel>

DWIDGET_USE_NAMESPACE

/**
 * @brief Dialog to display error messages when error occured
 */
class ErrorDialog : public DDialog
{
    Q_OBJECT

public:
    /**
     * @brief show Show error dialog
     * @param parent Parent widget
     * @param errMsg Error message
     * @param detail Error detail
     */
    static void show(QWidget *parent, const QString &errMsg, const QString &detail);

private:
    /**
     * @brief ErrorDialog private constructor
     * @param errMsg Error message
     * @param detail Error detail
     * @param parent Parent widget
     */
    ErrorDialog(const QString &errMsg, const QString &detail, QWidget *parent = nullptr);
    /**
     * @brief initUI Initialize UI elements
     */
    void initUI();

private:
    // Error message
    QString  m_errMsg       {};
    // Error detail
    QString  m_detail       {};
    // Error detail label
    DLabel  *m_detailLabel  {};
};

#endif // ERROR_DIALOG_H
