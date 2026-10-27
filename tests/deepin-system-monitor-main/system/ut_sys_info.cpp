// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Unit tests for core::system::SysInfo (deepin-system-monitor-main/system/sys_info.cpp).
// All external dependencies are stubbed: /proc files via fopen->fmemopen,
// /proc directory scans via QDir::entryInfoList, loadavg via QFile methods and
// hostname via the DBus property interface. No real system state is touched.

#include <gtest/gtest.h>

#include <QDBusReply>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <DSysInfo>

#include <arpa/inet.h>
#include <sys/utsname.h>
#include <unistd.h>

#include "common/hash.h"
#include "common/thread_manager.h"
#include "system/packet.h"
#include "system/sys_info.h"
#include "system/system_monitor_thread.h"
#include "ut_stub_helpers.h"

using stub_ext::StubExt;
using ut_helpers::DBusState;
using ut_helpers::ProcFiles;

namespace {

// ---------------------------------------------------------------------------
// /proc/net/tcp style content.
// Column layout: sl local rem st tx:rx tr:when retrnsmt uid timeout inode
// 0100007F:1F90 -> 127.0.0.1:8080 ; 00000000:0000 -> 0.0.0.0:0
// ---------------------------------------------------------------------------
static const char kProcNetTcp[] =
        "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
        "   0: 0100007F:1F90 00000000:0000 0A 00000000:00000000 00:00000000:00000000 00000000  1000        0 33892 1 0000000000000000 100 0 0 10 0\n";

// IPv6 socket with an IPv4-mapped address (::ffff:127.0.0.1:80 <-> ::ffff:10.0.0.2:1024).
// NOTE: the mapped-detection requires s6_addr32[2] == 0xFFFF0000, i.e. hex
// words FFFF0000...; real-kernel lines print 0000FFFF..., which the current
// implementation does NOT convert (see defects.json candidate).
static const char kProcNetTcp6Mapped[] =
        "  sl  local_address                         remote_address                        st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
        "   0: 0000000000000000FFFF00000100007F:0050 0000000000000000FFFF00000200000A:0400 01 00000000:00000000 00:00000000:00000000 00000000  1000        0 33893 1 0000000000000000 100 0 0 10 0\n";

// Pure IPv6 socket: 2001:db8::1:80 <-> 2001:db8::2:443.
static const char kProcNetTcp6Pure[] =
        "  sl  local_address                         remote_address                        st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
        "   0: 20010DB8000000000000000000000001:0050 20010DB8000000000000000000000002:01BB 01 00000000:00000000 00:00000000:00000000 00000000  1000        0 33894 1 0000000000000000 100 0 0 10 0\n";

static const char kProcNetUdp[] =
        "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode ref pointer drops\n"
        "   1: 00000000:0035 00000000:0000 07 00000000:00000000 00:00000000:00000000     0        0 22613 2 0000000000000000 100 0 0 10 0\n";

static const char kProcNetUdp6[] =
        "  sl  local_address                         remote_address                        st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode ref pointer drops\n"
        "   1: 00000000000000000000000000000000:0053 00000000000000000000000000000000:0000 07 00000000:00000000 00:00000000:00000000     0        0 22614 2 0000000000000000 100 0 0 10 0\n";

// Recompute the expected hash exactly the way parseSocks does.
static uint64_t sockHash(const QString &pattern)
{
    const QByteArray fmtbuf = pattern.toLocal8Bit();
    uint64_t cchash[2] {0, 0};
    util::common::hash(fmtbuf.constData(), fmtbuf.length(), util::common::global_seed, cchash);
    return cchash[0];
}

class UT_SysInfo : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_proc.clear();
        m_proc.install(m_stub);
    }

    void TearDown() override
    {
        m_proc.clear();
    }

    // Installs the QDir/QFileInfo stubs used by read_processes / read_threads.
    // fakeDirs maps a directory path to the entry names entryInfoList returns.
    void installDirStubs(const QHash<QString, QStringList> &fakeDirs)
    {
        m_dirEntries = fakeDirs;
        using EntryInfoFn = QFileInfoList (QDir::*)(QDir::Filters, QDir::SortFlags) const;
        auto *entries = &m_dirEntries;
        m_stub.set_lamda(static_cast<EntryInfoFn>(&QDir::entryInfoList),
                         [entries](const QDir *self, QDir::Filters, QDir::SortFlags) -> QFileInfoList {
                             QFileInfoList list;
                             const QDir curDir = self->path();
                             for (const QString &name : entries->value(self->path()))
                                 list.append(QFileInfo(curDir, name));
                             return list;
                         });
        // Every fake entry pretends to be a directory.
        using IsDirFn = bool (QFileInfo::*)() const;
        m_stub.set_lamda(static_cast<IsDirFn>(&QFileInfo::isDir),
                         [](const QFileInfo *) -> bool { return true; });
    }

    StubExt m_stub;
    ProcFiles m_proc;
    QHash<QString, QStringList> m_dirEntries;
};

