// Copyright (C) 2025 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#ifndef __bpf__

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <vector>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <netinet/in.h>

// mount_attr 结构体在 Linux 5.12+ 内核中定义，旧内核需要手动定义
#ifndef MOUNT_ATTR_SIZE_VER0
struct mount_attr {
    uint64_t attr_set;
    uint64_t attr_clr;
    uint64_t propagation;
    uint64_t userns_fd;
};
#define MOUNT_ATTR_SIZE_VER0 32
#endif

#define TASK_COMM_LEN 16
struct FileLog;

class DKapture
{
public:
#endif
    enum DataType
    {
        /**
         * 后缀小写表示非标系统节点，而是定制功能
         * 将下列枚举名字改成驼峰命名法，即可找到它在当前文件中的
         * 结构体定义，例如 PROC_PID_STAT 对应的
         * 结构体定义为 struct ProcPidStat {...}。
         */
        PROC_NONE,
        PROC_BEGIN,
        PROC_PID_IO,        // /proc/<pid>/io
        PROC_PID_STAT,      // /proc/<pid>/stat
        PROC_PID_STATM,     // /proc/<pid>/statm
        PROC_PID_traffic,   // /proc/<pid>/traffic
        PROC_PID_SCHEDSTAT, // /proc/<pid>/schedstat
        PROC_PID_FD,        // /proc/<pid>/fd
        PROC_PID_STATUS,    // /proc/<pid>/status
        PROC_PID_NET,       // /proc/<pid>/net
        PROC_PID_CMDLINE,   // /proc/<pid>/cmdline
        PROC_PID_ENV,       // /proc/<pid>/environ
        PROC_PID_CWD,       // /proc/<pid>/cwd
        PROC_PID_ROOT,      // /proc/<pid>/root
        PROC_PID_EXE,       // /proc/<pid>/exe
        PROC_PID_MAPS,      // /proc/<pid>/maps
        PROC_PID_sock,      // /proc/<pid>/sock
        PROC_END,
        // 待扩展
    };

    enum FileLogType
    {
        FILE_LOG_NONE,
        FILE_LOG_OPEN,
        FILE_LOG_CLOSE,
        FILE_LOG_GETXATTR,
        FILE_LOG_SETXATTR,
        FILE_LOG_LISTXATTR,
        FILE_LOG_REMOVEXATTR,
        FILE_LOG_GETACL,
        FILE_LOG_SETACL,
        FILE_LOG_CHOWN,
        FILE_LOG_CHMOD,
        FILE_LOG_STAT,
        FILE_LOG_MMAP,
        FILE_LOG_FLOCK,
        FILE_LOG_FCNTL,
        FILE_LOG_LINK,
        FILE_LOG_UNLINK,
        FILE_LOG_TRUNCATE,
        FILE_LOG_IOCTL,
        FILE_LOG_RENAME,
        FILE_LOG_FALLOCATE,
        FILE_LOG_READ,
        FILE_LOG_WRITE,
        FILE_LOG_READV,
        FILE_LOG_WRITEV,
        FILE_LOG_COPY_FILE_RANGE,
        FILE_LOG_SENDFILE,
        FILE_LOG_SPLICE,
        FILE_LOG_MKNOD,
        FILE_LOG_MKDIR,
        FILE_LOG_RMDIR,
        FILE_LOG_SYMLINK,
        FILE_LOG_LSEEK,
        FILE_LOG_MOUNT,
        FILE_LOG_UMOUNT,
    };

    enum LogLevel
    {
        ERROR = 0, // 错误级别日志
        WARN,      // 警告级别日志
        INFO,      // 信息级别日志
        DEBUG,     // 调试级别日志
        MAX        // 最大日志级别
    };

    struct DataHdr
    {
        enum DataType type;       // 数据类型
        unsigned int dsz;         // 数据大小
        char comm[TASK_COMM_LEN]; // 进程名称
        pid_t pid;                // 轻量进程 ID
        pid_t tgid;               // 线程组 ID
        long reserve[5];          // 保留字段，用于后向兼容
        char data[];              // 数据内容
    };

#ifndef __bpf__
    typedef int (*DKCallback)(void *ctx, const void *data, size_t data_sz);

