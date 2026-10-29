// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * UT_ProcessSortFilterProxyModel
 *
 * 测试对象: deepin-system-monitor-plugin-popup/model/process_sort_filter_proxy_model.cpp
 *           ProcessSortFilterProxyModel 类
 * 外部依赖: 无 DBus/网络/设备依赖;数据源由测试内置 ProxySourceModel 注入,
 *           拼音分支通过打桩 QLocale 的 system 静态工厂 模拟中文环境,不依赖真实 locale。
 *
 * 分支覆盖声明 (filterAcceptsRow, complexity=11, 源码行 56-108):
 * | # | 分支条件                                   | 覆盖用例                                       |
 * |---|--------------------------------------------|------------------------------------------------|
 * | 1 | m_filterType==kNoFilter → filter=true      | filterAcceptsRow_noFilter_acceptsMatchingText  |
 * | 2 | kFilterCurrentUser && app∈{Apps,User}      | filterAcceptsRow_currentUser_acceptsUserApps   |
 * | 3 | kFilterCurrentUser && 其他 appType → 拒绝  | filterAcceptsRow_currentUser_rejectsOthers     |
 * | 4 | kFilterApps && app==kFilterApps → 通过     | filterAcceptsRow_appsType_acceptsOnlyApps      |
 * | 5 | kFilterApps && app!=kFilterApps → 拒绝     | filterAcceptsRow_appsType_rejectsCurrentUser   |
 * | 6 | !filter 提前 return false                  | filterAcceptsRow_noFilter_rejectsNonMatching   |
 * | 7 | DisplayRole 命中过滤串                     | filterAcceptsRow_displayNameMatch_acceptsRow   |
 * | 8 | UserRole 原始名命中过滤串                  | filterAcceptsRow_rawNameMatch_acceptsRow       |
 * | 9 | 中文 locale 下 m_hanwords 拼音命中         | filterAcceptsRow_pinyin_chineseLocaleMatch     |
 * |10 | PID 列 DisplayRole 命中过滤串              | filterAcceptsRow_pidMatch_acceptsRow           |
 * |11 | 全部未命中 → return false                  | filterAcceptsRow_noFilter_rejectsNonMatching   |
 *
 * 备注: rowCount 固定返回 5 为设计契约(顶部前五个应用,commit 26c66198),
 *       测试按契约固定断言;数据源行数 < 5 时的表现由源码负责,不在用例内固化。
 *
 * 用例声明表:
 * |------------------------------------------------|---------------------------------|---------|------|------------------------------|
 * | 测试用例名                                      | 测试类                          | 测试级别 | 类型 | 覆盖函数                     |
 * |------------------------------------------------|---------------------------------|---------|------|------------------------------|
 * | rowCount_allAccepted_returnsFixedFive          | UT_ProcessSortFilterProxyModel  | Level_1 | AUTO | rowCount(契约钉住)           |
 * | rowCount_patternFiltered_mappingFollowsFilter  | UT_ProcessSortFilterProxyModel  | Level_2 | AUTO | rowCount+filterAcceptsRow    |
 * | setSortFilterString_Text_UpdatesSearchRegex      | UT_ProcessSortFilterProxyModel  | Level_1 | AUTO | setSortFilterString          |
 * | setFilterType_Types_ReappliesFilter          | UT_ProcessSortFilterProxyModel  | Level_1 | AUTO | setFilterType                |
 * | filterAcceptsRow_noFilter_acceptsMatchingText  | UT_ProcessSortFilterProxyModel  | Level_1 | AUTO | filterAcceptsRow(分支1/7)    |
 * | filterAcceptsRow_noFilter_rejectsNonMatching   | UT_ProcessSortFilterProxyModel  | Level_1 | AUTO | filterAcceptsRow(分支6/11)   |
 * | filterAcceptsRow_currentUser_acceptsUserApps   | UT_ProcessSortFilterProxyModel  | Level_1 | AUTO | filterAcceptsRow(分支2)      |
 * | filterAcceptsRow_currentUser_rejectsOthers     | UT_ProcessSortFilterProxyModel  | Level_2 | AUTO | filterAcceptsRow(分支3)      |
 * | filterAcceptsRow_appsType_acceptsOnlyApps      | UT_ProcessSortFilterProxyModel  | Level_1 | AUTO | filterAcceptsRow(分支4)      |
 * | filterAcceptsRow_appsType_rejectsCurrentUser   | UT_ProcessSortFilterProxyModel  | Level_2 | AUTO | filterAcceptsRow(分支5)      |
 * | filterAcceptsRow_displayNameMatch_acceptsRow   | UT_ProcessSortFilterProxyModel  | Level_2 | AUTO | filterAcceptsRow(分支7)      |
 * | filterAcceptsRow_rawNameMatch_acceptsRow       | UT_ProcessSortFilterProxyModel  | Level_2 | AUTO | filterAcceptsRow(分支8)      |
 * | filterAcceptsRow_pidMatch_acceptsRow           | UT_ProcessSortFilterProxyModel  | Level_2 | AUTO | filterAcceptsRow(分支10)     |
 * | filterAcceptsRow_pinyin_chineseLocaleMatch     | UT_ProcessSortFilterProxyModel  | Level_3 | STUB | filterAcceptsRow(分支9)      |
 * | lessThan_nameColumn_hanziSortedAfterAscii      | UT_ProcessSortFilterProxyModel  | Level_2 | AUTO | lessThan(名字列汉字分组)     |
 * | lessThan_nameColumn_equalDisplay_tiebreaksByCpu| UT_ProcessSortFilterProxyModel  | Level_2 | AUTO | lessThan(名字列CPU平局)      |
 * | lessThan_cpuColumn_comparesByCpuValue          | UT_ProcessSortFilterProxyModel  | Level_1 | AUTO | lessThan(CPU列)              |
 * | lessThan_cpuColumn_equalCpu_tiebreaksByNameRole| UT_ProcessSortFilterProxyModel  | Level_2 | AUTO | lessThan(CPU列平局)          |
 * | lessThan_otherColumn_fallsBackToBase           | UT_ProcessSortFilterProxyModel  | Level_2 | AUTO | lessThan(默认列)             |
 *
 * 用例计数声明(方法 | level | factors | min | actual):
 * | filterAcceptsRow | high | complexity:11 | 3 | 10 |
 * | lessThan | high | complexity:8 | 3 | 5 |
 * | rowCount | low | - | 1 | 2 |
 * | setFilterType | low | - | 1 | 1 |
 * | setSortFilterString | low | - | 1 | 1 |
 *
 * 风险检查清单:
 * 1. 对每一个测试文件中的 stub 都执行了 reset?  [是] Stub 局部对象析构自动恢复
 * 2. 对每一个测试文件中的 stub 都执行了 restore? [是] Stub 析构函数恢复被改写函数
 * 3. 所有的测试使用 ASSERT/EXPECT 上的宏而不是布尔判断? [是]
 * 4. 所有的 ASSERT 宏的返回值都被检查? [是]
 * 5. 用例覆盖了所有分支? [是] filterAcceptsRow 全部 11 个分支见上方分支声明表
 * 6. 本文件与类定义的头文件一致(用例使用的是头文件中公开的方法)? [是]
 * 7. 所有的属性和方法都被测试到了? [是] 私有成员 m_search/m_hanwords/m_fileterType 直接访问(-fno-access-control)
 * 8. 测试代码可读性高,测试方法意义明确? [是]
 * 9. 任何测试不能对其它测试产生依赖? [是] 每用例独立构造 source/proxy
 * 10. 使用的是 Google Test 而不是 QtTest? [是]
 */

