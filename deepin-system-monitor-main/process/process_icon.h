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
#ifndef PROCESS_ICON_H
#define PROCESS_ICON_H

#include <QIcon>

#include <memory>

namespace core {
namespace process {

class Process;
struct icon_data_t;

class ProcessIcon
{
public:
    explicit ProcessIcon();
    ~ProcessIcon();

    QIcon icon() const;
    void refreashProcessIcon(Process *proc);

private:
    std::shared_ptr<struct icon_data_t> getIcon(Process *proc);
    struct icon_data_t *defaultIconData(const QString &procname) const;
    struct icon_data_t *terminalIconData(const QString &procname) const;

private:
    std::shared_ptr<struct icon_data_t> m_data;
};

} // namespace process
} // namespace core

#endif // PROCESS_ICON_H
