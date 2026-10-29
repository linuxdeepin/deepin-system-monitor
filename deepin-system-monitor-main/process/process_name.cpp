// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "process_name.h"
#include "ddlog.h"
#include "process.h"
#include "process_db.h"
#include "process_name_cache.h"
#include "desktop_entry_cache.h"
#include "common/common.h"
#include "common/thread_manager.h"
#include "system/system_monitor_thread.h"
#include "system/system_monitor.h"
#include "wm/wm_window_list.h"

#include <QFileInfo>
#include <QApplication>
#include <QUrl>

using namespace common::init;
using namespace common::core;
using namespace core::wm;
using namespace core::system;
using namespace core::process;

namespace core {
namespace process {

ProcessName::ProcessName()
{

}

void ProcessName::refreashProcessName(Process *proc)
{
    if (proc) {
        qCDebug(app) << "Refreshing process name for pid" << proc->pid();
        proc->setName(ProcessName::normalizeProcessName(proc->name(), proc->cmdline()));
        m_name = proc->name();
        // display name
        m_displayName = getDisplayName(proc);
    }
}

QString ProcessName::normalizeProcessName(const QString &source, const QByteArrayList &cmdline)
{
    qCDebug(app) << "Normalizing process name" << source << "with cmdline size" << cmdline.size();
    if (cmdline.size() == 0 || source.length() < 15) {
        return source;
    }

    // windows apps case
    if (cmdline[0].length() > 3 && isalpha(cmdline[0].at(0)) && cmdline[0].at(1) == ':' && (cmdline[0].at(2) == '/' || cmdline[0].at(2) == '\\')) {
        qCDebug(app) << "Windows app case detected";
        return QFileInfo(cmdline[0]).fileName();
    }

    for (auto cmd : cmdline) {
        QString name = {basename(cmd)};
        if (name.startsWith(source.toLocal8Bit())) {
            qCDebug(app) << "Found matching name in cmdline:" << name;
            return name;
        }
    }

    return source;
}

QString ProcessName::getDisplayName(Process *proc)
{
    qCDebug(app) << "Getting display name for pid" << proc->pid() << "name" << proc->name();
    auto processDB = ProcessDB::instance();

    WMWindowList *windowList = processDB->windowList();
    DesktopEntryCache *desktopEntryCache = processDB->desktopEntryCache();

#ifdef BUILD_WAYLAND
    Q_UNUSED(desktopEntryCache);
#else
    if (!proc->cmdline().isEmpty()) {
        if (windowList->isTrayApp(proc->pid())) {
            qCDebug(app) << "Process is a tray app";
            // process with tray window
            auto title = windowList->getWindowTitle(proc->pid());

            if (!title.isEmpty()) {
                qCDebug(app) << "Found window title for tray app:" << title;
                return QString("%1: %2").arg(QApplication::translate("Process.Table", "Tray")).arg(title);

            } else if (proc->environ().contains("GIO_LAUNCHED_DESKTOP_FILE")) {
                qCDebug(app) << "Found GIO_LAUNCHED_DESKTOP_FILE for tray app";
                // can't grab window title, try use desktop file instead
                auto desktopFile = proc->environ()["GIO_LAUNCHED_DESKTOP_FILE"];
                auto entry = desktopEntryCache->entryWithDesktopFile(desktopFile);
                if (entry && !entry->displayName.isEmpty()) {
                    qCDebug(app) << "Found display name from desktop file:" << entry->displayName;
                    return QString("%1: %2").arg(QApplication::translate("Process.Table", "Tray")).arg(entry->displayName);
                }

            } else {
                qCDebug(app) << "Falling back to process name for tray app";
                return QString("%1: %2").arg(QApplication::translate("Process.Table", "Tray")).arg(proc->name());
            }
        } // ::if(traysAppsCache)

        if (windowList->isGuiApp(proc->pid())) {
            qCDebug(app) << "Process is a GUI app";
            // gui apps case, grab title with xcb call
            auto title = windowList->getWindowTitle(proc->pid());

            if (!title.isEmpty()) {
                qCDebug(app) << "Found window title for GUI app:" << title;
                if (proc->cmdline().size() > 1) {
                    auto url = QUrl(proc->cmdline()[proc->cmdline().size() - 1]);
                    auto finfo = QFileInfo(url.fileName());
                    auto rname = url.fileName();
                    rname.chop(finfo.completeSuffix().length() + 1);

                    // check if commandline ends with file name, if so prepend it before process name
                    if (url.isValid()
                            && (url.isLocalFile() || !url.host().isEmpty())
                            && !title.contains(rname)) {
                        qCDebug(app) << "Prepending filename to title:" << rname;
                        return QString("%1 - %2").arg(rname).arg(title);

                    } else {
                        return title;
                    }
                } // ::if(cmdline)

                return title;
            } // ::if(title)
        } // ::if(guiAppsCache)

        if (desktopEntryCache->entry(proc->name())) {
            qCDebug(app) << "Found desktop entry for process";
            // found desktop entry
            auto &entry = desktopEntryCache->entry(proc->name());
            if (!entry->startup_wm_class.isEmpty()) {
                qCDebug(app) << "Using StartupWMClass as display name:" << entry->startup_wm_class;
                return entry->startup_wm_class;
            }
            if (!entry->displayName.isEmpty()) {
                qCDebug(app) << "Using display name from desktop entry:" << entry->displayName;
                if (proc->cmdline().size() > 1) {
                    // check if last arg of cmdline is url, if so take it's filename
                    auto url = QUrl(proc->cmdline()[proc->cmdline().size() - 1]);
                    if (url.isValid() && (url.isLocalFile() || !url.host().isEmpty())) {
                        qCDebug(app) << "Prepending filename from URL to display name:" << url.fileName();
                        return QString("%1 - %2").arg(url.fileName()).arg(entry->displayName);
                    }
                }

                return entry->displayName;
            } // ::if(displayName)
        } // ::if(desktopEntryCache)

        // is shell?
        if (shellList.contains(proc->name())) {
            auto joined = proc->cmdline().join(' ');
            qCDebug(app) << "Process is a shell, using cmdline as display name:" << joined;
            return QString(joined);
        }

        if (proc->environ().contains("GIO_LAUNCHED_DESKTOP_FILE") && proc->environ().contains("GIO_LAUNCHED_DESKTOP_FILE_PID") && proc->environ()["GIO_LAUNCHED_DESKTOP_FILE_PID"].toInt() == proc->pid()) {
            qCDebug(app) << "Found GIO_LAUNCHED_DESKTOP_FILE in environment";
            // has gio info set in environment
            auto desktopFile = proc->environ()["GIO_LAUNCHED_DESKTOP_FILE"];
            auto entry = desktopEntryCache->entryWithDesktopFile(desktopFile);
            if (entry && !entry->displayName.isEmpty()) {
                qCDebug(app) << "Found display name from desktop file:" << entry->displayName;
                return entry->displayName;
            }
        }

        if (proc->cmdline()[0].startsWith("/opt")) {
            qCDebug(app) << "Process path starts with /opt";
            QString fname = QFileInfo(QString(proc->cmdline()[0]).split(' ')[0]).fileName();
            auto entry = desktopEntryCache->entryWithSubName(fname);
            if (entry && !entry->displayName.isEmpty()) {
                qCDebug(app) << "Found display name from sub-name entry:" << entry->displayName;
                return entry->displayName;
            }
        }
    } // ::if(cmdline)
#endif

    qCDebug(app) << "Falling back to process name:" << proc->name();
    return proc->name();
}

} // namespace process
} // namespace core
