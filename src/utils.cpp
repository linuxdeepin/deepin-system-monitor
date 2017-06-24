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
#include <QtX11Extras/QX11Info>
#include <X11/extensions/shape.h>
#include <fstream>
#include <qdiriterator.h>
#include <sstream>
#include <stdio.h>
#include <string>
#include <time.h>
#include <unordered_set>

namespace Utils {
    QMap<QString, QString> processDescriptions = getProcessDescriptions();

    QMap<QString, QString> getProcessDescriptions() {
        QMap<QString, QString> map;

        map["startdde"] =  "深度桌面环境-进程启动服务";
        map["dde-desktop"] =  "深度桌面环境-桌面";
        map["dde-polkit-agent"] =  "深度桌面环境-Polkit代理";
        map["dde-launcher"] =  "深度桌面环境-启动器";
        map["dde-dock"] =  "深度桌面环境-任务栏";
        map["dde-osd"] =  "深度桌面环境-屏幕显示";
        map["dde-system-daemon"] =  "深度桌面环境-系统守护进程";
        map["dde-session-daemon"] =  "深度桌面环境-会话守护进程";
        map["dde-session-initializer"] =  "深度桌面环境-会话初始化进程";
        map["dde-file-manager-daemon"] =  "深度文件管理器守护进程";
        map["dde-lockservice"] =  "深度桌面环境-锁屏服务";
        map["deepin-wm"] =  "深度窗口管理器";
        map["deepin-wm-switcher"] =  "深度窗口管理器切换器";
        map["deepin-notifications"] =  "深度系统通知";
        map["deepin-cloud-print-agent"] =  "深度云打印代理";
        map["deepin-menu"] =  "深度系统菜单服务";
        map["lastore-daemon"] =  "深度商店守护进程";
        map["bamfdaemon"] =  "窗口匹配守护进程";
        map["bamfdaemon-dbus-runner"] =  "窗口匹配守护进程DBus服务";
        map["ssh-agent"] =  "SSH代理";
        map["gvfsd"] =  "用户态虚拟文件系统守护进程";
        map["gvfsd-fuse"] =  "用户态虚拟文件系统守护进程Fuse服务器";
        map["lastore-session-helper"] =  "深度商店客户端助手进程";
        map["networkmanager"] =  "网络管理器";
        map["polkitd"] =  "PolicyKit DBus 服务";
        map["smbd"] =  "文件共享服务守护进程";
        map["xorg"] = "X服务";
        map["fcitx"] = "小企鹅输入法";
        map["fcitx-dbus-watcher"] = "小企鹅输入法 DBus 守护进程";
        map["sogou-qimpanel-watchdog"] = "搜狗输入法守护进程";
        map["sslocal"] = "ShadowSocks 本地客户端";
        map["pulseaudio"] = "声音服务";
        map["cupsd"] = "打印守护进程";
        map["lightdm"] = "简单显示管理器";
        map["systemd"] = "系统服务管理器";
        map["systemd-udevd"] = "系统服务管理器-设备管理守护进程";
        map["udisksd"] = "磁盘守护进程";
        map["accounts-daemon"] = "账户守护进程";
        map["dbus-daemon"] = "DBus 守护进程";
        map["mousearea"] = "鼠标事件守护进程";
        map["dconf-service"] = "DConf服务";
        map["gnome-keyring-daemon"] = "Gnome 密钥环守护进程";
        map["bluetoothd"] = "蓝牙守护进程";
        map["upowerd"] = "电源守护进程";
        map["modemmanager"] = "Modem设备管理器";
        map["applet.py"] = "系统打印托盘服务";
        map["chrome-sandbox"] = "Chrome浏览器沙盒";
        map["syndaemon"] = "Synaptics触摸板设备守护进程";

        return map;
    }

