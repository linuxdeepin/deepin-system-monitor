#ifndef ATTRIBUTESDIALOG_H
#define ATTRIBUTESDIALOG_H 

#include <ddialog.h>
#include <QWidget>
#include <QPaintEvent>
#include <QVBoxLayout>
#include <QLabel>

class AttributesDialog : public Dtk::Widget::DAbstractDialog
{
    Q_OBJECT
    
public:
    AttributesDialog(int pid);

    void paintEvent(QPaintEvent *);
};

#endif
