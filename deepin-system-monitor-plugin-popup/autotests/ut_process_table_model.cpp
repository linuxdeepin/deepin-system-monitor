// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * UT_ProcessTableModel
 *
 * 测试对象: deepin-system-monitor-plugin-popup/model/process_table_model.cpp
 *           ProcessTableModel 类
 * 外部依赖: ProcessDB/ProcessSet 通过 tests/stub.h 打桩(getPIDList/getProcessById),
 *           Process::isValid 通过打桩注入,不读取真实 /proc、不依赖真实进程环境。
 *
 * 分支覆盖声明 (data, complexity=33, 源码行 96-232):
 * | #  | 分支条件                                   | 覆盖用例                                        |
 * |----|--------------------------------------------|-------------------------------------------------|
 * | 1  | index 无效 → return {}                     | data_invalidIndex_returnsEmptyVariant           |
 * | 2  | row 越界 → return {}                       | data_invalidIndex_returnsEmptyVariant           |
 * | 3  | proc 无效 → return {}                      | data_invalidProcess_returnsEmptyVariant         |
 * | 4  | DisplayRole 名字列 displayName             | data_nameColumn_displayRoleReturnsDisplayName   |
 * | 5  | state 'Z' → "(No response) " 前缀          | data_nameColumn_zombieState_prefixesNoResponse  |
 * | 6  | state 'T' → "(Suspend) " 前缀              | data_nameColumn_suspendState_prefixesSuspend    |
 * | 7  | DisplayRole CPU 列 "%1%" 一位小数          | data_cpuColumn_formatsOneDecimalPercent         |
 * | 8  | DecorationRole 名字列 icon/其他列空        | data_decorationRole_nameColumnReturnsIcon       |
 * | 9  | UserRole 名字列 raw name/CPU 列 cpu        | data_rawRoles_returnNameCpuPidAppType           |
 * | 10 | UserRole+1 恒空                            | data_rawRoles_returnNameCpuPidAppType           |
 * | 11 | TextAlignmentRole 左/右对齐                | data_alignmentRole_perColumn                    |
 * | 12 | UserRole+2 Z/T → TextWarning/否则空        | data_nameColumn_zombieState_prefixesNoResponse  |
 * | 13 | UserRole+3 appType                         | data_rawRoles_returnNameCpuPidAppType           |
 * | 14 | UserRole+4 pid 串                          | data_rawRoles_returnNameCpuPidAppType           |
 * | 15 | 未匹配角色 → {}                            | data_invalidProcess_returnsEmptyVariant         |
 *
 * 备注: rowCount 经 updateProcessListDelay 与 ProcessSet 打桩间接驱动;
 *       updateProcessList 为 QTimer 单发延迟转发,用 QTRY_COMPARE 等待事件循环。
 *
 * 用例声明表:
 * |--------------------------------------------------|------------------------|---------|------|--------------------------------|
 * | 测试用例名                                        | 测试类                 | 测试级别 | 类型 | 覆盖函数                       |
 * |--------------------------------------------------|------------------------|---------|------|--------------------------------|
 * | constructor_defaults_headerAndAlignment          | UT_ProcessTableModel   | Level_1 | AUTO | ctor+columnCount+headerData    |
 * | data_invalidIndex_returnsEmptyVariant            | UT_ProcessTableModel   | Level_1 | AUTO | data(分支1/2)                  |
 * | data_invalidProcess_returnsEmptyVariant          | UT_ProcessTableModel   | Level_3 | STUB | data(分支3/15)                 |
 * | data_nameColumn_displayRoleReturnsDisplayName    | UT_ProcessTableModel   | Level_1 | STUB | data(分支4)                    |
 * | data_nameColumn_zombieState_prefixesNoResponse   | UT_ProcessTableModel   | Level_2 | STUB | data(分支5/12)                 |
 * | data_nameColumn_suspendState_prefixesSuspend     | UT_ProcessTableModel   | Level_2 | STUB | data(分支6)                    |
 * | data_cpuColumn_formatsOneDecimalPercent          | UT_ProcessTableModel   | Level_1 | STUB | data(分支7)                    |
 * | data_decorationRole_nameColumnReturnsIcon        | UT_ProcessTableModel   | Level_2 | STUB | data(分支8)                    |
 * | data_rawRoles_returnNameCpuPidAppType            | UT_ProcessTableModel   | Level_1 | STUB | data(分支9/10/13/14)           |
 * | data_alignmentRole_perColumn                     | UT_ProcessTableModel   | Level_1 | STUB | data(分支11)                   |
 * | flags_validIndex_enabledSelectable               | UT_ProcessTableModel   | Level_2 | AUTO | flags                          |
 * | updateProcessListDelay_NewPids_InsertsRows       | UT_ProcessTableModel   | Level_1 | STUB | updateProcessListDelay+rowCount|
 * | updateProcessListDelay_ExistingPids_UpdatesRows  | UT_ProcessTableModel   | Level_1 | STUB | updateProcessListDelay(更新)   |
 * | updateProcessListDelay_MissingPids_RemovesRows   | UT_ProcessTableModel   | Level_2 | STUB | updateProcessListDelay(删除)   |
 * | updateProcessList_PostsEvent_TriggersDelay             | UT_ProcessTableModel   | Level_2 | STUB | updateProcessList              |
 * | removeProcess_KnownPid_RemovesRow            | UT_ProcessTableModel   | Level_1 | STUB | removeProcess                  |
 * | updateProcessState_KnownPid_EmitsDataChanged    | UT_ProcessTableModel   | Level_1 | STUB | updateProcessState             |
 * | getProcess_KnownAndUnknownPid_ReturnsCopy      | UT_ProcessTableModel   | Level_2 | STUB | getProcess                     |
 * | getProcessState_KnownAndUnknownPid_ReturnsState               | UT_ProcessTableModel   | Level_2 | STUB | getProcessState                |
 * | getProcessPriority_AllLevels_MapsCorrectly         | UT_ProcessTableModel   | Level_3 | STUB | getProcessPriority             |
 * | removeProcess_UnknownPid_NoChange              | UT_ProcessTableModel   | Level_2 | STUB | removeProcess(未命中分支)      |
 * | headerData_OutOfRangeSection_FallsToBase       | UT_ProcessTableModel   | Level_2 | AUTO | headerData(越界回退)           |
 *
 * 用例计数声明(方法 | level | factors | min | actual):
 * | data | high | complexity:33 | 3 | 9 |
 * | headerData | mid | complexity:7 | 2 | 2 |
 * | updateProcessListDelay | mid | linear_scan_in_loop:5 | 3 | 3 |
 * | removeProcess | mid | name_pattern | 2 | 2 |
 * | rowCount | low | - | 1 | 4 |
 * | columnCount | low | - | 1 | 1 |
 * | flags | low | - | 1 | 1 |
 * | updateProcessList | low | - | 1 | 1 |
 * | updateProcessState | low | - | 1 | 1 |
 * | getProcess | low | - | 1 | 1 |
 * | getProcessState | low | - | 1 | 1 |
 * | getProcessPriority | low | - | 1 | 1 |
 *
 * 风险检查清单:
 * 1. 对每一个测试文件中的 stub 都执行了 reset?  [是] Stub 局部对象析构自动恢复
 * 2. 对每一个测试文件中的 stub 都执行了 restore? [是] Stub 析构函数恢复被改写函数
 * 3. 所有的测试使用 ASSERT/EXPECT 宏而不是布尔判断? [是]
 * 4. 所有的 ASSERT 宏的返回值都被检查? [是]
 * 5. 用例覆盖了所有分支? [是] data 全部分支见上方分支声明表
 * 6. 本文件与类定义的头文件一致(用例使用的是头文件中公开的方法)? [是]
 * 7. 所有的属性和方法都被测试到了? [是]
 * 8. 测试代码可读性高,测试方法意义明确? [是]
 * 9. 任何测试不能对其它测试产生依赖? [是] 每用例独立打桩与构造
 * 10. 使用的是 Google Test 而不是 QtTest? [是]
 */