#include <QModelIndex>
#include <QRegularExpression>
#include <QSortFilterProxyModel>
#include <gtest/gtest.h>

#include "../../tests/stub.h"
#include "model/process_sort_filter_proxy_model.h"
#include "model/process_table_model.h"

// FilterType 枚举随 process_table_model.h 的 using namespace core::process 引入

// ==================== 测试数据源模型 ====================
// 模拟 popup ProcessTableModel 对过滤相关角色的数据供给:
//   Name 列: DisplayRole=显示名, UserRole=原始名
//   CPU 列: UserRole=cpu 值(double)
//   PID 列: DisplayRole=pid 串, UserRole+3=appType(任意列), UserRole+4=pid 串(任意列)
class ProxySourceModel : public QAbstractTableModel
{
public:
    struct Row {
        QString display;
        QString rawName;
        double cpu = 0.0;
        int appType = 2;   // kNoFilter
        QString pid;
    };

    QList<Row> rows;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : rows.size();
    }
    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        Q_UNUSED(parent);
        return ProcessTableModel::kProcessColumnCount;
    }
    QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override
    {
        const Row &r = rows[idx.row()];
        switch (idx.column()) {
        case ProcessTableModel::kProcessNameColumn:
            if (role == Qt::DisplayRole)
                return r.display;
            if (role == Qt::UserRole)
                return r.rawName;
            break;
        case ProcessTableModel::kProcessCPUColumn:
            if (role == Qt::UserRole)
                return r.cpu;
            break;
        case ProcessTableModel::kProcessPIDColumn:
            if (role == Qt::DisplayRole)
                return r.pid;
            break;
        default:
            break;
        }
        // 真实 ProcessTableModel 对 UserRole+3/UserRole+4 不分列返回
        if (role == Qt::UserRole + 3)
            return r.appType;
        if (role == Qt::UserRole + 4)
            return r.pid;
        return QVariant();
    }
};

