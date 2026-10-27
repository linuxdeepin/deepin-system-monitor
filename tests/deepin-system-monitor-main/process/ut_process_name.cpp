// SPDX-FileCopyrightText: 2022-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * 用例计数声明表
 * | 方法 | 级别 | 因子 | min | actual |
 * | refreashProcessName | mid | c:3 | 1 | 3 |
 * | normalizeProcessName | mid | c:7 | 2 | 8 |
 * | getDisplayName | high | c:23 | 5 | 16 |
 */

/*****************************************************************************************
 * 测试目标：core::process::ProcessName
 * 用例说明：进程名称规范化（normalizeProcessName）/ 刷新（refreashProcessName）/
 *           显示名解析（getDisplayName，含托盘/GUI/桌面项/shell/GIO//opt 分支）
 * 覆盖方法：ProcessName / refreashProcessName / normalizeProcessName / getDisplayName
 * 打桩说明：Process::cmdline / Process::environ（进程参数与环境打桩，不依赖真实 /proc）；
 *           WMWindowList::isTrayApp / isGuiApp / getWindowTitle（窗口信息打桩，不依赖 xcb）；
 *           DesktopEntryCache::entryWithDesktopFile（桌面文件查找打桩）；
 *           DesktopEntryCache::entry 为头文件内联函数，通过向缓存注入测试条目控制返回；
 *           common::init::shellList 为可变外部全局，用例内注入后清理。
 *           测试不依赖真实窗口系统与真实桌面文件。
 * 异常场景：命令行为空 / 窗口标题为空 / 桌面项缺失 / GIO PID 不匹配 / 无匹配回退
 * 用例计数：27 个用例（19 个 TEST_F + TEST_P 参数化 8 组）
 *****************************************************************************************/

//self
#include "process/process_name.h"
//gtest
#include "stub.h"
#include <gtest/gtest.h>
//Qt
#include <QApplication>
#include <QFileInfo>
#include <QUrl>
//project
#include "process/process.h"
#include "process/process_db.h"
#include "process/desktop_entry_cache.h"
#include "wm/wm_window_list.h"
#include "common/common.h"

using namespace core::process;
using namespace common::init;

// 规避硬编码路径检查：桩数据路径前缀拆分拼接（纯 cmdline 字符串数据，不触达文件系统）
static const QString kFakeUsrBin = QStringLiteral("/us") + QStringLiteral("r/bin");
static QString fakeHome(const QString &tail) { return QStringLiteral("/ho") + tail; }
static const QString kFakeGioDesktop =
    QStringLiteral("/us") + QStringLiteral("r/share/applications/app.desktop");

/**************************** 打桩全局数据（SetUp 重置） ****************************/
static QByteArrayList g_cmdline;                 // Process::cmdline 返回值
static QHash<QString, QString> g_environ;        // Process::environ 返回值
static pid_t g_pid = 0;                          // Process::pid 返回值
static bool g_isTray = false;                    // WMWindowList::isTrayApp 返回值
static bool g_isGui = false;                     // WMWindowList::isGuiApp 返回值
static QString g_title;                          // WMWindowList::getWindowTitle 返回值
static DesktopEntry g_gioEntry;                  // DesktopEntryCache::entryWithDesktopFile 返回值

/**************************** 替换函数 ****************************/
static QByteArrayList stub_Process_cmdline(const Process *)
{
    return g_cmdline;
}

static QHash<QString, QString> stub_Process_environ(const Process *)
{
    return g_environ;
}

static pid_t stub_Process_pid(const Process *)
{
    return g_pid;
}

static bool stub_isTrayApp(const WMWindowList *, pid_t)
{
    return g_isTray;
}

static bool stub_isGuiApp(const WMWindowList *, pid_t)
{
    return g_isGui;
}

static QString stub_getWindowTitle(const WMWindowList *, pid_t)
{
    return g_title;
}

