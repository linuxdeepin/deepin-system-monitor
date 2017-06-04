#include "process_tree.h"
#include <QDebug>

ProcessTree::ProcessTree()
{
    processMap = new QMap<int, Process>();

    rootPid = INT_MAX;
}

ProcessTree::~ProcessTree()
{
    delete processMap;
}

void ProcessTree::scanProcesses(storedProcType processes)
{
    for(auto &i:processes) {
        int ppid = (&i.second)->ppid;
        int pid = (&i.second)->tid;

        if (!processMap->contains(pid)) {
            Process process;

            QList<int> *processList = new QList<int>();

            process.parentPid = ppid;
            process.childProcesses = processList;

            (*processMap)[pid] = process;
        }

        if (!processMap->contains(ppid)) {
            Process process;

            QList<int> *processList = new QList<int>();
            processList->append(pid);

            process.parentPid = -1;
            process.childProcesses = processList;

            (*processMap)[ppid] = process;
        } else {
            (*processMap)[ppid].childProcesses->append(pid);
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
        for (int childPid : *((*processMap)[pid].childProcesses)) {
            childrenPids.append(childPid);
            
            getChildPids(childPid);
        }
    }
}

void ProcessTree::printNode(int pid)
{
    qDebug() << "* Node " << pid;

    if (processMap->contains(pid)) {
        qDebug() << "### Child " << *(*processMap)[pid].childProcesses;

        for (int childPid : *((*processMap)[pid].childProcesses)) {
            printNode(childPid);
        }
    }
}

void ProcessTree::printTree()
{
    printNode(rootPid);
}