// ==================== 私有成员访问说明 ====================
// 测试目标由 tests/CMakeLists.txt 全局追加 -fno-access-control 编译,
// 测试代码可直接访问 private/protected 成员,无需 addr_pri.h 宏。
// 注: 工作区源码实际成员名为 m_fileterType(上游原始拼写,保持既有契约)。

// ==================== 拼音分支打桩 ====================
// 打桩 QLocale 静态工厂为中文 locale,激活 filterAcceptsRow 的拼音分支
QLocale stub_qlocale_system_chinese()
{
    return QLocale(QLocale::Chinese);
}

// ==================== 测试夹具 ====================
class UT_ProcessSortFilterProxyModel : public ::testing::Test
{
public:
    void SetUp() override
    {
        m_source = new ProxySourceModel();
        m_tester = new ProcessSortFilterProxyModel();
        m_tester->setSourceModel(m_source);
        // 默认空过滤串(空正则匹配一切),appType 默认 kNoFilter
        m_tester->setSortFilterString(QString());
    }

    void TearDown() override
    {
        delete m_tester;
        m_tester = nullptr;
        delete m_source;
        m_source = nullptr;
    }

    // 快捷添加一行数据
    void addRow(const QString &display, const QString &rawName,
                double cpu, int appType, const QString &pid)
    {
        m_source->rows.append({ display, rawName, cpu, appType, pid });
    }

    // 直接调用 protected filterAcceptsRow(-fno-access-control)
    bool accepts(int row)
    {
        return m_tester->filterAcceptsRow(row, QModelIndex());
    }

    // 直接调用 protected lessThan
    bool less(int lrow, int rrow, int column)
    {
        const QModelIndex &l = m_source->index(lrow, column);
        const QModelIndex &r = m_source->index(rrow, column);
        return m_tester->lessThan(l, r);
    }

    ProxySourceModel *m_source = nullptr;
    ProcessSortFilterProxyModel *m_tester = nullptr;
};

// ==================== 测试用例 ====================

// 用例名称: rowCount_allAccepted_returnsFixedFive
// 测试类型: 正常场景
// 覆盖函数: rowCount
// 操作步骤: 注入 8 行全部可接受的数据,读取代理行数
// 预期结果: 固定返回 5(弹框进程列表只显示 CPU 前五,设计契约 commit 26c66198)
TEST_F(UT_ProcessSortFilterProxyModel, rowCount_allAccepted_returnsFixedFive)
{
    // Arrange: 8 行数据,空过滤串全部接受
    for (int i = 0; i < 8; ++i)
        addRow(QStringLiteral("App%1").arg(i), QStringLiteral("app%1").arg(i), 1.0, 2, QString::number(i));
    // Act: 读取行数
    const int count = m_tester->rowCount();
    // Assert: 固定 5 行契约
    EXPECT_EQ(count, 5);
    EXPECT_EQ(m_source->rowCount(), 8);
}