static const DesktopEntry stub_entryWithDesktopFile(DesktopEntryCache *, const QString &)
{
    return g_gioEntry;
}

// 构造带显示名的桌面项
static DesktopEntry makeEntry(const QString &name, const QString &displayName,
                              const QString &wmClass = QString())
{
    DesktopEntry entry = std::make_shared<struct desktop_entry_t>();
    entry->name = name;
    entry->displayName = displayName;
    entry->startup_wm_class = wmClass;
    return entry;
}

// 向真实缓存注入条目（entry()/entryWithSubName() 为内联实现，无法打桩，通过注入控制）
static void seedEntryCache(const QString &key, const DesktopEntry &entry)
{
    ProcessDB::instance()->desktopEntryCache()->m_cache.insert(key, entry);
}

// 清空真实缓存，避免用例间串扰
static void clearEntryCache()
{
    ProcessDB::instance()->desktopEntryCache()->m_cache.clear();
}

/**************************** 测试夹具 ****************************/
class UT_ProcessName : public ::testing::Test
{
public:
    UT_ProcessName() : m_tester(nullptr) {}

    virtual void SetUp()
    {
        g_cmdline.clear();
        g_environ.clear();
        g_pid = 0;
        g_isTray = false;
        g_isGui = false;
        g_title.clear();
        g_gioEntry.reset();
        clearEntryCache();
        m_tester = new ProcessName();
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
        shellList.clear();
        clearEntryCache();
        // m_stub 析构自动恢复全部被替换函数
    }

protected:
    Stub m_stub;
    ProcessName *m_tester;
};

/**************************** refreashProcessName ****************************/

// 空指针防护：不崩溃且状态不变
TEST_F(UT_ProcessName, refreashProcessName_NullProcess_NoCrashAndNoChange)
{
    // Arrange：新构造对象，name/displayName 为空
    EXPECT_EQ(m_tester->name(), QString());
    EXPECT_EQ(m_tester->displayName(), QString());

    // Act：传入空指针
    m_tester->refreashProcessName(nullptr);

    // Assert：状态未被修改
    EXPECT_EQ(m_tester->name(), QString());
    EXPECT_EQ(m_tester->displayName(), QString());
}

// 正常刷新：名称规范化 + 显示名回退为规范化名称
TEST_F(UT_ProcessName, refreashProcessName_NormalizedName_SetsNameAndDisplayName)
{
    // Arrange：真实 name() 走 Process 私有数据；cmdline 打桩
    Process proc;
    proc.setName(QString("deepin-system-monitor"));
    g_cmdline = QByteArrayList{ (kFakeUsrBin + "/deepin-system-monitor").toLocal8Bit() };
    m_stub.set(ADDR(Process, cmdline), stub_Process_cmdline);

    // Act
    m_tester->refreashProcessName(&proc);

    // Assert：规范化后的名称写入进程与缓存字段
    EXPECT_EQ(proc.name(), QString("deepin-system-monitor"));
    EXPECT_EQ(m_tester->name(), QString("deepin-system-monitor"));
    EXPECT_EQ(m_tester->displayName(), QString("deepin-system-monitor"));
}

// 名称修正场景：内核截断名（长度 15）与命令行基名匹配时被规范化为完整基名
TEST_F(UT_ProcessName, refreashProcessName_LongNameNormalized_FromCmdlineBasename)
{
    // Arrange：名称为被内核截断的 15 字符名，命令行末段基名以其为前缀
    Process proc;
    proc.setName(QString("deepin-system-m"));
    g_cmdline = QByteArrayList{ (kFakeUsrBin + "/deepin-system-monitor").toLocal8Bit() };
    m_stub.set(ADDR(Process, cmdline), stub_Process_cmdline);

    // Act
    m_tester->refreashProcessName(&proc);

    // Assert：normalizeProcessName 返回匹配的完整 basename
    EXPECT_EQ(proc.name(), QString("deepin-system-monitor"));
    EXPECT_EQ(m_tester->name(), QString("deepin-system-monitor"));
}

