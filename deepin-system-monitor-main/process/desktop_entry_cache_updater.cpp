// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "desktop_entry_cache_updater.h"
#include "ddlog.h"

#include <DDesktopEntry>
#include <QDebug>
#include <QDir>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>

DCORE_USE_NAMESPACE
using namespace DDLog;

namespace core {
namespace process {

DesktopEntryCacheUpdater::DesktopEntryCacheUpdater(QObject *parent)
    : QObject(parent)
{
}

DesktopEntry DesktopEntryCacheUpdater::createEntry(const QFileInfo &fileInfo)
{
    qCDebug(app) << "Creating desktop entry for file:" << fileInfo.filePath();
    if (!fileInfo.exists()) {
        qCDebug(app) << "File does not exist:" << fileInfo.filePath();
        return {};
    }

    DDesktopEntry dde(fileInfo.filePath());
    DesktopEntry entry(new struct desktop_entry_t());

    // display name
    entry->displayName = dde.ddeDisplayName();
    // icon name
    auto icon = dde.stringValue("Icon");
    if (!icon.isEmpty()) {
        qCDebug(app) << "Icon found:" << icon;
        entry->icon = icon;
    }

    // exec & name
    auto tryExec = dde.stringValue("TryExec");
    auto execStr = dde.stringValue("Exec");
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QStringList execList = execStr.split("exec", QString::SkipEmptyParts, Qt::CaseInsensitive);
#else
    QStringList execList = execStr.split("exec", Qt::SkipEmptyParts, Qt::CaseInsensitive);
#endif
    QString exec = execList.size() > 1 ? execList.last().trimmed() : execStr.trimmed();
    exec = exec.remove("\"");
    if (tryExec == "/usr/bin/ll-cli") {
        qCDebug(app) << "Found linglong app";
        auto linglongStr = dde.stringValue("X-linglong");
        if (!linglongStr.isEmpty()) {
            qCDebug(app) << "linglong string:" << linglongStr;
            // Use QProcess to get application info
            QProcess process;
            process.start("/usr/bin/ll-cli", QStringList() << "info" << linglongStr);
            process.waitForFinished(5000);
            QString output = QString::fromUtf8(process.readAllStandardOutput());

            // Parse JSON output to get name field
            QJsonDocument jsonDoc = QJsonDocument::fromJson(output.toUtf8());
            if (!jsonDoc.isNull() && jsonDoc.isObject()) {
                qCDebug(app) << "Parsing linglong info json";
                QJsonObject jsonObj = jsonDoc.object();
                if (jsonObj.contains("name")) {
                    entry->name = jsonObj["name"].toString();
                    qCDebug(app) << "Found name in linglong json:" << entry->name;
                }
            }

            if (!exec.isEmpty()) {
                qCDebug(app) << "Exec is not empty, use exec";
                entry->exec = exec.split(' ');
            } else {
                qCDebug(app) << "Exec is empty, use tryExec";
                entry->exec = tryExec.split(' ');
            }
        }
    } else if (!tryExec.isEmpty()) {
        qCDebug(app) << "Using TryExec:" << tryExec;
        entry->exec = tryExec.split(' ');
        entry->name = QFileInfo(entry->exec[0]).baseName();
    } else if (!exec.isEmpty()) {
        qCDebug(app) << "Using Exec:" << exec;
        entry->exec = exec.split(' ');
        entry->name = QFileInfo(entry->exec[0]).baseName();
    } else {
        qCDebug(app) << "Using dde.name():" << dde.name();
        entry->name = dde.name().toLower();
    }
    // dde-file-manager plugins
    if (execStr.contains("dde-file-manager") && execStr.contains("plugin")) {
        qCDebug(app) << "Found dde-file-manager plugin";
        entry->name = fileInfo.completeBaseName();
    }
    if (icon.contains("dde-file-manager")) {
        qCDebug(app) << "Icon contains dde-file-manager";
        if (dde.stringValue("GenericName").trimmed() != "File Manager") {
            qCDebug(app) << "Not a file manager, skipping";
            return {};
        }
        entry->exec.clear();
        entry->exec.append("dde-file-manager");
        entry->name = "dde-file-manager";
    }

    // startup wm class & name
    auto wmclass = dde.stringValue("StartupWMClass").toLower();
    if (!wmclass.isEmpty()) {
        qCDebug(app) << "Found StartupWMClass:" << wmclass;
        entry->startup_wm_class = wmclass;
        entry->name = wmclass;
    }

    qCDebug(app) << "Desktop entry created:" << entry->name;
    return entry;
}

} // namespace process
} // namespace core
