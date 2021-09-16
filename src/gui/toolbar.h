/*
* Copyright (C) 2011 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      Wang Yong <wangyong@deepin.com>
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

#ifndef TOOLBAR_H
#define TOOLBAR_H

#include <DButtonBox>
#include <DSearchEdit>
#include <DWidget>

DWIDGET_USE_NAMESPACE

class MainWindow;
class QTimer;
class QAction;

/**
 * @brief Toolbar class
 */
class Toolbar : public DWidget
{
    Q_OBJECT

public:
    /**
     * @brief Toolbar constructor
     * @param parent Parent object
     */
    explicit Toolbar(QWidget *parent = nullptr);
    // Destroctor
    ~Toolbar();

    /**
     * @brief Filters events if this object has been installed as an event filter for the watched object
     * @param obj Object being watched
     * @param event Event to be filtered
     * @return To filter the event out, return true; otherwise return false
     */
    bool eventFilter(QObject *obj, QEvent *event);
    /**
     * @brief Check if search text input is empty
     * @return Return true if search input is empty, otherwise return false
     */
    bool isSearchContentEmpty();
    /**
     * @brief Get search input's text
     * @return Return searched text if none empty, otherwise return empty string
     */
    inline QString searchContent()
    {
        if (searchEdit) {
            return searchEdit->text();
        }
        return {};
    }

public Q_SLOTS:
    /**
     * @brief Handle search process
     */
    void handleSearch();
    /**
     * @brief Handle search text changed event
     */
    void handleSearchTextChanged();
    /**
     * @brief Set search input focus
     */
    void focusInput();
    /**
     * @brief Clear search input content
     */
    inline void clearSearchText()
    {
        if (searchEdit) {
            searchEdit->clearEdit();
        }
    }

Q_SIGNALS:
    /**
     * @brief Search signal
     * @param searchContent Search content to be handled
     */
    void search(const QString &searchContent);
    /**
     * @brief ESC pressed signal
     */
    void pressEsc();
    /**
     * @brief Tab pressed signal
     */
    void pressTab();

    /**
     * @brief Procss tab button triggered signal
     */
    void procTabButtonClicked();
    /**
     * @brief Sevice tab button triggered signal
     */
    void serviceTabButtonClicked();

private:
    // Button group
    DButtonBox *m_switchFuncTabBtnGrp {};
    // Process tab button
    DButtonBoxButton *m_procBtn {};
    // Service tab button
    DButtonBoxButton *m_svcBtn {};
    // Search text input
    DSearchEdit *searchEdit {};

    // Start timer when search content changed, do the actual search only when timer timed out
    QTimer *searchTimer;

    // Searched content cache
    QString searchTextCache;
};

#endif