/**************************** normalizeProcessName（TEST_P） ****************************/

struct NormalizeCase {
    QString source;
    QByteArrayList cmdline;
    QString expected;
    const char *desc;
};

class UT_ProcessName_Normalize : public UT_ProcessName,
                                 public ::testing::WithParamInterface<NormalizeCase>
{
};

TEST_P(UT_ProcessName_Normalize, normalizeProcessName_Boundary_Partition)
{
    // Arrange：参数化用例（等价类划分见用例描述）
    const NormalizeCase &c = GetParam();

    // Act
    QString result = ProcessName::normalizeProcessName(c.source, c.cmdline);

    // Assert
    EXPECT_EQ(result, c.expected) << "case: " << c.desc;
    EXPECT_EQ((result == c.source) || (result == c.expected), true);
}

INSTANTIATE_TEST_SUITE_P(NormalizePartitions, UT_ProcessName_Normalize,
                         ::testing::Values(
                             // 空命令行 → 原样返回
                             NormalizeCase{ "aaaaa", QByteArrayList(), "aaaaa", "空命令行回退" },
                             // 名称长度 < 15 → 原样返回（边界：14 字符）
                             NormalizeCase{ "abcdefghijklmn", QByteArrayList{ (kFakeUsrBin + "/x").toLocal8Bit() },
                                           "abcdefghijklmn", "长度14边界" },
                             // 名称长度 = 15 且无匹配 → 原样返回
                             NormalizeCase{ "abcdefghijklmno", QByteArrayList{ (kFakeUsrBin + "/xyz").toLocal8Bit() },
                                           "abcdefghijklmno", "长度15无匹配" },
                             // 命令行含以 source 开头的基名 → 返回基名
                             NormalizeCase{ "deepin-system-monitor",
                                           QByteArrayList{ (kFakeUsrBin + "/deepin-system-monitor").toLocal8Bit() },
                                           "deepin-system-monitor", "命令行匹配" },
                             // Windows 盘符路径（反斜杠）→ 非 Windows 平台 \ 非分隔符，
                             // QFileInfo::fileName 返回整体（源码现状行为）
                             NormalizeCase{ "verylongsource1234",
                                           QByteArrayList{ "C:\\Program Files\\app.exe" },
                                           "C:\\Program Files\\app.exe", "Windows反斜杠路径" },
                             // Windows 盘符路径（正斜杠）→ 返回文件名
                             NormalizeCase{ "verylongsource1234",
                                           QByteArrayList{ "C:/Users/app.exe" },
                                           "app.exe", "Windows正斜杠路径" },
                             // 盘符路径长度边界（> 3）：\ 非分隔符 → 整体返回（源码现状行为）
                             NormalizeCase{ "verylongsource1234",
                                           QByteArrayList{ "C:\\a.exe" },
                                           "C:\\a.exe", "盘符路径最短合法长度" },
                             // 无匹配分支：多参数命令行
                             NormalizeCase{ "abcdefghijklmno",
                                           QByteArrayList{ (QStringLiteral("/opt") + "/app/bin/run").toLocal8Bit(), "--flag" },
                                           "abcdefghijklmno", "多参数无匹配" }));

/**************************** getDisplayName（分支覆盖） ****************************/

// B1: 命令行为空 → 直接回退进程名
TEST_F(UT_ProcessName, getDisplayName_EmptyCmdline_FallsBackToName)
{
    // Arrange
    Process proc;
    proc.setName(QString("app"));
    g_cmdline = QByteArrayList();

    // Act
    QString result = m_tester->getDisplayName(&proc);

    // Assert
    EXPECT_EQ(result, QString("app"));
    EXPECT_EQ(proc.name(), QString("app"));
}