    /**
     * @brief 初始化 DKapture 实例，内部会申请各种必要资源并完成初始化
     * @param fp DKapture 日志输出文件
     * @param lvl DKapture 日志输出级别
     * @return 成功返回 0，失败则返回 -errno
     */
    virtual int open(FILE *fp = stdout, LogLevel lvl = INFO) = 0;

    /**
     * @brief 设置 DKapture 内存缓存数据的有效时间，当超过这个设定时间，数据
     *        仍未更新时，任意一个进程的下一次读操作，都会触发 DKapture 内部
     *        对数据进行更新。
     * @param ms 目标有效时间（单位：毫秒）。如果为 UINT64_MAX，则不修改，仅返回当前设定值
     * @return 返回旧的设定值
     */
    virtual unsigned long long lifetime(unsigned long long ms) = 0;

    /**
     * @brief 从指定的进程和数据类型中读取数据
     * @param dt 数据类型
     * @param pid 目标进程 ID，设置为0时表示读取所有进程(强烈建议不要
     *          使用该接口读取所有进程信息，因为进程的数量是不确定的，因此接口需要
     *          buffer也会变化，建议使用支持回调的重载读取所有进程信息)。
     * @param buf 数据存储缓冲区，DataHdr结构体的 data 成员指向实际的数据结构，
     *          可以通过数据类型快速找到数据结构体的定义，例如，如果数据类型
     *          是 DataType::PROC_PID_STAT，则它对应的实际数据结构体的定义
     *          为 struct ProcPidStat {...}，即将下划线分割全大写的枚举名
     *          改成驼峰名即可。
     * @param bsz 缓冲区大小，必须大于 dt 对应的数据结构体加上数据头部。
     * @return 成功返回读取的字节数，失败返回 -errno
     */
    virtual ssize_t read(DataType dt, pid_t pid, DataHdr *buf, size_t bsz) = 0;

    /**
     * @brief 从指定procfs节点路径读取数据，例如 /proc/1/stat，暂不
     *        支持 /proc/self/ 特殊路径。
     * @param path 数据路径，目前必须以 /proc 开头。
     * @param buf 数据存储缓冲区，DataHdr结构体的 data 成员指向实际的数据结构，
     *          可以通过数据类型快速找到数据结构体的定义，例如，如果数据类型
     *          是 DataType::PROC_PID_STAT，则它对应的实际数据结构体的定义
     *          为 struct ProcPidStat {...}，即将下划线分割全大写的枚举名
     *          改成驼峰名即可。
     * @param bsz 缓冲区大小，必须大于 dt 对应的数据结构体加上数据头部。
     * @return 成功返回读取的字节数，失败返回 -errno
     */
    virtual ssize_t read(const char *path, DataHdr *buf, size_t bsz) = 0;

    /**
     * @brief 读取指定进程的多个类型（节点）数据，一次性多个的效率要比多次读取高。
     * @param dts 数据类型列表
     * @param pid 目标进程 ID，设置为0时表示读取所有进程(强烈建议不要
     *          使用该接口读取所有进程信息，因为进程的数量是不确定的，因此接口需要
     *          buffer也会变化，建议使用支持回调的重载读取所有进程信息)。
     * @param buf 数据存储缓冲区，多个数据存储在buf中，不保证先后顺序与 dts 一致。
     *         用户可以通过 DataHdr 中 dsz 字段依次计算偏移遍历所有数据。
     * @param bsz 缓冲区大小。
     * @return 成功返回读取的字节数，失败返回 -errno
     */
    virtual ssize_t read(std::vector<DataType> &dts, pid_t pid, DataHdr *buf, size_t bsz) = 0;

