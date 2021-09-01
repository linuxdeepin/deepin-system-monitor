/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      yukuan <yukuan@uniontech.com>
* Maintainer:  yukuan <yukuan@uniontech.com>
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
#ifndef MEM_H
#define MEM_H

#include <QSharedDataPointer>

namespace core {
namespace system {

class MemInfoPrivate;
class MemInfo
{
public:
    explicit MemInfo();
    MemInfo(const MemInfo &other);
    MemInfo &operator=(const MemInfo &rhs);
    ~MemInfo();

    qulonglong memTotal() const;
    qulonglong memAvailable() const;
    qulonglong buffers() const;
    qulonglong cached() const;
    qulonglong active() const;
    qulonglong inactive() const;

    qulonglong swapTotal() const;
    qulonglong swapFree() const;
    qulonglong swapCached() const;
    qulonglong shmem() const;
    qulonglong slab() const;
    qulonglong dirty() const;
    qulonglong mapped() const;

    void readMemInfo();

private:
    QSharedDataPointer<MemInfoPrivate> d;
};

} // namespace system
} // namespace core

#endif // MEM_H
