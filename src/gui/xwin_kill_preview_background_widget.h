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
#ifndef XWIN_KILL_PREVIEW_BACKGROUND_WIDGET_H
#define XWIN_KILL_PREVIEW_BACKGROUND_WIDGET_H

#include <QWidget>
#include <QPixmap>

#include "wm/wm_info.h"

using namespace util::wm;

class QScreen;
class QPaintEvent;
class QResizeEvent;
class QRegion;
class hideEvent;
class XWinKillPreviewTooltipWidget;
class XWinKillPreviewBackgroundWidget : public QWidget
{
    Q_OBJECT
public:
    explicit XWinKillPreviewBackgroundWidget(QPixmap &background, QWidget *parent = nullptr);

signals:

public slots:
    void updateSelection(const QRegion &);
    inline void clearSelection()
    {
        m_selRegion = {};
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override;
    void resizeEvent(QResizeEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    void initUI();
    void initConnection();

private:
    QPixmap m_background;
    XWinKillPreviewTooltipWidget *m_tooltip;
    QRegion m_selRegion {};
};

#endif // XWIN_KILL_PREVIEW_BACKGROUND_WIDGET_H
