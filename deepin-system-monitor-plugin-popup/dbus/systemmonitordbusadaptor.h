#ifndef SYSTEMMONITORDBUSADAPTOR_H
#define SYSTEMMONITORDBUSADAPTOR_H

#include <QtCore/QObject>
#include <QtDBus/QtDBus>

class SystemMonitorDBusAdaptor :  public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.deepin.SystemMonitorPluginPopup")

public:
    explicit SystemMonitorDBusAdaptor(QObject *parent = nullptr);
    virtual ~SystemMonitorDBusAdaptor();

public:
public slots:
    Q_SCRIPTABLE void slotShowOrHideSystemMonitorPluginPopupWidget();

signals:
    Q_SCRIPTABLE void sigSendShowOrHideSystemMonitorPluginPopupWidget();
};

#endif // SYSTEMMONITORDBUSADAPTOR_H
