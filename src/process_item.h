#ifndef PROCESSITEM_H
#define PROCESSITEM_H

#include "list_item.h"
#include <proc/readproc.h>
#include "utils.h"

using namespace Utils;

class ProcessItem : public ListItem
{
    Q_OBJECT
    
public:
    ProcessItem(QPixmap processIcon, QString processName, QString dName, double processCpu, int processMemory, int processPid, QString processUser, char processState);
    
    bool sameAs(ListItem *item);
    void drawBackground(QRect rect, QPainter *painter, int index, bool isSelect);
    void drawForeground(QRect rect, QPainter *painter, int column, int index, bool isSelect);
    
    static bool sortByName(const ListItem *item1, const ListItem *item2, bool descendingSort);
    static bool sortByCPU(const ListItem *item1, const ListItem *item2, bool descendingSort);
    static bool sortByMemory(const ListItem *item1, const ListItem *item2, bool descendingSort);
    static bool sortByPid(const ListItem *item1, const ListItem *item2, bool descendingSort);
    static bool sortByDownload(const ListItem *item1, const ListItem *item2, bool descendingSort);
    static bool sortByUpload(const ListItem *item1, const ListItem *item2, bool descendingSort);
    static bool sortByWrite(const ListItem *item1, const ListItem *item2, bool descendingSort);
    static bool sortByRead(const ListItem *item1, const ListItem *item2, bool descendingSort);
    static bool search(const ListItem *item, QString searchContent);
    
    void setNetworkStatus(NetworkStatus nStatus);
    void setDiskStatus(DiskStatus dStatus);
    
    QString getName() const;
    QString getDisplayName() const;
    double getCPU() const;
    int getMemory() const;
    int getPid() const;
    NetworkStatus getNetworkStatus() const;
    DiskStatus getDiskStatus() const;
    
private:
    double cpu;
    int iconSize;
    int memory;
    int pid;
    
    int padding;
    
    NetworkStatus networkStatus;
    DiskStatus diskStatus;
    
    QPixmap iconPixmap;
    
    QString memoryString;
    QString name;
    QString displayName;
    QString path;
    QString user;
    char state;
};

#endif