    /**
     * @brief 从多个路径中读取数据，和上一重载类似，但更灵活，可以通过路径交叉指定
     *      多个进程和多个数据类型，例如 </proc/1/stat, /proc/2/statm, /proc/2/io>
     * @param paths 数据路径列表
     * @param buf 数据存储缓冲区，多个数据存储在buf中，不保证先后顺序与 paths 一致。
     *         用户可以通过 DataHdr 中 dsz 字段依次计算偏移遍历所有数据。
     * @param bsz 缓冲区大小
     * @return 成功返回读取的字节数，失败返回 -errno
     */
    virtual ssize_t read(std::vector<const char *> &paths, DataHdr *buf, size_t bsz) = 0;

    /**
     * @brief 从指定数据类型和多个进程中读取数据
     * @param dt 数据类型
     * @param pids 目标进程 ID 列表
     * @param buf 数据存储缓冲区
     * @param bsz 缓冲区大小
     * @return 成功返回读取的字节数，失败返回 -errno
     */
    virtual ssize_t read(DataType dt, std::vector<pid_t> &pids, DataHdr *buf, size_t bsz) = 0;

    /**
     * @brief 读取所有进程的指定数据类型，并一一通过回调函数返回。
     * @param dt 数据类型
     * @param cb 回调函数，每读取一个进程的一个数据类型，会同步调用该回调一次。
     *           对于特殊数据类型，例如 /proc/pid/fd，则会每个 fd 回调一次。
     *           回调的函数的实现请进口简短，否则会降低 DKapture 的效率。
     * @param ctx 传递给回调函数的用户上下文
     * @return 成功返回处理的数据条目数，失败返回 -errno
     */
    virtual ssize_t read(DataType dt, DKCallback cb, void *ctx) = 0;

    /**
     * @brief 一次完成读取所有进程的多个数据类型，效率比多次读高。
     * @param dts 数据类型集合，中间不合法的类型会被忽略。
     * @param cb 回调函数，每读取一个进程的一个数据类型，会同步调用该回调一次，
     *           但不保证顺序和 dts 一致。
     *           对于特殊数据类型，例如 /proc/pid/fd，则会每个 fd 回调一次。
     *           回调的函数的实现请进口简短，否则会降低 DKapture 的效率。
     * @param ctx 传递给回调函数的用户上下文
     * @return 成功返回处理的数据条目数，失败返回 -errno
     */
    virtual ssize_t read(std::vector<DataType> &dts, DKCallback cb, void *ctx) = 0;

    /**
     * @brief 一次完成读取所有进程的多个数据类型，效率比多次读高。
     * @param paths 目标节点的路径集合，中间不合法的路径会被忽略
     * @param cb 回调函数，每读取一个进程的一个数据类型，会同步调用该回调一次。
     *           但不保证顺序和 dts 一致。
     *           对于特殊数据类型，例如 /proc/pid/fd，则会每个 fd 回调一次。
     *           回调的函数的实现请进口简短，否则会降低 DKapture 的效率。
     * @param ctx 传递给回调函数的用户上下文
     * @return 成功返回处理的数据条目数，失败返回 -errno
     */
    virtual ssize_t read(std::vector<const char *> &paths, DKCallback cb, void *ctx) = 0;

