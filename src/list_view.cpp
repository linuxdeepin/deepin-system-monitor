#include "list_view.h"

ListView::ListView(int height, QWidget *parent) : QWidget(parent)
{
    rowHeight = height;
    renderY = 0;
}

void ListView::addItems(QList<ListItem*> items) {
    QPainter painter(this);
    
    listItems.append(items);
    
    repaint();
}

void ListView::clearItems() {
    listItems.clear();
    
    repaint();
}

void ListView::addSelections(QList<ListItem*> items) {
    
}

void ListView::clearSelections() {
    
}

void ListView::setTitles(QStringList *titles) {
    
}

void ListView::setSortAlgorithm() {
    
}

void ListView::setColumnWidths(QList<int> widths) {
    columnWidths = widths;
}

void ListView::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    int rowCounter = 0;
    for (ListItem *item:listItems) {
        int columnCounter = 0;
        int columnRenderX = 0;
        for (int columnWidth:columnWidths) {
            item->render(columnCounter, QRect(columnRenderX, renderY + rowCounter * rowHeight, columnWidth, rowHeight), &painter);
            
            columnRenderX += columnWidth;
            columnCounter++;
        }
        
        rowCounter++;
    }
}
