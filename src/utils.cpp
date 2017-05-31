/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2017 Deepin, Inc.
 *               2011 ~ 2017 Wang Yong
 *
 * Author:     Wang Yong <wangyong@deepin.com>
 * Maintainer: Wang Yong <wangyong@deepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "hashqstring.h"
#include "utils.h"
#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFontMetrics>
#include <QIcon>
#include <QLayout>
#include <QPainter>
#include <QPixmap>
#include <QStandardPaths>
#include <QString>
#include <QWidget>
#include <QtMath>
#include <fstream>
#include <qdiriterator.h>
#include <sstream>
#include <stdio.h>
#include <string>
#include <time.h>
#include <unordered_set>
#include <X11/extensions/shape.h>
#include <QtX11Extras/QX11Info>

namespace Utils {
    QString getImagePath(QString imageName)
    {
        QDir dir(qApp->applicationDirPath());
        dir.cdUp();

        return QDir(dir.filePath("image")).filePath(imageName);
    }

    QString getQssPath(QString qssName)
    {
        return QString(":/qss/%1").arg(qssName);
    }

    void applyQss(QWidget *widget, QString qssName)
    {
        QFile file(getQssPath(qssName));
        file.open(QFile::ReadOnly);
        QTextStream filetext(&file);
        QString stylesheet = filetext.readAll();
        widget->setStyleSheet(stylesheet);
        file.close();
    }

    QString getQrcPath(QString imageName)
    {
        return QString(":/image/%1").arg(imageName);
    }

    QSize getRenderSize(int fontSize, QString string)
    {
        QFont font;
        font.setPointSize(fontSize);
        QFontMetrics fm(font);

        int width = 0;
        int height = 0;
        foreach (auto line, string.split("\n")) {
            int lineWidth = fm.width(line);
            int lineHeight = fm.height();

            if (lineWidth > width) {
                width = lineWidth;
            }
            height += lineHeight;
        }

        return QSize(width, height);
    }

    void setFontSize(QPainter &painter, int textSize)
    {
        QFont font = painter.font() ;
        font.setPointSize(textSize);
        painter.setFont(font);
    }

    void removeChildren(QWidget *widget)
    {
        qDeleteAll(widget->children());
    }

    void removeLayoutChild(QLayout *layout, int index)
    {
        QLayoutItem *item = layout->itemAt(index);
        if (item != 0) {
            QWidget *widget = item->widget();
            if (widget != NULL) {
                widget->hide();
                widget->setParent(NULL);
                layout->removeWidget(widget);
            }
        }
    }

    void addLayoutWidget(QLayout *layout, QWidget *widget)
    {
        layout->addWidget(widget);
        widget->show();
    }

    QString formatMillisecond(int millisecond)
    {
        if (millisecond / 1000 < 3600) {
            // At least need return 1 seconds.
            return QDateTime::fromTime_t(std::max(1, millisecond / 1000)).toUTC().toString("mm:ss");
        } else {
            return QDateTime::fromTime_t(millisecond / 1000).toUTC().toString("hh:mm:ss");
        }
    }

    bool fileExists(QString path)
    {
        QFileInfo check_file(path);

        // check if file exists and if yes: Is it really a file and no directory?
        return check_file.exists() && check_file.isFile();
    }

    qreal easeInOut(qreal x)
    {
        return (1 - qCos(M_PI * x)) / 2;
    }

    qreal easeInQuad(qreal x)
    {
        return qPow(x, 2);
    }

    qreal easeOutQuad(qreal x)
    {
        return -1 * qPow(x - 1, 2) + 1;
    }

    qreal easeInQuint(qreal x)
    {
        return qPow(x, 5);
    }

    qreal easeOutQuint(qreal x)
    {
        return qPow(x - 1, 5) + 1;
    }

