#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QHBoxLayout>
#include "dmainwindow.h"

DWIDGET_USE_NAMESPACE

class MainWindow : public DMainWindow
{
    Q_OBJECT
    
public:
    MainWindow(DMainWindow *parent = 0);
    
private:
    QHBoxLayout *layout;
};

#endif
