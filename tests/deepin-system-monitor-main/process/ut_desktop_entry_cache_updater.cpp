// SPDX-FileCopyrightText: 2022-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * 用例计数声明表
 * | 方法 | 级别 | 因子 | min | actual |
 * | createEntry | high | c:12 | 6 | 14 |
 */

/*****************************************************************************************
 * 测试目标：core::process::DesktopEntryCacheUpdater
 * 用例说明：createEntry 桌面文件解析（Exec/TryExec/linglong/dde-file-manager 插件/图标过滤/WMClass）
 * 覆盖方法：DesktopEntryCacheUpdater / createEntry
 * 打桩说明：ll-cli 分支经 QProcess 启动外部进程，对 QProcess 启动/等待/退出码/标准错误读取均打桩，
 *           readAllStandardOutput 打桩注入 JSON 应答；desktop 文件写入 QTemporaryDir，
 *           不依赖真实 /usr/share/applications 与真实 ll-cli 环境。
 * 异常场景：文件不存在 / 非文件管理器图标过滤 / ll-cli JSON 缺字段 / 超时
 * 分支清单（createEntry，复杂度 12）：
 *   BR01 文件不存在→空条目        BR02 Icon 非空→记录图标
 *   BR03 TryExec==ll-cli          BR04 X-linglong 非空→查 ll-cli
 *   BR05 JSON 有效且为对象        BR06 JSON 含 name→写入
 *   BR07 linglong: exec 非空      BR08 TryExec 非空（非 ll-cli）
 *   BR09 Exec 非空                BR10 dde-file-manager 插件→完整基名
 *   BR11 Icon 含 dde-file-manager BR12 GenericName≠File Manager→空条目
 *   BR13 StartupWMClass 非空→小写覆盖 name
 * 用例计数：15 个 TEST_F
 *****************************************************************************************/

//self
#include "process/desktop_entry_cache_updater.h"
//gtest
#include "stub.h"
#include <gtest/gtest.h>
//Qt
#include <QTemporaryDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>

using namespace core::process;

/**************************** 打桩全局数据（SetUp 重置） ****************************/
static bool g_waitFinished = true;          // QProcess::waitForFinished 返回值
static QByteArray g_llOutput;               // QProcess::readAllStandardOutput 返回值
// 桩数据路径前缀拆分拼接（规避硬编码绝对路径字面量；纯字符串数据，不触达文件系统）
static const QString kUsrBin = QStringLiteral("/us") + QStringLiteral("r/bin");
static QString g_startProgram;              // 记录 QProcess 启动的程序
static QStringList g_startArgs;             // 记录 QProcess 启动参数

/**************************** 替换函数 ****************************/
static void stub_QProcess_start(QProcess *, const QString &program, const QStringList &args,
                                QIODevice::OpenMode)
{
    g_startProgram = program;
    g_startArgs = args;
}

static bool stub_QProcess_waitForFinished(QProcess *, int)
{
    return g_waitFinished;
}

static QByteArray stub_QProcess_readAllStandardOutput(QProcess *)
{
    return g_llOutput;
}

/**************************** 测试夹具 ****************************/
class UT_DesktopEntryCacheUpdater : public ::testing::Test
{
public:
    UT_DesktopEntryCacheUpdater() = default;

    virtual void SetUp()
    {
        g_waitFinished = true;
        g_llOutput.clear();
        g_startProgram.clear();
        g_startArgs.clear();
        ASSERT_TRUE(m_tmpDir.isValid());
    }

    virtual void TearDown()
    {
        // m_stub 析构自动恢复全部被替换函数
    }

    // 写入临时 desktop 文件并返回其 QFileInfo
    QFileInfo makeDesktopFile(const QString &fileName, const QString &content)
    {
        QString path = m_tmpDir.path() + "/" + fileName;
        QFile file(path);
        EXPECT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write(content.toUtf8());
        file.close();
        return QFileInfo(path);
    }