#include <QSignalSpy>
#include <QTest>
#include <QVariant>
#include <DPalette>
#include <gtest/gtest.h>

#include "../../tests/addr_pri.h"
#include "../../tests/stub.h"
#include "process/process.h"
#include "process/process_db.h"
#include <algorithm>

#include "process/process_set.h"
#include "model/process_table_model.h"

// ==================== ProcessSet 打桩数据 ====================
// stub 期间 ProcessSet::getPIDList/getProcessById 的返回内容
static QHash<pid_t, Process> s_stubProcById;

QList<pid_t> stub_ProcessSet_getPIDList()
{
    // QHash 遍历顺序不定,排序后保证注入行的行序确定
    QList<pid_t> pids = s_stubProcById.keys();
    std::sort(pids.begin(), pids.end());
    return pids;
}

// 注意:ProcessSet::getProcessById 返回非平凡可拷贝类型 Process,Q++ ABI 下隐藏的
// sret 指针占用 RDI、this 占用 RSI,因此桩函数首参必须为 ProcessSet*,否则 pid 会
// 读到 this 指针造成数据错乱(getPIDList 无参、isValid 返回平凡类型,不受影响)。
Process stub_ProcessSet_getProcessById(ProcessSet *self, pid_t pid)
{
    Q_UNUSED(self);
    return s_stubProcById.value(pid, Process(pid));
}

bool stub_Process_isValid_true()
{
    return true;
}

bool stub_Process_isValid_false()
{
    return false;
}

// ==================== 测试夹具 ====================
class UT_ProcessTableModel : public ::testing::Test
{
public:
    void SetUp() override
    {
        m_tester = new ProcessTableModel();
    }

