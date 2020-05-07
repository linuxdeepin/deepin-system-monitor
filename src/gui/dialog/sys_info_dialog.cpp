#include <QHBoxLayout>
#include <QResizeEvent>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QDebug>

#include <DSplitter>
#include <DTitlebar>
#include <DLabel>
#include <DApplication>
#include <DApplicationHelper>

#include "sys_info_dialog.h"
#include "gui/base_item_delegate.h"

#define DIALOG_DEFAULT_WIDTH 720
#define DIALOG_DEFAULT_HEIGHT 620
#define LEFTBAR_MIN_WIDTH 160
#define LEFTBAR_MAX_WIDTH 240

DWIDGET_USE_NAMESPACE

SysInfoDialog::SysInfoDialog(QWidget *parent)
    : DMainWindow(parent)
{
    initUI();
    initConnections();

    installEventFilter(this);
}

bool SysInfoDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == this) {
        if (event->type() == QEvent::KeyPress) {
            auto *kev = dynamic_cast<QKeyEvent *>(event);
            if (kev->matches(QKeySequence::Cancel)) {
                close();
                return true;
            }
        }
    }
    return DMainWindow::eventFilter(obj, event);
}

void SysInfoDialog::initUI()
{
    setMinimumSize(DIALOG_DEFAULT_WIDTH, DIALOG_DEFAULT_HEIGHT);
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowModality(Qt::WindowModal);
    setWindowFlags(Qt::Dialog);
    titlebar()->setQuitMenuDisabled(true);
    titlebar()->setMenuVisible(false);
    setTitlebarShadowEnabled(false);
    titlebar()->setTitle({});

    auto *wkspc = new QWidget(this);
    auto *wkspcLayout = new QHBoxLayout(wkspc);
    wkspcLayout->setMargin(0);
    wkspc->setLayout(wkspcLayout);

    auto *split = new DSplitter(Qt::Horizontal, wkspc);
    split->setHandleWidth(1);
    split->setChildrenCollapsible(false);
    split->setFocusPolicy(Qt::NoFocus);
    wkspcLayout->addWidget(split);

    auto *leftBar = new DFrame(split);
    leftBar->setMinimumWidth(LEFTBAR_MIN_WIDTH);
    leftBar->setMaximumWidth(LEFTBAR_MAX_WIDTH);
    leftBar->setFrameRounded(false);
    leftBar->setFrameStyle(QFrame::NoFrame);
    leftBar->setAutoFillBackground(true);
    auto *leftBarLayout = new QHBoxLayout(leftBar);
    leftBarLayout->setMargin(10);
    leftBar->setLayout(leftBarLayout);

    m_cmdListView = new DListView(leftBar);
    m_cmdListView->setItemSpacing(0);
    m_cmdListView->setItemSize({10, 40});
    m_cmdListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_cmdListView->setSelectionMode(QListView::SingleSelection);
    m_cmdListView->setFocusPolicy(Qt::TabFocus);
    auto pa = DApplicationHelper::instance()->applicationPalette();
    QColor itemColor = pa.color(DPalette::Base);
    pa.setColor(DPalette::ItemBackground, itemColor);
    pa.setColor(DPalette::Background, itemColor);
    DApplicationHelper::instance()->setPalette(m_cmdListView, pa);

    leftBarLayout->addWidget(m_cmdListView);

    m_views = new DStackedWidget(this);
    m_sysInfoView = new SysInfoView(this);
    m_hwInfoView = new HWInfoView(this);
    m_sysCfgView = new SysConfigView(this);

    auto *content = new QWidget(split);
    content->setAutoFillBackground(false);
    auto *layout = new QHBoxLayout(content);
    content->setLayout(layout);
    layout->setMargin(10);
    layout->setSpacing(0);
    layout->addWidget(m_views);

    split->addWidget(leftBar);
    split->addWidget(content);

    m_views->addWidget(m_sysInfoView);
    m_views->addWidget(m_hwInfoView);
    m_views->addWidget(m_sysCfgView);
    m_views->setCurrentIndex(kShowSysInfo);

    split->setSizes({LEFTBAR_MIN_WIDTH, width() - LEFTBAR_MIN_WIDTH});

    setCentralWidget(wkspc);

    m_shadow = new DShadowLine(wkspc);
    m_shadow->setFixedWidth(DIALOG_DEFAULT_WIDTH);
    m_shadow->setFixedHeight(10);
    m_shadow->move(0, 0);
    m_shadow->raise();
    m_shadow->show();

    populateCmdList();
}

void SysInfoDialog::initConnections()
{
    connect(m_cmdListView->selectionModel(), &QItemSelectionModel::currentChanged, this, [ = ](const QModelIndex & current, const QModelIndex & previous) {
        Q_UNUSED(previous)

        ActionType cmdId = static_cast<ActionType>(current.row());
        m_views->setCurrentIndex(cmdId);
    });
}

void SysInfoDialog::resizeEvent(QResizeEvent *event)
{
    DMainWindow::resizeEvent(event);

    if (m_shadow) {
        m_shadow->setFixedWidth(event->size().width());
        m_shadow->raise();
        m_shadow->show();
    }
}

void SysInfoDialog::showEvent(QShowEvent *event)
{
    if (m_shadow) {
        m_shadow->setFixedWidth(width());
        m_shadow->raise();
        m_shadow->show();
    }

    DMainWindow::showEvent(event);
}

void SysInfoDialog::populateCmdList()
{
    auto *model = new QStandardItemModel(m_cmdListView);

    auto *sysinfoItem = new QStandardItem();
    sysinfoItem->setData(DApplication::translate("System.Info.Dialog", "System Info"), Qt::DisplayRole);
    sysinfoItem->setData(kShowSysInfo, kValueRole);
    model->appendRow(sysinfoItem);

    auto *hwinfoItem = new QStandardItem();
    hwinfoItem->setData(DApplication::translate("System.Info.Dialog", "Device Specs"), Qt::DisplayRole);
    hwinfoItem->setData(kShowHWInfo, kValueRole);
    model->appendRow(hwinfoItem);

    auto *sysCfgItem = new QStandardItem();
    sysCfgItem->setData(DApplication::translate("System.Info.Dialog", "System Parameters"), Qt::DisplayRole);
    sysCfgItem->setData(kChangeSysCfg, kValueRole);
    model->appendRow(sysCfgItem);

    m_cmdListView->setModel(model);
    m_cmdListView->setCurrentIndex(model->index(0, 0));
}
