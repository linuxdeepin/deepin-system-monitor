// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QLibrary>
#include <QDir>
#include <QLibraryInfo>
#include <QJsonDocument>

#include "eventlogutils.h"
#include "ddlog.h"

using namespace DDLog;

EventLogUtils *EventLogUtils::m_instance(nullptr);

EventLogUtils &EventLogUtils::get()
{
    qCDebug(app) << "EventLogUtils get";
    if (m_instance == nullptr) {
        qCDebug(app) << "new EventLogUtils";
        m_instance = new EventLogUtils;
    }
    return *m_instance;
}

EventLogUtils::EventLogUtils()
{
    qCDebug(app) << "EventLogUtils constructor";
    QLibrary library("libdeepin-event-log.so");

    init =reinterpret_cast<bool (*)(const std::string &, bool)>(library.resolve("Initialize"));
    writeEventLog = reinterpret_cast<void (*)(const std::string &)>(library.resolve("WriteEventLog"));

    if (init == nullptr) {
        qCWarning(app) << "init is nullptr";
        return;
    }

    init("deepin-system-monitor", true);
}

void EventLogUtils::writeLogs(QJsonObject &data)
{
    qCDebug(app) << "EventLogUtils writeLogs";
    if (writeEventLog == nullptr) {
        qCWarning(app) << "writeEventLog is nullptr";
        return;
    }

    //std::string str = QJsonDocument(data).toJson(QJsonDocument::Compact).toStdString();
    writeEventLog(QJsonDocument(data).toJson(QJsonDocument::Compact).toStdString());
}
