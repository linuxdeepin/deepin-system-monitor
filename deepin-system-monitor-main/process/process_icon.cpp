// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "process_icon.h"
#include "ddlog.h"

#include "process_icon_cache.h"
#include "process.h"
#include "desktop_entry_cache.h"
#include "common/common.h"
#include "system/system_monitor.h"
#include "process/process_db.h"
#include "wm/wm_window_list.h"

#include <QFileInfo>
#include <QImage>

using namespace common::init;
using namespace common::core;
using namespace core::system;
using namespace core::process;
using namespace core::wm;

namespace core {
namespace process {

enum icon_data_type_t {
    kIconDataNameType,
    kIconDataPixmapType
};

struct icon_data_t {
    icon_data_type_t type;
    char __pad__[4];
    QString proc_name;
    bool desktopentry = false;

    virtual ~icon_data_t() {}
};

struct icon_data_name_type : public icon_data_t {
    QString icon_name;
};
struct icon_data_pix_map_type : public icon_data_t {
    QImage image;
};

ProcessIcon::ProcessIcon()
{
    qCDebug(app) << "ProcessIcon created";
}

ProcessIcon::~ProcessIcon()
{
    qCDebug(app) << "ProcessIcon destroyed";
}

void ProcessIcon::refreashProcessIcon(Process *proc)
{
    if (proc) {
        qCDebug(app) << "Refreshing process icon for pid" << proc->pid();
        ProcessIconCache *cache = ProcessIconCache::instance();
        if (cache->contains(proc->pid())) {
            auto *procIcon = cache->getProcessIcon(proc->pid());
            if (proc->name().compare(procIcon->m_data->proc_name, Qt::CaseInsensitive) == 0) {
                qCDebug(app) << "Icon cache hit for pid" << proc->pid();
                m_data = procIcon->m_data;
            } else {
                qCDebug(app) << "Icon cache hit but name mismatch, regenerating icon for pid" << proc->pid();
                m_data = getIcon(proc);
                procIcon->m_data = m_data;
            }

            if (m_data->desktopentry)
                ProcessDB::instance()->windowList()->addDesktopEntryApp(proc);
        } else {
            qCDebug(app) << "Icon cache miss for pid" << proc->pid();
            auto *procIcon = new ProcessIcon();
            m_data = getIcon(proc);
            procIcon->m_data = m_data;
            cache->addProcessIcon(proc->pid(), procIcon);
        }
    } else {
        qCWarning(app) << "proc is null in refreashProcessIcon";
    }
}

QIcon ProcessIcon::icon() const
{
    QIcon icon;
    qCDebug(app) << "Getting icon from ProcessIcon object";

    if (m_data) {
        if (m_data->type == kIconDataNameType) {
            auto *iconData = reinterpret_cast<struct icon_data_name_type *>(m_data.get());
            if (iconData) {
                qCDebug(app) << "Icon data is a name:" << iconData->icon_name;
                icon = QIcon::fromTheme(iconData->icon_name);
            }
        } else if (m_data->type == kIconDataPixmapType) {
            auto *iconData = reinterpret_cast<struct icon_data_pix_map_type *>(m_data.get());
            if (iconData) {
                qCDebug(app) << "Icon data is a pixmap";
                icon.addPixmap(QPixmap::fromImage(iconData->image));
            } // ::if(iconData)
        }
    } // ::if(m_data)

    return icon;
}

struct icon_data_t *ProcessIcon::defaultIconData(const QString &procname) const {
    // qCDebug(app) << "Creating default icon data for process:" << procname;
    auto *iconData = new struct icon_data_name_type();
    iconData->type = kIconDataNameType;
    iconData->proc_name = "[::default::]";
    iconData->icon_name = "application-x-executable";
    return iconData;
}

struct icon_data_t *ProcessIcon::terminalIconData(const QString &procname) const {
    // qCDebug(app) << "Creating terminal icon data for process:" << procname;
    auto *iconData = new struct icon_data_name_type();
    iconData->type = kIconDataNameType;
    iconData->proc_name = "[::terminal::]";
    iconData->icon_name = "terminal";
    return iconData;
}

QString ProcessIcon::getFileManagerString()
{
    qCDebug(app) << "Getting file manager string";
    DesktopEntryCache *desktopEntryCache = ProcessDB::instance()->desktopEntryCache();
    QList<QString> desktopEntryKeys = desktopEntryCache->getCache().keys();
    for (auto it = desktopEntryKeys.begin(); it != desktopEntryKeys.end(); it++) {
        if ((*it).contains("dde-file-manager") && desktopEntryCache->getCache().value((*it))->icon.contains("dde-file-manager")) {
            qCDebug(app) << "Found file manager string:" << *it;
            return (*it);
        }
    }
    qCDebug(app) << "File manager string not found";
    return "";
}

std::shared_ptr<icon_data_t> ProcessIcon::getIcon(Process *proc)
{
    qCDebug(app) << "Getting icon for process" << proc->pid() << proc->name();
    std::shared_ptr<icon_data_t> iconDataPtr;
    auto processDB = ProcessDB::instance();
    WMWindowList *windowList = processDB->windowList();
    DesktopEntryCache *desktopEntryCache = processDB->desktopEntryCache();

    if (!proc->cmdline().isEmpty()) {
        if (windowList->isTrayApp(proc->pid())) {
            qCDebug(app) << "Process is a tray app";
            if (proc->environ().contains("GIO_LAUNCHED_DESKTOP_FILE")) {
                qCDebug(app) << "Found GIO_LAUNCHED_DESKTOP_FILE";
                auto desktopFile = proc->environ()["GIO_LAUNCHED_DESKTOP_FILE"];
                auto entry = desktopEntryCache->entryWithDesktopFile(desktopFile);
                if (entry && !entry->icon.isEmpty()) {
                    qCDebug(app) << "Found icon from desktop file:" << entry->icon;
                    auto *iconData = new struct icon_data_name_type();
                    iconData->type = kIconDataNameType;
                    iconData->proc_name = proc->name();
                    iconData->icon_name = entry->icon;
                    iconDataPtr.reset(iconData);
                    return iconDataPtr;
                } else {
                    qCDebug(app) << "No icon found in desktop file entry";
                }
            }
        }

        if (windowList->isGuiApp(proc->pid())) {
            qCDebug(app) << "Process is a GUI app";
            const QImage &image = windowList->getWindowIcon(proc->pid());
            if (!image.isNull()) {
                qCDebug(app) << "Found icon from window properties";
                auto *iconData = new struct icon_data_pix_map_type();
                iconData->image = image;
                iconData->proc_name = proc->name();
                iconData->type = kIconDataPixmapType;
                iconDataPtr.reset(iconData);
                return iconDataPtr;
            } else {
                qCDebug(app) << "No icon found from window properties";
            }
        }

        if (desktopEntryCache->contains(proc->name())) {
            qCDebug(app) << "Found process name in desktop entry cache";
            DesktopEntry entry;
            entry = desktopEntryCache->entry(proc->name());
            if (entry && !entry->icon.isEmpty()) {
                qCDebug(app) << "Found icon from desktop entry cache:" << entry->icon;
                auto *iconData = new struct icon_data_name_type();
                iconData->desktopentry = true;
                iconData->type = kIconDataNameType;
                iconData->proc_name = proc->name();
                //由于文件管理器和主目录在desktopEntryCache的m_cache中的key都包含"dde-file-manager"
                //且value中的结构体里的name都是"dde-file-manager"，所以在这里对于文件管理器的图标要多做判断
                QString tempString = getFileManagerString();
                if (tempString.contains(proc->name())) {
                    qCDebug(app) << "Special handling for file manager icon";
                    iconData->icon_name = desktopEntryCache->getCache().value(tempString)->icon;
                } else {
                    iconData->icon_name = entry->icon;
                }
                iconDataPtr.reset(iconData);
                windowList->addDesktopEntryApp(proc);
                return iconDataPtr;
            } else {
                qCDebug(app) << "No icon found in desktop entry";
            }
        }

        QHash<QString, QString> allEnviron = proc->environ();
        if (allEnviron.contains("GIO_LAUNCHED_DESKTOP_FILE") && ((allEnviron.contains("XDG_DATA_DIRS") && !allEnviron["XDG_DATA_DIRS"].isEmpty())
                                                                 || (allEnviron.contains("GIO_LAUNCHED_DESKTOP_FILE_PID") && allEnviron["GIO_LAUNCHED_DESKTOP_FILE_PID"].toInt() == proc->pid()))) {
            qCDebug(app) << "Found GIO_LAUNCHED_DESKTOP_FILE in environment";
            auto desktopFile = proc->environ()["GIO_LAUNCHED_DESKTOP_FILE"];
            auto entry = desktopEntryCache->entryWithDesktopFile(desktopFile);
            if (entry && !entry->icon.isEmpty()) {
                qCDebug(app) << "Found icon from desktop file in environment:" << entry->icon;
                auto *iconData = new struct icon_data_name_type();
                iconData->desktopentry = true;
                iconData->type = kIconDataNameType;
                iconData->proc_name = proc->name();
                iconData->icon_name = entry->icon;
                iconDataPtr.reset(iconData);
                windowList->addDesktopEntryApp(proc);
                return iconDataPtr;
            } else {
                qCDebug(app) << "No icon found in desktop file from environment";
            }
        }

        if (shellList.contains(proc->name())) {
            qCDebug(app) << "Process is a shell, using terminal icon";
            iconDataPtr.reset(terminalIconData(proc->name()));
            return iconDataPtr;
        }

        if (!proc->cmdline().isEmpty() && proc->cmdline()[0].startsWith("/opt")) {
            qCDebug(app) << "Process path starts with /opt";
            QString fname = QFileInfo(QString(proc->cmdline()[0]).split(' ')[0]).fileName();
            auto entry = desktopEntryCache->entryWithSubName(fname);
            if (entry && !entry->icon.isEmpty()) {
                qCDebug(app) << "Found icon from subname in desktop entry:" << entry->icon;
                auto *iconData = new struct icon_data_name_type();
                iconData->type = kIconDataNameType;
                iconData->proc_name = proc->name();
                iconData->icon_name = entry->icon;
                iconDataPtr.reset(iconData);
                return iconDataPtr;
            } else {
                qCDebug(app) << "No icon found from subname in desktop entry";
            }
        }
    } else {
        qCDebug(app) << "Process has no cmdline";
    }

    // fallback to use default icon
    qCDebug(app) << "No icon found, falling back to default icon";
    iconDataPtr.reset(defaultIconData(proc->name()));
    return iconDataPtr;
}

} // namespace process
} // namespace core