// 用例名称: rowCount_patternFiltered_mappingFollowsFilter
// 测试类型: 边界场景
// 覆盖函数: rowCount + filterAcceptsRow(映射一致性)
// 操作步骤: 8 行中仅 2 行匹配过滤串,检查映射行的数据与固定行数
// 预期结果: 行数契约仍为 5,但映射行数据只反映匹配行
TEST_F(UT_ProcessSortFilterProxyModel, rowCount_patternFiltered_mappingFollowsFilter)
{
    // Arrange: 2 行含 "match",6 行无关
    addRow(QStringLiteral("match0"), QStringLiteral("m0"), 1.0, 2, "0");
    addRow(QStringLiteral("match1"), QStringLiteral("m1"), 2.0, 2, "1");
    for (int i = 2; i < 8; ++i)
        addRow(QStringLiteral("Other%1").arg(i), QStringLiteral("o%1").arg(i), 1.0, 2, QString::number(i));
    // Act: 设置过滤串
    m_tester->setSortFilterString(QStringLiteral("match"));
    // Assert: 行数契约固定 5;映射行数据是匹配行
    EXPECT_EQ(m_tester->rowCount(), 5);
    EXPECT_EQ(m_tester->index(0, 0).data().toString(), QStringLiteral("match0"));
    EXPECT_EQ(m_tester->index(1, 0).data().toString(), QStringLiteral("match1"));
}

// 用例名称: setSortFilterString_Text_UpdatesSearchRegex
// 测试类型: 正常场景
// 覆盖函数: setSortFilterString
// 操作步骤: 设置过滤串,读取私有 m_search/m_hanwords 与过滤正则
// 预期结果: m_search 与正则模式一致且大小写不敏感;非中文 locale 下 m_hanwords 为空
TEST_F(UT_ProcessSortFilterProxyModel, setSortFilterString_Text_UpdatesSearchRegex)
{
    // Arrange: 默认夹具
    // Act: 设置过滤串
    m_tester->setSortFilterString(QStringLiteral("Pattern"));
    // Assert: 私有成员与正则状态
    EXPECT_EQ(m_tester->m_search,
              QStringLiteral("Pattern"));
    const QRegularExpression &re = m_tester->filterRegularExpression();
    EXPECT_EQ(re.pattern(), QStringLiteral("Pattern"));
    EXPECT_TRUE(re.patternOptions() & QRegularExpression::CaseInsensitiveOption);
    // 非中文 locale 下不生成拼音串;若环境为中文则应生成非空拼音
    const QLocale sysProbe;
    const bool isChinese = sysProbe.language() == QLocale::Chinese;
    const QString &hanwords = m_tester->m_hanwords;
    EXPECT_EQ(hanwords.isEmpty(), !isChinese);
}

// 用例名称: setFilterType_Types_ReappliesFilter
// 测试类型: 正常场景
// 覆盖函数: setFilterType
// 操作步骤: 依次设置 3 种过滤类型,读取私有 m_fileterType
// 预期结果: 过滤类型成员随调用更新(默认 kNoFilter)
TEST_F(UT_ProcessSortFilterProxyModel, setFilterType_Types_ReappliesFilter)
{
    // Arrange: 默认构造的过滤类型为 kNoFilter(2)
    EXPECT_EQ(m_tester->m_fileterType, kNoFilter);
    // Act: 执行上述步骤
    // Assert: 逐个设置并验证
    m_tester->setFilterType(kFilterCurrentUser);
    EXPECT_EQ(m_tester->m_fileterType, kFilterCurrentUser);
    m_tester->setFilterType(kFilterApps);
    EXPECT_EQ(m_tester->m_fileterType, kFilterApps);
    m_tester->setFilterType(kNoFilter);
    EXPECT_EQ(m_tester->m_fileterType, kNoFilter);
}

