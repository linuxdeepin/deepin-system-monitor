#include <DHiDPIHelper>
#include <DStackedWidget>
#include <DTitlebar>
#include <QDebug>
#include <QHBoxLayout>

#include "constant.h"
#include "main_window.h"
#include "process_page_widget.h"
#include "settings.h"
#include "system_service_page_widget.h"
#include "toolbar.h"
#include "utils.h"

std::atomic<MainWindow *> MainWindow::m_instance {nullptr};
std::mutex MainWindow::m_mutex;

MainWindow::MainWindow(DWidget *parent)
    : DMainWindow(parent)
{
    m_settings = Settings::instance();
    Q_ASSERT(m_settings);
    m_settings->init();
}

MainWindow::~MainWindow() {}

void MainWindow::initUI()
{
    // Init window size.
    int width = Constant::WINDOW_MIN_WIDTH;
    int height = Constant::WINDOW_MIN_HEIGHT;

    if (m_settings->getOption("window_width").isValid()) {
        width = m_settings->getOption("window_width").toInt();
    }

    if (m_settings->getOption("window_height").isValid()) {
        height = m_settings->getOption("window_height").toInt();
    }

    resize(width, height);

    // ========title bar===========
    titlebar()->setIcon(QIcon(DHiDPIHelper::loadNxPixmap(Utils::getQrcPath("logo_24.svg"))));
    m_toolbar = new Toolbar(this, this);
    titlebar()->setCustomWidget(m_toolbar);

    // ========add custom menu items===========
    DMenu *menu = new DMenu();
    titlebar()->setMenu(menu);

    // kill process
    auto *killAction = new QAction(tr("Force to end application"), menu);
    connect(killAction, &QAction::triggered, this, [=]() { Q_EMIT killProcessPerformed(); });

    // display mode
    auto *modeMenu = new DMenu(tr("Mode"), menu);
    QActionGroup *modeGroup = new QActionGroup(modeMenu);
    modeGroup->setExclusive(true);
    auto *expandModeAction = new QAction(tr("Expand"), modeGroup);
    expandModeAction->setCheckable(true);
    auto *compactModeAction = new QAction(tr("Compact"), modeGroup);
    compactModeAction->setCheckable(true);
    compactModeAction->setChecked(true);
    modeMenu->addAction(expandModeAction);
    modeMenu->addAction(compactModeAction);

    connect(expandModeAction, &QAction::triggered, this, [=]() {
        m_settings->setOption("display_mode", kDisplayModeExpand);
        Q_EMIT displayModeChanged(kDisplayModeExpand);
    });
    connect(compactModeAction, &QAction::triggered, this, [=]() {
        m_settings->setOption("display_mode", kDisplayModeCompact);
        Q_EMIT displayModeChanged(kDisplayModeCompact);
    });

    menu->addAction(killAction);
    menu->addMenu(modeMenu);

    // ========stack view==========
    m_pages = new DStackedWidget(this);
    m_procPage = new ProcessPageWidget(m_pages);
    m_svcPage = new SystemServicePageWidget(m_pages);
    m_svcPage = new SystemServicePageWidget(m_pages);
    m_pages->addWidget(m_procPage);
    m_pages->addWidget(m_svcPage);

    setCentralWidget(m_pages);
}

void MainWindow::initConnections()
{
    connect(m_toolbar, &Toolbar::procTabButtonClicked, this, [=]() {
        m_pages->setCurrentIndex(m_pages->indexOf(m_procPage));
        m_procPage->setFocus();
    });
    connect(m_toolbar, &Toolbar::serviceTabButtonClicked, this, [=]() {
        m_pages->setCurrentIndex(m_pages->indexOf(m_svcPage));
        m_svcPage->setFocus();
    });
    connect(m_pages, &DStackedWidget::currentChanged, this,
            [=]() { m_toolbar->clearSearchText(); });
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    // TODO
}
void MainWindow::closeEvent(QCloseEvent *event)
{
    // TODO
}
