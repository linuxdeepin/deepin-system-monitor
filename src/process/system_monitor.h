#ifndef SYSTEM_MONITOR_H
#define SYSTEM_MONITOR_H


class SystemMonitor : public QObject
{
    Q_OBJECT
public:
    explicit SystemMonitor(QObject *parent = nullptr);

signals:

public slots:
};

#endif // SYSTEM_MONITOR_H