// ---------------------------------------------------------------------------
// Fresh SysInfo: private data zero-initialised, loadAvg allocated empty.
// Branch: SysInfoPrivate() member-init list (all zeros).
// ---------------------------------------------------------------------------
TEST_F(UT_SysInfo, Construction_DefaultObject_FieldsZero)
{
    // Act
    core::system::SysInfo info;

    // Assert
    EXPECT_EQ(0u, info.nprocesses());
    EXPECT_EQ(0u, info.nthreads());
    EXPECT_EQ(0u, info.nfds());
    EXPECT_TRUE(info.hostname().isEmpty());
    EXPECT_TRUE(info.arch().isEmpty());
    ASSERT_NE(nullptr, info.loadAvg().get());
    EXPECT_FLOAT_EQ(0.f, info.loadAvg()->lavg_1m);
    EXPECT_FLOAT_EQ(0.f, info.loadAvg()->lavg_5m);
    EXPECT_FLOAT_EQ(0.f, info.loadAvg()->lavg_15m);
}

// ---------------------------------------------------------------------------
// Copy semantics: private data is shared through QSharedDataPointer.
// Branch: copy ctor / operator= share d pointer.
// ---------------------------------------------------------------------------
TEST_F(UT_SysInfo, CopyAssignment_SharesUnderlyingData)
{
    // Arrange
    core::system::SysInfo source;
    source.set_nprocesses(42);   // private setter, reachable via -fno-access-control

    // Act
    core::system::SysInfo copied(source);
    core::system::SysInfo assigned;
    assigned = source;

    // Assert
    EXPECT_EQ(42u, copied.nprocesses());
    EXPECT_EQ(42u, assigned.nprocesses());
}

// ---------------------------------------------------------------------------
// read_file_nr: valid "/proc/sys/fs/file-nr" content.
// Branch: fopen ok -> fgets ok -> sscanf "%u" == 1 -> return value.
// ---------------------------------------------------------------------------
TEST_F(UT_SysInfo, Read_file_nr_ValidContent_ReturnsAllocatedCount)
{
    // Arrange
    m_proc.add("/proc/sys/fs/file-nr", "  1088\t0\t2097152\n");
    core::system::SysInfo info;

    // Act
    const quint32 nr = info.read_file_nr();

    // Assert
    EXPECT_EQ(1088u, nr);
}

// ---------------------------------------------------------------------------
// read_file_nr: fopen fails -> 0.
// Branch: fopen(PROC_PATH_FILE_NR) == nullptr -> warn + return 0.
// ---------------------------------------------------------------------------
TEST_F(UT_SysInfo, Read_file_nr_OpenFails_ReturnsZero)
{
    // Arrange: no file registered
    core::system::SysInfo info;

    // Act
    EXPECT_EQ(0u, info.read_file_nr());
}

