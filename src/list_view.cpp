#include "list_view.h"

ListView::ListView(QWidget *parent) : QWidget(parent)
{
    
}

void ListView::addItems(QList<ListItem> *items) {
    
}

void ListView::insertItems(ListItem *insertPosition, QList<ListItem> *items) {
    
}

void ListView::clearItems() {
    
}

void ListView::addSelections(QList<ListItem> *items) {
    
}

void ListView::clearSelections() {
    
}

void ListView::setTitles(QStringList *titles) {
    
}

void ListView::setSortAlgorithm() {
    
}

void ListView::setColumnWidths(QList<int> *widths) {
    
}

void ListView::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QPainterPath path;
    path.addRect(QRectF(rect()));
    painter.fillPath(path, QColor("#00A0A0"));
}
