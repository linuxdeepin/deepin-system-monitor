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

    //!
    //! \brief event 捕捉字体改变事件
    //! \param event 捕获的事件对象
    //! \return 成功返回true,失败返回false
    //!
    bool event(QEvent *event) override;

private:
    QStringList m_textList; // 获取到的悬浮框标签文字内容列表

    int m_leftWidth = 0;    // 设置字体左侧的宽度
    int m_rightWidth = 0;   // 设置字体右侧的宽度
};

#endif // SYSTEMMONITORTIPSWIDGET_H