// ---------------------------------------------------------------------------
// read_file_nr: content does not start with a number -> 0.
// Branch: fgets ok -> sscanf != 1 -> warn + return 0.
// ---------------------------------------------------------------------------
TEST_F(UT_SysInfo, Read_file_nr_MalformedContent_ReturnsZero)
{
    // Arrange
    m_proc.add("/proc/sys/fs/file-nr", "garbage\n");
    core::system::SysInfo info;

    // Act
    EXPECT_EQ(0u, info.read_file_nr());
}

// ---------------------------------------------------------------------------
// read_uptime: valid "seconds.microseconds" line.
// Branch: fopen ok -> fgets ok -> sscanf "%ld.%ld" == 2 -> assign + return.
// NOTE: the implementation multiplies the microsecond field by 1000000
// instead of 1000 (uptime "12345.67" -> tv_usec = 67000000). The current
// behaviour is asserted; the defect is recorded in defects.json.
// ---------------------------------------------------------------------------
TEST_F(UT_SysInfo, Read_uptime_ValidContent_AssignsSecondsAndMicros)
{
    // Arrange
    m_proc.add("/proc/uptime", "12345.67 456789.01\n");
    core::system::SysInfo info;

    // Act
    struct timeval up {};
    info.read_uptime(up);

    // Assert (documents current behaviour; usec scaling is defective, see defects)
    EXPECT_EQ(12345, up.tv_sec);
    EXPECT_EQ(67000000, up.tv_usec);
}

// ---------------------------------------------------------------------------
// read_uptime: malformed line -> uptime struct unchanged.
// Branch: sscanf != 2 -> warn + return without assignment.
// ---------------------------------------------------------------------------
TEST_F(UT_SysInfo, Read_uptime_MalformedContent_LeavesStructUnchanged)
{
    // Arrange
    m_proc.add("/proc/uptime", "not a time\n");
    core::system::SysInfo info;

    // Act
    struct timeval up {77, 88};
    info.read_uptime(up);

    // Assert
    EXPECT_EQ(77, up.tv_sec);
    EXPECT_EQ(88, up.tv_usec);
}

// ---------------------------------------------------------------------------
// read_uptime: missing file -> struct unchanged.
// Branch: fopen == nullptr -> warn + return.
// ---------------------------------------------------------------------------
TEST_F(UT_SysInfo, Read_uptime_OpenFails_LeavesStructUnchanged)
{
    // Arrange
    core::system::SysInfo info;

    // Act
    struct timeval up {1, 2};
    info.read_uptime(up);

    // Assert
    EXPECT_EQ(1, up.tv_sec);
    EXPECT_EQ(2, up.tv_usec);
}

// ---------------------------------------------------------------------------
// read_btime: "/proc/stat" containing a btime line among other lines.
// Branch: fgets loop -> strncmp("btime", 5) == 0 -> sscanf ok -> assign + return.
// ---------------------------------------------------------------------------
TEST_F(UT_SysInfo, Read_btime_ValidContent_ParsesBootTime)
{
    // Arrange
    m_proc.add("/proc/stat",
               "cpu  100 0 200 300 0 0 0 0 0 0\n"
               "btime 1600000000\n"
               "intr 12345\n");
    core::system::SysInfo info;

    // Act
    struct timeval btime {};
    info.read_btime(btime);

    // Assert
    EXPECT_EQ(1600000000, btime.tv_sec);
    EXPECT_EQ(0, btime.tv_usec);
}

// ---------------------------------------------------------------------------
// read_btime: no btime key in file -> struct unchanged.
// Branch: while loop exhausts -> warn + return.
// ---------------------------------------------------------------------------
TEST_F(UT_SysInfo, Read_btime_KeyMissing_LeavesStructUnchanged)
{
    // Arrange
    m_proc.add("/proc/stat", "cpu  100 0 200 300 0 0 0 0 0 0\n");
    core::system::SysInfo info;

    // Act
    struct timeval btime {5, 6};
    info.read_btime(btime);

    // Assert
    EXPECT_EQ(5, btime.tv_sec);
    EXPECT_EQ(6, btime.tv_usec);
}