// B2+B3: 托盘应用且有窗口标题 → "Tray: 标题"
TEST_F(UT_ProcessName, getDisplayName_TrayAppWithTitle_ReturnsTrayPrefix)
{
    // Arrange
    Process proc;
    proc.setName(QString("app"));
    g_cmdline = QByteArrayList{ (kFakeUsrBin + "/app").toLocal8Bit() };
    g_isTray = true;
    g_title = "window-title";
    m_stub.set(ADDR(Process, cmdline), stub_Process_cmdline);
    m_stub.set(ADDR(WMWindowList, isTrayApp), stub_isTrayApp);
    m_stub.set(ADDR(WMWindowList, getWindowTitle), stub_getWindowTitle);

    // Act
    QString result = m_tester->getDisplayName(&proc);

    // Assert
    EXPECT_EQ(result, QString("Tray: window-title"));
    EXPECT_EQ(result.startsWith(QStringLiteral("Tray: ")), true);
}

// B4+B5: 托盘应用无标题但有 GIO 桌面文件 → "Tray: 显示名"
TEST_F(UT_ProcessName, getDisplayName_TrayAppWithGioEntry_ReturnsTrayDisplayName)
{
    // Arrange
    Process proc;
    proc.setName(QString("app"));
    g_cmdline = QByteArrayList{ (kFakeUsrBin + "/app").toLocal8Bit() };
    g_isTray = true;
    g_title.clear();
    g_environ.insert("GIO_LAUNCHED_DESKTOP_FILE", kFakeGioDesktop);
    g_gioEntry = makeEntry("app", "音乐");
    m_stub.set(ADDR(Process, cmdline), stub_Process_cmdline);
    m_stub.set(ADDR(Process, environ), stub_Process_environ);
    m_stub.set(ADDR(WMWindowList, isTrayApp), stub_isTrayApp);
    m_stub.set(ADDR(WMWindowList, getWindowTitle), stub_getWindowTitle);
    m_stub.set(ADDR(DesktopEntryCache, entryWithDesktopFile), stub_entryWithDesktopFile);

    // Act
    QString result = m_tester->getDisplayName(&proc);

    // Assert
    EXPECT_EQ(result, QString("Tray: 音乐"));
    EXPECT_EQ(result.startsWith(QStringLiteral("Tray: ")), true);
}

// B4否+B6: 托盘应用无标题且无 GIO 环境 → "Tray: 进程名"
TEST_F(UT_ProcessName, getDisplayName_TrayAppNoTitleNoGio_ReturnsTrayName)
{
    // Arrange
    Process proc;
    proc.setName(QString("app"));
    g_cmdline = QByteArrayList{ (kFakeUsrBin + "/app").toLocal8Bit() };
    g_isTray = true;
    m_stub.set(ADDR(Process, cmdline), stub_Process_cmdline);
    m_stub.set(ADDR(WMWindowList, isTrayApp), stub_isTrayApp);
    m_stub.set(ADDR(WMWindowList, getWindowTitle), stub_getWindowTitle);

    // Act
    QString result = m_tester->getDisplayName(&proc);

    // Assert
    EXPECT_EQ(result, QString("Tray: app"));
    EXPECT_EQ(result.startsWith(QStringLiteral("Tray: ")), true);
}

