#ifndef LISTVIEW_H
#define LISTVIEW_H

#include <QWidget>
#include <QTimer>
#include "list_item.h"

typedef bool (* SortFunctionPtr) (const ListItem *item1, const ListItem *item2, bool descendingSort);

class ListView : public QWidget
{
    Q_OBJECT
    
public:
    ListView(int height=32, QWidget *parent = 0);
    
    void addItems(QList<ListItem*> items);
    void clearItems();
    
    void addSelections(QList<ListItem*> items, bool recordLastSelection=true);
    void clearSelections(bool clearLastSelection=true);
    
    void setTitles(QList<QString> titles, int height);
    void setColumnWidths(QList<int> widths);
    int adjustRenderOffset(int offset);
    
    QList<ListItem*> *listItems;
    QList<ListItem*> *selectionItems;
    ListItem *lastSelectItem;
    int renderOffset;
    
    void setSortAlgorithm(QList<SortFunctionPtr> *ptrs, int sortColumn=-1, bool descendingSort=false);
    
    void sortItems(int column, bool descendingSort);
    
    void initItems(QList<ListItem*> items);
    
    QList<SortFunctionPtr> *sortFunctionPtrs;
    QList<bool> *sortOrderes;
                                    
public slots:
    void renderAnimation();
    void hideScrollbar();
    
protected:
    void paintEvent(QPaintEvent *);
    bool eventFilter(QObject *, QEvent *event);
    void mouseMoveEvent(QMouseEvent *mouseEvent);
    void mousePressEvent(QMouseEvent *mouseEvent);
    void mouseReleaseEvent(QMouseEvent *mouseEvent);
    void wheelEvent(QWheelEvent *event);
    void keyPressEvent(QKeyEvent *keyEvent);
    
    bool isMouseAtScrollArea(int x);
    bool isMouseAtTitleArea(int y);
    
    void startScroll();
    
    int getScrollbarHeight();
    
    void pressHome();
    void pressEnd();
    void pressUp();
    void pressDown();
    void pressPageUp();
    void pressPageDown();
    
    void scrollPageUp();
    void scrollPageDown();
    void scrollHome();
    void scrollEnd();
    
    void shiftSelectHome();
    void shiftSelectEnd();
    void shiftSelectUp();
    void shiftSelectDown();
    void shiftSelectPageUp();
    void shiftSelectPageDown();
    
    void selectAll();
    
    void shiftSelect(int selectionStartIndex, int selectionEndIndex);
    
    void selectScrollUp(int scrollOffset);
    void selectScrollDown(int scrollOffset);
    void shiftSelectScrollUp(int scrollOffset);
    void shiftSelectScrollDown(int scrollOffset);
    
    void paintScrollbar(QPainter *painter);
    
    void startHideScrollbar();
    
    QList<int> calcuateRenderWidths();
    
    int rowHeight;
    
    bool mouseAtScrollArea;
    bool mouseDragScrollbar;
    
    int scrollbarDragWidth;
    int scrollbarDefaultWidth;
    
private:
    QList<int> columnWidths;
    int titleHeight;
    int scrollUnit;
    int scrollStartY;
    int scrollDistance;
    int oldRenderOffset;
    QList<QString> titleNames;
    
    QTimer *renderTimer;
    QTimer *hideScrollbarTimer;
    int animationDuration;
    int animationFrames;
    int renderTicker;
    
    int renderCounter;
    
    int defaultSortColumn;
    bool defaultSortOrder;
};

#endif