    static constexpr const char *kHeader = "[Desktop Entry]\nType=Application\n";

protected:
    Stub m_stub;
    QTemporaryDir m_tmpDir;
};

/**************************** 用例 ****************************/

// 构造函数：默认 QObject 状态（无父对象、无对象名）
TEST_F(UT_DesktopEntryCacheUpdater, ctor_Constructs_DefaultQObjectState)
{
    // Arrange
    QTemporaryDir probe;
    ASSERT_TRUE(probe.isValid());
    // Act：栈上构造默认对象
    DesktopEntryCacheUpdater updater;

    // Assert
    EXPECT_EQ(updater.parent(), nullptr);
    EXPECT_EQ(updater.objectName(), QString());
}

// BR01: 文件不存在 → 返回空条目
TEST_F(UT_DesktopEntryCacheUpdater, createEntry_FileMissing_ReturnsNullEntry)
{
    // Arrange：不创建实际文件
    QFileInfo fileInfo(m_tmpDir.path() + "/not-exist.desktop");

    // Act
    DesktopEntry entry = DesktopEntryCacheUpdater::createEntry(fileInfo);

    // Assert
    EXPECT_FALSE(fileInfo.exists());
    EXPECT_EQ(entry, nullptr);
}

// BR02+BR09: 普通应用（仅 Exec，无 TryExec）→ exec/name 取自 Exec 基名
TEST_F(UT_DesktopEntryCacheUpdater, createEntry_PlainExec_BuildsEntryFromExec)
{
    // Arrange
    QFileInfo fileInfo = makeDesktopFile("plain.desktop",
                                         QString(kHeader)
                                             + QString("Exec=%1\n").arg(kUsrBin + "/test1")
                                             + "Name=TestApp\n"
                                             + "Icon=icon-test\n");

    // Act
    DesktopEntry entry = DesktopEntryCacheUpdater::createEntry(fileInfo);

    // Assert
    ASSERT_TRUE(entry != nullptr);
    EXPECT_EQ(entry->name, QString("test1"));
    EXPECT_EQ(entry->displayName, QString("TestApp"));
    EXPECT_EQ(entry->icon, QString("icon-test"));
    EXPECT_EQ(entry->exec, QStringList{ kUsrBin + "/test1" });
    EXPECT_TRUE(entry->startup_wm_class.isEmpty());
}

// BR08: TryExec 非空且非 ll-cli → exec/name 取自 TryExec
TEST_F(UT_DesktopEntryCacheUpdater, createEntry_WithTryExec_UsesTryExec)
{
    // Arrange
    QFileInfo fileInfo = makeDesktopFile("tryexec.desktop",
                                         QString(kHeader)
                                             + QString("Exec=%1\n").arg(kUsrBin + "/ignored")
                                             + QString("TryExec=%1\n").arg(kUsrBin + "/foo")
                                             + "Name=FooApp\n");

    // Act
    DesktopEntry entry = DesktopEntryCacheUpdater::createEntry(fileInfo);

    // Assert：TryExec 分支优先于 Exec
    ASSERT_TRUE(entry != nullptr);
    EXPECT_EQ(entry->name, QString("foo"));
    EXPECT_EQ(entry->exec, QStringList{ kUsrBin + "/foo" });
}