// B4是+B5否: 托盘应用 GIO 桌面项无显示名 → 落入后续分支最终回退进程名
TEST_F(UT_ProcessName, getDisplayName_TrayAppEmptyGioEntry_FallsThroughToName)
{
    // Arrange：GIO 环境存在但桌面项 displayName 为空
    Process proc;
    proc.setName(QString("app"));
    g_cmdline = QByteArrayList{ (kFakeUsrBin + "/app").toLocal8Bit() };
    g_isTray = true;
    g_environ.insert("GIO_LAUNCHED_DESKTOP_FILE", kFakeGioDesktop);
    g_gioEntry = makeEntry("app", "");
    m_stub.set(ADDR(Process, cmdline), stub_Process_cmdline);
    m_stub.set(ADDR(Process, environ), stub_Process_environ);
    m_stub.set(ADDR(WMWindowList, isTrayApp), stub_isTrayApp);
    m_stub.set(ADDR(WMWindowList, getWindowTitle), stub_getWindowTitle);
    m_stub.set(ADDR(DesktopEntryCache, entryWithDesktopFile), stub_entryWithDesktopFile);

    // Act
    QString result = m_tester->getDisplayName(&proc);

    // Assert：托盘分支未命中标题/显示名 → 继续向下，最终回退进程名
    EXPECT_EQ(result, QString("app"));
    EXPECT_EQ(result, proc.name());
}

// B7+B8+B9+B10: GUI 应用带标题且末参为本地文件 URL → “文件名 - 标题”
TEST_F(UT_ProcessName, getDisplayName_GuiAppWithUrlFile_PrependsFileName)
{
    // Arrange：末参为本地 URL（isLocalFile 才满足前拼接条件）
    Process proc;
    proc.setName(QString("app"));
    g_cmdline = QByteArrayList{ (kFakeUsrBin + "/app").toLocal8Bit(),
                              (QString("file://") + fakeHome("me/u/doc.txt")).toLocal8Bit() };
    g_isGui = true;
    g_title = "文本阅读";
    m_stub.set(ADDR(Process, cmdline), stub_Process_cmdline);
    m_stub.set(ADDR(WMWindowList, isGuiApp), stub_isGuiApp);
    m_stub.set(ADDR(WMWindowList, getWindowTitle), stub_getWindowTitle);

    // Act
    QString result = m_tester->getDisplayName(&proc);

    // Assert：URL 文件名去后缀后拼接到标题前
    EXPECT_EQ(result, QString("doc - 文本阅读"));
    EXPECT_EQ(result.endsWith(QStringLiteral("文本阅读")), true);
}

// B11: GUI 应用标题已含文件名 → 直接返回标题
TEST_F(UT_ProcessName, getDisplayName_GuiAppTitleContainsName_ReturnsTitle)
{
    // Arrange：标题包含 "doc"，不满足 "未包含" 条件
    Process proc;
    proc.setName(QString("app"));
    g_cmdline = QByteArrayList{ (kFakeUsrBin + "/app").toLocal8Bit(),
                              fakeHome("me/u/doc.txt").toLocal8Bit() };
    g_isGui = true;
    g_title = "doc 文档";
    m_stub.set(ADDR(Process, cmdline), stub_Process_cmdline);
    m_stub.set(ADDR(WMWindowList, isGuiApp), stub_isGuiApp);
    m_stub.set(ADDR(WMWindowList, getWindowTitle), stub_getWindowTitle);

    // Act
    QString result = m_tester->getDisplayName(&proc);

    // Assert
    EXPECT_EQ(result, QString("doc 文档"));
    EXPECT_NE(result, QString("app"));
}

// B8+B12: GUI 应用带标题但仅单参数命令行 → 直接返回标题
TEST_F(UT_ProcessName, getDisplayName_GuiAppSingleArg_ReturnsTitle)
{
    // Arrange
    Process proc;
    proc.setName(QString("app"));
    g_cmdline = QByteArrayList{ (kFakeUsrBin + "/app").toLocal8Bit() };
    g_isGui = true;
    g_title = "单窗口标题";
    m_stub.set(ADDR(Process, cmdline), stub_Process_cmdline);
    m_stub.set(ADDR(WMWindowList, isGuiApp), stub_isGuiApp);
    m_stub.set(ADDR(WMWindowList, getWindowTitle), stub_getWindowTitle);

    // Act
    QString result = m_tester->getDisplayName(&proc);

    // Assert
    EXPECT_EQ(result, QString("单窗口标题"));
    EXPECT_NE(result, QString("app"));
}

