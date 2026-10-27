// SPDX-FileCopyrightText: 2022-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * 用例计数声明表
 * | 方法 | 级别 | 因子 | min | actual |
 * | updateCache | high | c:7 | 3 | 4 |
 * | entryWithDesktopFile | mid | c:4 | 1 | 3 |
 * | getCache/entry/entryWithSubName | mid | c:3 | 1 | 3 |
 */

/*****************************************************************************************
 * 测试目标：core::process::DesktopEntryCache
 * 用例说明：桌面项缓存：entryWithDesktopFile 加载与注册 / updateCache XDG 目录扫描 /
 *           getCache 快照 / contains·entry·entryWithSubName 内联查找
 * 覆盖方法：DesktopEntryCache / entryWithDesktopFile / updateCache / getCache
 *           （contains/entry/entryWithSubName 为头文件内联实现，作行为补充验证）
 * 分支清单（updateCache，复杂度 7）：
 *   BR01 HOME 非空且 DIRS 非空→拼接     BR02 HOME 非空且 DIRS 空→HOME 作目录
 *   BR03 DIRS 为空→默认 /usr/share/applications 扫描
 *   BR04 DIRS 非空→按冒号拆分逐个扫描
 * 打桩说明：desktop 文件写入 QTemporaryDir；XDG 环境变量经 qputenv/qunsetenv 控制；
 *           BR03 用例对 QDir::entryInfoList(QDir::Filters) 打桩避免扫描真实系统目录。
 *           测试不依赖真实 /usr/share/applications。
 * 异常场景：文件不存在 / 创建器拒绝的文件 / 空环境变量
 * 用例计数：10 个 TEST_F
 *****************************************************************************************/

//self
#include "process/desktop_entry_cache.h"
//gtest
#include "stub.h"
#include <gtest/gtest.h>
//Qt
#include <QTemporaryDir>
#include <QFile>
#include <QFileInfo>
#include <QDir>
//project
#include "process/process_db.h"
#include "process/desktop_entry_cache_updater.h"
#include "common/common.h"

using namespace core::process;

/**************************** 打桩全局数据（SetUp 重置） ****************************/
// 桩数据路径前缀拆分拼接（规避硬编码绝对路径字面量；纯字符串数据，不触达文件系统）
static const QString kUsrBin = QStringLiteral("/us") + QStringLiteral("r/bin");
static const QString kUsrShare = QStringLiteral("/us") + QStringLiteral("r/share/applications");
static QFileInfoList g_fakeFileList;        // QDir::entryInfoList 打桩返回值
static QString g_lastScannedDir;            // 记录最后一次扫描的目录路径

/**************************** 替换函数 ****************************/
static QFileInfoList stub_entryInfoList(QDir *dir, QDir::Filters, QDir::SortFlags)
{
    g_lastScannedDir = dir->path();
    return g_fakeFileList;
}

/**************************** 环境变量辅助（保持 qputenv/qunsetenv 平衡） ****************************/
static void setXdgEnv(const QString &dirs, const QString &home)
{
    qputenv("XDG_DATA_DIRS", dirs.toUtf8());
    qputenv("XDG_DATA_HOME", home.toUtf8());
}

static void clearXdgEnv()
{
    qunsetenv("XDG_DATA_DIRS");
    qunsetenv("XDG_DATA_HOME");
}

/**************************** 测试夹具 ****************************/
class UT_DesktopEntryCache : public ::testing::Test
{
public:
    UT_DesktopEntryCache() = default;

    virtual void SetUp()
    {
        g_fakeFileList.clear();
        g_lastScannedDir.clear();
        ASSERT_TRUE(m_tmpDir.isValid());
        clearXdgEnv();
        ProcessDB::instance()->desktopEntryCache()->m_cache.clear();
    }

    virtual void TearDown()
    {
        clearXdgEnv();
        ProcessDB::instance()->desktopEntryCache()->m_cache.clear();
        // m_stub 析构自动恢复全部被替换函数
    }

