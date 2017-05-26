#include <DApplication>
#include <DMainWindow>
#include <QApplication>
#include <QDesktopWidget>
#include <dutility.h>

#include "utils.h"
#include "main_window.h"
#include "network_traffic_filter.h"
#include <iostream>
#include <thread>

DWIDGET_USE_NAMESPACE

int main(int argc, char *argv[]) 
{
    DApplication::loadDXcbPlugin();

    DApplication app(argc, argv);

    if (app.setSingleInstance("deepin-system-monitor")) {
        app.loadTranslator();
        
        app.setOrganizationName("deepin");
        app.setApplicationName(QObject::tr("Deepin System Monitor"));
        app.setApplicationVersion("1.0");
        
        app.setTheme("dark");
        
        std::thread nethogs_monitor_thread(&NetworkTrafficFilter::nethogsMonitorThreadProc);

        MainWindow window;
        
        window.setMinimumSize(QSize(900, 700));
        DUtility::moveToCenter(&window);
        window.show();

        return app.exec();
    }

    return 0;
}
