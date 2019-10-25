#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <DButtonBox>
#include <DMainWindow>
#include <DSearchEdit>
#include <DWidget>
#include <mutex>
#include <thread>

#include "ui_common.h"

DWIDGET_USE_NAMESPACE

class Toolbar;
class SystemServicePageWidget;
class ProcessPageWidget;
class DStackWidget;
class Settings;

class MainWindow : public DMainWindow
{
    Q_OBJECT
    Q_DISABLE_COPY(MainWindow)

public:
    static MainWindow *instance()
    {
        MainWindow *sin = m_instance.load();
        if (!sin) {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!sin) {
                sin = new MainWindow();
                m_instance.store(sin);
            }
        }

        return sin;
    }

    inline Toolbar *toolbar() const { return m_toolbar; }
    inline ProcessPageWidget *processPage() const { return m_procPage; }
    inline SystemServicePageWidget *systemServicePage() const { return m_svcPage; }

Q_SIGNALS:
    void killProcessPerformed();
    void displayModeChanged(DisplayMode mode);

public Q_SLOTS:
    inline void initDisplay()
    {
        initUI();
        initConnections();
    }

protected:
    void initUI();
    void initConnections();

    virtual void resizeEvent(QResizeEvent *event);
    virtual void closeEvent(QCloseEvent *event);

private:
    MainWindow(DWidget *parent = nullptr);
    ~MainWindow();

    Settings *m_settings;

    Toolbar *m_toolbar;
    DStackedWidget *m_pages;
    ProcessPageWidget *m_procPage;
    SystemServicePageWidget *m_svcPage;

    static std::atomic<MainWindow *> m_instance;
    static std::mutex m_mutex;
};

#endif  // MAIN_WINDOW_H