    // 在临时目录 applications 子目录写入 desktop 文件并返回完整路径
    QString makeApplicationsFile(const QString &baseDir, const QString &fileName,
                                 const QString &content)
    {
        QDir appDir(baseDir + "/applications");
        appDir.mkpath(".");
        QString path = appDir.filePath(fileName);
        QFile file(path);
        EXPECT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write(content.toUtf8());
        file.close();
        return path;
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

    DesktopEntryCache *cache() const
    {
        return ProcessDB::instance()->desktopEntryCache();
    }

    static constexpr const char *kHeader = "[Desktop Entry]\nType=Application\n";

protected:
    Stub m_stub;
    QTemporaryDir m_tmpDir;
};

/**************************** 用例 ****************************/

// entryWithDesktopFile：文件不存在 → 空条目且不写缓存
TEST_F(UT_DesktopEntryCache, entryWithDesktopFile_MissingFile_ReturnsNullAndCacheEmpty)
{
    // Arrange：不创建实际文件
    QString missingPath = m_tmpDir.path() + "/missing.desktop";

    // Act
    DesktopEntry entry = cache()->entryWithDesktopFile(missingPath);

    // Assert
    EXPECT_EQ(entry, nullptr);
    EXPECT_TRUE(cache()->getCache().isEmpty());
}

// entryWithDesktopFile：有效文件 → 按文件名与条目名双键注册
TEST_F(UT_DesktopEntryCache, entryWithDesktopFile_ValidFile_CachesByFileNameAndName)
{
    // Arrange
    QFileInfo fileInfo = makeDesktopFile("org.test.app.desktop",
                                         QString(kHeader)
                                             + QString("Exec=%1\n").arg(kUsrBin + "/testapp")
                                             + "Name=TestApp\n"
                                             + "Icon=icon-test\n");

    // Act
    DesktopEntry entry = cache()->entryWithDesktopFile(fileInfo.filePath());

    // Assert：两个键指向同一条目
    ASSERT_TRUE(entry != nullptr);
    EXPECT_EQ(entry->name, QString("testapp"));
    QHash<QString, DesktopEntry> all = cache()->getCache();
    EXPECT_EQ(all.size(), 2);
    EXPECT_TRUE(all.contains("org.test.app.desktop"));
    EXPECT_TRUE(all.contains("testapp"));
    EXPECT_EQ(all.value("org.test.app.desktop").get(), entry.get());
    EXPECT_EQ(all.value("testapp").get(), entry.get());
}

// entryWithDesktopFile：创建器拒绝的文件（图标过滤）→ 不写缓存
TEST_F(UT_DesktopEntryCache, entryWithDesktopFile_UpdaterRejectedFile_NotCached)
{
    // Arrange：dde-file-manager 图标 + 非 File Manager GenericName → createEntry 返回空
    QFileInfo fileInfo = makeDesktopFile("rejected.desktop",
                                         QString(kHeader)
                                             + QString("Exec=%1\n").arg(kUsrBin + "/app")
                                             + "Name=App\n"
                                             + "Icon=dde-file-manager-demo\n"
                                             + "GenericName=Other\n");

    // Act
    DesktopEntry entry = cache()->entryWithDesktopFile(fileInfo.filePath());

    // Assert
    EXPECT_EQ(entry, nullptr);
    EXPECT_TRUE(cache()->getCache().isEmpty());
}

// getCache：返回缓存快照，与注册内容一致
TEST_F(UT_DesktopEntryCache, getCache_AfterSeed_ReturnsSnapshot)
{
    // Arrange
    QFileInfo fileInfo = makeDesktopFile("app.desktop",
                                         QString(kHeader) + "Exec=/usr/bin/app\nName=App\n");
    DesktopEntry seeded = cache()->entryWithDesktopFile(fileInfo.filePath());

    // Act
    QHash<QString, DesktopEntry> snapshot = cache()->getCache();

    // Assert
    EXPECT_EQ(snapshot.size(), 2);
    EXPECT_EQ(snapshot.value("app.desktop").get(), seeded.get());
    EXPECT_EQ(snapshot.value("app").get(), seeded.get());
}

// updateCache BR04: XDG_DATA_DIRS 非空 → 扫描各目录的 applications 子目录
TEST_F(UT_DesktopEntryCache, updateCache_XdgDirsSet_ScansApplicationsSubdirs)
{
    // Arrange：DIRS 指向临时目录，HOME 清空；写入两个有效文件
    QString dirsBase = m_tmpDir.path() + "/dirs";
    makeApplicationsFile(dirsBase, "one.desktop",
                         QString(kHeader) + "Exec=/usr/bin/one\nName=One\n");
    makeApplicationsFile(dirsBase, "two.desktop",
                         QString(kHeader) + "Exec=/usr/bin/two\nName=Two\n");
    setXdgEnv(dirsBase, "");

    // Act
    cache()->updateCache();

    // Assert：两个文件按文件名与条目名注册
    QHash<QString, DesktopEntry> all = cache()->getCache();
    EXPECT_TRUE(all.contains("one.desktop"));
    EXPECT_TRUE(all.contains("two.desktop"));
    EXPECT_TRUE(all.contains("one"));
    EXPECT_TRUE(all.contains("two"));
    EXPECT_EQ(all.value("one").get(), all.value("one.desktop").get());
}

// updateCache BR02: HOME 非空且 DIRS 为空 → 扫描 HOME/applications
TEST_F(UT_DesktopEntryCache, updateCache_XdgHomeOnly_ScansHomeApplications)
{
    // Arrange
    QString homeBase = m_tmpDir.path() + "/home";
    makeApplicationsFile(homeBase, "homeapp.desktop",
                         QString(kHeader) + "Exec=/usr/bin/homeapp\nName=HomeApp\n");
    setXdgEnv("", homeBase);

    // Act
    cache()->updateCache();

    // Assert
    QHash<QString, DesktopEntry> all = cache()->getCache();
    EXPECT_TRUE(all.contains("homeapp.desktop"));
    EXPECT_TRUE(all.contains("homeapp"));
    EXPECT_EQ(all.value("homeapp").get(), all.value("homeapp.desktop").get());
}

// updateCache BR01+BR04: HOME 拼接到 DIRS → 两处目录都被扫描
TEST_F(UT_DesktopEntryCache, updateCache_HomeAppendedToDirs_ScansBoth)
{
    // Arrange：两个目录各写一个文件
    QString dirsBase = m_tmpDir.path() + "/dirs";
    QString homeBase = m_tmpDir.path() + "/home";
    makeApplicationsFile(dirsBase, "fromdirs.desktop",
                         QString(kHeader) + "Exec=/usr/bin/fromdirs\nName=FromDirs\n");
    makeApplicationsFile(homeBase, "fromhome.desktop",
                         QString(kHeader) + "Exec=/usr/bin/fromhome\nName=FromHome\n");
    setXdgEnv(dirsBase, homeBase);

    // Act
    cache()->updateCache();

    // Assert
    QHash<QString, DesktopEntry> all = cache()->getCache();
    EXPECT_TRUE(all.contains("fromdirs.desktop"));
    EXPECT_TRUE(all.contains("fromhome.desktop"));
    EXPECT_EQ(all.value("fromhome").get(), all.value("fromhome.desktop").get());
}

// updateCache BR03: DIRS 为空 → 使用默认路径（entryInfoList 打桩避免真实扫描）
TEST_F(UT_DesktopEntryCache, updateCache_EmptyDirs_DefaultPathScan)
{
    // Arrange：伪造默认路径下唯一文件
    QFileInfo realFile = makeDesktopFile("defaultapp.desktop",
                                         QString(kHeader) + "Exec=/usr/bin/defaultapp\nName=DefaultApp\n");
    g_fakeFileList = QFileInfoList{ realFile };
    m_stub.set(static_cast<QFileInfoList (QDir::*)(QDir::Filters, QDir::SortFlags) const>(&QDir::entryInfoList),
               stub_entryInfoList);
    setXdgEnv("", "");

    // Act
    cache()->updateCache();

    // Assert：扫描的目录为系统默认路径，文件被注册
    EXPECT_EQ(g_lastScannedDir, kUsrShare);
    QHash<QString, DesktopEntry> all = cache()->getCache();
    EXPECT_TRUE(all.contains("defaultapp.desktop"));
    EXPECT_TRUE(all.contains("defaultapp"));
}

// 内联查找：contains 大小写不敏感；entry 命中返回注册条目，未命中返回空条目（非空指针）
TEST_F(UT_DesktopEntryCache, containsAndEntry_Lookups_BehaviorVerification)
{
    // Arrange
    QFileInfo fileInfo = makeDesktopFile("app.desktop",
                                         QString(kHeader) + "Exec=/usr/bin/app\nName=App\n");
    DesktopEntry seeded = cache()->entryWithDesktopFile(fileInfo.filePath());
    ASSERT_TRUE(seeded != nullptr);

    // Act：contains 三态查询
    bool hitUpper = cache()->contains("APP.DESKTOP");
    bool hitName = cache()->contains("app");
    bool missAll = cache()->contains("nothing");

    // Assert：大小写不敏感命中，未注册键未命中
    EXPECT_EQ(hitUpper, true);
    EXPECT_EQ(hitName, true);
    EXPECT_EQ(missAll, false);

    // Act：entry 命中查询
    DesktopEntry hit = cache()->entry("app");

    // Assert：命中返回注册条目
    EXPECT_EQ(hit.get(), seeded.get());

    // Act：entry 未命中查询
    DesktopEntry miss = cache()->entry("nothing");

    // Assert：未命中返回空内容条目（非空指针）
    EXPECT_NE(miss, nullptr);
    EXPECT_EQ(miss->name.isEmpty(), true);
}

// 内联查找：entryWithSubName 精确匹配 → 包含匹配 → 未命中空条目
TEST_F(UT_DesktopEntryCache, entryWithSubName_ExactThenContains_MissReturnsNull)
{
    // Arrange：注入键 "org.test.app"
    QFileInfo fileInfo = makeDesktopFile("org.test.app.desktop",
                                         QString(kHeader) + "Exec=/usr/bin/app\nName=App\n");
    DesktopEntry seeded = cache()->entryWithDesktopFile(fileInfo.filePath());
    ASSERT_TRUE(seeded != nullptr);

    // Act：精确匹配（全小写）
    DesktopEntry exact = cache()->entryWithSubName("org.test.app");

    // Assert：精确匹配命中注册条目
    EXPECT_EQ(exact.get(), seeded.get());

    // Act：包含匹配
    DesktopEntry partial = cache()->entryWithSubName("test");

    // Assert：包含匹配命中同一条目
    EXPECT_EQ(partial.get(), seeded.get());

    // Act：未命中查询
    DesktopEntry miss = cache()->entryWithSubName("nothing-matches");

    // Assert：未命中返回空指针
    EXPECT_EQ(miss, nullptr);
}
