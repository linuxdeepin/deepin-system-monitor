#ifndef PROCESSITEM_H
#define PROCESSITEM_H

#include "list_item.h"
#include <proc/readproc.h>

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
    
    QString getName() const;
    double getCPU() const;
    int getMemory() const;
    int getPid() const;
    
private:
    double cpu;
    int iconSize;
    int memory;
    int pid;
    
    int padding;
    
    QPixmap iconPixmap;
    
    QString memoryString;
    QString name;
    QString path;
    QString user;
};

#endif