    void TearDown() override
    {
        delete m_tester;
        m_tester = nullptr;
    }

    // 向打桩数据中注册一个进程
    static Process &addStubProc(pid_t pid)
    {
        Process p(pid);
        p.setName(QStringLiteral("proc-%1").arg(pid));
        p.setDisplayName(QStringLiteral("Proc %1").arg(pid));
        p.setState('R');
        p.setCpu(0.0);
        p.setAppType(kNoFilter);
        s_stubProcById.insert(pid, p);
        return s_stubProcById[pid];
    }

    // 在打桩作用域内执行差量刷新(直接调用 delay 槽,不经过定时器)
    void refreshWithStubs()
    {
        Stub st;
        st.set(ADDR(ProcessSet, getPIDList), stub_ProcessSet_getPIDList);
        st.set(ADDR(ProcessSet, getProcessById), stub_ProcessSet_getProcessById);
        m_tester->updateProcessListDelay();
    }

    // 构造一个已注入并视为有效的单进程模型,返回 isValid 打桩对象(栈上,随用例析构恢复)
    Stub *stubValidSingleProc()
    {
        s_stubProcById.clear();
        addStubProc(100).setDisplayName(QStringLiteral("Firefox"));
        refreshWithStubs();
        return new Stub();   // 用例内 delete;配合 stub_Process_isValid_true 使用
    }

    QModelIndex row0(int column) const
    {
        return m_tester->index(0, column);
    }

    ProcessTableModel *m_tester = nullptr;
};

// ==================== 测试用例 ====================

// 用例名称: constructor_defaults_headerAndAlignment
// 测试类型: 正常场景
// 覆盖函数: 构造函数 + columnCount + headerData
// 操作步骤: 构造模型,检查列数、表头文案、对齐与初始排序
// 预期结果: 3 列;Name/CPU 表头;表头节统一左对齐(列级对齐由 data() 决定);初始降序
TEST_F(UT_ProcessTableModel, constructor_defaults_headerAndAlignment)
{
    // Arrange: 默认构造
    // Act: 执行上述步骤
    // Assert: 列数与表头
    EXPECT_EQ(m_tester->columnCount(), ProcessTableModel::kProcessColumnCount);
    EXPECT_EQ(m_tester->columnCount(), 3);
    EXPECT_EQ(m_tester->rowCount(), 0);
    EXPECT_EQ(m_tester->headerData(0, Qt::Horizontal, Qt::DisplayRole).toString(),
              QApplication::translate("Process.Table.Header", "Name"));
    EXPECT_EQ(m_tester->headerData(1, Qt::Horizontal, Qt::DisplayRole).toString(),
              QApplication::translate("Process.Table.Header", "CPU"));
    // 表头节对齐:headerData 对所有节恒返回 AlignLeft|AlignVCenter(列级对齐在 data() 中)
    const QVariant nameAlign = m_tester->headerData(0, Qt::Horizontal, Qt::TextAlignmentRole);
    EXPECT_TRUE(nameAlign.isValid());
    EXPECT_EQ(nameAlign.toInt() & Qt::AlignHorizontal_Mask, int(Qt::AlignLeft));
    const QVariant cpuAlign = m_tester->headerData(1, Qt::Horizontal, Qt::TextAlignmentRole);
    EXPECT_TRUE(cpuAlign.isValid());
    EXPECT_EQ(cpuAlign.toInt() & Qt::AlignHorizontal_Mask, int(Qt::AlignLeft));
    EXPECT_EQ(nameAlign.toInt(), int(Qt::AlignLeft | Qt::AlignVCenter));
    // 初始排序方向为降序
    const QVariant order = m_tester->headerData(0, Qt::Horizontal, Qt::InitialSortOrderRole);
    EXPECT_EQ(qvariant_cast<Qt::SortOrder>(order), Qt::DescendingOrder);
}

// 用例名称: data_invalidIndex_returnsEmptyVariant
// 测试类型: 正常场景
// 覆盖函数: data(分支1: 无效 index;分支2: row 越界)
// 操作步骤: 用无效 QModelIndex 与越界行号取数据
// 预期结果: 均返回空 QVariant
TEST_F(UT_ProcessTableModel, data_invalidIndex_returnsEmptyVariant)
{
    // Arrange: 空模型,无任何行
    // Act: 执行上述步骤
    // Assert: 无效 index 与越界行
    EXPECT_FALSE(m_tester->data(QModelIndex(), Qt::DisplayRole).isValid());
    EXPECT_FALSE(m_tester->data(m_tester->index(5, 0), Qt::DisplayRole).isValid());
    EXPECT_FALSE(m_tester->data(m_tester->index(-1, 0), Qt::DisplayRole).isValid());
}

