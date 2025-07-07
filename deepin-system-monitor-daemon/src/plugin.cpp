// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "systemmonitorservice.h"
#include "ddlog.h"

using namespace DDLog;

static SystemMonitorService *monitorService = nullptr;

extern "C" int DSMRegister(const char *name, void *data)
{
    // qCDebug(app) << "DSMRegister with name:" << name;
    (void)data;
    monitorService = new SystemMonitorService(name);
    // qCDebug(app) << "SystemMonitorService created.";
    return 0;
}

extern "C" int DSMUnRegister(const char *name, void *data)
{
    // qCDebug(app) << "DSMUnRegister with name:" << name;
    (void)name;
    (void)data;
    monitorService->deleteLater();
    // qCDebug(app) << "SystemMonitorService will be deleted later.";
    monitorService = nullptr;
    return 0;
}