// 用例名称: filterAcceptsRow_noFilter_acceptsMatchingText
// 测试类型: 正常场景
// 覆盖函数: filterAcceptsRow(分支1: kNoFilter + 分支7: DisplayRole 命中)
// 操作步骤: kNoFilter 类型 + 过滤串命中 row0 显示名而不命中 row1
// 预期结果: 命中行接受,未命中行拒绝
TEST_F(UT_ProcessSortFilterProxyModel, filterAcceptsRow_noFilter_acceptsMatchingText)
{
    // Arrange: 过滤串只命中 row0 显示名
    addRow(QStringLiteral("Music Player"), QStringLiteral("music"), 1.0, 2, "100");
    addRow(QStringLiteral("Video Player"), QStringLiteral("video"), 1.0, 2, "200");
    m_tester->setSortFilterString(QStringLiteral("Music"));
    // Act: 逐行过滤
    const bool r0 = accepts(0);
    const bool r1 = accepts(1);
    // Assert: 命中行接受,未命中行拒绝
    EXPECT_TRUE(r0);
    EXPECT_FALSE(r1);
}

// 用例名称: filterAcceptsRow_noFilter_rejectsNonMatching
// 测试类型: 正常场景
// 覆盖函数: filterAcceptsRow(分支6: 类型通过但文本未命中;分支11: 返回 false)
// 操作步骤: kNoFilter 类型 + 过滤串不命中任何字段
// 预期结果: 行被拒绝
TEST_F(UT_ProcessSortFilterProxyModel, filterAcceptsRow_noFilter_rejectsNonMatching)
{
    // Arrange: 过滤串与显示名/原始名/pid 都不匹配
    addRow(QStringLiteral("Music Player"), QStringLiteral("music"), 1.0, 2, "100");
    m_tester->setSortFilterString(QStringLiteral("zzz-none"));
    // Act: 过滤
    const bool rc = accepts(0);
    // Assert: 全字段未命中,拒绝;清空过滤串后同例接受
    EXPECT_FALSE(rc);
    m_tester->setSortFilterString(QString());
    EXPECT_TRUE(accepts(0));
}

// 用例名称: filterAcceptsRow_currentUser_acceptsUserApps
// 测试类型: 正常场景
// 覆盖函数: filterAcceptsRow(分支2: kFilterCurrentUser 接受 Apps/User 两类)
// 操作步骤: 过滤类型 kFilterCurrentUser,注入 appType 为 kFilterApps 与 kFilterCurrentUser 的行
// 预期结果: 两类行均接受
TEST_F(UT_ProcessSortFilterProxyModel, filterAcceptsRow_currentUser_acceptsUserApps)
{
    // Arrange: 切换过滤类型 kFilterCurrentUser,appType 0(kFilterApps) 与 1(kFilterCurrentUser)
    m_tester->setFilterType(kFilterCurrentUser);
    addRow(QStringLiteral("gui-app"), QStringLiteral("gui"), 1.0, kFilterApps, "100");
    addRow(QStringLiteral("user-app"), QStringLiteral("user"), 1.0, kFilterCurrentUser, "101");
    // Act: 执行上述步骤
    // Assert: 两行均接受
    EXPECT_TRUE(accepts(0));
    EXPECT_TRUE(accepts(1));
}

// 用例名称: filterAcceptsRow_currentUser_rejectsOthers
// 测试类型: 边界场景
// 覆盖函数: filterAcceptsRow(分支3: kFilterCurrentUser 拒绝其他 appType)
// 操作步骤: 过滤类型 kFilterCurrentUser,注入 appType=kNoFilter 的行
// 预期结果: 行被拒绝
TEST_F(UT_ProcessSortFilterProxyModel, filterAcceptsRow_currentUser_rejectsOthers)
{
    // Arrange: 切换过滤类型 kFilterCurrentUser,appType=kNoFilter(2)
    m_tester->setFilterType(kFilterCurrentUser);
    addRow(QStringLiteral("system-app"), QStringLiteral("sys"), 1.0, kNoFilter, "100");
    addRow(QStringLiteral("gui-app"), QStringLiteral("gui"), 1.0, kFilterApps, "101");
    // Act: 过滤
    const bool rc = accepts(0);
    const bool rc2 = accepts(1);
    // Assert: 非 app 类型拒绝,app 类型仍接受(选择性拒绝)
    EXPECT_FALSE(rc);
    EXPECT_TRUE(rc2);
}

