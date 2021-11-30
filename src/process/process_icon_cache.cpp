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
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "process_icon_cache.h"
#include <DApplication>
#include <DGuiApplicationHelper>
#include <DPlatformTheme>
#include <QPointer>
DWIDGET_USE_NAMESPACE

namespace core {
namespace process {

ProcessIconCache *ProcessIconCache::m_instance = nullptr;

ProcessIconCache::ProcessIconCache(QObject *parent)
    : QObject(parent)
{
    // 由于之前获取dapplication::themetypechanged改变信号在有些平台获取不到，现通过获取DPlatformtheme方式获取
    static QPointer<DPlatformTheme> theme;

    if (!theme) {
        theme = DGuiApplicationHelper::instance()->applicationTheme();
        connect(theme, &DPlatformTheme::iconThemeNameChanged, this, [=]() {
            iconPixmapCache.clear();
        });
    }
}

} // namespace process
} // namespace core
