/*
 * Copyright (C) 2017 Lily Rivers (VioletDarkKitty)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License only.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "process_tools.h"
#include <string>
#include <unistd.h>
#include <fstream>
#include <QFileInfo>
#include <proc/sysinfo.h>
#include <time.h>
#include <qdiriterator.h>
#include <QIcon>
#include <QMap>
#include <unordered_set>
#include <unordered_map>
#include "hashqstring.h"

namespace processTools {
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

    /**
     * @brief getProcessStartDate Convert a timestamp into a date in the form that is used in the current locale
     * @param start_time The timestamp
     * @return A date string
     */
    QString getProcessStartDate(unsigned long long start_time)
    {
        __time_t time = getbtime() + start_time/Hertz;
        struct tm *tm = localtime(&time);
        char date[255];
        strftime(date, sizeof(date), "%Ex %ER", tm);
        return QString(date);
    }

    /**
     * @brief getProcessStatus Get the status of a process as a long string from its proc_t
     * @param p The proc_t structure of the process
     * @return The long string form of the process status
     */
    QString getProcessStatus(proc_t* p)
    {
        switch(p->state) {
            case 'S':
                return "Sleeping";
            break;

            case 'R':
                return "Running";
            break;

            case 'Z':
                return "Zombie";
            break;

            default:
                return "Unknown state: '" + QString(p->state) + "'";
        }
    }

    /**
     * @brief getProcessIconFromName Get the icon for a process given its name
     * @param procname The name of the process
     * @return The process' icon or the default executable icon if none was found
     */
    QIcon getProcessIconFromName(QString procName)
    {
        // apply some corrections to the process name
        // ie, sh should look for terminal icons, not anything containing sh
        static std::map<QString, QString> procNameCorrections({
            {"sh","terminal"}, {"bash","terminal"}, {"dconf-service", "dconf"}, {"gconfd-2", "dconf"}, {"deja-dup-monitor", "deja-dup"}
        });
        auto procPos = procNameCorrections.find(procName);
        if (procPos != procNameCorrections.end()) {
            procName = procPos->second;
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
            return defaultExecutableIcon;
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

        return icon;
    }
}
