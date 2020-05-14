/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2018 Deepin, Inc.
 *               2011 ~ 2018 Wang Yong
 *
 * Author:     Wang Yong <wangyong@deepin.com>
 * Maintainer: Wang Yong <wangyong@deepin.com>
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

#ifndef TOOLBAR_H
#define TOOLBAR_H

#include <DButtonBox>
#include <DSearchEdit>
#include <DWidget>

//#include "main_window.h"

DWIDGET_USE_NAMESPACE

class MainWindow;
class QTimer;
class QAction;

class Toolbar : public DWidget
{
    Q_OBJECT

public:
    Toolbar(MainWindow *m, QWidget *parent = nullptr);
    ~Toolbar();

    bool eventFilter(QObject *, QEvent *event);
    bool isSearchContentEmpty();
    inline QString searchContent()
    {
        if (searchEdit) {
            return searchEdit->text();
        }
        return {};
    }

public Q_SLOTS:
    void handleSearch();
    void handleSearchTextChanged();
    void focusInput();
    inline void clearSearchText()
    {
        if (searchEdit) {
            searchEdit->clearEdit();
        }
    }

Q_SIGNALS:
    void search(const QString &searchContent);
    void pressEsc();
    void pressTab();

    void procTabButtonClicked();
    void serviceTabButtonClicked();

private:
    MainWindow *m_mainWindow;

    DButtonBox *m_switchFuncTabBtnGrp;
    DButtonBoxButton *m_procBtn {nullptr};
    DButtonBoxButton *m_svcBtn {nullptr};
    DSearchEdit *searchEdit {nullptr};

    QTimer *searchTimer;

    QString searchTextCache;
};

#endif
