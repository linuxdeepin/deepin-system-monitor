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

#include "process_tree.h"
#include <QDebug>

ProcessTree::ProcessTree()
{
    processMap = new QMap<int, Process>();

    rootPid = INT_MAX;
}

ProcessTree::~ProcessTree()
{
    childrenPids.clear();
    
    for (auto v : processMap->values()) {
        v.childProcesses.clear();
    }

    processMap->clear();
    delete processMap;
}

void ProcessTree::scanProcesses(StoredProcType processes)
{
    for (auto &i:processes) {
        int ppid = (&i.second)->ppid;
        int pid = (&i.second)->tid;

        if (!processMap->contains(pid)) {
            Process process;

            process.parentPid = ppid;

            (*processMap)[pid] = process;
        }

        if (!processMap->contains(ppid)) {
            Process process;

            process.parentPid = -1;
            process.childProcesses.append(pid);

            (*processMap)[ppid] = process;
        } else {
            (*processMap)[ppid].childProcesses.append(pid);
        }

        if (pid < rootPid) {
            rootPid = pid;
        }
    }
}

QList<int> ProcessTree::getAllChildPids(int pid)
{
    childrenPids.clear();

    getChildPids(pid);

    return childrenPids;
}

void ProcessTree::getChildPids(int pid)
{
    if (processMap->contains(pid)) {
        for (int childPid : ((*processMap)[pid].childProcesses)) {
            childrenPids.append(childPid);
            
            getChildPids(childPid);
        }
    }
}

void ProcessTree::printNode(int pid)
{
    qDebug() << "* Node " << pid;

    if (processMap->contains(pid)) {
        qDebug() << "### Child " << (*processMap)[pid].childProcesses;

        for (int childPid : ((*processMap)[pid].childProcesses)) {
            printNode(childPid);
        }
    }
}

void ProcessTree::printTree()
{
    printNode(rootPid);
}

