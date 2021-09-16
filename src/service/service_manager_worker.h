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
#ifndef SERVICE_MANAGER_WORKER_H
#define SERVICE_MANAGER_WORKER_H

#include <QObject>
#include <QList>

class SystemServiceEntry;

class ServiceManagerWorker : public QObject
{
    Q_OBJECT
public:
    explicit ServiceManagerWorker(QObject *parent = nullptr);

Q_SIGNALS:
    void resultReady(const QList<SystemServiceEntry> list);

public Q_SLOTS:
    void startJob();

private:
    inline static QString readUnitDescriptionFromUnitFile(const QString &path);
};

#endif // SERVICE_MANAGER_WORKER_H
