#ifndef LISTVIEW_H
#define LISTVIEW_H

#include "list_item.h"
#include <QTimer>
#include <QWidget>

typedef bool (* SortAlgorithm) (const ListItem *item1, const ListItem *item2, bool descendingSort);

class ListView : public QWidget
{
    Q_OBJECT
    
public:
    ListView(QWidget *parent = 0);

    // ListView interfaces.
    void setRowHeight(int height);
    
    void addItems(QList<ListItem*> items);
    void clearItems();
    
    void addSelections(QList<ListItem*> items, bool recordLastSelection=true);
    void clearSelections(bool clearLastSelection=true);
    
    void refreshItems(QList<ListItem*> items);
    
    void setColumnTitles(QList<QString> titles, int height);
    void setColumnWidths(QList<int> widths);
    void setColumnSortingAlgorithms(QList<SortAlgorithm> *algorithms, int sortColumn=-1, bool descendingSort=false);
    
    // ListView operations.
    void selectAllItems();
    void selectFirstItem();
    void selectLastItem();
    void selectNextItem();
    void selectPrevItem();
    
    void shiftSelectPageDown();
    void shiftSelectPageUp();
    void shiftSelectToEnd();
    void shiftSelectToHome();
    void shiftSelectToNext();
    void shiftSelectToPrev();
    
    void scrollPageDown();
    void scrollPageUp();
    
    void ctrlScrollPageDown();
    void ctrlScrollPageUp();
    void ctrlScrollToEnd();
    void ctrlScrollToHome();
    
private slots:
    void scrollAnimation();
    void hideScrollbar();
    
private:
    bool eventFilter(QObject *, QEvent *event);
    void keyPressEvent(QKeyEvent *keyEvent);
    void mouseMoveEvent(QMouseEvent *mouseEvent);
    void mousePressEvent(QMouseEvent *mouseEvent);
    void mouseReleaseEvent(QMouseEvent *mouseEvent);
    void wheelEvent(QWheelEvent *event);
    
    void paintEvent(QPaintEvent *);
    void paintScrollbar(QPainter *painter);
    
    void selectNextItemWithOffset(int scrollOffset);
    void selectPrevItemWithOffset(int scrollOffset);
    
    void shiftSelectItemsWithBound(int selectionStartIndex, int selectionEndIndex);
    void shiftSelectNextItemWithOffset(int scrollOffset);
    void shiftSelectPrevItemWithOffset(int scrollOffset);
                        
    QList<int> getRenderWidths();
    bool isMouseAtScrollArea(int x);
    bool isMouseAtTitleArea(int y);
    int adjustRenderOffset(int offset);
    int getScrollbarHeight();
    void sortItemsByColumn(int column, bool descendingSort);
    void startScrollAnimation();
    void startScrollbarHideTimer();
    
    ListItem *lastSelectItem;
    QList<ListItem*> *listItems;
    QList<ListItem*> *selectionItems;
    QList<QString> columnTitles;
    QList<SortAlgorithm> *sortingAlgorithms;
    QList<bool> *sortingOrderes;
    QList<int> columnWidths;
    QTimer *hideScrollbarTimer;
    QTimer *scrollAnimationTimer;
    bool defaultSortingOrder;
    bool mouseAtScrollArea;
    bool mouseDragScrollbar;
    int defaultSortingColumn;
    int oldRenderOffset;
    int renderOffset;
    int rowHeight;
    int scrollAnimationDuration;
    int scrollAnimationFrames;
    int scrollAnimationTicker;
    int scrollDistance;
    int scrollStartY;
    int scrollUnit;
    int scrollbarDefaultWidth;
    int scrollbarDragWidth;
    int titleHeight;
};

#endif
