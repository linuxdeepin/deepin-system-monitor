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
#include <QIcon>

#include "wm/wm_info.h"

using namespace util::wm;

class QScreen;
class QPaintEvent;
class QResizeEvent;
class QRegion;
class hideEvent;
class QFont;
class QSize;

/**
 * @brief Preview background widget to show current screen snapshot & masked area
 */
class XWinKillPreviewBackgroundWidget : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief Preview background widget constructor
     * @param background Screen snapshot
     * @param parent Parent object
     */
    explicit XWinKillPreviewBackgroundWidget(QPixmap &background, QWidget *parent = nullptr);

signals:

public slots:
    /**
     * @brief Update selected region specified to create masked area
     * @param region Area where to paint mask
     */
    void updateSelection(const QRegion &region);
    /**
     * @brief Clear selected region left before if no selection valid
     */
    inline void clearSelection()
    {
        m_selRegion = {};
        update();
    }

protected:
    /**
     * @brief Paint event handler
     */
    void paintEvent(QPaintEvent *) override;

private:
    /**
     * @brief Initialize ui components
     */
    void initUI();
    /**
     * @brief Initialize connections
     */
    void initConnection();

private:
    // screen snapshot
    QPixmap m_background;
    // selected region if any (top level window that hovered by mouse)
    QRegion m_selRegion {};

    // styled kill help tooltip icon
    QIcon   m_icon {};
    // kill application help tooltip text
    QString m_text {};
    // help tip text size
    QSize   m_textSize{};
    // widget's bounded font
    QFont   m_font;
};

#endif // XWIN_KILL_PREVIEW_BACKGROUND_WIDGET_H
