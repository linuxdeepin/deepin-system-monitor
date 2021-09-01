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

#ifndef SERVICE_NAME_SUB_INPUT_DIALOG_H
#define SERVICE_NAME_SUB_INPUT_DIALOG_H

#include <DDialog>
#include <DLineEdit>
#include <DWidget>

DWIDGET_USE_NAMESPACE

/**
 * @brief Dialog shown to user when service sub name required
 */
class ServiceNameSubInputDialog : public DDialog
{
    Q_OBJECT

public:
    /**
     * @brief Dialog constructor
     * @param parent Parent object
     */
    explicit ServiceNameSubInputDialog(DWidget *parent = nullptr);

    /**
     * @brief Get service sub name
     * @return Sub name of service
     */
    QString getServiceSubName() const { return m_name; }
    /**
     * @brief Set service sub name
     * @param name Sub name of service
     */
    void setServiceSubName(const QString &name) { m_name = name; }

    /**
     * @brief result Get standard button enum result
     * @return Standard button enum result
     */
    int result() const { return m_result; }
    /**
     * @brief setResult Set standard button enum result
     * @param r Standard button enum result
     */
    void setResult(int r) { m_result = r; }

public Q_SLOTS:
    /**
     * @brief onButtonClicked Button clicked event handler
     * @param index Button index
     * @param text Button text
     */
    void onButtonClicked(int index, const QString &text);

protected:
    /**
     * @brief showEvent Show event handler
     * @param event Show event
     */
    void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;

private:
    // Service sub name
    QString m_name {};
    // Standard button enum result
    int m_result {0};
    // Service sub name text input
    DLineEdit *m_nameLineEdit {};

    //确认按钮翻译名称
    QString m_okButtonTranslateName {};
    //取消按钮翻译名称
    QString m_cancelButtonTranslateName {};
};

#endif  // SERVICE_NAME_SUB_INPUT_DIALOG_H
