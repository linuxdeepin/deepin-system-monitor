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
#ifndef BLOCK_DEVICE_H
#define BLOCK_DEVICE_H

#include "private/block_device_p.h"

#include <QSharedDataPointer>

namespace core {
namespace system {

class BlockDevice
{
public:
    BlockDevice();
    BlockDevice(const BlockDevice &other);
    BlockDevice &operator=(const BlockDevice &rhs);
    ~BlockDevice();

    QByteArray deviceName() const;
    QString model() const;
    qulonglong capacity() const;
    qulonglong blocksRead() const;
    qulonglong bytesRead() const;
    qreal sectorsReadPerSecond() const;
    qreal readRequestIssuedPerSecond() const;
    qreal readRequestMergedPerSecond() const;
    qreal readRequestMergedPercent() const;

    qreal percentUtilization() const;
    qulonglong transferPerSecond() const;
    qulonglong blocksWritten() const;
    qulonglong bytesWritten() const;
    qreal sectorsWrittenPerSecond() const;
    qreal writeRequestIssuedPerSecond() const;
    qreal writeRequestMergedPerSecond() const;
    qreal writeRequestMergedPercent() const;

protected:
    void readDeviceInfo();

private:
    QSharedDataPointer<BlockDevicePrivate> d;
};

inline QByteArray BlockDevice::deviceName() const
{
    return d->name;
}

inline QString BlockDevice::model() const
{
    return d->model;
}

inline qulonglong BlockDevice::capacity() const
{
    return d->capacity;
}

inline qulonglong BlockDevice::blocksRead() const
{
    return d->blk_read;
}

inline qulonglong BlockDevice::bytesRead() const
{
    return d->bytes_read;
}

inline qreal BlockDevice::sectorsReadPerSecond() const
{
    return d->rsec_ps;
}

inline qreal BlockDevice::readRequestIssuedPerSecond() const
{
    return d->r_ps;
}

inline qreal BlockDevice::readRequestMergedPerSecond() const
{
    return d->rrqm_ps;
}

inline qreal BlockDevice::readRequestMergedPercent() const
{
    return d->p_rrqm;
}

inline qreal BlockDevice::percentUtilization() const
{
    return d->p_util;
}

inline qulonglong BlockDevice::transferPerSecond() const
{
    return d->tps;
}

inline qulonglong BlockDevice::blocksWritten() const
{
    return d->blk_wrtn;
}

inline qulonglong BlockDevice::bytesWritten() const
{
    return d->bytes_wrtn;
}

inline qreal BlockDevice::sectorsWrittenPerSecond() const
{
    return d->wsec_ps;
}

inline qreal BlockDevice::writeRequestIssuedPerSecond() const
{
    return d->w_ps;
}

inline qreal BlockDevice::writeRequestMergedPerSecond() const
{
    return d->wrqm_ps;
}

inline qreal BlockDevice::writeRequestMergedPercent() const
{
    return d->p_wrqm;
}

} // namespace system
} // namespace core

#endif // BLOCK_DEVICE_H