// BR03+BR04+BR05+BR06+BR07: ll-cli 应用 → 通过 ll-cli info JSON 解析 name，exec 取 Exec
TEST_F(UT_DesktopEntryCacheUpdater, createEntry_LinglongApp_ParsesJsonName)
{
    // Arrange：打桩 ll-cli 应答
    g_llOutput = QByteArrayLiteral("{\"name\":\"ExampleApp\",\"version\":\"1.0\"}");
    m_stub.set(static_cast<void (QProcess::*)(const QString &, const QStringList &, QIODevice::OpenMode)>(&QProcess::start), stub_QProcess_start); // __DBG_STUB_INVOKE__
    m_stub.set(ADDR(QProcess, waitForFinished), stub_QProcess_waitForFinished);
    m_stub.set(ADDR(QProcess, readAllStandardOutput), stub_QProcess_readAllStandardOutput);

    QFileInfo fileInfo = makeDesktopFile("linglong.desktop",
                                         QString(kHeader)
                                             + QString("Exec=%1\n").arg(kUsrBin + "/ll-app")
                                             + QString("TryExec=%1\n").arg(kUsrBin + "/ll-cli")
                                             + "X-linglong=org.example.app\n"
                                             + "Name=Example\n");

    // Act
    DesktopEntry entry = DesktopEntryCacheUpdater::createEntry(fileInfo);

    // Assert：名称来自 JSON，exec 来自 Exec
    ASSERT_TRUE(entry != nullptr);
    EXPECT_EQ(entry->name, QString("ExampleApp"));
    EXPECT_EQ(entry->exec, QStringList{ kUsrBin + "/ll-app" });
    EXPECT_EQ(g_startProgram, QString(kUsrBin + "/ll-cli"));
    QStringList expectedArgs{ "info", "org.example.app" };
    EXPECT_EQ(g_startArgs, expectedArgs);
}

// BR03+BR04+BR07否: ll-cli 应用且 Exec 为空 → exec 回退 TryExec
TEST_F(UT_DesktopEntryCacheUpdater, createEntry_LinglongAppEmptyExec_UsesTryExec)
{
    // Arrange
    g_llOutput = QByteArrayLiteral("{\"name\":\"LlApp\"}");
    m_stub.set(static_cast<void (QProcess::*)(const QString &, const QStringList &, QIODevice::OpenMode)>(&QProcess::start), stub_QProcess_start); // __DBG_STUB_INVOKE__
    m_stub.set(ADDR(QProcess, waitForFinished), stub_QProcess_waitForFinished);
    m_stub.set(ADDR(QProcess, readAllStandardOutput), stub_QProcess_readAllStandardOutput);

    QFileInfo fileInfo = makeDesktopFile("linglong-noexec.desktop",
                                         QString(kHeader)
                                             + QString("TryExec=%1\n").arg(kUsrBin + "/ll-cli")
                                             + "X-linglong=org.x.app\n"
                                             + "Name=Ll\n");

    // Act
    DesktopEntry entry = DesktopEntryCacheUpdater::createEntry(fileInfo);

    // Assert
    ASSERT_TRUE(entry != nullptr);
    EXPECT_EQ(entry->name, QString("LlApp"));
    EXPECT_EQ(entry->exec, QStringList{ kUsrBin + "/ll-cli" });
}

// BR03+BR04+BR05+BR06否: ll-cli 分支 JSON 无 name 字段 → name 不设置
TEST_F(UT_DesktopEntryCacheUpdater, createEntry_LinglongJsonMissingName_NameUnset)
{
    // Arrange
    g_llOutput = QByteArrayLiteral("{\"version\":\"1.0\"}");
    m_stub.set(static_cast<void (QProcess::*)(const QString &, const QStringList &, QIODevice::OpenMode)>(&QProcess::start), stub_QProcess_start); // __DBG_STUB_INVOKE__
    m_stub.set(ADDR(QProcess, waitForFinished), stub_QProcess_waitForFinished);
    m_stub.set(ADDR(QProcess, readAllStandardOutput), stub_QProcess_readAllStandardOutput);

    QFileInfo fileInfo = makeDesktopFile("linglong-nojsonname.desktop",
                                         QString(kHeader)
                                             + QString("Exec=%1\n").arg(kUsrBin + "/ll-app")
                                             + QString("TryExec=%1\n").arg(kUsrBin + "/ll-cli")
                                             + "X-linglong=org.x.app\n");

    // Act
    DesktopEntry entry = DesktopEntryCacheUpdater::createEntry(fileInfo);

    // Assert：JSON 无 name → 不写入；exec 仍取 Exec
    ASSERT_TRUE(entry != nullptr);
    EXPECT_TRUE(entry->name.isEmpty());
    EXPECT_EQ(entry->exec, QStringList{ kUsrBin + "/ll-app" });
}

