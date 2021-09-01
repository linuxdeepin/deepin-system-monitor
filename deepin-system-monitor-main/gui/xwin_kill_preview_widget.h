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
#ifndef XWIN_KILL_PREVIEW_WIDGET_H
#define XWIN_KILL_PREVIEW_WIDGET_H

#include <QWidget>

namespace core {
namespace wm {
class WMInfo;
}
} // namespace core
class XWinKillPreviewBackgroundWidget;

/**
 * @brief Preview widget to show when interaction with user while kill application by window requested
 */
class XWinKillPreviewWidget : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief Constructor
     * @param parent Parent object
     */
    explicit XWinKillPreviewWidget(QWidget *parent = nullptr);
    /**
     * @brief Destructor
     */
    ~XWinKillPreviewWidget() override;

signals:
    /**
     * @brief Window clicked signal
     * @param pid Gui process's pid of whose widgets has been clicked
     */
    void windowClicked(pid_t pid);
    /**
     * @brief Cursor updated signal
     * @param cursor Updated cursor instance
     */
    void cursorUpdated(const QCursor &cursor);

public slots:

protected:
    /**
     * @brief mousePressEvent Mouse press event handler
     * @param event Mouse press event
     */
    void mousePressEvent(QMouseEvent *event) override;
    /**
     * @brief mouseMoveEvent Mouse move event handler
     * @param event Mouse move event
     */
    void mouseMoveEvent(QMouseEvent *event) override;
    /**
     * @brief keyPressEvent Key press event handler
     * @param event Key press event
     */
    void keyPressEvent(QKeyEvent *event) override;

private:
    /**
     * @brief Initialize ui components
     */
    void initUI();
    /**
     * @brief Initialize connections
     */
    void initConnections();

private:
    // Window manager (x11) instance
    core::wm::WMInfo *m_wminfo;
    // List of background widgets, per screen each
    QList<XWinKillPreviewBackgroundWidget *> m_backgroundList;
    // List of screens
    QList<QScreen *> m_screens;

    // Styled kill cursor
    QCursor m_killCursor;
    // Default cursor style
    QCursor m_defaultCursor;
};

#endif // XWIN_KILL_PREVIEW_WIDGET_H
