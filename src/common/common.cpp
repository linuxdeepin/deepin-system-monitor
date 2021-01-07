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

#include "common.h"
#include "stack_trace.h"
#include "hash.h"

#include <QPainter>
#include <QString>
#include <QtDBus>
#include <QDesktopServices>

namespace common {

int getStatusBarMaxWidth()
{
    // TODO: use more elegent way to calc bar width
    return 300;
}

void drawLoadingRing(QPainter &painter, int centerX, int centerY, int radius, int penWidth,
                     int loadingAngle, int rotationAngle, QColor foregroundColor,
                     double foregroundOpacity, QColor backgroundColor, double backgroundOpacity,
                     double percent)
{
    drawRing(painter, centerX, centerY, radius, penWidth, loadingAngle, rotationAngle,
             backgroundColor, backgroundOpacity);
    drawRing(painter, centerX, centerY, radius, penWidth, int(loadingAngle * percent), rotationAngle,
             foregroundColor, foregroundOpacity);
}

void drawRing(QPainter &painter, int centerX, int centerY, int radius, int penWidth,
              int loadingAngle, int rotationAngle, QColor color, double opacity)
{
    QRect drawingRect;

    drawingRect.setX(centerX - radius + penWidth);
    drawingRect.setY(centerY - radius + penWidth);
    drawingRect.setWidth(radius * 2 - penWidth * 2);
    drawingRect.setHeight(radius * 2 - penWidth * 2);

    painter.setOpacity(opacity);
    painter.setRenderHint(QPainter::Antialiasing);

    QPen pen(QBrush(QColor(color)), penWidth);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);

    int arcLengthApproximation = penWidth + penWidth / 3;
    painter.drawArc(drawingRect, 90 * 16 - arcLengthApproximation + rotationAngle * 16,
                    -loadingAngle * 16);
}

void setFontSize(QPainter &painter, int textSize)
{
    QFont font = painter.font();
    font.setPointSize(textSize);
    painter.setFont(font);
}

void openFilePathItem(const QString &path)
{
    bool result = QProcess::startDetached(QString("dde-file-manager --show-item \"%1\"").arg(path));
    if (!result) {
        QDesktopServices::openUrl(QUrl(path));
    }
}

namespace init {
QList<QString> shellList;
QList<QString> scriptList;
QList<QString> pathList;

static void init_shell_list()
{
    FILE *fp;
    fp = fopen("/etc/shells", "r");
    if (fp) {
        char buf[128] {};
        char *s;
        while ((s = fgets(buf, 128, fp))) {
            if (s[0] == '/') {
                auto sh = QLatin1String(basename(s));
                if (sh.endsWith('\n'))
                    sh.chop(1);
                if (!shellList.contains(sh)) {
                    shellList << sh;
                }
            }
        }
        fclose(fp);
    }
}

static void init_script_list()
{
    // fill scripting lang list (!!far from complete)
    scriptList << "/usr/bin/python";
    scriptList << "/usr/bin/perl";
    scriptList << "/usr/bin/php";
    scriptList << "/usr/bin/ruby";
}

static void init_path_list()
{
    // fill environment path
    auto paths = qgetenv("PATH");
    auto list = paths.split(':');
    if (list.size() > 0)
        for (auto path : list)
            pathList << path;
    else {
        // use default path
        pathList << "/bin";
        pathList << "/usr/bin";
    }
}

unsigned int kb_shift;
unsigned long HZ;

static void get_HZ()
{
    long ticks;

    if ((ticks = sysconf(_SC_CLK_TCK)) == -1) {
        perror("sysconf");
    }

    HZ = ulong(ticks);
}

static void get_kb_shift()
{
    int shift = 0;
    long size;

    /* One can also use getpagesize() to get the size of a page */
    if ((size = sysconf(_SC_PAGESIZE)) == -1) {
        perror("sysconf");
    }

    size >>= 10; /* Assume that a page has a minimum size of 1 kB */

    while (size > 1) {
        shift++;
        size >>= 1;
    }

    kb_shift = uint(shift);
}

void global_init()
{
    util::installCrashHandler();
    util::common::init_seed();

    init_shell_list();
    init_script_list();
    init_path_list();

    get_HZ();
    get_kb_shift();
}
} // namespace init

QString format::formatHz(quint32 freq, format::HzUnit base, int prec)
{
    int u = base;
    qreal v = freq;

    while (v > 1000. && u <= ZHz) {
        v /= 1000;
        u++;
    }

    return QString("%1%2").arg(v, 0, 'f', prec).arg(HzUnitSuffix[u]);
} // ::format::formatHz

int io::path_scanf(const char *path, const char *fmt, ...)
{
    using namespace alloc;
    using namespace error;

    FILE *fp;
    va_list fmt_va;
    uFile fPtr;

    errno = 0;

    fp = fopen(path, "r");
    if (!fp) {
        print_errno(errno, QString("open %1 failed").arg(path));
        return -EINVAL;
    }
    fPtr.reset(fp);

    va_start(fmt_va, fmt);
    int nr = vfscanf(fp, fmt, fmt_va);
    va_end(fmt_va);

    if (ferror(fp)) {
        print_errno(errno, QString("read %1 failed").arg(path));
    }

    return nr;
}

bool io::read_path_s32(const char *path, int32_t *value)
{
    bool ok = true;

    int32_t swp = 0;
    int nr = path_scanf(path, "%d", &swp);
    if (nr == 1) {
        *value = swp;
        return ok;
    }

    return !ok;
}

bool io::read_path_u32(const char *path, uint32_t *value)
{
    bool ok = true;

    uint32_t swp = 0;
    int nr = path_scanf(path, "%u", &swp);
    if (nr == 1) {
        *value = swp;
        return ok;
    }

    return !ok;
}

bool io::read_path_s64(const char *path, int64_t *value)
{
    bool ok = true;

    int64_t swp = 0;
    int nr = path_scanf(path, "%lld", &swp);
    if (nr == 1) {
        *value = swp;
        return ok;
    }

    return !ok;
}

bool io::read_path_u64(const char *path, uint64_t *value)
{
    bool ok = true;

    uint64_t swp = 0;
    int nr = path_scanf(path, "%llu", &swp);
    if (nr == 1) {
        *value = swp;
        return ok;
    }

    return !ok;
}

QString format::formatUnit(QVariant size, format::SizeUnit base, int prec, bool isSpeed)
{
    int u = base;
    if (!size.canConvert(QMetaType::Double))
        return {};
    qreal v = size.toReal();

    while (v > 1024. && u <= EB) {
        v /= 1024;
        u++;
    }

    if (isSpeed) {
        return QString("%1 %2%3").arg(v, 0, 'f', prec).arg(UnitSuffixExt[u]).arg("/s");
    } else {
        return QString("%1 %2").arg(v, 0, 'f', prec).arg(UnitSuffix[u]);
    }
}

// ::format::formatUnit
} // namespace common