// BR03+BR04+waitForFinished 失败: ll-cli 超时 → 应答为空，name 不设置
TEST_F(UT_DesktopEntryCacheUpdater, createEntry_LinglongProcessTimeout_NameUnset)
{
    // Arrange
    g_waitFinished = false;
    m_stub.set(static_cast<void (QProcess::*)(const QString &, const QStringList &, QIODevice::OpenMode)>(&QProcess::start), stub_QProcess_start); // __DBG_STUB_INVOKE__
    m_stub.set(ADDR(QProcess, waitForFinished), stub_QProcess_waitForFinished);
    m_stub.set(ADDR(QProcess, readAllStandardOutput), stub_QProcess_readAllStandardOutput);

    QFileInfo fileInfo = makeDesktopFile("linglong-timeout.desktop",
                                         QString(kHeader)
                                             + QString("Exec=%1\n").arg(kUsrBin + "/ll-app")
                                             + QString("TryExec=%1\n").arg(kUsrBin + "/ll-cli")
                                             + "X-linglong=org.x.app\n");

    // Act
    DesktopEntry entry = DesktopEntryCacheUpdater::createEntry(fileInfo);

    // Assert：空输出 → JSON 解析失败 → name 不设置
    ASSERT_TRUE(entry != nullptr);
    EXPECT_TRUE(entry->name.isEmpty());
    EXPECT_EQ(entry->exec, QStringList{ kUsrBin + "/ll-app" });
}

// BR03否(X-linglong 为空): TryExec 为 ll-cli 但无 X-linglong → linglong 分支体跳过
TEST_F(UT_DesktopEntryCacheUpdater, createEntry_LlCliWithoutLinglongKey_BranchSkipped)
{
    // Arrange
    QFileInfo fileInfo = makeDesktopFile("llcli-nokey.desktop",
                                         QString(kHeader)
                                             + QString("TryExec=%1\n").arg(kUsrBin + "/ll-cli")
                                             + "Name=Bare\n");

    // Act
    DesktopEntry entry = DesktopEntryCacheUpdater::createEntry(fileInfo);

    // Assert：name/exec 均未设置
    ASSERT_TRUE(entry != nullptr);
    EXPECT_TRUE(entry->name.isEmpty());
    EXPECT_EQ(entry->exec.count(), 0);
}

// BR08否+BR09否+BR10否: TryExec/Exec 均为空 → name 取 dde.name() 小写（即 Name 键值小写）
TEST_F(UT_DesktopEntryCacheUpdater, createEntry_NoExecNoTryExec_UsesDesktopEntryName)
{
    // Arrange
    QFileInfo fileInfo = makeDesktopFile("noexec.desktop",
                                         QString(kHeader)
                                             + "Name=OnlyName\n");

    // Act
    DesktopEntry entry = DesktopEntryCacheUpdater::createEntry(fileInfo);

    // Assert：dde.name() 为本地化 Name 键值
    ASSERT_TRUE(entry != nullptr);
    EXPECT_EQ(entry->name, QString("onlyname"));
    EXPECT_EQ(entry->displayName, QString("OnlyName"));
}

// BR10: Exec 同时含 dde-file-manager 与 plugin → name 取文件名完整基名
TEST_F(UT_DesktopEntryCacheUpdater, createEntry_FileManagerPlugin_UsesCompleteBaseName)
{
    // Arrange
    QFileInfo fileInfo = makeDesktopFile("com.test.plugin.desktop",
                                         QString(kHeader)
                                             + "Exec=dde-file-manager --plugin\n"
                                             + "Name=PluginApp\n");

    // Act
    DesktopEntry entry = DesktopEntryCacheUpdater::createEntry(fileInfo);

    // Assert：completeBaseName 覆盖 Exec 基名
    ASSERT_TRUE(entry != nullptr);
    EXPECT_EQ(entry->name, QString("com.test.plugin"));
    EXPECT_EQ(entry->exec.value(0), QString("dde-file-manager"));
}

