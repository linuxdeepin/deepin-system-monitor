#ifndef SYSTEM_SERVICE_ITEM_DELEGATE_H
#define SYSTEM_SERVICE_ITEM_DELEGATE_H

#include <QItemDelegate>

class QModelIndex;
class QPainter;
class QStyleOptionViewItem;

class SystemServiceItemDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    SystemServiceItemDelegate(QObject *parent);
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QWidget *createEditor(QWidget *, const QStyleOptionViewItem &,
                          const QModelIndex &) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

#endif  // SYSTEM_SERVICE_ITEM_DELEGATE_H