// ---------------------------------------------------------------------------
// read_btime: missing file -> struct unchanged.
// Branch: fopen == nullptr -> warn + return.
// ---------------------------------------------------------------------------
TEST_F(UT_SysInfo, Read_btime_OpenFails_LeavesStructUnchanged)
{
    // Arrange
    core::system::SysInfo info;

    // Act
    struct timeval btime {9, 9};
    info.read_btime(btime);

    // Assert
    EXPECT_EQ(9, btime.tv_sec);
}

// ---------------------------------------------------------------------------
// read_loadavg: file exists, readable, line has > 3 fields.
// Branch: exists() && open() -> readLine -> split size > 3 -> assign 3 values.
// ---------------------------------------------------------------------------
TEST_F(UT_SysInfo, Read_loadavg_ValidLine_SetsThreeAverages)
{
    // Arrange
    using ExistsFn = bool (QFile::*)() const;
    m_stub.set_lamda(static_cast<ExistsFn>(&QFile::exists),
                     [](const QFile *) -> bool { return true; });
    // QFile::open is virtual: patch its real code address (see helper)
    using OpenFn = bool (*)(QFile *, QIODevice::OpenMode);
    auto *openFn = reinterpret_cast<OpenFn>(ut_helpers::qtFileOpenSymbol());
    ASSERT_NE(nullptr, openFn);
    m_stub.set_lamda(openFn,
                     [](QFile *, QIODevice::OpenMode) -> bool { return true; });
    using ReadLineFn = QByteArray (QIODevice::*)(qint64);
    m_stub.set_lamda(static_cast<ReadLineFn>(&QIODevice::readLine),
                     [](QIODevice *, qint64) -> QByteArray {
                         return "0.41 0.46 0.36 2/2646 20183\n";
                     });

    core::system::SysInfo info;
    auto loadAvg = std::make_shared<core::system::load_avg_t>();

    // Act
    info.read_loadavg(loadAvg);

    // Assert
    EXPECT_FLOAT_EQ(0.41f, loadAvg->lavg_1m);
    EXPECT_FLOAT_EQ(0.46f, loadAvg->lavg_5m);
    EXPECT_FLOAT_EQ(0.36f, loadAvg->lavg_15m);
}

// ---------------------------------------------------------------------------
// read_loadavg: line with too few fields -> values unchanged.
// Branch: split size <= 3 -> warn + return (falls out of if).
// ---------------------------------------------------------------------------
TEST_F(UT_SysInfo, Read_loadavg_ShortLine_LeavesValuesUnchanged)
{
    // Arrange
    using ExistsFn = bool (QFile::*)() const;
    m_stub.set_lamda(static_cast<ExistsFn>(&QFile::exists),
                     [](const QFile *) -> bool { return true; });
    // QFile::open is virtual: patch its real code address (see helper)
    using OpenFn = bool (*)(QFile *, QIODevice::OpenMode);
    auto *openFn = reinterpret_cast<OpenFn>(ut_helpers::qtFileOpenSymbol());
    ASSERT_NE(nullptr, openFn);
    m_stub.set_lamda(openFn,
                     [](QFile *, QIODevice::OpenMode) -> bool { return true; });
    using ReadLineFn = QByteArray (QIODevice::*)(qint64);
    m_stub.set_lamda(static_cast<ReadLineFn>(&QIODevice::readLine),
                     [](QIODevice *, qint64) -> QByteArray { return "0.41 0.46\n"; });

    core::system::SysInfo info;
    auto loadAvg = std::make_shared<core::system::load_avg_t>();
    loadAvg->lavg_1m = 9.f;

    // Act
    info.read_loadavg(loadAvg);

    // Assert
    EXPECT_FLOAT_EQ(9.f, loadAvg->lavg_1m);
}

