#ifndef PROCESSITEM_H
#define PROCESSITEM_H

#include "list_item.h"
#include "network_tools.h"
#include <proc/readproc.h>

using namespace networkTools;

class ProcessItem : public ListItem
{
    Q_OBJECT
    
public:
    ProcessItem(QPixmap processIcon, QString processName, double processCpu, int processMemory, int processPid, QString processUser);
    
    bool sameAs(ListItem *item);
    void drawBackground(QRect rect, QPainter *painter, int index, bool isSelect);
    void drawForeground(QRect rect, QPainter *painter, int column, int index, bool isSelect);
    
    static bool sortByName(const ListItem *item1, const ListItem *item2, bool descendingSort);
    static bool sortByCPU(const ListItem *item1, const ListItem *item2, bool descendingSort);
    static bool sortByMemory(const ListItem *item1, const ListItem *item2, bool descendingSort);
    static bool sortByPid(const ListItem *item1, const ListItem *item2, bool descendingSort);
    static bool sortByDownload(const ListItem *item1, const ListItem *item2, bool descendingSort);
    static bool sortByUpload(const ListItem *item1, const ListItem *item2, bool descendingSort);
    
    void setNetworkStatus(networkStatus nStatus);
    
    QString getName() const;
    double getCPU() const;
    int getMemory() const;
    int getPid() const;
    networkStatus getNetworkStatus() const;
    
private:
    double cpu;
    int iconSize;
    int memory;
    int pid;
    
    int padding;
    
    networkStatus status;
    
    QPixmap iconPixmap;
    
    QString memoryString;
    QString name;
    QString path;
    QString user;
};

#endif
