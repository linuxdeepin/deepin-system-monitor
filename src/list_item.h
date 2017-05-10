#ifndef LISTITEM_H
#define LISTITEM_H

#include <QObject>
#include <QPainter>

class ListItem : public QObject
{
    Q_OBJECT
    
public:
    ListItem();
    
    virtual void render(QRect rect, QPainter *painter, int column, bool isSelect)=0;
    virtual void renderBackground(QRect rect, QPainter *painter, int index, bool isSelect)=0;
};

#endif