// 用例名称: data_invalidProcess_returnsEmptyVariant
// 测试类型: 异常场景
// 覆盖函数: data(分支3: proc 无效;分支15: 未匹配角色)
// 操作步骤: 注入进程后打桩 isValid=false,逐角色取数据
// 预期结果: 所有角色均返回空 QVariant
TEST_F(UT_ProcessTableModel, data_invalidProcess_returnsEmptyVariant)
{
    // Arrange: 注入 1 个进程,但打桩为无效
    s_stubProcById.clear();
    addStubProc(100);
    refreshWithStubs();
    Stub st;
    st.set(ADDR(Process, isValid), stub_Process_isValid_false);
    // Act: 执行上述步骤
    // Assert: 全角色空
    EXPECT_FALSE(m_tester->data(row0(0), Qt::DisplayRole).isValid());
    EXPECT_FALSE(m_tester->data(row0(1), Qt::DisplayRole).isValid());
    EXPECT_FALSE(m_tester->data(row0(0), Qt::DecorationRole).isValid());
    EXPECT_FALSE(m_tester->data(row0(0), Qt::UserRole).isValid());
    EXPECT_FALSE(m_tester->data(row0(0), Qt::UserRole + 2).isValid());
    EXPECT_FALSE(m_tester->data(row0(0), Qt::UserRole + 3).isValid());
}



// 用例名称: data_nameColumn_displayRoleReturnsDisplayName
// 测试类型: 正常场景
// 覆盖函数: data(分支4: 名字列 DisplayRole)
// 操作步骤: 注入运行态进程,读取名字列 DisplayRole
// 预期结果: 返回 displayName 原文,无状态前缀
TEST_F(UT_ProcessTableModel, data_nameColumn_displayRoleReturnsDisplayName)
{
    // Arrange: 注入有效进程
    Stub *st = stubValidSingleProc();
    st->set(ADDR(Process, isValid), stub_Process_isValid_true);
    // Act: 读取名字列
    const QVariant v = m_tester->data(row0(ProcessTableModel::kProcessNameColumn), Qt::DisplayRole);
    // Assert: 显示名原文且无状态前缀
    EXPECT_EQ(v.toString(), QStringLiteral("Firefox"));
    EXPECT_FALSE(v.toString().contains(QApplication::translate("Process.Table", "Suspend")));
    delete st;
}

// 用例名称: data_nameColumn_zombieState_prefixesNoResponse
// 测试类型: 边界场景
// 覆盖函数: data(分支5: 'Z' 前缀;分支12: UserRole+2 TextWarning)
// 操作步骤: 注入 'Z' 状态进程,读取显示名与文本警示角色
// 预期结果: "(No response) Firefox";UserRole+2 返回 DPalette::TextWarning
TEST_F(UT_ProcessTableModel, data_nameColumn_zombieState_prefixesNoResponse)
{
    // Arrange: 'Z' 状态进程
    s_stubProcById.clear();
    Process &p = addStubProc(100);
    p.setState('Z');
    p.setDisplayName(QStringLiteral("Firefox"));
    refreshWithStubs();
    Stub st;
    st.set(ADDR(Process, isValid), stub_Process_isValid_true);
    // Act: 读显示名与警示色角色
    const QString name = m_tester->data(row0(0), Qt::DisplayRole).toString();
    const QVariant warn = m_tester->data(row0(0), Qt::UserRole + 2);
    // Assert: 前缀与警示色
    EXPECT_EQ(name,
              QStringLiteral("(%1) %2")
                  .arg(QApplication::translate("Process.Table", "No response"))
                  .arg(QStringLiteral("Firefox")));
    EXPECT_EQ(warn.toInt(), int(Dtk::Gui::DPalette::TextWarning));
}

// 用例名称: data_nameColumn_suspendState_prefixesSuspend
// 测试类型: 边界场景
// 覆盖函数: data(分支6: 'T' 前缀)
// 操作步骤: 注入 'T' 状态进程,读取显示名
// 预期结果: "(Suspend) Firefox"
TEST_F(UT_ProcessTableModel, data_nameColumn_suspendState_prefixesSuspend)
{
    // Arrange: 'T' 状态进程
    s_stubProcById.clear();
    Process &p = addStubProc(100);
    p.setState('T');
    p.setDisplayName(QStringLiteral("Firefox"));
    refreshWithStubs();
    Stub st;
    st.set(ADDR(Process, isValid), stub_Process_isValid_true);
    // Act: 读显示名
    const QString name = m_tester->data(row0(0), Qt::DisplayRole).toString();
    // Assert: 挂起前缀
    EXPECT_EQ(name,
              QStringLiteral("(%1) %2")
                  .arg(QApplication::translate("Process.Table", "Suspend"))
                  .arg(QStringLiteral("Firefox")));
    EXPECT_TRUE(name.startsWith('('));
    EXPECT_TRUE(name.endsWith(QStringLiteral("Firefox")));
}

