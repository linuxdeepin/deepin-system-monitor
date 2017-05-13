#ifndef PROCESSITEM_H
#define PROCESSITEM_H

#include "list_item.h"
#include <proc/readproc.h>

class ProcessItem : public ListItem
{
    Q_OBJECT
    
public:
    ProcessItem();
    
    bool sameAs(ListItem *item);
    void drawBackground(QRect rect, QPainter *painter, int index, bool isSelect);
    void drawForeground(QRect rect, QPainter *painter, int column, bool isSelect);
    
    void init(QPixmap pPixmap, QString pName, int pCpu, int pMemory, int pPid);
    
    QString getName() const;
    int getCPU() const;
    int getMemory() const;
    int getPid() const;
    
private:
    int cpu;
    int iconSize;
    int memory;
    int pid;
    
    int padding;
    
    QPixmap iconPixmap;
    
    QString memoryString;
    QString name;
};

#endif