    QString convertSizeUnit(long bytes, QString unitSuffix)
    {
        if (bytes < qPow(2, 10)) {
            return QString("%1 %2").arg(bytes).arg(unitSuffix);
        } else if (bytes < qPow(2, 20)) {
            return QString("%1 K%2").arg(QString::number(qreal(bytes) / qPow(2, 10), 'f', 1)).arg(unitSuffix);
        } else if (bytes < qPow(2, 30)) {
            return QString("%1 M%2").arg(QString::number(qreal(bytes) / qPow(2, 20), 'f', 1)).arg(unitSuffix);
        } else if (bytes < qPow(2, 40)) {
            return QString("%1 G%2").arg(QString::number(qreal(bytes) / qPow(2, 30), 'f', 1)).arg(unitSuffix);
        } else if (bytes < qPow(2, 50)) {
            return QString("%1 T%2").arg(QString::number(qreal(bytes) / qPow(2, 40), 'f', 1)).arg(unitSuffix);
        }

        return QString::number(bytes);
    }

    void drawRing(QPainter &painter, int centerX, int centerY, int radius, int penWidth, int loadingAngle, int rotationAngle, QString color, double opacity)
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
        painter.drawArc(drawingRect, 90 * 16 - arcLengthApproximation + rotationAngle * 16, -loadingAngle * 16);
    }

    void drawLoadingRing(QPainter &painter, int centerX, int centerY, int radius, int penWidth, int loadingAngle, int rotationAngle, QString color, double backgroundOpacity, double percent)
    {
        drawRing(painter, centerX, centerY, radius, penWidth, loadingAngle, rotationAngle, color, backgroundOpacity);
        drawRing(painter, centerX, centerY, radius, penWidth, loadingAngle * percent, rotationAngle, color, 1);
    }

    QString formatByteCount(double v, const char** orders, int nb_orders)
    {
        int order = 0;
        while (v >= 1024 && order + 1 < nb_orders) {
            order++;
            v  = v/1024;
        }
        char buffer1[30];
        snprintf(buffer1, sizeof(buffer1), "%.1lf %s", v, orders[order]);

        return QString(buffer1);
    }

    QString formatByteCount(double v)
    {
        static const char* orders[] = { "B", "KB", "MB", "GB" };

        return formatByteCount(v, orders, sizeof(orders)/sizeof(orders[0]));
    }

    QString formatBandwidth(double v)
    {
        static const char* orders[] = { "KB/s", "MB/s", "GB/s" };

        return formatByteCount(v, orders, sizeof(orders)/sizeof(orders[0]));
    }

    bool getProcPidIO(int pid, ProcPidIO &io ) {
        std::stringstream ss;
        ss << "/proc/" << pid << "/io";
        std::ifstream ifs( ss.str().c_str() );
        if ( ifs.good() ) {
            while ( ifs.good() && !ifs.eof() ) {
                std::string s;
                getline( ifs, s );
                unsigned long t;
                if ( sscanf( s.c_str(), "rchar: %lu", &t ) == 1 ) io.rchar = t;
                else if ( sscanf( s.c_str(), "wchar: %lu", &t ) == 1 ) io.wchar = t;
                else if ( sscanf( s.c_str(), "syscr: %lu", &t ) == 1 ) io.syscr = t;
                else if ( sscanf( s.c_str(), "syscw: %lu", &t ) == 1 ) io.syscw = t;
                else if ( sscanf( s.c_str(), "read_bytes: %lu", &t ) == 1 ) io.read_bytes = t;
                else if ( sscanf( s.c_str(), "write_bytes: %lu", &t ) == 1 ) io.write_bytes = t;
                else if ( sscanf( s.c_str(), "cancelled_write_bytes: %lu", &t ) == 1 ) io.cancelled_write_bytes = t;
            }
        } else {
            return false;
        }

        return true;
    }

    // from http://stackoverflow.com/questions/24581908/c-lstat-on-proc-pid-exe
    /**
     * @brief exe_of Obtain the executable path a process is running
     * @param pid: Process ID
     * @param sizeptr: If specified, the allocated size is saved here
     * @param lenptr: If specified, the path length is saved here
     * @return the dynamically allocated pointer to the path, or NULL with errno set if an error occurs.
     */
    char* exe_of(const pid_t pid, size_t *const sizeptr, size_t *const lenptr)
    {
        char   *exe_path = NULL;
        size_t  exe_size = 1024;
        ssize_t exe_used;
        char    path_buf[64];
        unsigned int path_len;

        path_len = snprintf(path_buf, sizeof path_buf, "/proc/%ld/exe", (long)pid);
        if (path_len < 1 || path_len >= sizeof path_buf) {
            errno = ENOMEM;
            return NULL;
        }

        while (1) {

            exe_path = (char*)malloc(exe_size);
            if (!exe_path) {
                errno = ENOMEM;
                return NULL;
            }

            exe_used = readlink(path_buf, exe_path, exe_size - 1);
            if (exe_used == (ssize_t)-1) {
                free(exe_path);
                return NULL;
            }

            if (exe_used < (ssize_t)1) {
                /* Race condition? */
                errno = ENOENT;
                free(exe_path);
                return NULL;
            }

            if (exe_used < (ssize_t)(exe_size - 1))
                break;

            free(exe_path);
            exe_size += 1024;
        }

        /* Try reallocating the exe_path to minimum size.
         * This is optional, and can even fail without
         * any bad effects. */
        {
            char *temp;

            temp = (char*)realloc(exe_path, exe_used + 1);
            if (temp) {
                exe_path = temp;
                exe_size = exe_used + 1;
            }
        }

        if (sizeptr)
            *sizeptr = exe_size;

        if (lenptr)
            *lenptr = exe_used;

        exe_path[exe_used] = '\0';
        return exe_path;
    }

    /**
     * @brief explode Explode a string based on
     * @param s The string to explode
     * @param c The seperator to use
     * @return A vector of the exploded string
     */
    const std::vector<std::string> explode(const std::string& s, const char& c)
    {
        std::string buff{""};
        std::vector<std::string> v;

        for(auto n:s)
        {
            if(n != c) buff+=n; else
                if(n == c && buff != "") { v.push_back(buff); buff = ""; }
        }
        if(buff != "") v.push_back(buff);

        return v;
    }

    /**
     * @brief getProcessNameFromCmdLine Get the name of the process from its PID
     * @param pid the pid of the process to get the name for
     * @return The name of the process
     */
    QString getProcessNameFromCmdLine(const pid_t pid)
    {
        std::string cmdline = getProcessCmdline(pid).toStdString();

        if (cmdline.size()<1) {
            return "";
        }

        // maintain linux paths
        std::replace(cmdline.begin(),cmdline.end(),'\\','/');

        auto args = explode(cmdline,' ');
        QString name = QFileInfo(QString::fromStdString(args[0])).fileName();
        // replace the name of some processes with their first argument, eg, python, php, ruby etc
        // QString does not support hash
        static std::unordered_set<QString> nameMap({"python", "python3", "ruby", "php", "perl"});
        auto pos = nameMap.find(name);
        if (pos != nameMap.end()) {
            return QFileInfo(QString::fromStdString(args[1])).fileName();
        } else {
            return name;
        }
    }

    /**
     * @brief getProcessCmdline Get the command line that the process was executed with from its PID
     * @param pid The pid of the process to get
     * @return The command line that the process was run from
     */
    QString getProcessCmdline(pid_t pid)
    {
        std::string temp;
        try {
            std::fstream fs;
            fs.open("/proc/"+std::to_string((long)pid)+"/cmdline", std::fstream::in);
            std::getline(fs,temp);
            fs.close();
        } catch(std::ifstream::failure e) {
            return "FAILED TO READ PROC";
        }

        // change \0 to ' '
        std::replace(temp.begin(),temp.end(),'\0',' ');

        if (temp.size()<1) {
            return "";
        }
        return QString::fromStdString(temp);
    }

    /**
     * @brief getProcessName Get the name of the process from a proc_t
     * @param p The proc_t structure to use for getting the name of the process
     * @return
     */
    QString getProcessName(proc_t* p)
    {
        QString processName = "ERROR";
        char* temp = NULL;//exe_of(p->tid,NULL,NULL);
        // if exe_of fails here, it will be because permission is denied
        if (temp!=NULL) {
            processName = QFileInfo(temp).fileName();
            free(temp);
        } else {
            // next try to read from /proc/*//*cmdline
            processName = getProcessNameFromCmdLine(p->tid);
            if (processName=="") {
                // fallback on /proc/*//*stat program name value
                // bad because it is limited to 16 chars
                processName = p->cmd;
            }
        }
        return processName;
    }

    /**
     * @brief getTotalCpuTime Read the data from /proc/stat and get the total time the cpu has been busy
     * @return The total cpu time
     */
    unsigned long long getTotalCpuTime()
    {
        // from https://github.com/scaidermern/top-processes/blob/master/top_proc.c#L54
        FILE* file = fopen("/proc/stat", "r");
        if (file == NULL) {
            perror("Could not open stat file");
            return 0;
        }

        char buffer[1024];
        unsigned long long user = 0, nice = 0, system = 0, idle = 0;
        // added between Linux 2.5.41 and 2.6.33, see man proc(5)
        unsigned long long iowait = 0, irq = 0, softirq = 0, steal = 0, guest = 0, guestnice = 0;

        char* ret = fgets(buffer, sizeof(buffer) - 1, file);
        if (ret == NULL) {
            perror("Could not read stat file");
            fclose(file);
            return 0;
        }
        fclose(file);

        sscanf(buffer,
               "cpu  %16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu",
               &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal, &guest, &guestnice);

        // sum everything up (except guest and guestnice since they are already included
        // in user and nice, see http://unix.stackexchange.com/q/178045/20626)
        return user + nice + system + idle + iowait + irq + softirq + steal;
    }

    /**
     * @brief calculateCPUPercentage
     * @param before - old proc_t of the process
     * @param after - new proc_t of the process
     * @param cpuTime - the last total cpu time measurement
     * @return The cpu percentage of the process
     */
    double calculateCPUPercentage(const proc_t* before, const proc_t* after, const unsigned long long &cpuTime)
    {
        double cpuTimeA = getTotalCpuTime() - cpuTime;
        unsigned long long processcpuTime = ((after->utime + after->stime)
                                             - (before->utime + before->stime));
        /// TODO: GSM has an option to divide by # cpus
        return (processcpuTime / cpuTimeA) * 100.0 * sysconf(_SC_NPROCESSORS_CONF);
    }

    bool isGuiApp(QString procName)
    {
        // search /usr/share/applications for the desktop file that corresponds to the proc and get its locale name.
        QDirIterator dir("/usr/share/applications", QDirIterator::Subdirectories);
        std::string desktopFile;
        while(dir.hasNext()) {
            if (dir.fileInfo().suffix() == "desktop") {
                if (dir.fileName().toLower().contains(procName.toLower())) {
                    desktopFile = dir.filePath().toStdString();
                    break;
                }
            }
            dir.next();
        }

        return desktopFile.size() != 0;
    }
    
    QString getDisplayNameFromName(QString procName)
    {
        // search /usr/share/applications for the desktop file that corresponds to the proc and get its locale name.
        QDirIterator dir("/usr/share/applications", QDirIterator::Subdirectories);
        std::string desktopFile;
        while(dir.hasNext()) {
            if (dir.fileInfo().suffix() == "desktop") {
                if (dir.fileName().toLower().contains(procName.toLower())) {
                    desktopFile = dir.filePath().toStdString();
                    break;
                }
            }
            dir.next();
        }

        if (desktopFile.size() == 0) {
            return procName;
        }

        std::ifstream in;
        in.open(desktopFile);
        QString displayName = procName;
        while(!in.eof()) {
            std::string line;
            std::getline(in,line);

            QString lineContent = QString::fromStdString(line);

            QString localNameFlag = QString("Name[%1]=").arg(QLocale::system().name());
            QString nameFlag = "Name=";
            QString genericNameFlag = QString("GenericName[%1]=").arg(QLocale::system().name());

            // qDebug() << lineContent;

            if (lineContent.startsWith(localNameFlag)) {
                displayName = lineContent.remove(0, localNameFlag.size());

                break;
            } else if (lineContent.startsWith(genericNameFlag)) {
                displayName = lineContent.remove(0, genericNameFlag.size());

                break;
            } else if (lineContent.startsWith(nameFlag)) {
                displayName = lineContent.remove(0, nameFlag.size());

                continue;
            } else {
                continue;
            }
        }
        in.close();

        return displayName;
    }

    /**
     * @brief getProcessIconFromName Get the icon for a process given its name
     * @param procname The name of the process
     * @return The process' icon or the default executable icon if none was found
     */
    QPixmap getProcessIconFromName(QString procName, QMap<QString, QPixmap> *processIconMapCache)
    {
        // check we havent already got the icon in the cache
        if (processIconMapCache->contains(procName)) {
            return processIconMapCache->value(procName);
        }

        // search /usr/share/applications for the desktop file that corresponds to the proc and get its icon
        QDirIterator dir("/usr/share/applications", QDirIterator::Subdirectories);
        std::string desktopFile;
        QIcon defaultExecutableIcon = QIcon::fromTheme("application-x-executable");
        while(dir.hasNext()) {
            if (dir.fileInfo().suffix() == "desktop") {
                if (dir.fileName().toLower().contains(procName.toLower())) {
                    desktopFile = dir.filePath().toStdString();
                    break;
                }
            }
            dir.next();
        }

        if (desktopFile.size() == 0) {
            QPixmap pixmap = defaultExecutableIcon.pixmap(24, 24);
            (*processIconMapCache)[procName] = pixmap;

            return pixmap;
        }

        std::ifstream in;
        in.open(desktopFile);
        QIcon icon = defaultExecutableIcon;
        QString iconName;
        while(!in.eof()) {
            std::string line;
            std::getline(in,line);
            iconName = QString::fromStdString(line);
            if (iconName.startsWith("Icon=")) {
                iconName.remove(0,5); // remove the first 5 chars
            } else {
                continue;
            }

            if (iconName.contains("/")) {
                // this is probably a path to the file, use that instead of the theme icon name
                icon = QIcon(iconName);
            } else {
                icon = QIcon::fromTheme(iconName,defaultExecutableIcon);
                break;
            }
        }
        in.close();

        QPixmap pixmap = icon.pixmap(24, 24);
        (*processIconMapCache)[procName] = pixmap;

        return pixmap;
    }
}