    /**
     * @brief kmemleak_scan_start 和 kmemleak_scan_stop 结合使用
     *        扫描指定进程可能导致的内核内存泄漏，不能 100% 确定是泄漏，
     *        只能判断在 start ~ stop 期间申请了却没有被释放的内存。
     * @param pid 被扫描的进程，指定 0 则表示所有进程
     * @param cb 回调函数，针对所有可能导致溢出的函数调用栈，该回调函数都会被
     *           调用一次，回调函数的调用实际上，是同步发生在 kmemleak_scan_stop
     *           的调用期间。
     *      输出示例：
     *          212992 bytes in 52 allocations from stack
     *              0 [<ffffffffa85bddaa>] __alloc_pages+0x25a
     *              1 [<ffffffffa85bddaa>] __alloc_pages+0x25a
     *              2 [<ffffffffc05cc813>] ttm_pool_alloc_page+0x43
     *              3 [<ffffffffc05cd75f>] ttm_pool_alloc+0x17f
     *              4 [<ffffffffc062e95b>] radeon_ttm_tt_populate+0x4b
     *              5 [<ffffffffc05c69c4>] ttm_tt_populate+0xa4
     *              6 [<ffffffffc05c773f>] ttm_bo_handle_move_mem+0x16f
     *              7 [<ffffffffc05c809b>] ttm_bo_validate+0xeb
     *              8 [<ffffffffc05c8266>] ttm_bo_init_reserved+0x146
     *              9 [<ffffffffc05c82f7>] ttm_bo_init_validate+0x57
     *              10 [<ffffffffc062fbd3>] radeon_bo_create+0x153
     *              11 [<ffffffffc0644aac>] radeon_gem_object_create+0xbc
     *              12 [<ffffffffc0644d27>] radeon_gem_create_ioctl+0x77
     *              13 [<ffffffffc03706e7>] drm_ioctl_kernel+0xd7
     *              14 [<ffffffffc0370a4b>] drm_ioctl+0x2ab
     *              15 [<ffffffffc0610062>] radeon_drm_ioctl+0x52
     *              16 [<ffffffffa865f5dc>] __x64_sys_ioctl+0x9c
     *              17 [<ffffffffa82063b4>] x64_sys_call+0x15a4
     *              18 [<ffffffffa9188f78>] do_syscall_64+0x38
     *              19 [<ffffffffa9200134>] entry_SYSCALL_64_after_hwframe+0x78
     * @param ctx 用户自定义的，传递给回调函数的上下文指针。
     */
    virtual int kmemleak_scan_start(pid_t pid, DKCallback cb, void *ctx) = 0;
    virtual int kmemleak_scan_stop(void) = 0;

    /**
     * @brief 监控指定路径的文件事件
     * @param path 目标文件路径，必须是绝对路径，目前不支持目录
     *        设置为 null，则监控整个系统的文件。
     *        目前仅支持监控单个文件或整个系统的文件，不支持多个特定文件。
     * @param cb 回调函数，在回调函数中，用户可以将 data 参数强转为FileLog
     *        指针，再根据FileLog类型字段，转换指定日志结构体。例如：
     *
     *        FileLog *log = (FileLog *)data;
     *        if (log->log_type == FILE_LOG_OPEN)
     *        {
     *            OpenLog *open_log = (OpenLog *)log;
     *            // 处理 open_log
     *        }
     *
     *        将 cb 设为 null, 可以取消监控该路径。
     *        注意：回调的执行是异步的，由另外一个线程完成，非调用该函数的线程。
     *             不需要监控路径后，请使用 cb = null 参数注销
     *             否则会造成内存泄漏
     * @param ctx 用户上下文，回调函数会传递该上下文
     * @return 成功返回 0，失败返回 -errno
     */
    virtual int file_watch(
        const char *path, DKCallback cb, void *ctx) = 0;

    /**
     * @brief 文件系统事件监控
     * @param path 挂载点路径，或文件系统所在分区设备路径
     *        如果 mount /dev/sda1 /mnt，则 path 可以是 /mnt 或 /dev/sda1。
     *        指定路径为 null，则监控所有文件系统挂/卸载事件。
     *        只支持过滤 mount umount 两个系统调用的路径，其他的不支持,
     *        例如 fsopen fsmount 等。
     * @param cb 回调函数，在回调函数中，用户可以将 data 参数强转为FsLog
     *        指针。例如：
     *        FsLog *log = (FsLog *)data;
     *        将 cb 设为 null, 可以取消监控该路径。
     *        注意：不需要监控路径后，请使用 cb = null 参数注销
     *        否则会造成内存泄漏
     * @param ctx 用户上下文，回调函数会传递该上下文
     * @return 成功返回 0，失败返回 -errno
     */
    virtual int fs_watch(
        const char *path, DKCallback cb, void *ctx) = 0;

