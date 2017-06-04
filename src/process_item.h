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
    void mergeItem(ListItem *item);
    void setDiskStatus(DiskStatus dStatus);
    void setNetworkStatus(NetworkStatus nStatus);
    
private:
    DiskStatus diskStatus;
    NetworkStatus networkStatus;
    QPixmap iconPixmap;
    QString displayName;
    QString name;
    QString path;
    QString user;
    char state;
    double cpu;
    int iconSize;
    int padding;
    int pid;
    int textPadding;
    long memory;
};

#endif
