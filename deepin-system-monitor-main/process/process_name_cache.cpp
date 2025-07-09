// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "process_name_cache.h"
#include "ddlog.h"

using namespace DDLog;

namespace core {
namespace process {

ProcessNameCache *ProcessNameCache::m_instance = nullptr;

ProcessNameCache::ProcessNameCache(QObject *parent)
    : QObject(parent)
{
    // qCDebug(app) << "ProcessNameCache created";
}

} // namespace process
} // namespace core