    /**
     * @brief 中断事件监控，回调返回监控数据结构有如下两种
     *      struct soft_irq_event_t
     *      struct irq_event_t
     *      请按结构体中的 type 字段枚举类型区分返回的到底是哪个数据结构
     * @param cb 回调函数
     * @param ctx 用户传递给回调函数的参数
     * @return 成功返回 0，失败返回 -errno
     */
    virtual int irq_watch(DKCallback cb, void *ctx) = 0;

    /**
     * @brief 关闭 DKapture 实例，释放所有资源
     * @return 成功返回 0，失败返回 -errno
     */
    virtual int close(void) = 0;

    /**
     * @brief 创建一个新的 DKapture 实例
     * @return 返回 DKapture 实例的指针
     */
    static DKapture *new_instance();

    /**
     * @brief 析构函数，释放资源
     */
    virtual ~DKapture() {}
};

#endif

// 以下数据统计时间起点均为“dkapture启动时”，而非目标进程启动时，暂未考虑溢出问题
struct ProcPidTraffic
{
    // int ifindex;   // 网络接口索引号
    size_t rbytes; // 接收的总字节数
    size_t wbytes; // 发送的总字节数
    // size_t rpkts;  // 接受的总包数量
    // size_t wpkts;  // 发送的总包数量
};

struct ProcPidCmdline
{
    // 短时间难以实现
};

struct ProcPidEnviron
{
    // 短时间难以实现
};

struct ProcPidStatus
{
    int umask;
    /**
     * state 字段取值定义：
     * 只取最高非0位表进程状态，请忽略其他bit位
     * 这里DKapture因为考虑采集效率原因，没有对
     * state字段进行字符串转化，请用户按下表自行
     * 按需转换，后面的值对应state最高位
     *  "R (running)",      // 0x00
     *  "S (sleeping)",     // 0x01
     *  "D (disk sleep)",   // 0x02
     *  "T (stopped)",      // 0x04
     *  "t (tracing stop)", // 0x08
     *  "X (dead)",         // 0x10
     *  "Z (zombie)",       // 0x20
     *  "P (parked)",       // 0x40
     *  "I (idle)",         // 0x80
     */
    int state;
    pid_t tracer_pid;
    uid_t uid[4]; // uid euid suid fsuid
    gid_t gid[4]; // gid egid sgid fsgid
    // 其他字段按需沟通补充
};

/**
 * 进程文件描述符，对于回调模式，每进程的每文件描述符会回调一次
 */
struct ProcPidFd
{
    /**
     * /proc/pid/fd 下都是软连接，以下信息均属于被链接的目标文件
     */
    unsigned long inode; // 目标文件的inode编号
    int fd;              // 文件描述符编号
    dev_t dev;           // 目标文件所在设备编号
    mode_t i_mode;       // 目标文件模式属性，同 stat 系统调用返回的 st_mode 字段
};

struct ProcPidSchedstat
{
    /**
     * 1) time spent on the cpu
     * 2) time spent waiting on a runqueue
     * 3) # of timeslices run on this cpu
     */
    unsigned long long cpu_time;
    unsigned long long rq_wait_time;
    unsigned long long timeslices;
};

/**
 * 该节点IO数据统计的是用户态进程，对于内核线程，该节点固定输出全0，
 * 因此，为了优化，内核线程的该结构体信息不输出
 */
struct ProcPidIo
{
    /**
     * 这里的字段和 proc/pid/io 中的含义略有不同，
     * proc/pid/io 中的统计包含了线程组中已死亡和未死亡子线程的io，
     * 而 dkapture 给的这些字段只统计了当前线程。
     */
    size_t rchar;
    size_t wchar;
    size_t syscr;
    size_t syscw;
    size_t read_bytes;
    size_t write_bytes;
    size_t cancelled_write_bytes;
};

/**
 * 内核线程该结构体信息输出固定为全0，另外
 * 多线程进程，所有线程共享内存描述符，即statm输出结果相同
 * 因此为了优化，内核线程的该结构体信息不输出，多线程进程仅输出领头线程的信息。
 */
