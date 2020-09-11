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
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "toolbar.h"

#include "application.h"
#include "main_window.h"
#include "constant.h"
#include "utils.h"

#include <DButtonBox>
#include <DSearchEdit>

#include <QDebug>
#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QTimer>

using namespace Utils;

Toolbar::Toolbar(QWidget *parent)
    : DWidget(parent)
{
    installEventFilter(this);  // add event filter
    setMouseTracking(true);    // make MouseMove can response

    // =========layout=========
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // =========tab button=========
    m_switchFuncTabBtnGrp = new DButtonBox(this);
    m_switchFuncTabBtnGrp->setFixedWidth(240);
    m_procBtn = new DButtonBoxButton(
        DApplication::translate("Title.Bar.Switch", "Processes"), m_switchFuncTabBtnGrp);
    m_procBtn->setCheckable(true);
    m_procBtn->setChecked(true);
    m_procBtn->setEnabled(false);
    m_procBtn->setFocusPolicy(Qt::TabFocus);
    m_svcBtn = new DButtonBoxButton(
        DApplication::translate("Title.Bar.Switch", "Services"), m_switchFuncTabBtnGrp);
    m_svcBtn->setCheckable(true);
    m_svcBtn->setEnabled(false);
    m_svcBtn->setFocusPolicy(Qt::TabFocus);
    QList<DButtonBoxButton *> list;
    list << m_procBtn << m_svcBtn;
    m_switchFuncTabBtnGrp->setButtonList(list, true);

    setFocusProxy(m_procBtn);

    m_procBtn->installEventFilter(this);
    m_svcBtn->installEventFilter(this);

    connect(m_procBtn, &DButtonBoxButton::clicked, this, [ = ]() { Q_EMIT procTabButtonClicked(); });
    connect(m_svcBtn, &DButtonBoxButton::clicked, this, [ = ]() { Q_EMIT serviceTabButtonClicked(); });

    // =========search=========
    searchEdit = new DSearchEdit(this);
    searchEdit->setFixedWidth(360);
    searchEdit->setPlaceHolder(DApplication::translate("Title.Bar.Search", "Search"));
    searchEdit->setEnabled(false);

    layout->addWidget(m_switchFuncTabBtnGrp, 0, Qt::AlignLeft);
    layout->addStretch();
    layout->addWidget(searchEdit, 0, Qt::AlignHCenter);
    layout->addStretch();

    searchTimer = new QTimer(this);
    searchTimer->setSingleShot(true);
    connect(searchTimer, &QTimer::timeout, this, &Toolbar::handleSearch);

    connect(searchEdit, &DSearchEdit::textChanged, this, &Toolbar::handleSearchTextChanged);

    auto *mw = gApp->mainWindow();
    connect(mw, &MainWindow::loadingStatusChanged, this, [=](bool loading) {
        if (loading) {
            m_procBtn->setEnabled(false);
            m_svcBtn->setEnabled(false);

            searchEdit->setEnabled(false);
        } else {
            m_procBtn->setEnabled(true);
            m_svcBtn->setEnabled(true);

            searchEdit->setEnabled(true);
        }
    });
}

Toolbar::~Toolbar() {}

bool Toolbar::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        if (obj == this) {
            QKeyEvent *keyEvent = dynamic_cast<QKeyEvent *>(event);
            if (keyEvent->key() == Qt::Key_Escape) {
                searchEdit->clearEdit();
                pressEsc();
            }
        } else if (obj == searchEdit) {
            QKeyEvent *keyEvent = dynamic_cast<QKeyEvent *>(event);
            if (keyEvent->key() == Qt::Key_Tab) {
                pressTab();
            }
        } else if (obj == m_procBtn) {
            auto *kev = dynamic_cast<QKeyEvent *>(event);
            if (kev->key() == Qt::Key_Right) {
                m_svcBtn->setFocus();
                return true;
            }
        } else if (obj == m_svcBtn) {
            auto *kev = dynamic_cast<QKeyEvent *>(event);
            if (kev->key() == Qt::Key_Left) {
                m_procBtn->setFocus();
                return true;
            }
        }
    }

    return DWidget::eventFilter(obj, event);
}

void Toolbar::handleSearch()
{
    if (searchEdit->text() == searchTextCache) {
        search(searchTextCache);
    }
}

void Toolbar::handleSearchTextChanged()
{
    searchTextCache = searchEdit->text();

    if (searchTimer->isActive()) {
        searchTimer->stop();
    }
    searchTimer->start(50);
}

void Toolbar::focusInput()
{
    if (searchEdit->text() != "") {
        searchEdit->lineEdit()->selectAll();
        searchEdit->lineEdit()->setFocus();
    } else {
        searchEdit->lineEdit()->setFocus();
    }
}

bool Toolbar::isSearchContentEmpty()
{
    if (searchEdit->text().isEmpty()) {
        return false;
    }
    return true;
}
