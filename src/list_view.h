#ifndef LISTVIEW_H
#define LISTVIEW_H

#include "list_item.h"
#include <QTimer>
#include <QWidget>

typedef bool (* SortFunctionPtr) (const ListItem *item1, const ListItem *item2, bool descendingSort);

class ListView : public QWidget
{
    Q_OBJECT
    
public:
    ListView(QWidget *parent = 0);
    
    void addItems(QList<ListItem*> items);
    void addSelections(QList<ListItem*> items, bool recordLastSelection=true);
    void clearItems();
    void clearSelections(bool clearLastSelection=true);
    void initItems(QList<ListItem*> items);
    void setColumnWidths(QList<int> widths);
    void setSortAlgorithm(QList<SortFunctionPtr> *ptrs, int sortColumn=-1, bool descendingSort=false);
    void setTitles(QList<QString> titles, int height);
    
public slots:
    void renderAnimation();
    void hideScrollbar();
    
protected:
    bool eventFilter(QObject *, QEvent *event);
    bool isMouseAtScrollArea(int x);
    bool isMouseAtTitleArea(int y);
    int adjustRenderOffset(int offset);
    int getScrollbarHeight();
    void keyPressEvent(QKeyEvent *keyEvent);
    void mouseMoveEvent(QMouseEvent *mouseEvent);
    void mousePressEvent(QMouseEvent *mouseEvent);
    void mouseReleaseEvent(QMouseEvent *mouseEvent);
    void paintEvent(QPaintEvent *);
    void paintScrollbar(QPainter *painter);
    void pressDown();
    void pressEnd();
    void pressHome();
    void pressPageDown();
    void pressPageUp();
    void pressUp();
    void scrollEnd();
    void scrollHome();
    void scrollPageDown();
    void scrollPageUp();
    void selectAll();
    void selectScrollDown(int scrollOffset);
    void selectScrollUp(int scrollOffset);
    void setRowHeight(int height);
    void shiftSelect(int selectionStartIndex, int selectionEndIndex);
    void shiftSelectDown();
    void shiftSelectEnd();
    void shiftSelectHome();
    void shiftSelectPageDown();
    void shiftSelectPageUp();
    void shiftSelectScrollDown(int scrollOffset);
    void shiftSelectScrollUp(int scrollOffset);
    void shiftSelectUp();
    void sortItems(int column, bool descendingSort);
    void startHideScrollbar();
    void startScroll();
    void wheelEvent(QWheelEvent *event);
    
private:
    ListItem *lastSelectItem;
    QList<ListItem*> *listItems;
    QList<ListItem*> *selectionItems;
    QList<QString> titleNames;
    QList<SortFunctionPtr> *sortFunctionPtrs;
    QList<bool> *sortOrderes;
    QList<int> calcuateRenderWidths();
    QList<int> columnWidths;
    QTimer *hideScrollbarTimer;
    QTimer *renderTimer;
    bool defaultSortOrder;
    bool mouseAtScrollArea;
    bool mouseDragScrollbar;
    int animationDuration;
    int animationFrames;
    int defaultSortColumn;
    int oldRenderOffset;
    int renderCounter;
    int renderOffset;
    int renderTicker;
    int rowHeight;
    int scrollDistance;
    int scrollStartY;
    int scrollUnit;
    int scrollbarDefaultWidth;
    int scrollbarDragWidth;
    int titleHeight;
};

#endif