// ---------------------------------------------------------------------------
// read_loadavg: file missing -> values unchanged.
// Branch: exists() == false -> skip whole if body -> warn.
// ---------------------------------------------------------------------------
TEST_F(UT_SysInfo, Read_loadavg_FileMissing_LeavesValuesUnchanged)
{
    // Arrange: default QFile::exists on a fake path returns false; stub it
    // explicitly to stay independent from the real file system.
    using ExistsFn = bool (QFile::*)() const;
    m_stub.set_lamda(static_cast<ExistsFn>(&QFile::exists),
                     [](const QFile *) -> bool { return false; });

    core::system::SysInfo info;
    auto loadAvg = std::make_shared<core::system::load_avg_t>();
    loadAvg->lavg_5m = 7.f;

    // Act
    info.read_loadavg(loadAvg);

    // Assert
    EXPECT_FLOAT_EQ(7.f, loadAvg->lavg_5m);
}

// ---------------------------------------------------------------------------
// read_processes: counts only numeric directory entries.
// Branches: info.isDir() && fileName().toInt() > 0 -> increment;
// non-numeric names ("self", ".", "..") are rejected.
// ---------------------------------------------------------------------------
TEST_F(UT_SysInfo, Read_processes_NumericDirsOnly_CountsMatches)
{
    // Arrange
    installDirStubs({ { "/proc", { ".", "..", "1000", "1001", "self", "cpuinfo" } } });
    core::system::SysInfo info;

    // Act
    const quint32 count = info.read_processes();

    // Assert
    EXPECT_EQ(2u, count);
}

// ---------------------------------------------------------------------------
// read_threads: for every numeric /proc dir, adds its task dir entry count.
// Branches: outer isDir + numeric; taskDir.entryInfoList().count() summed.
// ---------------------------------------------------------------------------
TEST_F(UT_SysInfo, Read_threads_SumsTaskDirEntries)
{
    // Arrange
    installDirStubs({ { "/proc", { "1000", "1001" } },
                      { "/proc/1000/task", { "1000", "1001" } },
                      { "/proc/1001/task", { "1001" } } });
    core::system::SysInfo info;

    // Act
    const quint32 threads = info.read_threads();

    // Assert
    EXPECT_EQ(3u, threads);
}

// ---------------------------------------------------------------------------
// readSysInfo: aggregates nfds/nprocs/nthreads/uptime/btime/loadavg from all
// sources with every source stubbed.
// Branches: all read_* success paths combined.
// ---------------------------------------------------------------------------
TEST_F(UT_SysInfo, ReadSysInfo_AllSourcesStubbed_AggregatesValues)
{
    // Arrange
    m_proc.add("/proc/sys/fs/file-nr", "  1088\t0\t2097152\n");
    m_proc.add("/proc/uptime", "12345.67 456789.01\n");
    m_proc.add("/proc/stat", "btime 1600000000\n");
    installDirStubs({ { "/proc", { "1000", "1001" } },
                      { "/proc/1000/task", { "1000", "1001" } },
                      { "/proc/1001/task", { "1001" } } });
    using ExistsFn = bool (QFile::*)() const;
    m_stub.set_lamda(static_cast<ExistsFn>(&QFile::exists),
                     [](const QFile *) -> bool { return true; });
    // QFile::open is virtual: patch its real code address (see helper)
    using OpenFn = bool (*)(QFile *, QIODevice::OpenMode);
    auto *openFn = reinterpret_cast<OpenFn>(ut_helpers::qtFileOpenSymbol());
    ASSERT_NE(nullptr, openFn);
    m_stub.set_lamda(openFn,
                     [](QFile *, QIODevice::OpenMode) -> bool { return true; });
    using ReadLineFn = QByteArray (QIODevice::*)(qint64);
    m_stub.set_lamda(static_cast<ReadLineFn>(&QIODevice::readLine),
                     [](QIODevice *, qint64) -> QByteArray {
                         return "0.41 0.46 0.36 2/2646 20183\n";
                     });

    core::system::SysInfo info;

    // Act
    info.readSysInfo();

    // Assert
    EXPECT_EQ(1088u, info.nfds());
    EXPECT_EQ(2u, info.nprocesses());
    EXPECT_EQ(3u, info.nthreads());
    EXPECT_EQ(12345, info.uptime().tv_sec);
    EXPECT_EQ(1600000000, info.btime().tv_sec);
    EXPECT_FLOAT_EQ(0.41f, info.loadAvg()->lavg_1m);
}

