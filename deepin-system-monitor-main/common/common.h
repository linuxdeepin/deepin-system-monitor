/*
* Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      maojj <maojunjie@uniontech.com>
* Maintainer:  maojj <maojunjie@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef COMMON_H
#define COMMON_H

#include <QString>
#include <QVariant>
#include <QDebug>

#include <memory>

#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <sys/time.h>

class QPainter;

namespace common {

namespace compat {

template<typename T>
struct Callback;

template<typename Ret, typename... Params>
struct Callback<Ret(Params...)> {
    template<typename... Args>
    static Ret callback(Args... args)
    {
        return func(args...);
    }
    static std::function<Ret(Params...)> func;
};
template<typename Ret, typename... Params>
std::function<Ret(Params...)> Callback<Ret(Params...)>::func;

} // namespace compat

namespace alloc {

struct uFDClose {
    void operator()(int *fd)
    {
        close(*fd);
    }
};

struct uFileClose {
    void operator()(FILE *fp)
    {
        fclose(fp);
    }
};

struct uCloseDir {
    void operator()(DIR *dir)
    {
        closedir(dir);
    }
};

using uFD = std::unique_ptr<int, uFDClose>;
using uFile = std::unique_ptr<FILE, uFileClose>;
using uDir = std::unique_ptr<DIR, uCloseDir>;

} // namespace alloc

namespace init {

extern QList<QString> shellList;
extern QList<QString> scriptList;
extern QList<QString> pathList;

// disk sector size shift
#define SECTOR_SHIFT 9
// sector size (512 bytes)
#define SECTOR_SIZE (1 << SECTOR_SHIFT)

extern unsigned int kb_shift;
extern unsigned long HZ;

void global_init();

} // namespace init

namespace format {

enum SizeUnit {
    B, // 1024 ^ 0
    KB, // 1024 ^ 1
    MB, // 1024 ^ 2
    GB, // 1024 ^ 3
    TB, // 1024 ^ 4
    PB, // 1024 ^ 5
    EB // 1024 ^ 6
};

const char *const UnitSuffix[] = {"B", "K", "M", "G", "T", "P", "E"};
const char *const UnitSuffixExt[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB"};

QString formatUnit(QVariant size, SizeUnit base = B, int prec = 1, bool isSpeed = false);

enum HzUnit {
    KHz, // 10 ^ 3
    MHz, // 10 ^ 6
    GHz, // 10 ^ 9
    THz, // 10 ^ 12
    PHz, // 10 ^ 15
    EHz, // 10 ^ 18
    ZHz // 10 ^ 21
};
const char *const HzUnitSuffix[] = {"KHz", "MHz", "GHz", "THz", "PHz", "EHz", "ZHz"};
QString formatHz(quint32 freq, HzUnit base = KHz, int prec = 2);

} // namespace format

namespace error {

inline void print_errno(decltype(errno) e, const QString msg)
{
    qWarning() << QString("Error: [%1] %2, ").arg(e).arg(strerror(e)) << msg;
}

} // namespace error

namespace time {

inline const struct timeval operator-(const struct timeval &lhs, const struct timeval &rhs)
{
    struct timeval res {
    };
    timersub(&lhs, &rhs, &res);
    return res;
}
inline const struct timeval operator+(const struct timeval &lhs, const struct timeval &rhs)
{
    struct timeval res {
    };
    timeradd(&lhs, &rhs, &res);
    return res;
}
inline struct timeval &operator-=(struct timeval &lhs, const struct timeval &rhs)
{
    timersub(&lhs, &rhs, &lhs);
    return lhs;
}
inline struct timeval &operator+=(struct timeval &lhs, const struct timeval &rhs)
{
    timeradd(&lhs, &rhs, &lhs);
    return lhs;
}

} // namespace time

int  getStatusBarMaxWidth();
void displayShortcutHelpDialog(const QRect &rect);
void drawLoadingRing(QPainter &painter, int centerX, int centerY, int radius, int penWidth,
                     int loadingAngle, int rotationAngle, QColor foregroundColor,
                     double foregroundOpacity, QColor backgroundColor, double backgroundOpacity,
                     double percent);
void drawRing(QPainter &painter, int centerX, int centerY, int radius, int penWidth,
              int loadingAngle, int rotationAngle, QColor color, double opacity);

void openFilePathItem(const QString &path);
bool startWithHanzi(const QString &text);
} // namespace common

#endif
