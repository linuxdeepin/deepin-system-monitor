#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <QString>
#include <QDebug>

static void print_err(decltype(errno) e, const QString &msg)
{
    qWarning() << QString("Error: [%1] %2, ").arg(e).arg(strerror(e)) << msg;
};

#endif // LOGGER_HPP
