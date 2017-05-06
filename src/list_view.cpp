#include "list_view.h"
#include "utils.h"

ListView::ListView(int height, QWidget *parent) : QWidget(parent)
{
    rowHeight = height;
    
    titleHeight = 0;
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

void ListView::setTitles(QList<QString> titles, int height) {
    titleNames = titles;
    titleHeight = height;
}

void ListView::setSortAlgorithm() {
    
}

void ListView::setColumnWidths(QList<int> widths) {
    columnWidths = widths;
}

void ListView::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Calcuate title widths;
    QList<int> renderWidths;
    if (columnWidths.contains(-1)) {
        for (int columnWidth:columnWidths) {
            if (columnWidth != -1) {
                renderWidths << columnWidth;
            } else {
                int totalWidthOfOtherColumns = 0;
                for (auto w:columnWidths) {
                    if (w != -1) {
                        totalWidthOfOtherColumns += w;
                    }
                }
                
                renderWidths << rect().width() - totalWidthOfOtherColumns;
            }
        }
    } else {
        renderWidths = columnWidths;
    }
    
    // Draw title.
    int renderY = 0;
    if (titleHeight > 0) {
        int columnCounter = 0;
        int columnRenderX = 0;
        for (int renderWidth:renderWidths) {
            painter.setOpacity(1);
            Utils::setFontSize(painter, 11);
            painter.setPen(QPen(QColor("#666666")));
            painter.drawText(QRect(columnRenderX, 0, renderWidth, titleHeight), Qt::AlignLeft | Qt::AlignVCenter, titleNames[columnCounter]);
            
            columnRenderX += renderWidth;
            columnCounter++;
        }
        
        renderY += titleHeight;
    }
    
    // Draw context.
    int rowCounter = 0;
    for (ListItem *item:listItems) {
        item->renderBackground(rowCounter, QRect(0, renderY + rowCounter * rowHeight, rect().width(), rowHeight), &painter);
        
        int columnCounter = 0;
        int columnRenderX = 0;
        for (int renderWidth:renderWidths) {
            item->render(columnCounter, QRect(columnRenderX, renderY + rowCounter * rowHeight, renderWidth, rowHeight), &painter);
            
            columnRenderX += renderWidth;
            columnCounter++;
        }
        
        rowCounter++;
    }
}

