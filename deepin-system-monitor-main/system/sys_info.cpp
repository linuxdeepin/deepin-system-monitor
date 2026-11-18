// Copyright (C) 2019 ~ 2020 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sys_info.h"

#include "common/time_period.h"
#include "common/sample.h"
#include "common/common.h"
#include "system/system_monitor.h"
#include "common/thread_manager.h"
#include "system/system_monitor_thread.h"
#include "packet.h"
#include <DSysInfo>

#include <QString>
#include <QtDBus>
#include <QFileInfo>
#include <QDir>

#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <errno.h>
#include <stdio.h>
#include <sys/sysinfo.h>
#include <arpa/inet.h>
#include "common/hash.h"

#include <linux/netlink.h>
#include <linux/sock_diag.h>
#include <linux/inet_diag.h>
#include <sys/socket.h>
#include <string.h>

using namespace common::alloc;
using namespace common::core;
using namespace common::error;
using namespace core::system;
DCORE_USE_NAMESPACE

#define PROC_PATH_FILE_NR "/proc/sys/fs/file-nr"
#define PROC_PATH_UPTIME "/proc/uptime"
#define PROC_PATH_STAT "/proc/stat"
#define PROC_PATH_LOADAVG "/proc/loadavg"

namespace core {
namespace system {

SysInfo::SysInfo()
    : d(new SysInfoPrivate())
{
    d->loadAvg = std::make_shared<struct load_avg_t>();
}
SysInfo::SysInfo(const SysInfo &other)
    : d(other.d)
{
}
SysInfo &SysInfo::operator=(const SysInfo &rhs)
{
    if (this == &rhs)
        return *this;

    d = rhs.d;
    return *this;
}
SysInfo::~SysInfo()
{

}

SysInfo *SysInfo::instance()
{
    auto *monitor = ThreadManager::instance()->thread<SystemMonitorThread>(BaseThread::kSystemMonitorThread)->systemMonitorInstance();
    return monitor->sysInfo();
}

bool SysInfo::readSockStat(SockStatMap &statMap)
{
    bool ok {true};
    statMap.clear();

    auto queryDiag = [](int family, int proto, SockStatMap & statMap) -> bool {
        int nl_fd = socket(AF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, NETLINK_SOCK_DIAG);
        if (nl_fd < 0) {
            return false;
        }

        struct inet_diag_req_v2 req;
        memset(&req, 0, sizeof(req));
        req.sdiag_family = family;
        req.sdiag_protocol = proto;
        // All socket states for both TCP and UDP: 0xFFF == (1 << 12) - 1
        req.idiag_states = 0xFFF;
        req.idiag_ext = 0;

        struct sockaddr_nl nladdr;
        memset(&nladdr, 0, sizeof(nladdr));
        nladdr.nl_family = AF_NETLINK;

        struct nlmsghdr nlh;
        memset(&nlh, 0, sizeof(nlh));
        nlh.nlmsg_len = NLMSG_LENGTH(sizeof(req));
        nlh.nlmsg_type = SOCK_DIAG_BY_FAMILY;
        nlh.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;

        struct iovec iov[2];
        iov[0].iov_base = &nlh;
        iov[0].iov_len = sizeof(nlh);
        iov[1].iov_base = &req;
        iov[1].iov_len = sizeof(req);

        struct msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_name = &nladdr;
        msg.msg_namelen = sizeof(nladdr);
        msg.msg_iov = iov;
        msg.msg_iovlen = 2;

        if (sendmsg(nl_fd, &msg, 0) < 0) {
            close(nl_fd);
            return false;
        }

        const int BUFLEN = 65536;
        QByteArray buf(BUFLEN, '\0');

        bool ret {true};
        while (true) {
            struct sockaddr_nl nladdr2;
            memset(&nladdr2, 0, sizeof(nladdr2));

            struct iovec iov2;
            iov2.iov_base = buf.data();
            iov2.iov_len = BUFLEN;

            struct msghdr msg2;
            memset(&msg2, 0, sizeof(msg2));
            msg2.msg_name = &nladdr2;
            msg2.msg_namelen = sizeof(nladdr2);
            msg2.msg_iov = &iov2;
            msg2.msg_iovlen = 1;

            ssize_t len = recvmsg(nl_fd, &msg2, 0);
            if (len < 0) {
                if (errno == EINTR) {
                    continue;
                }
                ret = false;
                break;
            }
            if (len == 0) {
                break;
            }

            struct nlmsghdr *nlhdr = (struct nlmsghdr *)buf.data();
            bool done = false;

            while (NLMSG_OK(nlhdr, (size_t)len)) {
                if (nlhdr->nlmsg_type == NLMSG_DONE) {
                    done = true;
                    break;
                }
                if (nlhdr->nlmsg_type == NLMSG_ERROR) {
                    ret = false;
                    done = true;
                    break;
                }
                if (nlhdr->nlmsg_type != SOCK_DIAG_BY_FAMILY) {
                    nlhdr = NLMSG_NEXT(nlhdr, len);
                    continue;
                }

                struct inet_diag_msg *diag = (struct inet_diag_msg *)NLMSG_DATA(nlhdr);
                if (diag->idiag_inode == 0) {
                    nlhdr = NLMSG_NEXT(nlhdr, len);
                    continue;
                }

                auto stat = QSharedPointer<struct sock_stat_t>::create();
                stat->ino = diag->idiag_inode;
                stat->sa_family = family;
                stat->proto = proto;
                stat->s_port = ntohs(diag->id.idiag_sport);
                stat->d_port = ntohs(diag->id.idiag_dport);
                stat->uid = diag->idiag_uid;

                if (family == AF_INET) {
                    stat->s_addr.in4.s_addr = diag->id.idiag_src[0];
                    stat->d_addr.in4.s_addr = diag->id.idiag_dst[0];
                } else if (family == AF_INET6) {
                    memcpy(stat->s_addr.in6.s6_addr32, diag->id.idiag_src, 16);
                    memcpy(stat->d_addr.in6.s6_addr32, diag->id.idiag_dst, 16);

                    // convert ipv4 mapped ipv6 address to ipv4
                    if (stat->s_addr.in6.s6_addr32[0] == 0x0 &&
                            stat->s_addr.in6.s6_addr32[1] == 0x0 &&
                            stat->s_addr.in6.s6_addr32[2] == 0xffff0000) {
                        stat->sa_family = AF_INET;
                        stat->s_addr.in4.s_addr = stat->s_addr.in6.s6_addr32[3];
                        stat->d_addr.in4.s_addr = stat->d_addr.in6.s6_addr32[3];
                    }
                } else {
                    // Unknown address family, skip this entry
                    nlhdr = NLMSG_NEXT(nlhdr, len);
                    continue;
                }

                QString patternA {}, patternB {};
                QByteArray fmtbuf {};
                uint64_t cchash[2] {};
                uint64_t hash {};

                if (stat->sa_family == AF_INET) {
                    char saddr_str[INET_ADDRSTRLEN + 1] {}, daddr_str[INET_ADDRSTRLEN + 1] {};
                    inet_ntop(AF_INET, &stat->s_addr.in4, saddr_str, INET_ADDRSTRLEN);
                    inet_ntop(AF_INET, &stat->d_addr.in4, daddr_str, INET_ADDRSTRLEN);

                    patternA = QString("%1:%2-%3:%4").arg(saddr_str).arg(stat->s_port).arg(daddr_str).arg(stat->d_port);
                    if (proto == IPPROTO_TCP) {
                        patternB = QString("%1:%2-%3:%4").arg(daddr_str).arg(stat->d_port).arg(saddr_str).arg(stat->s_port);
                    }
                } else if (stat->sa_family == AF_INET6) {
                    char saddr6_str[INET6_ADDRSTRLEN + 1] {}, daddr6_str[INET6_ADDRSTRLEN + 1] {};
                    inet_ntop(AF_INET6, &stat->s_addr.in6, saddr6_str, INET6_ADDRSTRLEN);
                    inet_ntop(AF_INET6, &stat->d_addr.in6, daddr6_str, INET6_ADDRSTRLEN);

                    patternA = QString("%1:%2-%3:%4").arg(saddr6_str).arg(stat->s_port).arg(daddr6_str).arg(stat->d_port);
                    if (proto == IPPROTO_TCP) {
                        patternB = QString("%1:%2-%3:%4").arg(daddr6_str).arg(stat->d_port).arg(saddr6_str).arg(stat->s_port);
                    }
                }

                fmtbuf = patternA.toLocal8Bit();
                util::common::hash(fmtbuf.constData(), fmtbuf.length(), util::common::global_seed, cchash);
                hash = cchash[0];
                statMap.insert(hash, stat);

                if (proto == IPPROTO_TCP) {
                    fmtbuf = patternB.toLocal8Bit();
                    util::common::hash(fmtbuf.constData(), fmtbuf.length(), util::common::global_seed, cchash);
                    hash = cchash[0];
                    statMap.insert(hash, stat);
                }

                nlhdr = NLMSG_NEXT(nlhdr, len);
            }

            if (done) {
                break;
            }
        }

        close(nl_fd);
        return ret;
    };

    ok = queryDiag(AF_INET, IPPROTO_TCP, statMap);
    ok = queryDiag(AF_INET, IPPROTO_UDP, statMap) && ok;
    ok = queryDiag(AF_INET6, IPPROTO_TCP, statMap) && ok;
    ok = queryDiag(AF_INET6, IPPROTO_UDP, statMap) && ok;

    return ok;
}

void SysInfo::readSysInfo()
{
    d->nfds = read_file_nr();
    d->nprocs = read_processes();
    d->nthrs = read_threads();
    
    read_uptime(d->uptime);
    read_btime(d->btime);
    read_loadavg(d->loadAvg);
}

void SysInfo::readSysInfoStatic()
{
    d->uid = getuid();
    d->euid = geteuid();
    d->gid = getgid();
    d->egid = getegid();

    d->user_name = userName(d->uid);
    d->effective_user_name = userName(d->euid);
    if (d->effective_user_name.isEmpty())
        d->effective_user_name = QByteArray(getenv("USER"));

    d->group_name = groupName(d->gid);
    d->effective_group_name = groupName(d->egid);
    if (d->effective_group_name.isEmpty())
        d->effective_group_name = QByteArray(getenv("GROUP"));
    d->hostname = read_hostname();
    d->arch = read_arch();
    d->version = read_version();
}

quint32 SysInfo::read_file_nr()
{
    FILE *fp;
    unsigned int file_nr = 0;

    errno = 0;
    if ((fp = fopen(PROC_PATH_FILE_NR, "r"))) {
        uFile fPtr;
        fPtr.reset(fp);

        char buf[128] {};
        if (fgets(buf, sizeof(buf), fp)) {
            int nm = sscanf(buf, "%u", &file_nr);
            if (nm == 1)
                return file_nr;

            print_errno(errno, QString("read %1 failed").arg(PROC_PATH_FILE_NR));
            return 0;
        } // ::if(fgets)
    } // ::if(fopen)

    print_errno(errno, QString("open %1 failed").arg(PROC_PATH_FILE_NR));
    return 0;
}

quint32 SysInfo::read_threads()
{
    QDir dir("/proc");
    QFileInfoList infoList = dir.entryInfoList();
    quint32 threads = 0;
    for (QFileInfo info : infoList) {
        if (info.isDir() && info.fileName().toInt() > 0) {
            QDir taskDir("/proc/" + info.fileName() + "/task");
            threads += taskDir.entryInfoList().count();
        }
    }

    return threads;
}

quint32 SysInfo::read_processes()
{
    QDir dir("/proc");
    QFileInfoList infoList = dir.entryInfoList();
    quint32 processes = 0;
    for (QFileInfo info : infoList) {
        if (info.isDir() && info.fileName().toInt() > 0) {
            processes++;
        }
    }

    return processes;
}

QString SysInfo::read_hostname()
{
    QDBusInterface busIf("org.freedesktop.hostname1",
                         "/org/freedesktop/hostname1",
                         "org.freedesktop.hostname1",
                         QDBusConnection::systemBus());
    auto name = busIf.property("Hostname");
    return name.toString();
}

QString SysInfo::read_arch()
{
    QString buffer {};

    struct utsname os {
    };
    auto rc = uname(&os);
    if (!rc)
        buffer = os.machine;

    return buffer;
}

QString SysInfo::read_version()
{
    auto ver = QString("%1 %2")
               //etc/os-version 下 MajorVersion
               .arg(DSysInfo::majorVersion())
               //etc/os-version 下 EditionName
               .arg(DSysInfo::uosEditionName());
    return ver;
}

void SysInfo::read_uptime(struct timeval &uptime)
{
    FILE *fp;
    errno = 0;

    if ((fp = fopen(PROC_PATH_UPTIME, "r"))) {
        uFile fPtr;
        fPtr.reset(fp);

        char buf[128] {};
        double up_sec {};
        if (fgets(buf, sizeof(buf), fp)) {
            int nm = sscanf(buf, "%lf", &up_sec);
            if (nm == 1) {
                uptime.tv_sec = static_cast<time_t>(up_sec);
                uptime.tv_usec = static_cast<suseconds_t>((up_sec - uptime.tv_sec) * 1000000);

                return;
            }
        } // ::if(gets)

        print_errno(errno, QString("read %1 failed").arg(PROC_PATH_UPTIME));
        return;
    } // ::if(fopen)

    print_errno(errno, QString("open %1 failed").arg(PROC_PATH_UPTIME));
}

void SysInfo::read_btime(struct timeval &btime)
{
    FILE *fp;
    errno = 0;
    if ((fp = fopen(PROC_PATH_STAT, "r"))) {
        uFile fPtr;
        fPtr.reset(fp);

        char line[4095] {};
        long nsec {0};
        while (fgets(line, sizeof(line), fp)) {
            if (!strncmp(line, "btime", 5)) {
                int nm = sscanf(line + 5, "%ld", &nsec);
                if (nm == 1) {
                    btime.tv_sec = nsec;
                    btime.tv_usec = 0;

                    return;
                }
            } // ::if(cmp)
        } // ::while

        print_errno(errno, QString("read %1 failed").arg(PROC_PATH_STAT));
        return;
    } // ::if(fopen)

    print_errno(errno, QString("open %1 failed").arg(PROC_PATH_STAT));
}

void SysInfo::read_loadavg(LoadAvg &loadAvg)
{
    QFile file("/proc/loadavg");
    if (file.exists() && file.open(QFile::ReadOnly)) {
        // 只需要读取第一行数据
        QByteArray lineData = file.readLine();
        file.close();
        /*样例数据:
            $ cat /proc/loadavg
            0.41 0.46 0.36 2/2646 20183
        */

        // 分割数据 取前3
        QStringList cpuStatus =  QString(lineData).split(" ", QString::SkipEmptyParts);

        if (cpuStatus.size() > 3) {
            loadAvg->lavg_1m = cpuStatus[0].toFloat();
            loadAvg->lavg_5m = cpuStatus[1].toFloat();
            loadAvg->lavg_15m = cpuStatus[2].toFloat();

            return ;
        }
    }

    print_errno(errno, QString("call sysinfo failed"));
}

} // namespace system
} // namespace core
