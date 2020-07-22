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
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SYSTEM_SERVICE_TABLE_VIEW_H
#define SYSTEM_SERVICE_TABLE_VIEW_H

#include "base_table_view.h"

#include <DLabel>
#include <DSpinner>

DWIDGET_USE_NAMESPACE

class SystemServiceSortFilterProxyModel;
class SystemServiceTableModel;
class SystemServiceEntry;
class MainWindow;
class ServiceManager;
class ErrorContext;
class QShortcut;

class SystemServiceTableView : public BaseTableView
{
    Q_OBJECT

public:
    SystemServiceTableView(DWidget *parent = nullptr);
    ~SystemServiceTableView() override;

    inline SystemServiceTableModel *getSourceModel() const
    {
        return m_model;
    }

    bool eventFilter(QObject *obj, QEvent *event) override;

Q_SIGNALS:
    void sectionVisibilityChanged(int, bool);

public Q_SLOTS:
    void startService();
    void stopService();
    void restartService();
    void setServiceStartupMode(bool autoStart);
    void refresh();

protected Q_SLOTS:
    virtual void search(const QString &pattern);
    void displayHeaderContextMenu(const QPoint &);
    void displayTableContextMenu(const QPoint &);
    virtual bool loadSettings();
    virtual void saveSettings();

protected:
    int sizeHintForColumn(int) const override;
    void resizeEvent(QResizeEvent *event) override;
    void selectionChanged(const QItemSelection &selected,
                          const QItemSelection &deselected) override;

private:
    void initUI(bool settingsLoaded = false);
    void initConnections();
    void handleTaskError(const ErrorContext &ec) const;
    void adjustInfoLabelVisibility();
    inline void refreshServiceStatus(const QString sname);

private:
    SystemServiceTableModel *m_model                {};
    SystemServiceSortFilterProxyModel *m_proxyModel {};

    // selected service name
    QVariant m_selectedSName  {};

    DMenu *m_contextMenu                {};
    DMenu *m_headerContextMenu          {};
    DLabel *m_noMatchingResultLabel     {};
    DSpinner *m_spinner                 {};

    QAction *m_startServiceAction           {};
    QAction *m_stopServiceAction            {};
    QAction *m_restartServiceAction         {};
    QMenu *m_setServiceStartupModeMenu      {};
    QAction *m_setAutoStartAction           {};
    QAction *m_setManualStartAction         {};
    QAction *m_refreshAction                {};
    QAction *m_loadStateHeaderAction        {};
    QAction *m_activeStateHeaderAction      {};
    QAction *m_subStateHeaderAction         {};
    QAction *m_stateHeaderAction            {};
    QAction *m_descriptionHeaderAction      {};
    QAction *m_pidHeaderAction              {};
    QAction *m_startupModeHeaderAction      {};

    QShortcut *m_refreshKP      {};
    QShortcut *m_startKP        {};
    QShortcut *m_stopKP         {};
    QShortcut *m_restartKP      {};

    bool m_loading {false};
};

#endif  // SYSTEM_SERVICE_TABLE_VIEW_H
