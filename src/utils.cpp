/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2018 Deepin, Inc.
 *               2011 ~ 2018 Wang Yong
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

#include <stdio.h>
#include <time.h>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_set>

#include <DDesktopEntry>
#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFontMetrics>
#include <QIcon>
#include <QImageReader>
#include <QLayout>
#include <QPainter>
#include <QPixmap>
#include <QStandardPaths>
#include <QString>
#include <QWidget>
#include <QtDBus>
#include <QtMath>
#include <QtX11Extras/QX11Info>

#include <X11/extensions/shape.h>

#include "hashqstring.h"
#include "utils.h"

DCORE_USE_NAMESPACE

namespace Utils {
static QMap<QString, QString> desktopfileMaps = getDesktopfileMap();

QMap<QString, QString> getDesktopfileMap()
{
    QMap<QString, QString> map;

    map["/opt/kingsoft/wps-office/office6/wps"] = "/usr/share/applications/wps-office-wps.desktop";
    map["/opt/kingsoft/wps-office/office6/wpp"] = "/usr/share/applications/wps-office-wpp.desktop";
    map["/opt/kingsoft/wps-office/office6/et"] = "/usr/share/applications/wps-office-et.desktop";

    return map;
}

QList<xcb_window_t> getTrayWindows()
{
    QDBusInterface busInterface("com.deepin.dde.TrayManager", "/com/deepin/dde/TrayManager",
                                "org.freedesktop.DBus.Properties", QDBusConnection::sessionBus());
    QDBusMessage reply = busInterface.call("Get", "com.deepin.dde.TrayManager", "TrayIcons");
    QVariant v = reply.arguments().first();
    const QDBusArgument &argument = v.value<QDBusVariant>().variant().value<QDBusArgument>();

    argument.beginArray();
    QList<xcb_window_t> xids;
    while (!argument.atEnd()) {
        xcb_window_t xid;

        argument >> xid;
        xids << xid;
    }
    argument.endArray();

    return xids;
}

int getStatusBarMaxWidth()
{
    // TODO: use more elegent way to calc bar width
    return 300;
}

int getWindowPid(DWindowManager *windowManager, xcb_window_t window)
{
    int windowPid = -1;

    QString flatpakAppid = windowManager->getWindowFlatpakAppid(window);
    if (flatpakAppid != "") {
        PROCTAB *proc =
            openproc(PROC_FILLMEM | PROC_FILLSTAT | PROC_FILLSTATUS | PROC_FILLUSR | PROC_FILLCOM);
        static proc_t proc_info;
        memset(&proc_info, 0, sizeof(proc_t));

        std::map<int, proc_t> processes;
        while (readproc(proc, &proc_info) != NULL) {
            processes[proc_info.tid] = proc_info;
        }
        closeproc(proc);

        for (auto &p : processes) {
            int pid = (&p.second)->tid;

            QString flatpakAppidEnv = Utils::getProcessEnvironmentVariable(pid, "FLATPAK_APPID");
            if (flatpakAppidEnv == flatpakAppid) {
                QString tempName = windowManager->getWindowName(window);

                if (windowPid < pid) {
                    windowPid = pid;
                }
            }
        }
    } else {
        windowPid = windowManager->getWindowPid(window);
    }

    return windowPid;
}

long getProcessMemory(QString cmdline, long residentMemroy, long shareMemory)
{
    if (cmdline.startsWith("/usr/lib/virtualbox/VirtualBox") && cmdline.contains("--startvm")) {
        return residentMemroy * sysconf(_SC_PAGESIZE);
    } else {
        return (residentMemroy - shareMemory) * sysconf(_SC_PAGESIZE);
    }
}

QPixmap getDesktopFileIcon(std::string desktopFile, int iconSize)
{
    std::ifstream in;
    in.open(desktopFile);
    QIcon defaultExecutableIcon = QIcon::fromTheme("application-x-executable");
    QIcon icon;
    QString iconName;
    bool foundIconLine = false;

    while (!in.eof()) {
        std::string line;
        std::getline(in, line);
        iconName = QString::fromStdString(line);

        if (iconName.startsWith("Icon")) {
            iconName = iconName.split("=").last().simplified();
            foundIconLine = true;
        } else {
            continue;
        }

        if (iconName.contains("/")) {
            // this is probably a path to the file, use that instead of the theme icon name
            icon = QIcon(iconName);
        } else {
            icon = QIcon::fromTheme(iconName, defaultExecutableIcon);
            break;
        }
    }
    in.close();

    // Use default icon instead, if not found "Icon=" line in desktop file.
    if (!foundIconLine) {
        icon = defaultExecutableIcon;
    }

    return icon.pixmap(iconSize, iconSize);
}

QPixmap getProcessIcon(int pid, std::string desktopFile,
                       QScopedPointer<FindWindowTitle> &findWindowTitle, int iconSize)
{
    QPixmap icon;
    QString flatpakAppidEnv = Utils::getProcessEnvironmentVariable(pid, "FLATPAK_APPID");
    if (flatpakAppidEnv != "") {
        QIcon flatpakAppIcon = QIcon(getFlatpakAppIcon(flatpakAppidEnv));
        icon = flatpakAppIcon.pixmap(iconSize, iconSize);
    } else if (desktopFile.size() == 0) {
        qreal devicePixelRatio = qApp->devicePixelRatio();
        icon = findWindowTitle->getWindowIcon(findWindowTitle->getWindow(pid),
                                              int(iconSize * devicePixelRatio));
        icon.setDevicePixelRatio(devicePixelRatio);
    } else {
        icon = getDesktopFileIcon(desktopFile, iconSize);
    }

    return icon;
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

QString getDisplayNameFromName(QString procName, std::string desktopFile, bool displayProcessName)
{
    QString procname = procName.toLower();
    //    if (processDescriptions.contains(procname)) {
    //        if (displayProcessName) {
    //            return QString("%1    ( %2 )").arg(processDescriptions[procname], procName);
    //        } else {
    //            return processDescriptions[procname];
    //        }
    //    }

    if (desktopFile.size() == 0) {
        return procName;
    }

    DDesktopEntry entry(QString::fromStdString(desktopFile));

    if (entry.contains("X-Deepin-Vendor") && entry.stringValue("X-Deepin-Vendor") == "deepin")
        if (!entry.genericName().isEmpty())
            return entry.genericName();

    if (!entry.name().isEmpty()) {
        return entry.name();
    }

    if (entry.contains("X-Deepin-Vendor") && entry.stringValue("X-Deepin-Vendor") == "deepin")
        if (!entry.stringValue("GenericName").isEmpty()) {
            return entry.stringValue("GenericName");
        }

    if (!entry.stringValue("Name").isEmpty()) {
        return entry.stringValue("Name");
    }

    return procName;
}

QString getProcessEnvironmentVariable(pid_t pid, QString environmentName)
{
    std::string temp;
    try {
        std::fstream fs;
        fs.open("/proc/" + std::to_string(long(pid)) + "/environ", std::fstream::in);
        std::getline(fs, temp);
        fs.close();
    } catch (std::ifstream::failure &e) {
        Q_UNUSED(e)
        return "FAILED TO READ PROC";
    }

    // change \0 to ' '
    std::replace(temp.begin(), temp.end(), '\0', '\n');

    if (temp.size() < 1) {
        return "";
    }

    foreach (auto environmentVariable, QString::fromStdString(temp).trimmed().split("\n")) {
        if (environmentVariable.startsWith(QString("%1=").arg(environmentName))) {
            return environmentVariable.remove(0, QString("%1=").arg(environmentName).length());
        }
    }

    return "";
}

QString getProcessCmdline(pid_t pid)
{
    std::string temp;
    try {
        std::fstream fs;
        fs.open("/proc/" + std::to_string(long(pid)) + "/cmdline", std::fstream::in);
        std::getline(fs, temp);
        fs.close();
    } catch (std::ifstream::failure &e) {
        Q_UNUSED(e)
        return "FAILED TO READ PROC";
    }

    // change \0 to ' '
    std::replace(temp.begin(), temp.end(), '\0', ' ');

    if (temp.size() < 1) {
        return "";
    }

    return QString::fromStdString(temp).trimmed();
}

/**
 * @brief getProcessName Get the name of the process from a proc_t
 * @param p The proc_t structure to use for getting the name of the process
 * @return
 */
QString getProcessName(proc_t *p, QString cmdline)
{
    if (cmdline.startsWith("c:\\")) {
        // Return exe name if process is wine program.
        QStringList cmdlinePaths = cmdline.split("\\");
        return cmdlinePaths[cmdlinePaths.length() - 1];
    } else {
        QString processName = "ERROR";
        processName = getProcessNameFromCmdLine(p->tid);
        if (processName == "") {
            // fallback on /proc/*//*stat program name value
            // bad because it is limited to 16 chars
            processName = p->cmd;
        }

        return processName;
    }
}

QString getProcessNameFromCmdLine(const pid_t pid)
{
    std::string cmdline = getProcessCmdline(pid).toStdString();

    if (cmdline.size() < 1) {
        return "";
    }

    // Maintain linux paths.
    std::replace(cmdline.begin(), cmdline.end(), '\\', '/');

    // Get cmdline arguments and first argument name.
    auto args = explode(cmdline, ' ');
    QString name = QFileInfo(QString::fromStdString(args[0])).fileName();

    // Get first argument that start with '/' if first argument is script program, such as
    // 'python'.
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

std::string getProcessDesktopFile(int pid, QString name, QString cmdline,
                                  QMap<int, int> trayProcessMap)
{
    std::string desktopFile;
    if (trayProcessMap.contains(pid)) {
        desktopFile =
            Utils::getProcessEnvironmentVariable(pid, "GIO_LAUNCHED_DESKTOP_FILE").toStdString();

        // Find desktop file from process name if found environ variable
        // 'GIO_LAUNCHED_DESKTOP_FILE' failed from tray process.
        if (desktopFile.size() == 0) {
            desktopFile = getDesktopFileFromName(pid, name, cmdline);
        }
    } else {
        desktopFile = getDesktopFileFromName(pid, name, cmdline);
    }

    return desktopFile;
}

QString getQrcPath(QString imageName)
{
    return QString(":/image/%1").arg(imageName);
}

QDir getFlatpakAppPath(QString flatpakAppid)
{
    QProcess whichProcess;
    QString exec = "flatpak";
    QStringList params;
    params << "info";
    params << flatpakAppid;
    whichProcess.start(exec, params);
    whichProcess.waitForFinished();
    QString output(whichProcess.readAllStandardOutput());

    return QDir(output.split("Location:")[1].split("\n")[0].simplified());
}

QString getFlatpakAppIcon(QString flatpakAppid)
{
    QString exec = "flatpak";

    QStringList params;
    params << "info";
    params << flatpakAppid;

    // set the LANGUAGE env so that the output is fixed to be English,
    // or below code may break in other locales like pt_BR.
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("LANGUAGE", "en_US");

    QProcess whichProcess;
    whichProcess.setProcessEnvironment(env);
    whichProcess.start(exec, params);
    whichProcess.waitForFinished();
    QString output(whichProcess.readAllStandardOutput());

    const QString dirPath = output.split("Location:")[1].split("\n")[0].simplified();

    QDir flatpakDir = QDir(dirPath);
    flatpakDir.cd("export");
    flatpakDir.cd("share");
    flatpakDir.cd("icons");
    flatpakDir.cd("hicolor");
    flatpakDir.cd("scalable");
    flatpakDir.cd("apps");

    const QString appID = flatpakAppid.split("app/")[1].split("/")[0];
    return flatpakDir.filePath(QString("%1.svg").arg(appID));
}

bool fileExists(QString path)
{
    QFileInfo check_file(path);

    // check if file exists and if yes: Is it really a file and no directory?
    return check_file.exists() && check_file.isFile();
}

bool getProcPidIO(int pid, ProcPidIO &io)
{
    std::stringstream ss;
    ss << "/proc/" << pid << "/io";
    std::ifstream ifs(ss.str().c_str());
    if (ifs.good()) {
        while (ifs.good() && !ifs.eof()) {
            std::string s;
            getline(ifs, s);
            unsigned long t;
            if (sscanf(s.c_str(), "rchar: %lu", &t) == 1)
                io.rchar = t;
            else if (sscanf(s.c_str(), "wchar: %lu", &t) == 1)
                io.wchar = t;
            else if (sscanf(s.c_str(), "syscr: %lu", &t) == 1)
                io.syscr = t;
            else if (sscanf(s.c_str(), "syscw: %lu", &t) == 1)
                io.syscw = t;
            else if (sscanf(s.c_str(), "read_bytes: %lu", &t) == 1)
                io.read_bytes = t;
            else if (sscanf(s.c_str(), "write_bytes: %lu", &t) == 1)
                io.write_bytes = t;
            else if (sscanf(s.c_str(), "cancelled_write_bytes: %lu", &t) == 1)
                io.cancelled_write_bytes = t;
        }
    } else {
        io.rchar = 0;
        io.wchar = 0;
        io.read_bytes = 0;
        io.write_bytes = 0;

        return false;
    }

    return true;
}

std::string getDesktopFileFromName(int pid, QString procName, QString cmdline)
{
    if (desktopfileMaps.contains(cmdline)) {
        return desktopfileMaps[cmdline].toStdString();
    } else {
        // Need found desktop file from process environment, if process is wine program.
        if (cmdline.startsWith("c:\\")) {
            QString gioDesktopFile =
                Utils::getProcessEnvironmentVariable(pid, "GIO_LAUNCHED_DESKTOP_FILE");

            return gioDesktopFile.toStdString();
        } else {
            QDirIterator dir("/usr/share/applications", QDirIterator::Subdirectories);
            std::string desktopFile;

            // Convert to lower characters.
            QString procname = procName.toLower();

            // Replace "_" instead "-", avoid some applications desktop file can't found, such
            // as, sublime text.
            procname.replace("_", "-");

            // Concat desktop file.
            QString processFilename = procname + ".desktop";

            if (GUI_BLACKLIST_MAP.find(procname) == GUI_BLACKLIST_MAP.end()) {
                while (dir.hasNext()) {
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
    }
}

double calculateCPUPercentage(const proc_t *before, const proc_t *after,
                              const unsigned long long &prevCpuTime,
                              const unsigned long long &cpuTime)
{
    double totalCpuTime = cpuTime - prevCpuTime;
    unsigned long long processcpuTime =
        ((after->utime + after->stime) - (before->utime + before->stime));

    return (processcpuTime / totalCpuTime) * 100.0;
}

void blurRect(DWindowManager *windowManager, int widgetId, QRectF rect)
{
    QVector<uint32_t> data;

    qreal devicePixelRatio = qApp->devicePixelRatio();
    data << rect.x() << rect.y() << rect.width() * devicePixelRatio
         << rect.height() * devicePixelRatio << RECTANGLE_RADIUS << RECTANGLE_RADIUS;
    windowManager->setWindowBlur(widgetId, data);
}

void blurRects(DWindowManager *windowManager, int widgetId, QList<QRectF> rects)
{
    QVector<uint32_t> data;
    qreal devicePixelRatio = qApp->devicePixelRatio();
    foreach (auto rect, rects) {
        data << rect.x() << rect.y() << rect.width() * devicePixelRatio
             << rect.height() * devicePixelRatio << RECTANGLE_RADIUS << RECTANGLE_RADIUS;
    }
    windowManager->setWindowBlur(widgetId, data);
}

void clearBlur(DWindowManager *windowManager, int widgetId)
{
    QVector<uint32_t> data;
    data << 0 << 0 << 0 << 0 << 0 << 0;
    windowManager->setWindowBlur(widgetId, data);
}

void drawLoadingRing(QPainter &painter, int centerX, int centerY, int radius, int penWidth,
                     int loadingAngle, int rotationAngle, QColor foregroundColor,
                     double foregroundOpacity, QColor backgroundColor, double backgroundOpacity,
                     double percent)
{
    drawRing(painter, centerX, centerY, radius, penWidth, loadingAngle, rotationAngle,
             backgroundColor, backgroundOpacity);
    drawRing(painter, centerX, centerY, radius, penWidth, loadingAngle * percent, rotationAngle,
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
    XRectangle *reponseArea = new XRectangle;
    reponseArea->x = 0;
    reponseArea->y = 0;
    reponseArea->width = 0;
    reponseArea->height = 0;

    XShapeCombineRectangles(QX11Info::display(), wid, ShapeInput, 0, 0, reponseArea, 1, ShapeSet,
                            YXBanded);

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
    QFont font = painter.font();
    font.setPointSize(textSize);
    painter.setFont(font);
}

/**
 * @brief explode Explode a string based on
 * @param s The string to explode
 * @param c The seperator to use
 * @return A vector of the exploded string
 */
const std::vector<std::string> explode(const std::string &s, const char &c)
{
    std::string buff {""};
    std::vector<std::string> v;

    for (auto n : s) {
        if (n != c) {
            buff += n;
        } else if (n == c && buff != "") {
            v.push_back(buff);
            buff = "";
        }
    }
    if (buff != "") {
        v.push_back(buff);
    }

    return v;
}
}  // namespace Utils