void Utils::passInputEvent(int wid)
{
    XRectangle* reponseArea = new XRectangle;
    reponseArea->x = 0;
    reponseArea->y = 0;
    reponseArea->width = 0;
    reponseArea->height = 0;

    XShapeCombineRectangles(QX11Info::display(), wid, ShapeInput, 0, 0, reponseArea ,1 ,ShapeSet, YXBanded);

    delete reponseArea;
}

void Utils::drawTooltipBackground(QPainter &painter, QRect rect, qreal opacity)
{
    painter.setOpacity(opacity);
    QPainterPath path;
    path.addRoundedRect(QRectF(rect), RECTANGLE_RADIUS, RECTANGLE_RADIUS);
    painter.fillPath(path, QColor("#F5F5F5"));

    QPen pen(QColor("#000000"));
    painter.setOpacity(0.04);
    pen.setWidth(1);
    painter.setPen(pen);
    painter.drawPath(path);
}    

void Utils::drawTooltipText(QPainter &painter, QString text, QString textColor, int textSize, QRectF rect)
{
    Utils::setFontSize(painter, textSize);
    painter.setOpacity(1);
    painter.setPen(QPen(QColor(textColor)));
    painter.drawText(rect, Qt::AlignCenter, text);
}    

void Utils::blurRect(WindowManager *windowManager, int widgetId, QRectF rect)
{
    QVector<uint32_t> data;
    
    data << rect.x() << rect.y() << rect.width() << rect.height() << RECTANGLE_RADIUS << RECTANGLE_RADIUS;
    windowManager->setWindowBlur(widgetId, data);
}    

void Utils::blurRects(WindowManager *windowManager, int widgetId, QList<QRectF> rects)
{
    QVector<uint32_t> data;
    foreach (auto rect, rects) {
        data << rect.x() << rect.y() << rect.width() << rect.height() << RECTANGLE_RADIUS << RECTANGLE_RADIUS;
    }
    windowManager->setWindowBlur(widgetId, data);
}    

void Utils::clearBlur(WindowManager *windowManager, int widgetId)
{
    QVector<uint32_t> data;
    data << 0 << 0 << 0 << 0 << 0 << 0;
    windowManager->setWindowBlur(widgetId, data);
}