struct ProcPidStatm
{
    unsigned long size;
    unsigned long resident;
    unsigned long shared;
    unsigned long text;
    unsigned long data;
};

/**
 * 全量轻量级进程信息（线程），包括内核线程。
 */
struct ProcPidStat
{ // 请参考 man proc 查看各字段含义
    /**
     * state 字段取值定义：
     * 只取最高非0位表进程状态，请忽略其他bit位
     * 这里DKapture因为考虑采集效率原因，没有对
     * state字段进行字符串转化，请用户按下表自行
     * 按需转换，后面的值对应state最高位
     *  "R (running)",      // 0x00
     *  "S (sleeping)",     // 0x01
     *  "D (disk sleep)",   // 0x02
     *  "T (stopped)",      // 0x04
     *  "t (tracing stop)", // 0x08
     *  "X (dead)",         // 0x10
     *  "Z (zombie)",       // 0x20
     *  "P (parked)",       // 0x40
     *  "I (idle)",         // 0x80
     */
    int state;
    pid_t ppid;
    pid_t pgid;
    pid_t sid;
    int tty_nr;
    int tty_pgrp;
    unsigned int flags;
    unsigned long cmin_flt;
    unsigned long cmaj_flt;
    unsigned long min_flt;
    unsigned long maj_flt;
    unsigned long long utime;
    unsigned long long stime;
    unsigned long long cutime;
    unsigned long long cstime;
    int priority;
    int nice;
    int num_threads;
    unsigned long long start_time;
    unsigned long vsize;
    unsigned long rss;
    unsigned long rsslim;
    unsigned long start_code;
    unsigned long end_code;
    unsigned long start_stack;
    unsigned long kstkesp;
    unsigned long kstkeip;
    unsigned long signal;
    unsigned long blocked;
    unsigned long sigignore;
    unsigned long sigcatch;
    unsigned long wchan;
    int exit_signal;
    int processor;
    unsigned int rt_priority;
    unsigned int policy;
    unsigned long long delayacct_blkio_ticks;
    unsigned long guest_time;
    long cguest_time;
    unsigned long start_data;
    unsigned long end_data;
    unsigned long start_brk;
    unsigned long arg_start;
    unsigned long arg_end;
    unsigned long env_start;
    unsigned long env_end;
    int exit_code;
};

struct ProcPidSock
{
    int fd;              // 套接字文件描述符
    unsigned long ino;   // 套接字的 inode 编号
    unsigned int family; // 套接字协议族，例如 AF_INET, AF_INET6 等
    unsigned int type;   // 套接字类型，例如 SOCK_STREAM, SOCK_DGRAM 等
    unsigned int state;  // 套接字状态，例如 TCP_ESTABLISHED, TCP_LISTEN 等
    union
    { // 本地 IP 地址，使用网络字节序
        unsigned int lip;
        struct in6_addr lipv6;
    };
    union
    { // 远程 IP 地址，使用网络字节序
        unsigned int rip;
        struct in6_addr ripv6;
    };
    short lport; // 本地端口
    short rport; // 远端端口
};

#ifndef __bpf__
struct FileLog
{
    uid_t uid;
    pid_t pid;
    char comm[16];
    enum DKapture::FileLogType log_type;
} __attribute__((__packed__));

struct OpenLog : public FileLog
{
    unsigned long i_ino;
    long ret;
    unsigned int f_mode;
} __attribute__((__packed__));

struct CloseLog : public FileLog
{
    unsigned long i_ino;
} __attribute__((__packed__));

struct XattrLog : public FileLog
{
    unsigned long i_ino;
    union
    {
        uint32_t name_list;
        uint32_t name; // the name string offset to 'action' field
    };
    uint32_t value; // the value's offset to 'action' field
    size_t size;
    long ret;
    char action[]; // must be less than 4096 - sizeof(FileLog) - sizeof(XattrLog)
} __attribute__((__packed__));

