// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "process_icon_cache.h"
#include "ddlog.h"
#include <DApplication>
#include <DGuiApplicationHelper>
#include <DPlatformTheme>
#include <QPointer>

DWIDGET_USE_NAMESPACE
using namespace DDLog;

namespace core {
namespace process {

ProcessIconCache *ProcessIconCache::m_instance = nullptr;

ProcessIconCache::ProcessIconCache(QObject *parent)
    : QObject(parent)
{
    qCDebug(app) << "ProcessIconCache object created";
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, [=]() {
        qCDebug(app) << "Icon theme changed, clearing pixmap cache";
        iconPixmapCache.clear();
    });
}

} // namespace process
} // namespace core
