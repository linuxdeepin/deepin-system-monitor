#include <QModelIndex>
#include <QPainter>
#include <QStyleOptionViewItem>

#include "system_service_item_delegate.h"

SystemServiceItemDelegate::SystemServiceItemDelegate(QObject *parent)
    : QItemDelegate(parent)
{
}

void SystemServiceItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                      const QModelIndex &index) const
{
    Q_UNUSED(index);
    Q_UNUSED(option);
    painter->save();
    // TODO: fix style
    QItemDelegate::paint(painter, option, index);
    painter->restore();
}

QWidget *SystemServiceItemDelegate::createEditor(QWidget *, const QStyleOptionViewItem &,
                                                 const QModelIndex &) const
{
    return nullptr;
}

QSize SystemServiceItemDelegate::sizeHint(const QStyleOptionViewItem &option,
                                          const QModelIndex &index) const
{
    QSize size = QItemDelegate::sizeHint(option, index);
    size.setHeight(std::max(36, size.height()));
    return size;
}