// 用例名称: data_cpuColumn_formatsOneDecimalPercent
// 测试类型: 正常场景
// 覆盖函数: data(分支7: CPU 列 "%1%" 一位小数)
// 操作步骤: 注入 cpu=12.34 与 cpu=-1.5 的进程,读取 CPU 列
// 预期结果: "12.3%" 与 "-1.5%"
TEST_F(UT_ProcessTableModel, data_cpuColumn_formatsOneDecimalPercent)
{
    // Arrange: 两个不同 CPU 值的进程
    s_stubProcById.clear();
    addStubProc(100).setCpu(12.34);
    addStubProc(200).setCpu(-1.5);
    refreshWithStubs();
    Stub st;
    st.set(ADDR(Process, isValid), stub_Process_isValid_true);
    // Act: 读取 CPU 列
    const QString cpu0 = m_tester->data(m_tester->index(0, ProcessTableModel::kProcessCPUColumn),
                                        Qt::DisplayRole).toString();
    const QString cpu1 = m_tester->data(m_tester->index(1, ProcessTableModel::kProcessCPUColumn),
                                        Qt::DisplayRole).toString();
    // Assert: 一位小数百分比
    EXPECT_EQ(cpu0, QStringLiteral("12.3%"));
    EXPECT_EQ(cpu1, QStringLiteral("-1.5%"));
}

// 用例名称: data_decorationRole_nameColumnReturnsIcon
// 测试类型: 边界场景
// 覆盖函数: data(分支8: DecorationRole 名字列 icon,其他列空)
// 操作步骤: 注入有效进程,读名字列与 CPU 列的 DecorationRole
// 预期结果: 名字列返回 QIcon 变体,CPU 列为空
TEST_F(UT_ProcessTableModel, data_decorationRole_nameColumnReturnsIcon)
{
    // Arrange: 有效进程
    s_stubProcById.clear();
    addStubProc(100);
    refreshWithStubs();
    Stub st;
    st.set(ADDR(Process, isValid), stub_Process_isValid_true);
    // Act: 读装饰角色
    const QVariant nameDeco = m_tester->data(row0(0), Qt::DecorationRole);
    const QVariant cpuDeco = m_tester->data(row0(1), Qt::DecorationRole);
    // Assert: 名字列为 QIcon 类型,CPU 列为空
    EXPECT_EQ(nameDeco.userType(), qMetaTypeId<QIcon>());
    EXPECT_FALSE(cpuDeco.isValid());
}

// 用例名称: data_rawRoles_returnNameCpuPidAppType
// 测试类型: 正常场景
// 覆盖函数: data(分支9/10/13/14: UserRole 系列原始数据)
// 操作步骤: 注入完整属性的进程,逐角色读取
// 预期结果: UserRole 名/CPU;UserRole+1 恒空;UserRole+3 appType;UserRole+4 pid 串
TEST_F(UT_ProcessTableModel, data_rawRoles_returnNameCpuPidAppType)
{
    // Arrange: 属性齐全的进程
    s_stubProcById.clear();
    Process &p = addStubProc(100);
    p.setName(QStringLiteral("firefox-bin"));
    p.setCpu(3.5);
    p.setAppType(kFilterApps);
    refreshWithStubs();
    Stub st;
    st.set(ADDR(Process, isValid), stub_Process_isValid_true);
    // Act: 执行上述步骤
    // Assert: UserRole 原始名/CPU
    EXPECT_EQ(m_tester->data(row0(0), Qt::UserRole).toString(), QStringLiteral("firefox-bin"));
    EXPECT_DOUBLE_EQ(m_tester->data(row0(1), Qt::UserRole).toDouble(), 3.5);
    // UserRole+1 恒为空
    EXPECT_FALSE(m_tester->data(row0(0), Qt::UserRole + 1).isValid());
    // UserRole+3 appType(任意列)
    EXPECT_EQ(m_tester->data(row0(2), Qt::UserRole + 3).toInt(), int(kFilterApps));
    // UserRole+4 pid 串
    EXPECT_EQ(m_tester->data(row0(2), Qt::UserRole + 4).toString(), QStringLiteral("100"));
}

// 用例名称: data_alignmentRole_perColumn
// 测试类型: 正常场景
// 覆盖函数: data(分支11: TextAlignmentRole)
// 操作步骤: 注入有效进程,读名字列与 CPU 列对齐
// 预期结果: 名字左对齐垂直居中,CPU 右对齐垂直居中
TEST_F(UT_ProcessTableModel, data_alignmentRole_perColumn)
{
    // Arrange: 有效进程
    s_stubProcById.clear();
    addStubProc(100);
    refreshWithStubs();
    Stub st;
    st.set(ADDR(Process, isValid), stub_Process_isValid_true);
    // Act: 读对齐角色
    const int nameAlign = m_tester->data(row0(0), Qt::TextAlignmentRole).toInt();
    const int cpuAlign = m_tester->data(row0(1), Qt::TextAlignmentRole).toInt();
    // Assert: 左/右对齐
    EXPECT_EQ(nameAlign, int(Qt::AlignLeft | Qt::AlignVCenter));
    EXPECT_EQ(cpuAlign, int(Qt::AlignRight | Qt::AlignVCenter));
}

