#include "user_list_view.h"
#include "ddlog.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QDebug>

using namespace DDLog;

UserListView::UserListView(QWidget *parent): DListView(parent)
    , m_currentIndex(0)
{
    qCDebug(app) << "UserListView constructor";
    // 禁用横向滚动条,防止内容被截断
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void UserListView::resetStatus(const QModelIndex &index)
{
    qCDebug(app) << "UserListView resetStatus, index:" << index.row();
    m_currentIndex = index.row();

    DListView::clearSelection();
    DListView::setSelectionMode(DListView::SingleSelection);
    DListView::setCurrentIndex(index);
}

void UserListView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    qCDebug(app) << "UserListView currentChanged, current:" << current.row() << "previous:" << previous.row();
    DListView::setSelectionMode(DListView::SingleSelection);
    DListView::currentChanged(current, previous);
}

QModelIndex UserListView::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
    qCDebug(app) << "UserListView moveCursor, cursorAction:" << cursorAction;
    DListView::setSelectionMode(DListView::MultiSelection);
    return DListView::moveCursor(cursorAction, modifiers);
}

void UserListView::keyPressEvent(QKeyEvent *event)
{
    // qCDebug(app) << "UserListView keyPressEvent, key:" << event->key();

    if (event->key() == Qt::Key_Up) {
        qCDebug(app) << "Up key pressed";
        QModelIndex nextIndex = model()->index(m_currentIndex - 1, 0);
        if (nextIndex.isValid()) {
            if (isRowHidden(m_currentIndex - 1)) {
                if (m_currentIndex - 1 <= 0) {
                    return;
                }
                m_currentIndex--;
                keyPressEvent(event);
                return;
            }
            setCurrentIndex(nextIndex);
            Q_EMIT clicked(nextIndex);
        }
        return;
    } else if (event->key() == Qt::Key_Down) {
        qCDebug(app) << "Down key pressed";
        QModelIndex nextIndex = model()->index(m_currentIndex + 1, 0);
        if (nextIndex.isValid()) {
            if (isRowHidden(m_currentIndex + 1)) {
                if (model()->rowCount() - 1 <= m_currentIndex + 1) {
                    return;
                }
                m_currentIndex++;
                keyPressEvent(event);
                return;
            }
            setCurrentIndex(nextIndex);
            Q_EMIT clicked(nextIndex);
        }
        return;
    }
    return DListView::keyPressEvent(event);
}
void UserListView::mousePressEvent(QMouseEvent *event)
{
    // qCDebug(app) << "UserListView mousePressEvent, button:" << event->button();
    if (event->button() == Qt::RightButton) {
        qCDebug(app) << "Right button pressed";
        Q_EMIT signalRightButtonClicked(QPoint(event->x(), event->y()));

        return;
    }
    return DListView::mousePressEvent(event);
}
