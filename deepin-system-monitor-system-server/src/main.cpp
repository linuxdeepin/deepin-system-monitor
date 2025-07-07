// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QCoreApplication>

#include "systemdbusserver.h"
#include "logger.h"
#include "ddlog.h"

using namespace DDLog;

int main(int argc, char *argv[])
{
    MLogger();
    QCoreApplication a(argc, argv);
    qCDebug(app) << "QCoreApplication created";

    SystemDBusServer dbusServer;
    dbusServer.exitDBusServer(10000);

    qCDebug(app) << "Starting system server event loop...";
    return a.exec();
}
