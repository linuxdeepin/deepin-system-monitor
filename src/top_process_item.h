#ifndef TOPPROCESSITEM_H
#define TOPPROCESSITEM_H

#include "list_item.h"
#include <proc/readproc.h>

class TopProcessItem : public ListItem
{
    Q_OBJECT
    
public:
    TopProcessItem(QPixmap processIcon, QString processName, int processMemory, int processPid);
    
    bool sameAs(ListItem *item);
    void drawBackground(QRect rect, QPainter *painter, int index, bool isSelect);
    void drawForeground(QRect rect, QPainter *painter, int column, int index, bool isSelect);
    
    static bool sortByMemory(const ListItem *item1, const ListItem *item2);
    
    QString getName() const;
    int getMemory() const;
    int getPid() const;
    QPixmap getIcon() const;
    
private:
    int iconSize;
    int memory;
    int pid;
    
    int padding;
    
    QPixmap iconPixmap;
    
    QString memoryString;
    QString name;
};

#endif
