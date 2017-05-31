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
    
    const char *descriptionText = QT_TRANSLATE_NOOP("MainWindow", 
                                                    "深度系统监视器是一款设计直观易用、功能强大的系统监视器。"
                                                    "它支持进程的CPU、内存、网络和磁盘等多种状态显示，"
                                                    "并简化用户进程的显示和说明，方便用户更快速的使用。"
                                                    );

    const QString acknowledgementLink = "https://www.deepin.org/acknowledgments/deepin-system-monitor#thanks";

    DApplication app(argc, argv);

    if (app.setSingleInstance("deepin-system-monitor")) {
        app.loadTranslator();
        
        app.setOrganizationName("deepin");
        app.setApplicationName(QObject::tr("Deepin System Monitor"));
        app.setApplicationVersion("1.0");
        
        app.setProductIcon(QPixmap::fromImage(QImage(Utils::getQrcPath("logo_96.svg"))));
        app.setProductName(DApplication::translate("MainWindow", "Deepin System Monitor"));
        app.setApplicationDescription(DApplication::translate("MainWindow", descriptionText) + "\n");
        app.setApplicationAcknowledgementPage(acknowledgementLink);
        
        app.setTheme("dark");
        app.setWindowIcon(QIcon(Utils::getQrcPath("deepin-system-monitor.svg")));
        
        std::thread nethogs_monitor_thread(&NetworkTrafficFilter::nethogsMonitorThreadProc);

        MainWindow window;
        
        window.setMinimumSize(QSize(1024, 700));
        DUtility::moveToCenter(&window);
        window.show();

        return app.exec();
    }

    return 0;
}