// ---------------------------------------------------------------------------
// readSysInfoStatic: identity fields come from the real user context while
// hostname comes from the (stubbed) hostname1 DBus property.
// Branches: getuid/geteuid/getgid/getegid + userName/groupName lookups +
// read_hostname via property stub.
// ---------------------------------------------------------------------------
TEST_F(UT_SysInfo, ReadSysInfoStatic_IdentityConsistentWithRealUser)
{
    // Arrange
    DBusState dbus;
    dbus.hostname = "ut-test-host";
    ut_helpers::installDBusStubs(m_stub, dbus);

    core::system::SysInfo info;

    // Act
    info.readSysInfoStatic();

    // Assert
    EXPECT_EQ(getuid(), info.uid());
    EXPECT_EQ(geteuid(), info.euid());
    EXPECT_EQ(getgid(), info.gid());
    EXPECT_EQ(getegid(), info.egid());

    const QByteArray expectedUser = core::system::SysInfo::userName(getuid());
    EXPECT_FALSE(expectedUser.isEmpty());
    EXPECT_EQ(expectedUser, info.realUserName());

    const QString expectedArch = [] {
        struct utsname un {};
        if (uname(&un) != 0)
            return QString();
        return QString(un.machine);
    }();
    EXPECT_EQ(expectedArch, info.arch());

    const QString expectedVersion = QString("%1 %2")
                                            .arg(DSysInfo::majorVersion())
                                            .arg(DSysInfo::uosEditionName());
    EXPECT_EQ(expectedVersion, info.version());
    EXPECT_EQ(QString("ut-test-host"), info.hostname());
}

// ---------------------------------------------------------------------------
// Static userName/groupName helpers.
// Branches: getpwuid hit -> name; getpwuid miss -> empty QByteArray;
// getgrgid hit / miss likewise.
// ---------------------------------------------------------------------------
TEST_F(UT_SysInfo, UserName_GivenRealUid_ReturnsPasswdName)
{
    // Act
    const QByteArray name = core::system::SysInfo::userName(getuid());

    // Assert
    const passwd *pwd = getpwuid(getuid());
    ASSERT_NE(nullptr, pwd);
    EXPECT_EQ(QByteArray(pwd->pw_name), name);
}

TEST_F(UT_SysInfo, UserName_GivenInvalidUid_ReturnsEmpty)
{
    // Act
    const QByteArray name = core::system::SysInfo::userName(59999);

    // Assert (boundary: no passwd entry for this uid on a standard system)
    EXPECT_TRUE(name.isEmpty());
}

TEST_F(UT_SysInfo, GroupName_GivenInvalidGid_ReturnsEmpty)
{
    // Act
    const QByteArray name = core::system::SysInfo::groupName(59999);

    // Assert (boundary: no group entry for this gid on a standard system)
    EXPECT_TRUE(name.isEmpty());
}

