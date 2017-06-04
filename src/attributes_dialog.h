#ifndef ATTRIBUTESDIALOG_H
#define ATTRIBUTESDIALOG_H 

#include <QLabel>
#include <QPaintEvent>
#include <QVBoxLayout>
#include <QWidget>
#include <ddialog.h>

class AttributesDialog : public Dtk::Widget::DAbstractDialog
{
    Q_OBJECT
    
public:
    AttributesDialog(int pid);

    void paintEvent(QPaintEvent *);
};

#endif
