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

#ifndef KILL_PROCESS_CONFIRM_DIALOG_H
#define KILL_PROCESS_CONFIRM_DIALOG_H

#include <DDialog>

DWIDGET_USE_NAMESPACE

/**
 * @brief Dialog shown when process kill operation requested
 */
class KillProcessConfirmDialog : public DDialog
{
    Q_OBJECT

public:
    /**
     * @brief KillProcessConfirmDialog Default constructor
     * @param parent Parent object
     */
    explicit KillProcessConfirmDialog(QWidget *parent = nullptr);

    /**
     * @brief result Get standard button enum result
     * @return Standard button enum result
     */
    inline int result() const { return m_result; }
    /**
     * @brief setResult Set standard button enum result
     * @param r Standard button enum result
     */
    inline void setResult(int r) { m_result = r; }

public Q_SLOTS:
    /**
     * @brief onButtonClicked Button click event handler
     * @param index Button index
     * @param text Button text
     */
    void onButtonClicked(int index, const QString &text);

private:
    // Standard button enum result
    int m_result {0};
};

#endif  // KILL_PROCESS_CONFIRM_DIALOG_H
