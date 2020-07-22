/*
* Copyright (C) 2011 ~ 2020 Uniontech Software Technology Co.,Ltd
*
* Author:      Wang Yong <wangyong@deepin.com>
* Maintainer:  maojj <maojunjie@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "utils.h"

#include <QApplication>
#include <QDir>
#include <QFontMetrics>
#include <QIcon>
#include <QLayout>
#include <QPainter>
#include <QPixmap>
#include <QString>
#include <QWidget>
#include <QtDBus>
#include <QtMath>
#include <QtX11Extras/QX11Info>

#include <fstream>
#include <string>

#include <X11/extensions/shape.h>

#include <proc/readproc.h>


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
        while (readproc(proc, &proc_info) != nullptr) {
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

void blurRect(DWindowManager *windowManager, WId widgetId, QRect rect)
{
    QVector<uint32_t> data;

    qreal devicePixelRatio = qApp->devicePixelRatio();
    data << uint32_t(rect.x()) << uint32_t(rect.y()) << uint32_t(rect.width() * devicePixelRatio)
         << uint32_t(rect.height() * devicePixelRatio) << RECTANGLE_RADIUS << RECTANGLE_RADIUS;
    windowManager->setWindowBlur(int(widgetId), data);
}

void blurRects(DWindowManager *windowManager, WId widgetId, QList<QRect> rects)
{
    QVector<uint32_t> data;
    qreal devicePixelRatio = qApp->devicePixelRatio();
    foreach (auto rect, rects) {
        data << uint32_t(rect.x()) << uint32_t(rect.y()) << uint32_t(rect.width() * devicePixelRatio)
             << uint32_t(rect.height() * devicePixelRatio) << RECTANGLE_RADIUS << RECTANGLE_RADIUS;
    }
    windowManager->setWindowBlur(int(widgetId), data);
}

void clearBlur(DWindowManager *windowManager, WId widgetId)
{
    QVector<uint32_t> data;
    data << 0 << 0 << 0 << 0 << 0 << 0;
    windowManager->setWindowBlur(int(widgetId), data);
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

void passInputEvent(WId wid)
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
    if (item != nullptr) {
        QWidget *widget = item->widget();
        if (widget != nullptr) {
            widget->hide();
            widget->setParent(nullptr);
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