struct AclEntry // posix acl entry
{
    short e_tag;
    unsigned short e_perm;
    uint32_t e_id;
} __attribute__((__packed__));

struct AclLog : public FileLog
{
    unsigned long i_ino;
    uint32_t name;      // the name string offset to 'action' field
    uint32_t acl_entry; // the acl entry's offset to 'action' field
    size_t count;
    long ret;
    char action[]; // must be less than 4096 - sizeof(FileLog) - sizeof(AclLog)
} __attribute__((__packed__));

struct ChownLog : public FileLog
{
    unsigned long i_ino;
    uint32_t uid;
    uint32_t gid;
    long ret;
    char action[];

} __attribute__((__packed__));

struct ChmodLog : public FileLog
{
    unsigned long i_ino;
    uint16_t mode;
    long ret;
    char action[];

} __attribute__((__packed__));

struct StatLog : public FileLog
{
    unsigned long i_ino;
    uint32_t request_mask;
    uint32_t query_flags;
    long ret;
    char action[];

} __attribute__((__packed__));

struct MmapLog : public FileLog
{
    unsigned long i_ino;
    unsigned long addr;
    unsigned long len;
    unsigned long prot;
    unsigned long flag;
    unsigned long pgoff;
    long ret;
    char action[];

} __attribute__((__packed__));

struct FlckLog : public FileLog
{
    unsigned long i_ino;
    long arg;
    long ret;
    char action[];
} __attribute__((__packed__));

struct FcntlLog : public FileLog
{
    unsigned long i_ino;
    unsigned int cmd;
    unsigned long arg;
    long ret;
    char action[];

} __attribute__((__packed__));

struct LinkLog : public FileLog
{
    unsigned long i_ino;
    unsigned long i_ino_new;
    unsigned long dir_ino;
    long ret;
    char action[];
} __attribute__((__packed__));

struct TruncateLog : public FileLog
{
    unsigned long i_ino;
    unsigned long length;
    long ret;
    char action[];

} __attribute__((__packed__));

struct IoctlLog : public FileLog
{
    unsigned long i_ino;
    unsigned int cmd;
    unsigned long arg;
    long ret;
    char action[];
} __attribute__((__packed__));

struct RenameLog : public FileLog
{
    unsigned long i_ino;
    unsigned int old_name;
    unsigned int new_name;
    long ret;
    char action[];
} __attribute__((__packed__));

struct FallocateLog : public FileLog
{
    unsigned long i_ino;
    int mode;
    unsigned long offset;
    unsigned long len;
    long ret;
    char action[];
} __attribute__((__packed__));

struct RwLog : public FileLog
{
    unsigned long i_ino;
    unsigned long count; // size of kernel buf
    unsigned long pos;
    long ret;
    char action[];
} __attribute__((__packed__));

struct RwvLog : public FileLog
{
    unsigned long i_ino;
    unsigned int sz_arr; // offset(againt 'action') of an array of reading size
    unsigned int count;  // count of 'size' in array
    unsigned long pos;
    long ret;
    char action[];
} __attribute__((__packed__));

struct CopyLog : public FileLog
{
    unsigned long from_ino;
    unsigned long to_ino;
    unsigned long from_pos;
    unsigned long to_pos;
    unsigned long size;
    long ret;
    char action[];
} __attribute__((__packed__));
struct DirLog : public FileLog
{
    unsigned long dir_ino;
    unsigned long ino;
    uint16_t mode;
    uint32_t dev;
    long ret;
    char action[];
} __attribute__((__packed__));

struct SymLinkLog : public FileLog
{
    unsigned long dir_ino;
    unsigned long ino; // new inode linked to old name
    unsigned int oldname;
    long ret;
    char action[];
} __attribute__((__packed__));

struct SeekLog : public FileLog
{
    unsigned long i_ino;
    loff_t offset;
    int whence;
    long ret;
    char action[];
} __attribute__((__packed__));


#endif

#define TASK_COMM_LEN 16
#define FS_NAME_LEN 32
#define DATA_LEN 512
#define PATH_MAX 4096

