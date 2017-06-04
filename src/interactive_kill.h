#ifndef INTERACTIVEKILL_H
#define INTERACTIVEKILL_H

#include <QWidget>
#include <QPaintEvent>
#include <QKeyEvent>
#include "start_tooltip.h"
#include "window_manager.h"

// #undef Bool

class InteractiveKill : public QWidget
{
    Q_OBJECT
 
public:
    InteractiveKill(QWidget *parent = 0);
    ~InteractiveKill();
    
    void mousePressEvent(QMouseEvent *mouseEvent);
    void mouseMoveEvent(QMouseEvent *mouseEvent);
    void keyPressEvent(QKeyEvent *keyEvent);
    void paintEvent(QPaintEvent *);
    
signals:
    void killWindow(int pid);
    
private:
    StartTooltip* startTooltip;
    WindowManager* windowManager;
    
    QImage cursorImage;
    int cursorX;
    int cursorY;
    
    QList<WindowRect> windowRects;
    QList<int> windowPids;
    QPixmap screenPixmap;
    
    WindowRect killWindowRect;
};

#endif