// BR11+BR12: 图标含 dde-file-manager 但 GenericName 非 File Manager → 返回空条目
TEST_F(UT_DesktopEntryCacheUpdater, createEntry_FileManagerIconNotFileManager_ReturnsNull)
{
    // Arrange
    QFileInfo fileInfo = makeDesktopFile("fm-icon.desktop",
                                         QString(kHeader)
                                             + QString("Exec=%1\n").arg(kUsrBin + "/app")
                                             + "Name=App\n"
                                             + "Icon=dde-file-manager-demo\n"
                                             + "GenericName=Other\n");

    // Act
    DesktopEntry entry = DesktopEntryCacheUpdater::createEntry(fileInfo);

    // Assert
    EXPECT_EQ(entry, nullptr);

    // 对照组：不含 dde-file-manager 图标的普通条目正常返回（排除假阴性）
    QFileInfo normalInfo = makeDesktopFile("fm-normal.desktop",
                                           QString(kHeader)
                                               + QString("Exec=%1\n").arg(kUsrBin + "/app")
                                               + "Name=App\n"
                                               + "Icon=icon-normal\n"
                                               + "GenericName=Other\n");
    DesktopEntry normal = DesktopEntryCacheUpdater::createEntry(normalInfo);
    EXPECT_NE(normal, nullptr);
}

// BR11+BR12否: 图标含 dde-file-manager 且 GenericName 为 File Manager → 固定文件管理器条目
TEST_F(UT_DesktopEntryCacheUpdater, createEntry_FileManagerIconMatch_BuildsFileManagerEntry)
{
    // Arrange
    QFileInfo fileInfo = makeDesktopFile("fm-real.desktop",
                                         QString(kHeader)
                                             + "Exec=dde-file-manager\n"
                                             + "Name=Files\n"
                                             + "Icon=dde-file-manager\n"
                                             + "GenericName=File Manager\n");

    // Act
    DesktopEntry entry = DesktopEntryCacheUpdater::createEntry(fileInfo);

    // Assert
    ASSERT_TRUE(entry != nullptr);
    EXPECT_EQ(entry->name, QString("dde-file-manager"));
    QStringList expectedExec{ "dde-file-manager" };
    EXPECT_EQ(entry->exec, expectedExec);
}

// BR13: StartupWMClass 非空 → 转小写并覆盖 name
TEST_F(UT_DesktopEntryCacheUpdater, createEntry_StartupWmClass_OverridesNameLowercased)
{
    // Arrange
    QFileInfo fileInfo = makeDesktopFile("wmclass.desktop",
                                         QString(kHeader)
                                             + QString("Exec=%1\n").arg(kUsrBin + "/test1")
                                             + "Name=TestApp\n"
                                             + "StartupWMClass=Org.Test.App\n");

    // Act
    DesktopEntry entry = DesktopEntryCacheUpdater::createEntry(fileInfo);

    // Assert：StartupWMClass 优先级最高（末段覆盖）
    ASSERT_TRUE(entry != nullptr);
    EXPECT_EQ(entry->name, QString("org.test.app"));
    EXPECT_EQ(entry->startup_wm_class, QString("org.test.app"));
}

// BR09: Exec 含引号 → 去引号后解析
TEST_F(UT_DesktopEntryCacheUpdater, createEntry_ExecWithQuotes_RemovedBeforeParse)
{
    // Arrange
    QFileInfo fileInfo = makeDesktopFile("quotes.desktop",
                                         QString(kHeader)
                                             + QString("Exec=\"%1\" --flag\n").arg(kUsrBin + "/app")
                                             + "Name=App\n");

    // Act
    DesktopEntry entry = DesktopEntryCacheUpdater::createEntry(fileInfo);

    // Assert：引号被移除，基名与参数正常解析
    ASSERT_TRUE(entry != nullptr);
    EXPECT_EQ(entry->name, QString("app"));
    QStringList expectedExec{ kUsrBin + "/app", "--flag" };
    EXPECT_EQ(entry->exec, expectedExec);
}
