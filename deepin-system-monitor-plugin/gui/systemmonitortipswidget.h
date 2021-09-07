/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd.
*
* Author:     yukuan  <yukuan@uniontech.com>
*
* Maintainer: yukuan  <yukuan@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef SYSTEMMONITORTIPSWIDGET_H
#define SYSTEMMONITORTIPSWIDGET_H

#include <QFrame>
class SystemMonitorTipsWidget : public QFrame
{
public:
    explicit SystemMonitorTipsWidget(QWidget *parent = nullptr);

    //!
    //! \brief setSystemMonitorTipsText 设置系统监视器tips标签文字信息
    //! \param strList
    //!
    void setSystemMonitorTipsText(QStringList strList);

protected:
    //!
    //! \brief paintEvent 重绘系统监视器插件提示框
    //! \param event
    //!
    void paintEvent(QPaintEvent *event) override;

    // 获取字体改变事件
    bool event(QEvent *event) override;

private:
    QStringList m_textList;
};

#endif // SYSTEMMONITORTIPSWIDGET_H
