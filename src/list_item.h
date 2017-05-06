#ifndef LISTITEM_H
#define LISTITEM_H

#include <QObject>
#include <QPainter>

class ListItem : public QObject
{
    Q_OBJECT
    
public:
    ListItem();
    
    virtual void render(int column, QRect rect, QPainter *painter)=0;
    virtual void renderBackground(int index, QRect rect, QPainter *painter)=0;
    virtual void renderSelection(QRect rect, QPainter *painter)=0;
};

#endif
