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

static void onNethogsUpdate(int action, NethogsMonitorRecord const* update)
{
	NetworkTrafficFilter::setRowUpdate(action, *update);
}

static void nethogsMonitorThreadProc()
{
	const int status = nethogsmonitor_loop(&onNethogsUpdate);
	NetworkTrafficFilter::setNetHogsMonitorStatus(status);
}

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
        
        std::thread nethogs_monitor_thread(&nethogsMonitorThreadProc);

        MainWindow window;
        
        window.setMinimumSize(QSize(900, 700));
        DUtility::moveToCenter(&window);
        window.show();

        return app.exec();
        
        nethogsmonitor_breakloop();	
        nethogs_monitor_thread.join();
    }

    return 0;
}