struct mount_args
{
    pid_t pid;
    pid_t tid;
    unsigned int mnt_ns;
    char comm[TASK_COMM_LEN];
    int ret;
    /**
     * mount 系统调用传入的5个用户参数
     */
    char source[PATH_MAX / 2];
    char target[PATH_MAX / 2];
    char filesystemtype[FS_NAME_LEN];
    unsigned long flags;
    char data[DATA_LEN];
};

struct umount_args
{
    pid_t pid;
    pid_t tid;
    unsigned int mnt_ns;
    char comm[TASK_COMM_LEN];
    int ret;
    /**
     * umount 两个参数
     */
    char target[4016];
    int flags;
};

struct fsopen_args
{
    pid_t pid;
    pid_t tid;
    unsigned int mnt_ns;
    char comm[TASK_COMM_LEN];
    int ret;
    /**
     * fsopen 系统调用的两个参数
     */
    int flags;
    char fsname[FS_NAME_LEN];
};

struct fsconfig_args
{
    pid_t pid;
    pid_t tid;
    unsigned int mnt_ns;
    char comm[TASK_COMM_LEN];
    int ret;
    /**
     * fsconfig 系统调用的5个参数
     */
    int fd;
    int cmd;
    char key[256];
    char value[3754];
    int aux;
};

struct fsmount_args
{
    pid_t pid;
    pid_t tid;
    unsigned int mnt_ns;
    char comm[TASK_COMM_LEN];
    int ret;
    /**
     * fsmount 系统调用的3个参数
     */
    int fs_fd;
    int flags;
    int attr_flags;
};

struct move_mount_args
{
    pid_t pid;
    pid_t tid;
    unsigned int mnt_ns;
    char comm[TASK_COMM_LEN];
    int ret;
    /**
     * move_mount 系统调用的5个参数
     */
    int from_dfd;
    char from_pathname[2000];
    int to_dfd;
    char to_pathname[2000];
    unsigned int flags;
};

struct fspick_args
{
    pid_t pid;
    pid_t tid;
    unsigned int mnt_ns;
    char comm[TASK_COMM_LEN];
    int ret;
    /**
     * fspick 系统调用的3个参数
     */
    int dfd;
    char path[4000];
    unsigned int flags;
};

struct mount_setattr_args
{
    pid_t pid;
    pid_t tid;
    unsigned int mnt_ns;
    char comm[TASK_COMM_LEN];
    int ret;
    /**
     * mount_setattr 系统调用的5个参数
     */
    int dfd;
    char path[4000];
    unsigned int flags;
    struct mount_attr uattr;
    size_t usize;
};

struct open_tree_args
{
    pid_t pid;
    pid_t tid;
    unsigned int mnt_ns;
    char comm[TASK_COMM_LEN];
    int ret;
    /**
     * open_tree 系统调用的3个参数
     */
    int dfd;
    char filename[4056];
    unsigned flags;
};

enum IrqType
{
	SOFT_IRQ,
	IRQ,
};

#ifndef __VMLINUX_H__
enum
{
	HI_SOFTIRQ = 0,
	TIMER_SOFTIRQ = 1,
	NET_TX_SOFTIRQ = 2,
	NET_RX_SOFTIRQ = 3,
	BLOCK_SOFTIRQ = 4,
	IRQ_POLL_SOFTIRQ = 5,
	TASKLET_SOFTIRQ = 6,
	SCHED_SOFTIRQ = 7,
	HRTIMER_SOFTIRQ = 8,
	RCU_SOFTIRQ = 9,
	NR_SOFTIRQS = 10,
};
#endif

struct soft_irq_event_t
{
	pid_t pid;
	pid_t tid;
	char comm[16];
	unsigned long long delta;
	int ret;
	int vec_nr;
	enum IrqType type;
};

struct irq_event_t
{
	pid_t pid;
	pid_t tid;
	char comm[16];
	unsigned long long  delta;
	int ret;
	int vec_nr;
	enum IrqType type;
	char name[128];
};
