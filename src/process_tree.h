#ifndef PROCESSTREE_H
#define PROCESSTREE_H

#include <proc/readproc.h>
#include <proc/sysinfo.h>
#include <QObject>
#include <QMap>

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
    
    void scanProcesses(storedProcType processes);
    QList<int> getAllChildPids(int pid);
    void getChildPids(int pid);
    
    void printTree();
    void printNode(int pid);
    
private:
    QMap<int, Process> *processMap;
    int rootPid;
    
    QList<int> childrenPids;
};

#endif
