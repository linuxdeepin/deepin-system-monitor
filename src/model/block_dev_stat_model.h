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
#ifndef BLOCK_DEV_STAT_MODEL_H
#define BLOCK_DEV_STAT_MODEL_H

#include <QObject>

/**
 * @brief Block device performance stat model
 */
class BlockDevStatModel : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Model constructor
     * @param parent Parent object
     */
    explicit BlockDevStatModel(QObject *parent = nullptr);

signals:

public slots:
};

#endif // BLOCK_DEV_STAT_MODEL_H