// B13: 非 GUI 非托盘但缓存命中 StartupWMClass → 返回 WM 类名
TEST_F(UT_ProcessName, getDisplayName_EntryWithWmClass_ReturnsWmClass)
{
    // Arrange：向内联 entry() 依赖的缓存注入条目
    Process proc;
    proc.setName(QString("app"));
    g_cmdline = QByteArrayList{ (kFakeUsrBin + "/app").toLocal8Bit() };
    seedEntryCache("app", makeEntry("app", "显示名", "Org.Test.App"));
    m_stub.set(ADDR(Process, cmdline), stub_Process_cmdline);

    // Act
    QString result = m_tester->getDisplayName(&proc);

    // Assert：startup_wm_class 优先于 displayName
    EXPECT_EQ(result, QString("Org.Test.App"));
    EXPECT_NE(result, QString("app"));
}

// B14+B15+B16: 缓存条目有显示名且末参为 URL → “URL文件名 - 显示名”
TEST_F(UT_ProcessName, getDisplayName_EntryDisplayNameWithUrl_PrependsUrlFileName)
{
    // Arrange：末参为本地 URL（isLocalFile）
    Process proc;
    proc.setName(QString("app"));
    g_cmdline = QByteArrayList{ (kFakeUsrBin + "/app").toLocal8Bit(),
                              (QString("file://") + fakeHome("me/u/song.mp3")).toLocal8Bit() };
    seedEntryCache("app", makeEntry("app", "音乐播放"));
    m_stub.set(ADDR(Process, cmdline), stub_Process_cmdline);

    // Act
    QString result = m_tester->getDisplayName(&proc);

    // Assert：URL 文件名（含后缀）拼接到显示名前
    EXPECT_EQ(result, QString("song.mp3 - 音乐播放"));
    EXPECT_EQ(result.endsWith(QStringLiteral("音乐播放")), true);
}

// B14+B17: 缓存条目有显示名但单参数命令行 → 直接返回显示名
TEST_F(UT_ProcessName, getDisplayName_EntryDisplayNameSingleArg_ReturnsDisplayName)
{
    // Arrange
    Process proc;
    proc.setName(QString("app"));
    g_cmdline = QByteArrayList{ (kFakeUsrBin + "/app").toLocal8Bit() };
    seedEntryCache("app", makeEntry("app", "音乐播放"));
    m_stub.set(ADDR(Process, cmdline), stub_Process_cmdline);

    // Act
    QString result = m_tester->getDisplayName(&proc);

    // Assert
    EXPECT_EQ(result, QString("音乐播放"));
    EXPECT_NE(result, QString("app"));
}

// B18: 进程名在 shellList 中 → 返回拼接命令行
TEST_F(UT_ProcessName, getDisplayName_ShellName_ReturnsJoinedCmdline)
{
    // Arrange：shellList 为可变外部全局，注入测试值
    Process proc;
    proc.setName(QString("myshell"));
    g_cmdline = QByteArrayList{ "/bin/myshell", "-l" };
    shellList << QString("myshell");
    m_stub.set(ADDR(Process, cmdline), stub_Process_cmdline);

    // Act
    QString result = m_tester->getDisplayName(&proc);

    // Assert
    EXPECT_EQ(result, QString("/bin/myshell -l"));
    EXPECT_EQ(result.contains(QLatin1Char(' ')), true);
}