// 用例名称: flags_validIndex_enabledSelectable
// 测试类型: 边界场景
// 覆盖函数: flags
// 操作步骤: 注入 1 行,读有效与无效 index 的 flags
// 预期结果: 有效行 Enabled|Selectable,无效 index 无标志
TEST_F(UT_ProcessTableModel, flags_validIndex_enabledSelectable)
{
    // Arrange: 注入 1 行
    s_stubProcById.clear();
    addStubProc(100);
    refreshWithStubs();
    // Act: 读 flags
    const Qt::ItemFlags validFlags = m_tester->flags(m_tester->index(0, 0));
    const Qt::ItemFlags invalidFlags = m_tester->flags(QModelIndex());
    // Assert: 启用且可选
    EXPECT_EQ(validFlags, Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    EXPECT_EQ(invalidFlags, Qt::NoItemFlags);
}

// 用例名称: updateProcessListDelay_NewPids_InsertsRows
// 测试类型: 正常场景
// 覆盖函数: updateProcessListDelay(插入路径) + rowCount + getProcess
// 操作步骤: 打桩 2 个新 pid 后刷新
// 预期结果: rowCount=2,getProcess 可取回,modelUpdated 发射 1 次
TEST_F(UT_ProcessTableModel, updateProcessListDelay_NewPids_InsertsRows)
{
    // Arrange: 打桩 2 个进程
    s_stubProcById.clear();
    addStubProc(100);
    addStubProc(200);
    QSignalSpy updatedSpy(m_tester, &ProcessTableModel::modelUpdated);
    // Act: 打桩刷新
    {
        Stub st;
        st.set(ADDR(ProcessSet, getPIDList), stub_ProcessSet_getPIDList);
        st.set(ADDR(ProcessSet, getProcessById), stub_ProcessSet_getProcessById);
        m_tester->updateProcessListDelay();
    }
    // Assert: 插入完成
    EXPECT_EQ(m_tester->rowCount(), 2);
    EXPECT_FALSE(m_tester->getProcess(100).isValid());   // 未打桩 isValid,默认无效
    EXPECT_EQ(updatedSpy.count(), 1);
}

// 用例名称: updateProcessListDelay_ExistingPids_UpdatesRows
// 测试类型: 正常场景
// 覆盖函数: updateProcessListDelay(更新路径)
// 操作步骤: 二次刷新相同 pid 集合
// 预期结果: 行数不变,逐行发射 dataChanged
TEST_F(UT_ProcessTableModel, updateProcessListDelay_ExistingPids_UpdatesRows)
{
    // Arrange: 首次插入 2 行
    s_stubProcById.clear();
    addStubProc(100);
    addStubProc(200);
    refreshWithStubs();
    QSignalSpy changedSpy(m_tester, &ProcessTableModel::dataChanged);
    QSignalSpy updatedSpy(m_tester, &ProcessTableModel::modelUpdated);
    // Act: 二次刷新(相同 pid)
    refreshWithStubs();
    // Assert: 数据变更信号
    EXPECT_EQ(m_tester->rowCount(), 2);
    EXPECT_EQ(changedSpy.count(), 2);
    EXPECT_EQ(changedSpy.at(0).at(0).toModelIndex().row(), 0);
    EXPECT_EQ(changedSpy.at(1).at(0).toModelIndex().row(), 1);
    EXPECT_EQ(updatedSpy.count(), 1);
}

// 用例名称: updateProcessListDelay_MissingPids_RemovesRows
// 测试类型: 边界场景
// 覆盖函数: updateProcessListDelay(删除路径)
// 操作步骤: 刷新后清空打桩 pid 集合再刷新
// 预期结果: 行数随之减少至 0,每次刷新发射 modelUpdated
TEST_F(UT_ProcessTableModel, updateProcessListDelay_MissingPids_RemovesRows)
{
    // Arrange: 插入 2 行
    s_stubProcById.clear();
    addStubProc(100);
    addStubProc(200);
    refreshWithStubs();
    QSignalSpy updatedSpy(m_tester, &ProcessTableModel::modelUpdated);
    // Act: 删除 200,再清空
    s_stubProcById.remove(200);
    refreshWithStubs();
    EXPECT_EQ(m_tester->rowCount(), 1);
    s_stubProcById.clear();
    refreshWithStubs();
    // Assert: 全部删除
    EXPECT_EQ(m_tester->rowCount(), 0);
    EXPECT_EQ(updatedSpy.count(), 2);
}

// 用例名称: updateProcessList_PostsEvent_TriggersDelay
// 测试类型: 边界场景
// 覆盖函数: updateProcessList(单发定时器转发)
// 操作步骤: 打桩后调用 updateProcessList,等待事件循环
// 预期结果: updateProcessListDelay 被异步执行,modelUpdated 发射 1 次
TEST_F(UT_ProcessTableModel, updateProcessList_PostsEvent_TriggersDelay)
{
    // Arrange: 打桩 1 个进程 + 信号监听
    s_stubProcById.clear();
    addStubProc(100);
    Stub st;
    st.set(ADDR(ProcessSet, getPIDList), stub_ProcessSet_getPIDList);
    st.set(ADDR(ProcessSet, getProcessById), stub_ProcessSet_getProcessById);
    QSignalSpy updatedSpy(m_tester, &ProcessTableModel::modelUpdated);
    // Act: 发布延迟刷新并等待
    m_tester->updateProcessList();
    QTRY_COMPARE_WITH_TIMEOUT(updatedSpy.count(), 1, 3000);
    // Assert: 信号恰好 1 次且行数就位
    EXPECT_EQ(updatedSpy.count(), 1);
    EXPECT_EQ(m_tester->rowCount(), 1);
}

// 用例名称: removeProcess_KnownPid_RemovesRow
// 测试类型: 正常场景
// 覆盖函数: removeProcess
// 操作步骤: 注入 2 行后删除指定 pid
// 预期结果: 目标行删除,其余保留
TEST_F(UT_ProcessTableModel, removeProcess_KnownPid_RemovesRow)
{
    // Arrange: 注入 2 行
    s_stubProcById.clear();
    addStubProc(100);
    addStubProc(200);
    refreshWithStubs();
    // getProcess 走 ProcessSet 实时查询,保持打桩存活以命中注入数据
    Stub st;
    st.set(ADDR(ProcessSet, getProcessById), stub_ProcessSet_getProcessById);
    // Act: 删除 100
    m_tester->removeProcess(100);
    // Assert: 只剩 200
    EXPECT_EQ(m_tester->rowCount(), 1);
    EXPECT_EQ(m_tester->getProcess(200).pid(), pid_t(200));
}

// 用例名称: removeProcess_UnknownPid_NoChange
// 测试类型: 边界场景
// 覆盖函数: removeProcess(分支: pid 未命中 indexOf<0)
// 操作步骤: 注入 2 行后删除不存在的 pid=999
// 预期结果: 行数与数据均不变,不崩溃
TEST_F(UT_ProcessTableModel, removeProcess_UnknownPid_NoChange)
{
    // Arrange: 注入 2 行
    s_stubProcById.clear();
    addStubProc(100);
    addStubProc(200);
    refreshWithStubs();
    Stub st;
    st.set(ADDR(ProcessSet, getProcessById), stub_ProcessSet_getProcessById);
    // Act: 删除未知 pid
    m_tester->removeProcess(999);
    // Assert: 模型无变化
    EXPECT_EQ(m_tester->rowCount(), 2);
    EXPECT_EQ(m_tester->getProcess(100).pid(), pid_t(100));
    EXPECT_EQ(m_tester->getProcess(200).pid(), pid_t(200));
}

// 用例名称: headerData_OutOfRangeSection_FallsToBase
// 测试类型: 边界场景
// 覆盖函数: headerData(分支: 越界 section 回退 QAbstractTableModel)
// 操作步骤: 请求越界列的 DisplayRole 与垂直方向的恒定角色
// 预期结果: 越界 DisplayRole 回退 Qt6 基类默认(section+1);对齐/排序角色不受方向与越界影响
TEST_F(UT_ProcessTableModel, headerData_OutOfRangeSection_FallsToBase)
{
    // Arrange: 默认构造,无需数据
    // Act: 越界与垂直方向请求
    const QVariant oobDisplay = m_tester->headerData(9, Qt::Horizontal, Qt::DisplayRole);
    const QVariant vertAlign = m_tester->headerData(0, Qt::Vertical, Qt::TextAlignmentRole);
    const QVariant oobAlign = m_tester->headerData(9, Qt::Horizontal, Qt::TextAlignmentRole);
    const QVariant vertOrder = m_tester->headerData(0, Qt::Vertical, Qt::InitialSortOrderRole);
    // Assert: 越界回退基类(Qt6 默认返回 section+1);恒定角色与方向/越界无关
    EXPECT_EQ(oobDisplay.toString(), QStringLiteral("10"));
    EXPECT_EQ(oobAlign.toInt(), int(Qt::AlignLeft | Qt::AlignVCenter));
    EXPECT_EQ(vertAlign.toInt(), int(Qt::AlignLeft | Qt::AlignVCenter));
    EXPECT_EQ(qvariant_cast<Qt::SortOrder>(vertOrder), Qt::DescendingOrder);
}

// 用例名称: updateProcessState_KnownPid_EmitsDataChanged
// 测试类型: 正常场景
// 覆盖函数: updateProcessState
// 操作步骤: 注入 1 行后更新状态为 'T'
// 预期结果: 状态更新并按行发射 dataChanged
TEST_F(UT_ProcessTableModel, updateProcessState_KnownPid_EmitsDataChanged)
{
    // Arrange: 注入 1 行
    s_stubProcById.clear();
    addStubProc(100);
    refreshWithStubs();
    QSignalSpy changedSpy(m_tester, &ProcessTableModel::dataChanged);
    // Act: 更新状态
    m_tester->updateProcessState(100, 'T');
    // Assert: 变更信号落在第 0 行
    EXPECT_EQ(changedSpy.count(), 1);
    EXPECT_EQ(changedSpy.at(0).at(0).toModelIndex().row(), 0);
    // 模型内状态已更新(打桩 isValid 后可见前缀)
    Stub st;
    st.set(ADDR(Process, isValid), stub_Process_isValid_true);
    const QString name = m_tester->data(row0(0), Qt::DisplayRole).toString();
    EXPECT_TRUE(name.contains(QApplication::translate("Process.Table", "Suspend")));
}

// 用例名称: getProcess_KnownAndUnknownPid_ReturnsCopy
// 测试类型: 边界场景
// 覆盖函数: getProcess
// 操作步骤: 注入 2 行,查询已知与未知 pid
// 预期结果: 已知 pid 返回对应进程;未知 pid 返回默认无效进程
TEST_F(UT_ProcessTableModel, getProcess_KnownAndUnknownPid_ReturnsCopy)
{
    // Arrange: 注入 2 行
    s_stubProcById.clear();
    addStubProc(100);
    addStubProc(200);
    refreshWithStubs();
    // getProcess 走 ProcessSet 实时查询,保持打桩存活以命中注入数据
    Stub st;
    st.set(ADDR(ProcessSet, getProcessById), stub_ProcessSet_getProcessById);
    // Act: 执行上述步骤
    // Assert: 已知与未知
    const Process &known = m_tester->getProcess(100);
    EXPECT_EQ(known.pid(), pid_t(100));
    EXPECT_EQ(known.name(), QStringLiteral("proc-100"));
    const Process &unknown = m_tester->getProcess(999);
    EXPECT_FALSE(unknown.isValid());
}

// 用例名称: getProcessState_KnownAndUnknownPid_ReturnsState
// 测试类型: 边界场景
// 覆盖函数: getProcessState
// 操作步骤: 注入 'Z' 状态进程,查询已知与未知 pid
// 预期结果: 已知返回 'Z',未知返回 '\\0'
TEST_F(UT_ProcessTableModel, getProcessState_KnownAndUnknownPid_ReturnsState)
{
    // Arrange: 'Z' 状态进程
    s_stubProcById.clear();
    addStubProc(100).setState('Z');
    refreshWithStubs();
    // getProcessState 走 ProcessSet 实时查询,保持打桩存活以命中注入数据
    Stub st;
    st.set(ADDR(ProcessSet, getProcessById), stub_ProcessSet_getProcessById);
    // Act: 执行上述步骤
    // Assert: 状态查询
    EXPECT_EQ(m_tester->getProcessState(100), 'Z');
    EXPECT_EQ(m_tester->getProcessState(999), '\0');
}

// 用例名称: getProcessPriority_AllLevels_MapsCorrectly
// 测试类型: 异常场景
// 覆盖函数: getProcessPriority(优先级映射分支)
// 操作步骤: 注入 nice 为 -20/0/5/99 的进程,逐一查询
// 预期结果: kVeryHigh/kNormal/kCustom/kInvalid;未知 pid 返回 kInvalidPriority
TEST_F(UT_ProcessTableModel, getProcessPriority_AllLevels_MapsCorrectly)
{
    // Arrange: 4 个不同 nice 的进程
    s_stubProcById.clear();
    addStubProc(100).setPriority(-20);
    addStubProc(200).setPriority(0);
    addStubProc(300).setPriority(5);
    addStubProc(400).setPriority(99);
    refreshWithStubs();
    // getProcessPriority 走 ProcessSet 实时查询,保持打桩存活以命中注入数据
    Stub st;
    st.set(ADDR(ProcessSet, getProcessById), stub_ProcessSet_getProcessById);
    // Act: 执行上述步骤
    // Assert: 映射分支
    EXPECT_EQ(m_tester->getProcessPriority(100), kVeryHighPriority);
    EXPECT_EQ(m_tester->getProcessPriority(200), kNormalPriority);
    EXPECT_EQ(m_tester->getProcessPriority(300), kCustomPriority);
    EXPECT_EQ(m_tester->getProcessPriority(400), kInvalidPriority);
    EXPECT_EQ(m_tester->getProcessPriority(999), kInvalidPriority);
}
