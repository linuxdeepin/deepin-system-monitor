#ifndef PROCESSTREE_H
#define PROCESSTREE_H

#include <QMap>
#include <QObject>
#include <proc/readproc.h>
#include <proc/sysinfo.h>

class ProcessTree : public QObject
{
    Q_OBJECT
    
    typedef std::map<int, proc_t> storedProcType;
    
    struct Process
    {
        int parentPid;
        QList<int> *childProcesses;
    };
    
public:
    ProcessTree();
    ~ProcessTree();
    
    QList<int> getAllChildPids(int pid);
    void getChildPids(int pid);
    void printNode(int pid);
    void printTree();
    void scanProcesses(storedProcType processes);
    
private:
    QList<int> childrenPids;
    QMap<int, Process> *processMap;
    int rootPid;
};

#endif