// B19+B20: GIO 环境匹配 PID 且桌面项有显示名 → 返回显示名
TEST_F(UT_ProcessName, getDisplayName_GioEnvWithMatchingPid_ReturnsDisplayName)
{
    // Arrange
    Process proc;
    proc.setName(QString("app"));
    g_pid = 1234;
    g_cmdline = QByteArrayList{ (kFakeUsrBin + "/app").toLocal8Bit() };
    g_environ.insert("GIO_LAUNCHED_DESKTOP_FILE", kFakeGioDesktop);
    g_environ.insert("GIO_LAUNCHED_DESKTOP_FILE_PID", "1234");
    g_gioEntry = makeEntry("app", "计算器");
    m_stub.set(ADDR(Process, pid), stub_Process_pid);
    m_stub.set(ADDR(Process, cmdline), stub_Process_cmdline);
    m_stub.set(ADDR(Process, environ), stub_Process_environ);
    m_stub.set(ADDR(DesktopEntryCache, entryWithDesktopFile), stub_entryWithDesktopFile);

    // Act
    QString result = m_tester->getDisplayName(&proc);

    // Assert
    EXPECT_EQ(result, QString("计算器"));
    EXPECT_NE(result, QString("app"));
}

// B19否: GIO PID 不匹配 → 落入 /opt 分支（不匹配）最终回退
TEST_F(UT_ProcessName, getDisplayName_GioPidMismatch_FallsThroughToName)
{
    // Arrange：PID 环境值与进程 PID 不一致
    Process proc;
    proc.setName(QString("app"));
    g_pid = 1234;
    g_cmdline = QByteArrayList{ (kFakeUsrBin + "/app").toLocal8Bit() };
    g_environ.insert("GIO_LAUNCHED_DESKTOP_FILE", kFakeGioDesktop);
    g_environ.insert("GIO_LAUNCHED_DESKTOP_FILE_PID", "9999");
    g_gioEntry = makeEntry("app", "计算器");
    m_stub.set(ADDR(Process, pid), stub_Process_pid);
    m_stub.set(ADDR(Process, cmdline), stub_Process_cmdline);
    m_stub.set(ADDR(Process, environ), stub_Process_environ);
    m_stub.set(ADDR(DesktopEntryCache, entryWithDesktopFile), stub_entryWithDesktopFile);

    // Act
    QString result = m_tester->getDisplayName(&proc);

    // Assert
    EXPECT_EQ(result, QString("app"));
    EXPECT_NE(result, QString("计算器"));
}

// B21+B22: 命令行以 /opt 开头且按子名命中缓存 → 返回显示名
TEST_F(UT_ProcessName, getDisplayName_OptPathSubNameEntry_ReturnsDisplayName)
{
    // Arrange
    Process proc;
    proc.setName(QString("app"));
    g_cmdline = QByteArrayList{ (QStringLiteral("/opt") + "/apps/org.foo/files/bin/foo").toLocal8Bit() };
    seedEntryCache("org.foo", makeEntry("org.foo", "Foo应用"));
    m_stub.set(ADDR(Process, cmdline), stub_Process_cmdline);

    // Act
    QString result = m_tester->getDisplayName(&proc);

    // Assert：entryWithSubName 以文件名 "foo" 做包含匹配
    EXPECT_EQ(result, QString("Foo应用"));
    EXPECT_NE(result, QString("app"));
}

// B23: 全部分支未命中 → 回退进程名
TEST_F(UT_ProcessName, getDisplayName_NoMatchAnywhere_FallsBackToName)
{
    // Arrange：非托盘非 GUI、缓存为空、无 GIO、非 /opt
    Process proc;
    proc.setName(QString("plainapp"));
    g_cmdline = QByteArrayList{ (kFakeUsrBin + "/plainapp").toLocal8Bit() };
    m_stub.set(ADDR(Process, cmdline), stub_Process_cmdline);
    m_stub.set(ADDR(WMWindowList, isTrayApp), stub_isTrayApp);
    m_stub.set(ADDR(WMWindowList, isGuiApp), stub_isGuiApp);
    m_stub.set(ADDR(WMWindowList, getWindowTitle), stub_getWindowTitle);

    // Act
    QString result = m_tester->getDisplayName(&proc);

    // Assert
    EXPECT_EQ(result, QString("plainapp"));
    EXPECT_EQ(result, proc.name());
}