    QPixmap getDesktopFileIcon(std::string desktopFile, int iconSize)
    {

        std::ifstream in;
        in.open(desktopFile);
        QIcon defaultExecutableIcon = QIcon::fromTheme("application-x-executable");
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

        QPixmap pixmap = icon.pixmap(iconSize, iconSize);

        return pixmap;
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

    QString formatBandwidth(double v)
    {
        static const char* orders[] = { "KB/s", "MB/s", "GB/s", "TB/s" };

        return formatUnitSize(v, orders, sizeof(orders)/sizeof(orders[0]));
    }

    QString formatByteCount(double v)
    {
        static const char* orders[] = { "B", "KB", "MB", "GB", "TB" };

        return formatUnitSize(v, orders, sizeof(orders)/sizeof(orders[0]));
    }

    QString formatUnitSize(double v, const char** orders, int nb_orders)
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

    QString formatMillisecond(int millisecond)
    {
        if (millisecond / 1000 < 3600) {
            // At least need return 1 seconds.
            return QDateTime::fromTime_t(std::max(1, millisecond / 1000)).toUTC().toString("mm:ss");
        } else {
            return QDateTime::fromTime_t(millisecond / 1000).toUTC().toString("hh:mm:ss");
        }
    }

    QString getDisplayNameFromName(QString procName, std::string desktopFile, bool displayProcessName)
    {
        QString procname = procName.toLower();
        if (processDescriptions.contains(procname)) {
            if (displayProcessName) {
                return QString("%1    ( %2 )").arg(processDescriptions[procname], procName);
            } else {
                return processDescriptions[procname];
            }
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

    QString getImagePath(QString imageName)
    {
        QDir dir(qApp->applicationDirPath());
        dir.cdUp();

        return QDir(dir.filePath("image")).filePath(imageName);
    }

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
        processName = getProcessNameFromCmdLine(p->tid);
        if (processName == "") {
            // fallback on /proc/*//*stat program name value
            // bad because it is limited to 16 chars
            processName = p->cmd;
        }

        return processName;
    }

    QString getProcessNameFromCmdLine(const pid_t pid)
    {
        std::string cmdline = getProcessCmdline(pid).toStdString();

        if (cmdline.size()<1) {
            return "";
        }

        // Maintain linux paths.
        std::replace(cmdline.begin(),cmdline.end(),'\\','/');

        // Get cmdline arguments and first argument name.
        auto args = explode(cmdline, ' ');
        QString name = QFileInfo(QString::fromStdString(args[0])).fileName();

        // Get first argument that start with '/' if first argument is script program, such as 'python'.
        auto pos = SCRIPT_PROGRAM_MAP.find(name);
        if (pos != SCRIPT_PROGRAM_MAP.end() && args.size() > 1) {
            for (unsigned int i = 1; i < args.size(); i++) {
                QString argument = QString::fromStdString(args[i]);

                // Return first argument that start with '/'.
                if (argument.startsWith("/")) {
                    return QFileInfo(argument).fileName();
                }
            }

            for (unsigned int j = 1; j < args.size(); j++) {
                QString argument = QString::fromStdString(args[j]);

                // Return first argument that not start with '-'.
                if (!argument.startsWith("-")) {
                    return QFileInfo(argument).fileName();
                }
            }
        }

        return name;
    }

    QString getQrcPath(QString imageName)
    {
        return QString(":/image/%1").arg(imageName);
    }

    QString getQssPath(QString qssName)
    {
        return QString(":/qss/%1").arg(qssName);
    }

    bool fileExists(QString path)
    {
        QFileInfo check_file(path);

        // check if file exists and if yes: Is it really a file and no directory?
        return check_file.exists() && check_file.isFile();
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

    std::string getDesktopFileFromName(QString procName)
    {
        QDirIterator dir("/usr/share/applications", QDirIterator::Subdirectories);
        std::string desktopFile;

        // Convert to lower characters.
        QString procname = procName.toLower();
        
        // Replace "_" instead "-", avoid some applications desktop file can't found, such as, sublime text.
        procname.replace("_", "-");
        
        // Concat desktop file.
        QString processFilename = procname + ".desktop";

        if (GUI_BLACKLIST_MAP.find(procname) == GUI_BLACKLIST_MAP.end()) {
            while(dir.hasNext()) {
                if (dir.fileInfo().suffix() == "desktop") {
                    if (dir.fileName().toLower().contains(processFilename)) {
                        desktopFile = dir.filePath().toStdString();
                        break;
                    }
                }
                dir.next();
            }
        }

        return desktopFile;
    }

    double calculateCPUPercentage(const proc_t* before, const proc_t* after, const unsigned long long &prevCpuTime, const unsigned long long &cpuTime)
    {
        double totalCpuTime = cpuTime - prevCpuTime;
        unsigned long long processcpuTime = ((after->utime + after->stime) - (before->utime + before->stime));

        return (processcpuTime / totalCpuTime) * 100.0;
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

    /**
     * @brief getTotalCpuTime Read the data from /proc/stat and get the total time the cpu has been busy
     * @return The total cpu time
     */
    unsigned long long getTotalCpuTime(unsigned long long &workTime)
    {
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
        
        workTime = user + nice + system;

        // sum everything up (except guest and guestnice since they are already included
        // in user and nice, see http://unix.stackexchange.com/q/178045/20626)
        return user + nice + system + idle + iowait + irq + softirq + steal;
    }

    void addLayoutWidget(QLayout *layout, QWidget *widget)
    {
        layout->addWidget(widget);
        widget->show();
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

    void blurRect(WindowManager *windowManager, int widgetId, QRectF rect)
    {
        QVector<uint32_t> data;

        data << rect.x() << rect.y() << rect.width() << rect.height() << RECTANGLE_RADIUS << RECTANGLE_RADIUS;
        windowManager->setWindowBlur(widgetId, data);
    }

    void blurRects(WindowManager *windowManager, int widgetId, QList<QRectF> rects)
    {
        QVector<uint32_t> data;
        foreach (auto rect, rects) {
            data << rect.x() << rect.y() << rect.width() << rect.height() << RECTANGLE_RADIUS << RECTANGLE_RADIUS;
        }
        windowManager->setWindowBlur(widgetId, data);
    }

    void clearBlur(WindowManager *windowManager, int widgetId)
    {
        QVector<uint32_t> data;
        data << 0 << 0 << 0 << 0 << 0 << 0;
        windowManager->setWindowBlur(widgetId, data);
    }

    void drawLoadingRing(QPainter &painter,
                         int centerX,
                         int centerY,
                         int radius,
                         int penWidth,
                         int loadingAngle,
                         int rotationAngle,
                         QString foregroundColor,
                         double foregroundOpacity,
                         QString backgroundColor,
                         double backgroundOpacity,
                         double percent)
    {
        drawRing(painter, centerX, centerY, radius, penWidth, loadingAngle, rotationAngle, backgroundColor, backgroundOpacity);
        drawRing(painter, centerX, centerY, radius, penWidth, loadingAngle * percent, rotationAngle, foregroundColor, foregroundOpacity);
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

    void drawTooltipBackground(QPainter &painter, QRect rect, qreal opacity)
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

    void drawTooltipText(QPainter &painter, QString text, QString textColor, int textSize, QRectF rect)
    {
        setFontSize(painter, textSize);
        painter.setOpacity(1);
        painter.setPen(QPen(QColor(textColor)));
        painter.drawText(rect, Qt::AlignCenter, text);
    }

    void passInputEvent(int wid)
    {
        XRectangle* reponseArea = new XRectangle;
        reponseArea->x = 0;
        reponseArea->y = 0;
        reponseArea->width = 0;
        reponseArea->height = 0;

        XShapeCombineRectangles(QX11Info::display(), wid, ShapeInput, 0, 0, reponseArea ,1 ,ShapeSet, YXBanded);

        delete reponseArea;
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

    void setFontSize(QPainter &painter, int textSize)
    {
        QFont font = painter.font() ;
        font.setPointSize(textSize);
        painter.setFont(font);
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

        for (auto n:s) {
            if (n != c) {
                buff+=n;
            } else if (n == c && buff != "") {
                v.push_back(buff); buff = "";
            }
        }
        if (buff != "") {
            v.push_back(buff);
        }

        return v;
    }
}