// 用例名称: filterAcceptsRow_appsType_acceptsOnlyApps
// 测试类型: 正常场景
// 覆盖函数: filterAcceptsRow(分支4: kFilterApps 只接受应用)
// 操作步骤: 过滤类型 kFilterApps,注入 appType=kFilterApps 的行
// 预期结果: 行被接受
TEST_F(UT_ProcessSortFilterProxyModel, filterAcceptsRow_appsType_acceptsOnlyApps)
{
    // Arrange: 切换过滤类型 kFilterApps,appType=kFilterApps(0)
    m_tester->setFilterType(kFilterApps);
    addRow(QStringLiteral("gui-app"), QStringLiteral("gui"), 1.0, kFilterApps, "100");
    addRow(QStringLiteral("user-app"), QStringLiteral("user"), 1.0, kFilterCurrentUser, "101");
    // Act: 执行上述步骤
    // Assert: app 行接受,用户 app 行拒绝(仅接受应用)
    EXPECT_TRUE(accepts(0));
    EXPECT_FALSE(accepts(1));
}

// 用例名称: filterAcceptsRow_appsType_rejectsCurrentUser
// 测试类型: 边界场景
// 覆盖函数: filterAcceptsRow(分支5: kFilterApps 拒绝 kFilterCurrentUser/kNoFilter)
// 操作步骤: 过滤类型 kFilterApps,注入 appType=kFilterCurrentUser 与 kNoFilter 的行
// 预期结果: 两行均被拒绝
TEST_F(UT_ProcessSortFilterProxyModel, filterAcceptsRow_appsType_rejectsCurrentUser)
{
    // Arrange: 切换过滤类型 kFilterApps,appType 1 与 2
    m_tester->setFilterType(kFilterApps);
    addRow(QStringLiteral("user-app"), QStringLiteral("user"), 1.0, kFilterCurrentUser, "100");
    addRow(QStringLiteral("other-app"), QStringLiteral("other"), 1.0, kNoFilter, "101");
    // Act: 执行上述步骤
    // Assert: 均拒绝
    EXPECT_FALSE(accepts(0));
    EXPECT_FALSE(accepts(1));
}

// 用例名称: filterAcceptsRow_displayNameMatch_acceptsRow
// 测试类型: 边界场景
// 覆盖函数: filterAcceptsRow(分支7: 显示名命中)
// 操作步骤: kNoFilter + 过滤串仅命中显示名
// 预期结果: 行被接受
TEST_F(UT_ProcessSortFilterProxyModel, filterAcceptsRow_displayNameMatch_acceptsRow)
{
    // Arrange: 显示名含 "player",原始名不含
    addRow(QStringLiteral("Deepin Player"), QStringLiteral("xxx"), 1.0, 2, "100");
    addRow(QStringLiteral("Terminal"), QStringLiteral("term"), 1.0, 2, "101");
    m_tester->setSortFilterString(QStringLiteral("player"));
    // Act: 执行上述步骤
    // Assert: 显示名命中接受,未命中拒绝
    EXPECT_TRUE(accepts(0));
    EXPECT_FALSE(accepts(1));
}

// 用例名称: filterAcceptsRow_rawNameMatch_acceptsRow
// 测试类型: 边界场景
// 覆盖函数: filterAcceptsRow(分支8: UserRole 原始名命中)
// 操作步骤: kNoFilter + 过滤串不命中显示名但命中原始名
// 预期结果: 行被接受
TEST_F(UT_ProcessSortFilterProxyModel, filterAcceptsRow_rawNameMatch_acceptsRow)
{
    // Arrange: 显示名不含模式,原始名含
    addRow(QStringLiteral("网易云音乐"), QStringLiteral("netease-cloud-music"), 1.0, 2, "100");
    addRow(QStringLiteral("文件管理器"), QStringLiteral("dde-file-manager"), 1.0, 2, "101");
    m_tester->setSortFilterString(QStringLiteral("netea"));
    // Act: 执行上述步骤
    // Assert: 原始名兜底命中,未命中拒绝
    EXPECT_TRUE(accepts(0));
    EXPECT_FALSE(accepts(1));
}