// ---------------------------------------------------------------------------
// readSockStat: only /proc/net/tcp present. The single TCP socket must be
// registered under both directions (bidirectional mapping) and parseSocks
// must report failure for the three missing files.
// Branches: fopen fail -> false; header line (nr == 0) skipped; TCP reverse
// mapping inserted.
// ---------------------------------------------------------------------------
TEST_F(UT_SysInfo, ReadSockStat_TcpOnly_InsertsForwardAndReverseEntries)
{
    // Arrange
    m_proc.add("/proc/net/tcp", kProcNetTcp);
    core::system::SockStatMap statMap;

    // Act
    const bool ok = core::system::SysInfo::readSockStat(statMap);

    // Assert
    EXPECT_FALSE(ok);   // 3 of 4 files missing

    const uint64_t fwd = sockHash("127.0.0.1:8080-0.0.0.0:0");
    const uint64_t rev = sockHash("0.0.0.0:0-127.0.0.1:8080");
    EXPECT_EQ(2, statMap.size());
    ASSERT_TRUE(statMap.contains(fwd));
    ASSERT_TRUE(statMap.contains(rev));

    auto stat = statMap.value(fwd);
    EXPECT_EQ(8080u, stat->s_port);
    EXPECT_EQ(0u, stat->d_port);
    EXPECT_EQ(1000u, stat->uid);
    EXPECT_EQ(33892, stat->ino);
    EXPECT_EQ(AF_INET, stat->sa_family);
    EXPECT_EQ(IPPROTO_TCP, stat->proto);
    // both directions point at the very same record
    EXPECT_EQ(stat.data(), statMap.value(rev).data());
}

// ---------------------------------------------------------------------------
// readSockStat: all four files present -> ok == true, TCP entries doubled,
// UDP entries single.
// Branches: all four parseSocks success paths.
// ---------------------------------------------------------------------------
TEST_F(UT_SysInfo, ReadSockStat_AllFilesPresent_ReturnsTrue)
{
    // Arrange
    m_proc.add("/proc/net/tcp", kProcNetTcp);
    m_proc.add("/proc/net/udp", kProcNetUdp);
    m_proc.add("/proc/net/tcp6", kProcNetTcp6Pure);
    m_proc.add("/proc/net/udp6", kProcNetUdp6);
    core::system::SockStatMap statMap;

    // Act
    const bool ok = core::system::SysInfo::readSockStat(statMap);

    // Assert
    EXPECT_TRUE(ok);
    // TCP: 2 mappings, UDP: 1, TCP6: 2, UDP6: 1
    EXPECT_EQ(6, statMap.size());
    EXPECT_TRUE(statMap.contains(sockHash("127.0.0.1:8080-0.0.0.0:0")));
    EXPECT_TRUE(statMap.contains(sockHash("0.0.0.0:53-0.0.0.0:0")));
    // IPv6 formatting must match inet_ntop's compressed form
    EXPECT_TRUE(statMap.contains(sockHash("b80d:120::100:0:80-b80d:120::200:0:443")));
    // all-zero IPv6 renders as "::"; pattern becomes ":::53-:::0"
    EXPECT_TRUE(statMap.contains(sockHash(":::83-:::0")));  // port hex 0053 -> 83
}

// ---------------------------------------------------------------------------
// readSockStat: header line and sockets still waiting (inode == 0) are
// skipped.
// Branches: nr == 0 -> continue; ino == 0 -> continue.
// ---------------------------------------------------------------------------
TEST_F(UT_SysInfo, ReadSockStat_HeaderAndWaitingSockets_Skipped)
{
    // Arrange
    m_proc.add("/proc/net/tcp",
               "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
               "   0: 0100007F:1F90 00000000:0000 0A 00000000:00000000 00:00000000:00000000 00000000  1000        0     0 1 0000000000000000 100 0 0 10 0\n");
    core::system::SockStatMap statMap;

    // Act
    const bool ok = core::system::SysInfo::readSockStat(statMap);

    // Assert
    EXPECT_FALSE(ok);
    EXPECT_EQ(0, statMap.size());
}

