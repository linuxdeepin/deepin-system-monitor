#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <mutex>
#include <thread>

#include <DButtonBox>
#include <DMainWindow>
#include <DSearchEdit>
#include <DShadowLine>
#include <DSpinner>
#include <DStackedWidget>
#include <DWidget>
#include <QTimer>

#include "ui_common.h"

DWIDGET_USE_NAMESPACE

class Toolbar;
class SystemServicePageWidget;
class ProcessPageWidget;
class Settings;

class MainWindow : public DMainWindow
{
    Q_OBJECT
    Q_DISABLE_COPY(MainWindow)

public:
    static MainWindow *instance()
    {
        static MainWindow *m_instance;

        if (!m_instance)
            m_instance = new MainWindow();

        return m_instance;
    }

    inline Toolbar *toolbar() const { return m_toolbar; }
    inline ProcessPageWidget *processPage() const { return m_procPage; }
    inline SystemServicePageWidget *systemServicePage() const { return m_svcPage; }

Q_SIGNALS:
    void displayModeChanged(DisplayMode mode);
    void loadingStatusChanged(bool loading);
    void authProgressStarted();
    void authProgressEnded();

public Q_SLOTS:
    inline void initDisplay()
    {
        initUI();
        initConnections();
    }

    void displayShortcutHelpDialog();

protected:
    void initUI();
    void initConnections();

    virtual void resizeEvent(QResizeEvent *event);
    virtual void closeEvent(QCloseEvent *event);
    bool eventFilter(QObject *obj, QEvent *event);
    virtual void showEvent(QShowEvent *event);

private:
    MainWindow(DWidget *parent = nullptr);
    ~MainWindow();

    Settings *m_settings;

    QAction *m_killAction                       {};
    DMenu   *m_modeMenu                         {};

    Toolbar                 *m_toolbar          {};
    DStackedWidget          *m_pages            {};
    ProcessPageWidget       *m_procPage         {};
    SystemServicePageWidget *m_svcPage          {};
    DShadowLine             *m_tbShadow         {};

    bool m_loading {true};
    char __unused__[7];     // ##padding##
    QTimer *m_timer {nullptr};
};

#endif  // MAIN_WINDOW_H