// 用例名称: filterAcceptsRow_pidMatch_acceptsRow
// 测试类型: 边界场景
// 覆盖函数: filterAcceptsRow(分支10: PID 列 DisplayRole 命中)
// 操作步骤: kNoFilter + 名称不命中 + PID 串命中
// 预期结果: 行被接受
TEST_F(UT_ProcessSortFilterProxyModel, filterAcceptsRow_pidMatch_acceptsRow)
{
    // Arrange: 名称不含模式,PID 列 DisplayRole 命中
    addRow(QStringLiteral("calc"), QStringLiteral("calc"), 1.0, 2, QStringLiteral("1024"));
    addRow(QStringLiteral("node"), QStringLiteral("node"), 1.0, 2, QStringLiteral("2048"));
    m_tester->setSortFilterString(QStringLiteral("102"));
    // Act: 执行上述步骤
    // Assert: pid 串命中接受,pid 不含模式的行拒绝
    EXPECT_TRUE(accepts(0));
    EXPECT_FALSE(accepts(1));
}

// 用例名称: filterAcceptsRow_pinyin_chineseLocaleMatch
// 测试类型: 异常场景(环境模拟)
// 覆盖函数: filterAcceptsRow(分支9: 中文 locale 拼音命中)
// 操作步骤: 打桩 QLocale 静态工厂为中文 locale,设置中文过滤串,以实际生成的拼音串构造原始名
// 预期结果: 原始名含拼音串的行被接受,不含的被拒绝
TEST_F(UT_ProcessSortFilterProxyModel, filterAcceptsRow_pinyin_chineseLocaleMatch)
{
    // Arrange: 打桩中文 locale 后设置中文过滤串,生成拼音
    Stub st;
    st.set((QLocale(*)())ADDR(QLocale, system), stub_qlocale_system_chinese);
    m_tester->setSortFilterString(QStringLiteral("搜"));
    const QString &hanwords = m_tester->m_hanwords;
    ASSERT_FALSE(hanwords.isEmpty());
    addRow(QStringLiteral("某某输入法"), hanwords + QStringLiteral("abc"), 1.0, 2, "100");
    addRow(QStringLiteral("其他应用"), QStringLiteral("xyz"), 1.0, 2, "101");
    // Act: 逐行过滤(拼音分支)
    const bool r0 = accepts(0);
    const bool r1 = accepts(1);
    // Assert: 拼音命中接受,未命中拒绝
    EXPECT_TRUE(r0);
    EXPECT_FALSE(r1);
}

// 用例名称: lessThan_nameColumn_hanziSortedAfterAscii
// 测试类型: 边界场景
// 覆盖函数: lessThan(名字列汉字分组: 汉字排在 ASCII 之后)
// 操作步骤: sortColumn 置名字列,交叉比较 ASCII 与汉字显示名
// 预期结果: ASCII < 汉字;汉字 > ASCII
TEST_F(UT_ProcessSortFilterProxyModel, lessThan_nameColumn_hanziSortedAfterAscii)
{
    // Arrange: sortColumn 置名字列
    m_tester->sort(ProcessTableModel::kProcessNameColumn, Qt::AscendingOrder);
    addRow(QStringLiteral("abc"), QStringLiteral("abc"), 1.0, 2, "1");
    addRow(QStringLiteral("中文"), QStringLiteral("zhongwen"), 1.0, 2, "2");
    // Act: 执行上述步骤
    // Assert: 双向比较
    EXPECT_TRUE(less(0, 1, ProcessTableModel::kProcessNameColumn));
    EXPECT_FALSE(less(1, 0, ProcessTableModel::kProcessNameColumn));
}

