#include <DApplication>
#include <DApplicationHelper>
#include <DPalette>
#include <QDebug>
#include <QPainter>
#include <QPainterPath>

#include "base_table_view.h"

BaseTableView::BaseTableView(DWidget *parent)
    : DTreeView(parent)
{
    m_itemDelegate = new BaseItemDelegate(this);
    setItemDelegate(m_itemDelegate);

    m_headerView = new BaseHeaderView(Qt::Horizontal, this);
    setHeader(m_headerView);
    m_headerView->setSectionsMovable(true);
    m_headerView->setSectionsClickable(true);
    m_headerView->setSectionResizeMode(DHeaderView::Interactive);
    m_headerView->setStretchLastSection(true);
    m_headerView->setSortIndicatorShown(true);
    m_headerView->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_headerView->setContextMenuPolicy(Qt::CustomContextMenu);

    setRootIsDecorated(false);
    setItemsExpandable(false);
    setFrameStyle(QFrame::NoFrame);
    viewport()->setAutoFillBackground(false);
    // if set to true, qt will draw item background for you, thats not what you want here!
    setAlternatingRowColors(false);
}

void BaseTableView::paintEvent(QPaintEvent *event)
{
    QPainter painter(viewport());
    painter.save();
    painter.setRenderHints(QPainter::Antialiasing);
    painter.setOpacity(1);
    painter.setClipping(true);

    QWidget *wnd = DApplication::activeWindow();
    DPalette::ColorGroup cg;
    if (!wnd) {
        cg = DPalette::Inactive;
    } else {
        cg = DPalette::Active;
    }

    auto style = dynamic_cast<DStyle *>(DApplication::style());
    auto *dAppHelper = DApplicationHelper::instance();
    auto palette = dAppHelper->applicationPalette();

    QBrush bgBrush(palette.color(cg, DPalette::Base));

    QStyleOptionFrame option;
    initStyleOption(&option);
    int radius = style->pixelMetric(DStyle::PM_FrameRadius, &option);

    QRect rect = viewport()->rect();
    QRectF clipRect(rect.x(), rect.y() - rect.height(), rect.width(), rect.height() * 2);
    QRectF subRect(rect.x(), rect.y() - rect.height(), rect.width(), rect.height());
    QPainterPath clipPath, subPath;
    clipPath.addRoundedRect(clipRect, radius, radius);
    subPath.addRect(subRect);
    clipPath = clipPath.subtracted(subPath);

    painter.fillPath(clipPath, bgBrush);

    painter.restore();
    DTreeView::paintEvent(event);
}

// **********************************************************************************************
// draw with this method kinda buggy, when horizontal scrollbar appears, forground text will draw
// outside of the rectangle
// **********************************************************************************************
// void BaseTableView::drawRow(QPainter *painter, const QStyleOptionViewItem &options,
//                            const QModelIndex &index) const
//{
//    painter->save();
//    painter->setRenderHint(QPainter::Antialiasing);

//#ifdef ENABLE_INACTIVE_DISPLAY
//    QWidget *wnd = DApplication::activeWindow();
//#endif
//    DPalette::ColorGroup cg;
//    if (!(options.state & DStyle::State_Enabled)) {
//        cg = DPalette::Disabled;
//    } else {
//#ifdef ENABLE_INACTIVE_DISPLAY
//        if (!wnd) {
//            cg = DPalette::Inactive;
//        } else {
//            if (wnd->isModal()) {
//                cg = DPalette::Inactive;
//            } else {
//                cg = DPalette::Active;
//            }
//        }
//#else
//        cg = DPalette::Active;
//#endif
//    }

//    DStyle *style = dynamic_cast<DStyle *>(DApplication::style());

//    int radius = style->pixelMetric(DStyle::PM_FrameRadius, &options);
//    int margin = style->pixelMetric(DStyle::PM_ContentsMargins, &options);

//    DApplicationHelper *dAppHelper = DApplicationHelper::instance();
//    DPalette palette = dAppHelper->applicationPalette();
//    QBrush background;
//    QPen forground;
//    if (index.row() % 2 == 0) {
//        background = palette.color(cg, DPalette::AlternateBase);
//    } else {
//        background = palette.color(cg, DPalette::Base);
//    }
//    if (options.state & DStyle::State_Enabled) {
//        if (selectedIndexes().contains(index)) {
//            background = palette.color(cg, DPalette::Highlight);
//        }
//    }

//    QPainterPath path;
//    QRect rect = options.rect;

//    rect.setX(rect.x() + margin);
//    rect.setWidth(rect.width() - margin);
//    path.addRoundedRect(rect, radius, radius);
//    painter->fillPath(path, background);

//    painter->restore();
//    QTreeView::drawRow(painter, options, index);
//}
