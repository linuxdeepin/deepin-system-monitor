#include <DApplication>
#include <DApplicationHelper>
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
    if (m_settings)
        m_settings->init();
}

MainWindow::~MainWindow() {}

void MainWindow::initUI()
{
    // Init window size.
    int width = Constant::WINDOW_MIN_WIDTH;
    int height = Constant::WINDOW_MIN_HEIGHT;

    if (m_settings->getOption(kSettingKeyWindowWidth).isValid()) {
        width = m_settings->getOption(kSettingKeyWindowWidth).toInt();
    }

    if (m_settings->getOption(kSettingKeyWindowHeight).isValid()) {
        height = m_settings->getOption(kSettingKeyWindowHeight).toInt();
    }

    resize(width, height);

    // ========title bar===========
    titlebar()->setIcon(QIcon::fromTheme("deepin-system-monitor"));
    m_toolbar = new Toolbar(this, this);
    titlebar()->setCustomWidget(m_toolbar);

    // ========add custom menu items===========
    DMenu *menu = new DMenu();
    titlebar()->setMenu(menu);

    // kill process
    m_killAction = new QAction(
        DApplication::translate("Title.Bar.Context.Menu", "Force end application"), menu);
    connect(m_killAction, &QAction::triggered, this, [=]() { Q_EMIT killProcessPerformed(); });

    // display mode
    m_modeMenu = new DMenu(DApplication::translate("Title.Bar.Context.Menu", "View"), menu);
    QActionGroup *modeGroup = new QActionGroup(m_modeMenu);
    modeGroup->setExclusive(true);
    auto *expandModeAction =
        new QAction(DApplication::translate("Title.Bar.Context.Menu", "Expand"), modeGroup);
    expandModeAction->setCheckable(true);
    auto *compactModeAction =
        new QAction(DApplication::translate("Title.Bar.Context.Menu", "Compact"), modeGroup);
    compactModeAction->setCheckable(true);
    m_modeMenu->addAction(expandModeAction);
    m_modeMenu->addAction(compactModeAction);

    QVariant vmode = m_settings->getOption(kSettingKeyDisplayMode);
    if (vmode.isValid()) {
        bool ok = false;
        int mode = vmode.toInt(&ok);
        if (ok) {
            if (mode == kDisplayModeExpand) {
                expandModeAction->setChecked(true);
            } else if (mode == kDisplayModeCompact) {
                compactModeAction->setChecked(true);
            }
        } else {
            expandModeAction->setChecked(true);
            m_settings->setOption(kSettingKeyDisplayMode, kDisplayModeExpand);
            Q_EMIT displayModeChanged(kDisplayModeExpand);
        }
    }

    connect(expandModeAction, &QAction::triggered, this, [=]() {
        m_settings->setOption(kSettingKeyDisplayMode, kDisplayModeExpand);
        Q_EMIT displayModeChanged(kDisplayModeExpand);
    });
    connect(compactModeAction, &QAction::triggered, this, [=]() {
        m_settings->setOption(kSettingKeyDisplayMode, kDisplayModeCompact);
        Q_EMIT displayModeChanged(kDisplayModeCompact);
    });

    menu->addAction(m_killAction);
    menu->addMenu(m_modeMenu);

    DApplicationHelper *dAppHelper = DApplicationHelper::instance();
    connect(dAppHelper, &DApplicationHelper::themeTypeChanged, this,
            [=]() { m_settings->setOption(kSettingKeyThemeType, DApplicationHelper::LightType); });

    // ========stack view==========
    m_pages = new DStackedWidget(this);
    m_procPage = new ProcessPageWidget(m_pages);
    m_svcPage = new SystemServicePageWidget(m_pages);
    m_pages->addWidget(m_procPage);
    m_pages->addWidget(m_svcPage);

    setCentralWidget(m_pages);

    installEventFilter(this);
}

void MainWindow::initConnections()
{
    connect(m_toolbar, &Toolbar::procTabButtonClicked, this, [=]() {
        m_pages->setCurrentIndex(m_pages->indexOf(m_procPage));
        m_modeMenu->setEnabled(true);
        m_killAction->setEnabled(true);
    });
    connect(m_toolbar, &Toolbar::serviceTabButtonClicked, this, [=]() {
        m_pages->setCurrentIndex(m_pages->indexOf(m_svcPage));
        m_modeMenu->setEnabled(false);
        m_killAction->setEnabled(false);
    });
    connect(m_pages, &DStackedWidget::currentChanged, this,
            [=]() { m_toolbar->clearSearchText(); });
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)

    m_settings->setOption(kSettingKeyWindowWidth, width());
    m_settings->setOption(kSettingKeyWindowHeight, height());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)

    m_settings->setOption(kSettingKeyWindowWidth, width());
    m_settings->setOption(kSettingKeyWindowHeight, height());

    if (m_settings)
        m_settings->flush();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        auto *kev = dynamic_cast<QKeyEvent *>(event);
        if (kev->matches(QKeySequence::Quit)) {
            DApplication::quit();
            return true;
        } else if (kev->matches(QKeySequence::Find)) {
            toolbar()->focusInput();
            return true;
        } else {
            return false;
        }
    } else {
        return DMainWindow::eventFilter(obj, event);
    }
}
