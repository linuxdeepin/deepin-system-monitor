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
#ifndef XWIN_KILL_PREVIEW_TOOLTIP_WIDGET_H
#define XWIN_KILL_PREVIEW_TOOLTIP_WIDGET_H

#include <QWidget>
#include <QIcon>

class QPaintEvent;
class QString;
class QFont;

class XWinKillPreviewTooltipWidget : public QWidget
{
    Q_OBJECT
public:
    explicit XWinKillPreviewTooltipWidget(QWidget *parent = nullptr);

signals:

public slots:

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void initUI();

private:
    QIcon   m_icon {};
    QString m_text {};
    QSize   m_textSize{};
    QFont   m_font;
};

#endif // XWIN_KILL_PREVIEW_TOOLTIP_WIDGET_H
