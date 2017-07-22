/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2017 Deepin, Inc.
 *               2011 ~ 2017 Wang Yong
 *
 * Author:     Wang Yong <wangyong@deepin.com>
 * Maintainer: Wang Yong <wangyong@deepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */ 

#ifndef PROCESSITEM_H
#define PROCESSITEM_H

#include "list_item.h"
#include "utils.h"
#include <proc/readproc.h>

using namespace Utils;

class ProcessItem : public ListItem
{
    Q_OBJECT
    
public:
    ProcessItem(QPixmap processIcon, QString processName, QString dName, double processCpu, long processMemory, int processPid, QString processUser, char processState);
    
    bool sameAs(ListItem *item);
    void drawBackground(QRect rect, QPainter *painter, int index, bool isSelect);
    void drawForeground(QRect rect, QPainter *painter, int column, int index, bool isSelect);
    
    static bool search(const ListItem *item, QString searchContent);
    static bool sortByCPU(const ListItem *item1, const ListItem *item2, bool descendingSort);
    static bool sortByDiskRead(const ListItem *item1, const ListItem *item2, bool descendingSort);
    static bool sortByDiskWrite(const ListItem *item1, const ListItem *item2, bool descendingSort);
    static bool sortByMemory(const ListItem *item1, const ListItem *item2, bool descendingSort);
    static bool sortByName(const ListItem *item1, const ListItem *item2, bool descendingSort);
    static bool sortByNetworkDownload(const ListItem *item1, const ListItem *item2, bool descendingSort);
    static bool sortByNetworkUpload(const ListItem *item1, const ListItem *item2, bool descendingSort);
    static bool sortByPid(const ListItem *item1, const ListItem *item2, bool descendingSort);
    
    DiskStatus getDiskStatus() const;
    NetworkStatus getNetworkStatus() const;
    QString getDisplayName() const;
    QString getName() const;
    QString getUser() const;
    double getCPU() const;
    int getPid() const;
    long getMemory() const;
    bool isNameDisplayComplete();
    void mergeItemInfo(double childCpu, long childMemory, DiskStatus childDiskStatus, NetworkStatus childNetworkStatus);
    void setDiskStatus(DiskStatus dStatus);
    void setNetworkStatus(NetworkStatus nStatus);
    
public slots:
    void changeTheme(QString theme);
    void initTheme();
    
private:
    DiskStatus diskStatus;
    NetworkStatus networkStatus;
    QPixmap iconPixmap;
    QString displayName;
    QString evenLineColor;
    QString name;
    QString oddLineColor;
    QString path;
    QString selectLineColor;
    QString textColor;
    QString user;
    bool displayNameComplete;
    char state;
    double cpu;
    double evenLineOpacity;
    double oddLineOpacity;
    double selectOpacity;
    double textLeftOpacity;
    double textRightOpacity;
    int iconSize;
    int padding;
    int pid;
    int textPadding;
    long memory;
};

#endif