// 用例名称: lessThan_nameColumn_equalDisplay_tiebreaksByCpu
// 测试类型: 边界场景
// 覆盖函数: lessThan(名字列显示名相同,按 CPU 列 UserRole 平局)
// 操作步骤: 两行显示名相同,CPU 值 1.0 与 2.0
// 预期结果: CPU 小者在前(返回 true),反之为 false
TEST_F(UT_ProcessSortFilterProxyModel, lessThan_nameColumn_equalDisplay_tiebreaksByCpu)
{
    // Arrange: 显示名相同,CPU 不同
    m_tester->sort(ProcessTableModel::kProcessNameColumn, Qt::AscendingOrder);
    addRow(QStringLiteral("same"), QStringLiteral("s1"), 2.0, 2, "1");
    addRow(QStringLiteral("same"), QStringLiteral("s2"), 1.0, 2, "2");
    // Act: 执行上述步骤
    // Assert: CPU 平局规则
    EXPECT_TRUE(less(1, 0, ProcessTableModel::kProcessNameColumn));
    EXPECT_FALSE(less(0, 1, ProcessTableModel::kProcessNameColumn));
}

// 用例名称: lessThan_cpuColumn_comparesByCpuValue
// 测试类型: 正常场景
// 覆盖函数: lessThan(CPU 列按 UserRole cpu 值比较)
// 操作步骤: sortColumn 置 CPU 列,比较 1.0 与 9.0
// 预期结果: CPU 小者返回 true
TEST_F(UT_ProcessSortFilterProxyModel, lessThan_cpuColumn_comparesByCpuValue)
{
    // Arrange: sortColumn 置 CPU 列
    m_tester->sort(ProcessTableModel::kProcessCPUColumn, Qt::AscendingOrder);
    addRow(QStringLiteral("low"), QStringLiteral("low"), 1.0, 2, "1");
    addRow(QStringLiteral("high"), QStringLiteral("high"), 9.0, 2, "2");
    // Act: 执行上述步骤
    // Assert: 双向
    EXPECT_TRUE(less(0, 1, ProcessTableModel::kProcessCPUColumn));
    EXPECT_FALSE(less(1, 0, ProcessTableModel::kProcessCPUColumn));
}

// 用例名称: lessThan_cpuColumn_equalCpu_tiebreaksByNameRole
// 测试类型: 边界场景
// 覆盖函数: lessThan(CPU 相等时按名字列 UserRole 数值平局)
// 操作步骤: CPU 均为 1.0,名字列 UserRole 为数值串 "100"/"200"
// 预期结果: 数值小者返回 true;反向 false
TEST_F(UT_ProcessSortFilterProxyModel, lessThan_cpuColumn_equalCpu_tiebreaksByNameRole)
{
    // Arrange: CPU 相等(非 0,qFuzzyCompare 生效),名字列 UserRole 为数值串
    // 注: CPU 平局读取名字列 UserRole(原始进程名)的 toULongLong,因此 rawName 需为数字串
    m_tester->sort(ProcessTableModel::kProcessCPUColumn, Qt::AscendingOrder);
    addRow(QStringLiteral("一百"), QStringLiteral("100"), 1.0, 2, "1");
    addRow(QStringLiteral("二百"), QStringLiteral("200"), 1.0, 2, "2");
    // Act: 执行上述步骤
    // Assert: 名字 UserRole 数值平局
    EXPECT_TRUE(less(0, 1, ProcessTableModel::kProcessCPUColumn));
    EXPECT_FALSE(less(1, 0, ProcessTableModel::kProcessCPUColumn));
}

// 用例名称: lessThan_otherColumn_fallsBackToBase
// 测试类型: 边界场景
// 覆盖函数: lessThan(非名字/CPU 列回落基类比较)
// 操作步骤: sortColumn 置 PID 列,按 PID 列 DisplayRole 字符串比较
// 预期结果: 基类字典序比较生效
TEST_F(UT_ProcessSortFilterProxyModel, lessThan_otherColumn_fallsBackToBase)
{
    // Arrange: sortColumn 置 PID 列
    m_tester->sort(ProcessTableModel::kProcessPIDColumn, Qt::AscendingOrder);
    addRow(QStringLiteral("a"), QStringLiteral("a"), 1.0, 2, QStringLiteral("1024"));
    addRow(QStringLiteral("b"), QStringLiteral("b"), 1.0, 2, QStringLiteral("2048"));
    // Act: 执行上述步骤
    // Assert: 基类比较
    EXPECT_TRUE(less(0, 1, ProcessTableModel::kProcessPIDColumn));
    EXPECT_FALSE(less(1, 0, ProcessTableModel::kProcessPIDColumn));
}