// ---------------------------------------------------------------------------
// readSockStat: IPv4-mapped IPv6 addresses are converted to AF_INET.
// Branch: family == AF_INET6 -> s6_addr32[0..1] == 0 && [2] == 0xffff0000 ->
// sa_family = AF_INET, addresses copied from the 4th word.
// ---------------------------------------------------------------------------
TEST_F(UT_SysInfo, ReadSockStat_IPv4MappedIPv6_ConvertsToIPv4)
{
    // Arrange
    m_proc.add("/proc/net/tcp6", kProcNetTcp6Mapped);
    core::system::SockStatMap statMap;

    // Act
    core::system::SysInfo::readSockStat(statMap);

    // Assert
    ASSERT_FALSE(statMap.isEmpty());
    auto stat = statMap.value(sockHash("127.0.0.1:80-10.0.0.2:1024"));
    ASSERT_FALSE(stat.isNull());
    EXPECT_EQ(AF_INET, stat->sa_family);
    EXPECT_EQ(80u, stat->s_port);
    EXPECT_EQ(1024u, stat->d_port);
    EXPECT_EQ(33893, stat->ino);
}

// ---------------------------------------------------------------------------
// readSockStat: pure IPv6 addresses keep AF_INET6 and are keyed by the
// compressed inet_ntop representation.
// Branch: mapped-address predicate false -> family stays AF_INET6.
// ---------------------------------------------------------------------------
TEST_F(UT_SysInfo, ReadSockStat_PureIPv6_KeepsFamilyAndCompressedKey)
{
    // Arrange
    m_proc.add("/proc/net/tcp6", kProcNetTcp6Pure);
    core::system::SockStatMap statMap;

    // Act
    core::system::SysInfo::readSockStat(statMap);

    // Assert
    ASSERT_FALSE(statMap.isEmpty());
    // The implementation parses %08x words and renders via inet_ntop, which
    // byte-swaps each word on little-endian (defect candidate, see defects.json).
    auto stat = statMap.value(sockHash("b80d:120::100:0:80-b80d:120::200:0:443"));
    ASSERT_FALSE(stat.isNull());
    EXPECT_EQ(AF_INET6, stat->sa_family);
    EXPECT_EQ(80u, stat->s_port);
    EXPECT_EQ(443u, stat->d_port);
    EXPECT_EQ(33894, stat->ino);
}

// ---------------------------------------------------------------------------
// readSockStat: no file available at all -> empty map, false.
// Branch: every fopen fails -> every parseSocks returns false.
// ---------------------------------------------------------------------------
TEST_F(UT_SysInfo, ReadSockStat_NoFilesAvailable_EmptyMapAndFalse)
{
    // Arrange
    core::system::SockStatMap statMap;

    // Act
    const bool ok = core::system::SysInfo::readSockStat(statMap);

    // Assert
    EXPECT_FALSE(ok);
    EXPECT_EQ(0, statMap.size());
}

// ---------------------------------------------------------------------------
// instance(): resolves the monitor-owned SysInfo through the thread manager.
// Branch: ThreadManager::instance()->thread<>() -> systemMonitorInstance()->
// sysInfo(); requires a real attached SystemMonitorThread (its heavy
// constructor is proven safe in the baseline suite).
// ---------------------------------------------------------------------------
TEST_F(UT_SysInfo, Instance_ReturnsThreadOwnedSysInfo)
{
    // Arrange: the harness may already have attached a SystemMonitorThread;
    // reuse it, otherwise attach a fresh one (its heavy constructor is
    // proven safe in the baseline suite).
    auto *tm = common::core::ThreadManager::instance();
    auto *thread = tm->thread<core::system::SystemMonitorThread>(
            common::core::BaseThread::kSystemMonitorThread);
    if (!thread) {
        thread = new core::system::SystemMonitorThread();
        tm->attach(thread);
    }
    ASSERT_NE(nullptr, thread);
    ASSERT_NE(nullptr, thread->systemMonitorInstance());

    // Act
    core::system::SysInfo *info = core::system::SysInfo::instance();

    // Assert
    ASSERT_NE(nullptr, info);
    EXPECT_EQ(thread->systemMonitorInstance()->sysInfo(), info);
}

} // namespace
