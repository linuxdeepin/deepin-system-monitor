// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "tray_apps_cache.h"
#include "ddlog.h"

#include "wm/wm_info.h"
#include "wm/wm_window_list.h"

using namespace core::wm;

namespace core {
namespace process {

TrayAppsCache::TrayAppsCache()
{
    qCDebug(app) << "TrayAppsCache object created";
}

void TrayAppsCache::updateCache(const QList<pid_t> &cache)
{
    qCDebug(app) << "Updating tray apps cache with" << cache.size() << "pids";
    m_cache = cache;
}

} // namespace process
} // namespace core
