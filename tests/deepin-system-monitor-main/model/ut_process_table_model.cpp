// Copyright (C) 2019-2026 UnionTech Software Technology Co., Ltd.
// SPDX-FileCopyrightText: 2019-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// rewritten unit tests for ProcessTableModel (deepin-system-monitor-main/model/process_table_model.cpp)
// based on upstream/master @ 3df42339004d7aa3e2cf2cc7b09de81fb8c981c2
//
// 用例计数声明（满足 test-code-gen.md §4 下限表，level/factors 取自 .ut inventory t18）:
//   data()                      high  (cx=52, factors: branches=52, role=8)  -> min 3  -> 39 (Display 15 + UserRole 11 + role 9 + 优先级 7 + 参数化映射对账)
//   headerData()                high  (cx=18, factors: columns=13, role=4)   -> min 3  -> 17 (Display 13 + alignment/sort/base 4)
//   updateProcessList           low   (cx=2)                                 -> min 1  -> 2
//   updateProcessListDelay      mid   (cx=5, loop)                           -> min 2  -> 5
//   updateProcessListWithUserSpecified mid (cx=5, loop)                    -> min 2  -> 3
//   updateProcessState          low   (cx=3)                                 -> min 1  -> 2
//   updateProcessPriority       low   (cx=3)                                 -> min 1  -> 2
//   removeProcess               mid   (cx=3)                                 -> min 1  -> 2
//   rowCount / columnCount      mid   (cx=2)                                 -> min 1  -> 4
//   getProcessState/getProcess/getProcessPriority*/getTotal*/setUserModeName low -> min 1 -> 18
//
// 最小清单（test-types.md §8）自检:
//   [x] 1. 公开方法已列出，每个方法 >= 1 用例
//   [x] 2. 每个输入维度按等价类划分（role 维度 8 类 / column 维度 13 类 / state 维度 / pid 存在性）
//   [x] 3. 等价类边界值显式覆盖（row=0 / row=size / column=kProcessColumnCount / pid=-1 / prio=19,20）
//   [x] 4. 同质多组输入用 TEST_P 参数化（DisplayRole 列矩阵 / UserRole 列矩阵 / headerData 列矩阵 / 优先级名映射）
//   [x] 5. 分支清单已列出并映射到用例名（见下方 BRANCH MAP）
//   [x] 6. 每条 if/switch/early-return 分支有触发用例
//   [x] 7. 无显式 throw 路径（源码无 throw），负面路径用空值/越界输入构造
//   [x] 8. 负面场景（无效 index / 越界 row / 越界 column / 未知 pid / 无效 Process）有专门用例
//   [x] 9. 负面用例验证状态保持（未知 pid 操作后 rowCount/内容不变）
//   [x] 10. stub_ext 打桩（ProcessSet::getPIDList / getProcessById / QTimer::singleShot），未引入 gMock
//
// ============================ BRANCH MAP ============================
// 来源: get_code_snippet ProcessTableModel::data(const QModelIndex&, int) @
//       deepin-system-monitor-main/model/process_table_model.cpp:240-401
// B1  : !index.isValid()                                  -> return QVariant()
// B2  : row<0 || row>=m_procIdList.size()                 -> return QVariant()
// B3  : !m_processList[row].isValid()                     -> return QVariant()
// B4a : Display/AccessibleText, Name, state=='Z'          -> "(No response) %name"
// B4b : Display/AccessibleText, Name, state=='T'          -> "(Suspend) %name"
// B4c : Display/AccessibleText, Name, else                -> displayName()
// B4d : Display/AccessibleText, CPU                       -> "%1%"
// B4e : Display/AccessibleText, User                      -> userName()
// B4f : Display/AccessibleText, Mem/ShareMem/VTRMem       -> formatUnit_memory_disk(proc.value, KB)
// B4g : Display/AccessibleText, Upload/Download           -> formatUnit_net(8*proc.value, B, 1, true)
// B4h : Display/AccessibleText, DiskRead/DiskWrite        -> formatUnit_memory_disk(proc.value, B, 1, true)
// B4i : Display/AccessibleText, PID                       -> QString::number(pid)
// B4j : Display/AccessibleText, Nice                      -> QString::number(priority)
// B4k : Display/AccessibleText, Priority                  -> getPriorityName(priority)
// B4l : Display/AccessibleText, default column            -> QVariant()
// B5  : DecorationRole, Name column                       -> proc.icon()（其他列/其他 role -> QVariant()）
// B6  : UserRole, Name                                    -> displayName(); Mem/Share/VTR -> qulonglong;
//         CPU -> qreal; Upload -> sentBps; Download -> recvBps; PID -> pid; Nice -> priority;
//         DiskRead/Write -> readBps/writeBps; default -> QVariant()
// B7  : UserRole+1, Upload                                -> sentBps(); Download -> recvBps(); default -> QVariant()
// B8  : TextAlignmentRole                                 -> AlignLeft|AlignVCenter
// B9  : UserRole+2, Name, state=='Z'||'T'                 -> DPalette::TextWarning else QVariant()
// B10 : UserRole+3                                        -> appType()
// B11 : UserRole+4, Name, cmdlineString() 非空            -> cmdlineString() else displayName()
// B12 : 其他 role                                         -> QVariant()
// 用例映射:
//   Data_InvalidIndex_ReturnsEmptyVariant                    -> B1
//   Data_RowOutOfBounds_ReturnsEmptyVariant                  -> B2
//   Data_InvalidProcess_ReturnsEmptyVariant                  -> B3
//   Data_DisplayRole_ReturnsExpectedText (TEST_P)            -> B4a~B4l (列矩阵)
//   Data_DisplayRole_ZombieState_PrefixesNoResponse (TEST_P) -> B4a
//   Data_DisplayRole_SuspendedState_PrefixesSuspend (TEST_P) -> B4b
//   Data_PriorityName_MapsKnownValues (TEST_P)               -> B4k (getPriorityName 矩阵)
//   Data_DecorationRole_NameColumn_ReturnsProcessIcon        -> B5
//   Data_DecorationRole_OtherColumn_ReturnsEmptyVariant      -> B5 else
//   Data_UserRole_ReturnsRawValue (TEST_P)                   -> B6
//   Data_UserRolePlus1_ReturnsSentRecvBps (TEST_P)           -> B7
//   Data_TextAlignmentRole_ReturnsLeftAndVCenter             -> B8
//   Data_UserRolePlus2_ZombieState_ReturnsTextWarning        -> B9 true
//   Data_UserRolePlus2_RunningState_ReturnsEmptyVariant      -> B9 false
//   Data_UserRolePlus3_ReturnsAppType                        -> B10
//   Data_UserRolePlus4_NonEmptyCmdline_ReturnsCmdline        -> B11 true
//   Data_UserRolePlus4_EmptyCmdline_FallsBackToName          -> B11 false
//   Data_ToolTipRole_ReturnsEmptyVariant                     -> B12
//   HeaderData_DisplayRole_ReturnsTranslatedHeader (TEST_P)  -> headerData H1 (13 列)
//   HeaderData_OutOfRangeSection_FallsBackToBaseImplementation -> H1 default
//   HeaderData_AccessibleTextRole_MatchesDisplayText         -> H1 AccessibleText
//   HeaderData_TextAlignmentRole_ReturnsLeftAndVCenter       -> H2
//   HeaderData_InitialSortOrderRole_ReturnsDescending        -> H3
//   HeaderData_OtherRole_FallsBackToBaseImplementation       -> H4
//   UpdateProcessList_UserModeNull_SchedulesDelaySlot        -> updateProcessList B1
//   UpdateProcessList_UserModeSet_SchedulesUserSpecifiedSlot -> updateProcessList B2
//   UpdateProcessListDelay_InsertsNewProcessRow              -> insert 分支
//   UpdateProcessListDelay_UpdatesExistingProcessRow         -> update 分支 + dataChanged
//   UpdateProcessListDelay_SkipsInvalidProcess               -> invalid 分支
//   UpdateProcessListDelay_RemovesStaleRows                  -> remove 分支
//   UpdateProcessListDelay_EmitsModelUpdated                 -> modelUpdated 信号
//   UpdateProcessListWithUserSpecified_KeepsMatchingUser     -> user 匹配分支
//   UpdateProcessListWithUserSpecified_SkipsInvalidProcess   -> invalid 分支
//   UpdateProcessListWithUserSpecified_ClearsStaleRows       -> remove 分支
//   UpdateProcessState_ExistingPid_UpdatesState              -> indexOf 命中分支
//   UpdateProcessState_UnknownPid_KeepsState                 -> indexOf 未命中分支
//   UpdateProcessPriority_ExistingPid_UpdatesPriority        -> indexOf 命中分支
//   UpdateProcessPriority_UnknownPid_KeepsPriority           -> indexOf 未命中分支
//   RemoveProcess_ExistingPid_RemovesRows                    -> 命中分支
//   RemoveProcess_UnknownPid_KeepsRows                       -> 未命中分支
//   RowCount_RootParent_ReturnsProcessCount                  -> rowCount 根分支
//   RowCount_ValidParent_ReturnsZero                         -> rowCount 非根分支
//   ColumnCount_RootParent_ReturnsFixedCount                 -> columnCount 根分支
//   ColumnCount_ValidParent_ReturnsZero                      -> columnCount 非根分支
//   GetProcessState_ExistingPid_ReturnsCachedState           -> getProcessState
//   GetProcessState_UnknownPid_ReturnsZero                   -> getProcessState else
//   GetProcess_ExistingPid_ReturnsProcessCopy                -> getProcess
//   GetProcess_UnknownPid_ReturnsDefaultInstance             -> getProcess else
//   GetProcessPriority_ExistingPid_ReturnsEnumMapping        -> getProcessPriority
//   GetProcessPriority_UnknownPid_ReturnsInvalidPriority     -> getProcessPriority else
//   GetProcessPriorityValue_ExistingPid_ReturnsRawPriority   -> getProcessPriorityValue
//   GetProcessPriorityValue_UnknownPid_ReturnsNormalPriority -> getProcessPriorityValue else
//   GetTotalCPUUsage_SumsProcessCpu                          -> getTotalCPUUsage
//   GetTotalCPUUsage_EmptyModel_ReturnsZero                  -> getTotalCPUUsage 边界
//   GetTotalMemoryUsage_SumsProcessMemory                    -> getTotalMemoryUsage
//   GetTotalSharedMemoryUsage_SumsProcessShm                 -> getTotalSharedMemoryUsage
//   GetTotalVirtualMemoryUsage_SumsProcessVmsize             -> getTotalVirtualMemoryUsage
//   GetTotalUpload_SumsSentBps                               -> getTotalUpload
//   GetTotalDownload_SumsRecvBps                             -> getTotalDownload
//   GetTotalDiskRead_SumsReadBps                             -> getTotalDiskRead
//   GetTotalDiskWrite_SumsWriteBps                           -> getTotalDiskWrite
//   SetUserModeName_SameName_DoesNotEmitModelUpdated         -> setUserModeName else
//   SetUserModeName_DifferentName_EmitsModelUpdated          -> setUserModeName true
// ====================================================================

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include <QApplication>
#include <QSignalSpy>
#include <QTimer>
#include <QVariant>
#include <QIcon>
#include <QHash>
#include <DPalette>
#include <DApplication>

#include "model/process_table_model.h"
#include "process/process.h"
#include "process/process_set.h"
#include "process/private/process_p.h"
#include "common/common.h"
#include "common/sample.h"

#include "stub.h"

DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

namespace {

/***************************************STUB begin*********************************************/
static QList<pid_t> g_stubPidList {};
static QMap<pid_t, Process> g_stubProcMap {};
static QString g_singleShotSlot {};

// getProcessState/getProcess/getProcessPriority* 走 ProcessDB::instance()->processSet()->getProcessById(pid)，
// 按旧用例约定打桩 ProcessSet::getProcessById，用映射表喂数据（stub.h 仅有 set()）。
static QHash<pid_t, Process> g_getByIdMap;
static Process stub_getProcessById_map(ProcessSet *set, pid_t pid)
{
    Q_UNUSED(set);
    return g_getByIdMap.value(pid);
}

static QList<pid_t> stub_getPIDList()
{
    return g_stubPidList;
}

static Process stub_getProcessById(ProcessSet *self, pid_t pid)
{
    Q_UNUSED(self);
    return g_stubProcMap.value(pid);
}

static void stub_singleShot(int msec, const QObject *receiver, const char *member)
{
    Q_UNUSED(msec);
    Q_UNUSED(receiver);
    g_singleShotSlot = QString::fromLatin1(member);
}
/***************************************STUB end***********************************************/

constexpr pid_t kPidA = 4321;
constexpr pid_t kPidB = 4322;

Process makeValidProcess(pid_t pid, const QString &name = "utproc", const QString &user = "utuser")
{
    Process p(pid);
    p.d->valid = true;
    p.d->state = 'R';
    p.d->shm = 512;
    p.d->vmsize = 8192;
    p.setCpu(12.5);
    p.setMemory(4096);          // rss = memory + shm
    p.setPriority(0);
    p.setName(name);
    p.setDisplayName(name);
    p.setUserName(user);
    p.setAppType(0);
    p.setNetIoBps(100.0, 200.0);   // recvBps = 100, sentBps = 200
    struct IOPS diskIo { 1024.0, 2048.0 };
    p.d->diskIOSpeedSample->addSample(new IOPSSampleFrame(diskIo));
    return p;
}

// 向受测模型直接注入一行数据（绕开 ProcessDB 单例），保持所有用例的 Arrange 原子性
void injectRow(ProcessTableModel *model, const Process &proc)
{
    model->m_procIdList << proc.pid();
    model->m_processList << proc;
}

int rowIndexByPid(const ProcessTableModel *model, pid_t pid)
{
    return model->m_procIdList.indexOf(pid);
}

// DisplayRole 列矩阵参数
struct DisplayCase {
    int column;
    QString expected;
};

// UserRole 列矩阵参数
struct UserRoleCase {
    int column;
    QVariant expected;
    bool isULongLong;   // true 时按 toULongLong 比较（QVariant 类型严格匹配）
};

// headerData 列矩阵参数（key 直接引用 process_table_model.h 公共 constexpr）
struct HeaderCase {
    int column;
    const char *headerKey;
};

// 优先级名映射参数（getPriorityName 矩阵，含边界 19/20）
struct PriorityCase {
    int priority;
    const char *nameKey;
};

}   // namespace

// ============================== 构造与规模 ==============================

class ProcessTableModelTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_tester.reset(new ProcessTableModel());
        m_modelUpdatedSpy.reset(new QSignalSpy(m_tester.data(), &ProcessTableModel::modelUpdated));
    }
    void TearDown() override
    {
        m_modelUpdatedSpy.reset();
        m_tester.reset();
    }
    QScopedPointer<ProcessTableModel> m_tester;
    QScopedPointer<QSignalSpy> m_modelUpdatedSpy;
};

TEST_F(ProcessTableModelTest, Ctor_InitialState_HasZeroRowsAndFixedColumns)
{
    // Assert: 空模型行为
    EXPECT_EQ(m_tester->rowCount(), 0);
    EXPECT_EQ(m_tester->columnCount(), ProcessTableModel::kProcessColumnCount);
    EXPECT_TRUE(m_tester->m_procIdList.isEmpty());
    EXPECT_TRUE(m_tester->m_processList.isEmpty());
}

// ============================== data(): B1~B3 无效输入 ==============================

TEST_F(ProcessTableModelTest, Data_InvalidIndex_ReturnsEmptyVariant)
{
    // branch B1: !index.isValid() -> QVariant()
    // Assert: 无效索引任意 role 都返回空
    EXPECT_EQ(m_tester->data(QModelIndex(), Qt::DisplayRole), QVariant());
    EXPECT_EQ(m_tester->data(QModelIndex(), Qt::UserRole + 4), QVariant());
}

TEST_F(ProcessTableModelTest, Data_RowOutOfBounds_ReturnsEmptyVariant)
{
    // Arrange: 注入一行有效数据
    injectRow(m_tester.data(), makeValidProcess(kPidA));

    // branch B2: row 越界（row=-1 / row=size）
    const QModelIndex below = m_tester->createIndex(-1, ProcessTableModel::kProcessNameColumn);
    const QModelIndex above = m_tester->createIndex(1, ProcessTableModel::kProcessNameColumn);

    // Assert
    EXPECT_EQ(m_tester->data(below, Qt::DisplayRole), QVariant());
    EXPECT_EQ(m_tester->data(above, Qt::DisplayRole), QVariant());
}

TEST_F(ProcessTableModelTest, Data_InvalidProcess_ReturnsEmptyVariant)
{
    // Arrange: 注入一行 valid=false 的进程
    Process p(kPidA);
    p.d->valid = false;
    p.setDisplayName("invalid");
    injectRow(m_tester.data(), p);

    // branch B3: !proc.isValid() -> QVariant()
    // Assert
    EXPECT_EQ(m_tester->data(m_tester->createIndex(0, ProcessTableModel::kProcessNameColumn), Qt::DisplayRole), QVariant());
    EXPECT_EQ(m_tester->data(m_tester->createIndex(0, ProcessTableModel::kProcessCPUColumn), Qt::DisplayRole), QVariant());
}

// ============================== data(): DisplayRole 列矩阵 (B4) ==============================

class ProcessTableModelDisplayParamTest : public ::testing::TestWithParam<DisplayCase> {
protected:
    void SetUp() override
    {
        m_model = new ProcessTableModel();
        injectRow(m_model, makeValidProcess(kPidA));
    }
    void TearDown() override { delete m_model; }
    ProcessTableModel *m_model = nullptr;
};

TEST_P(ProcessTableModelDisplayParamTest, Data_DisplayRole_ReturnsExpectedText)
{
    const DisplayCase &c = GetParam();
    // Act
    const QVariant v = m_model->data(m_model->createIndex(0, c.column), Qt::DisplayRole);
    // Assert: 精确字符串值
    EXPECT_EQ(v.toString(), c.expected);
}

INSTANTIATE_TEST_SUITE_P(DisplayColumnMatrix, ProcessTableModelDisplayParamTest,
    ::testing::Values(
        DisplayCase{ProcessTableModel::kProcessNameColumn, "utproc"},
        DisplayCase{ProcessTableModel::kProcessCPUColumn, QString("%1%").arg(12.5, 0, 'f', 1)},
        DisplayCase{ProcessTableModel::kProcessUserColumn, "utuser"},
        DisplayCase{ProcessTableModel::kProcessMemoryColumn,
                    common::format::formatUnit_memory_disk(QVariant::fromValue<qulonglong>(4096), common::format::KB)},
        DisplayCase{ProcessTableModel::kProcessShareMemoryColumn,
                    common::format::formatUnit_memory_disk(QVariant::fromValue<qulonglong>(512), common::format::KB)},
        DisplayCase{ProcessTableModel::kProcessVTRMemoryColumn,
                    common::format::formatUnit_memory_disk(QVariant::fromValue<qulonglong>(8192), common::format::KB)},
        DisplayCase{ProcessTableModel::kProcessUploadColumn,
                    common::format::formatUnit_net(QVariant::fromValue<qreal>(8 * 200.0), common::format::B, 1, true)},
        DisplayCase{ProcessTableModel::kProcessDownloadColumn,
                    common::format::formatUnit_net(QVariant::fromValue<qreal>(8 * 100.0), common::format::B, 1, true)},
        DisplayCase{ProcessTableModel::kProcessDiskReadColumn,
                    common::format::formatUnit_memory_disk(QVariant::fromValue<qreal>(1024.0), common::format::B, 1, true)},
        DisplayCase{ProcessTableModel::kProcessDiskWriteColumn,
                    common::format::formatUnit_memory_disk(QVariant::fromValue<qreal>(2048.0), common::format::B, 1, true)},
        DisplayCase{ProcessTableModel::kProcessPIDColumn, "4321"},
        DisplayCase{ProcessTableModel::kProcessNiceColumn, "0"},
        DisplayCase{ProcessTableModel::kProcessPriorityColumn,
                    QApplication::translate("Process.Priority", "Normal")},
        // 等价类: 枚举之外的列 -> 空串（B4l）
        DisplayCase{ProcessTableModel::kProcessColumnCount, QString()}
    ));

// zombie / suspend 状态前缀（B4a / B4b）用同一断言形态 -> TEST_P
class ProcessTableModelStatePrefixParamTest : public ProcessTableModelDisplayParamTest {};

TEST_P(ProcessTableModelStatePrefixParamTest, Data_DisplayRole_PrefixesSpecialState)
{
    const DisplayCase &c = GetParam();
    // Assert 前置: 基线状态 'R' 无前缀
    ASSERT_EQ(m_model->data(m_model->createIndex(0, ProcessTableModel::kProcessNameColumn), Qt::DisplayRole).toString(), "utproc");

    // Arrange: 设置特殊状态
    m_model->m_processList[0].setState(c.column == 1 ? 'Z' : 'T');

    // Act + Assert
    EXPECT_EQ(m_model->data(m_model->createIndex(0, ProcessTableModel::kProcessNameColumn), Qt::DisplayRole).toString(), c.expected);
}

INSTANTIATE_TEST_SUITE_P(StatePrefixMatrix, ProcessTableModelStatePrefixParamTest,
    ::testing::Values(
        DisplayCase{1, QString("(%1) %2").arg(QApplication::translate("Process.Table", "No response"), "utproc")},
        DisplayCase{2, QString("(%1) %2").arg(QApplication::translate("Process.Table", "Suspend"), "utproc")}
    ));

// ============================== data(): 优先级名映射 (B4k) ==============================

class ProcessTableModelPriorityParamTest : public ::testing::TestWithParam<PriorityCase> {
protected:
    void SetUp() override
    {
        m_model = new ProcessTableModel();
        Process p = makeValidProcess(kPidA);
        p.setPriority(GetParam().priority);
        injectRow(m_model, p);
    }
    void TearDown() override { delete m_model; }
    ProcessTableModel *m_model = nullptr;
};

TEST_P(ProcessTableModelPriorityParamTest, Data_PriorityName_MapsKnownValues)
{
    // Act + Assert: getPriorityName 映射矩阵（含区间边界 19 / 20）
    EXPECT_EQ(m_model->data(m_model->createIndex(0, ProcessTableModel::kProcessPriorityColumn), Qt::DisplayRole).toString(),
              QApplication::translate("Process.Priority", GetParam().nameKey));
    // 状态保持: Nice 列同步展示原始值
    EXPECT_EQ(m_model->data(m_model->createIndex(0, ProcessTableModel::kProcessNiceColumn), Qt::DisplayRole).toString(),
              QString::number(GetParam().priority));
}

INSTANTIATE_TEST_SUITE_P(PriorityNameMatrix, ProcessTableModelPriorityParamTest,
    ::testing::Values(
        PriorityCase{-20, "Very high"},
        PriorityCase{-10, "High"},
        PriorityCase{0, "Normal"},
        PriorityCase{10, "Low"},
        PriorityCase{19, "Very low"},
        PriorityCase{5, "Custom"},      // 区间 [-20,19] 内的非标准值
        PriorityCase{20, "Invalid"},    // 边界外: == kCustomPriority 但映射为 Invalid
        PriorityCase{100, "Invalid"}
    ));

// ============================== data(): DecorationRole (B5) ==============================

TEST_F(ProcessTableModelTest, Data_DecorationRole_NameColumn_ReturnsProcessIcon)
{
    // Arrange: 未刷新图标缓存的进程，proc.icon() 为 null 图标
    // （refreashProcessIcon 依赖 ProcessIconCache/AM/DBus 等环境，不在单测中触发）
    injectRow(m_tester.data(), makeValidProcess(kPidA));

    // branch B5: Decoration 仅 Name 列返回 proc.icon()
    // Act
    const QVariant v = m_tester->data(m_tester->createIndex(0, ProcessTableModel::kProcessNameColumn), Qt::DecorationRole);

    // Assert: QIcon 变体（当前为 null 图标，不崩溃）
    EXPECT_TRUE(v.canConvert<QIcon>());
    EXPECT_TRUE(v.value<QIcon>().isNull());
}

TEST_F(ProcessTableModelTest, Data_DecorationRole_OtherColumn_ReturnsEmptyVariant)
{
    // Arrange
    injectRow(m_tester.data(), makeValidProcess(kPidA));

    // Act + Assert: B5 else 分支
    EXPECT_EQ(m_tester->data(m_tester->createIndex(0, ProcessTableModel::kProcessCPUColumn), Qt::DecorationRole), QVariant());
    EXPECT_EQ(m_tester->data(m_tester->createIndex(0, ProcessTableModel::kProcessPIDColumn), Qt::DecorationRole), QVariant());
}

// ============================== data(): UserRole (B6) ==============================

class ProcessTableModelUserRoleParamTest : public ::testing::TestWithParam<UserRoleCase> {
protected:
    void SetUp() override
    {
        m_model = new ProcessTableModel();
        injectRow(m_model, makeValidProcess(kPidA));
    }
    void TearDown() override { delete m_model; }
    ProcessTableModel *m_model = nullptr;
};

TEST_P(ProcessTableModelUserRoleParamTest, Data_UserRole_ReturnsRawValue)
{
    const UserRoleCase &c = GetParam();
    // Act
    const QVariant v = m_model->data(m_model->createIndex(0, c.column), Qt::UserRole);
    // Assert: 原始值精确比较
    if (c.isULongLong)
        EXPECT_EQ(v.toULongLong(), c.expected.toULongLong());
    else
        EXPECT_EQ(v, c.expected);
}

INSTANTIATE_TEST_SUITE_P(UserRoleColumnMatrix, ProcessTableModelUserRoleParamTest,
    ::testing::Values(
        UserRoleCase{ProcessTableModel::kProcessNameColumn, "utproc", false},
        UserRoleCase{ProcessTableModel::kProcessCPUColumn, 12.5, false},
        UserRoleCase{ProcessTableModel::kProcessMemoryColumn, QVariant::fromValue<qulonglong>(4096), true},
        UserRoleCase{ProcessTableModel::kProcessShareMemoryColumn, QVariant::fromValue<qulonglong>(512), true},
        UserRoleCase{ProcessTableModel::kProcessVTRMemoryColumn, QVariant::fromValue<qulonglong>(8192), true},
        UserRoleCase{ProcessTableModel::kProcessUploadColumn, 200.0, false},
        UserRoleCase{ProcessTableModel::kProcessDownloadColumn, 100.0, false},
        UserRoleCase{ProcessTableModel::kProcessDiskReadColumn, 1024.0, false},
        UserRoleCase{ProcessTableModel::kProcessDiskWriteColumn, 2048.0, false},
        UserRoleCase{ProcessTableModel::kProcessPIDColumn, 4321, false},
        UserRoleCase{ProcessTableModel::kProcessNiceColumn, 0, false},
        // 等价类: 枚举之外的列 -> 空（B6 default）
        UserRoleCase{ProcessTableModel::kProcessColumnCount, QVariant(), false}
    ));

// ============================== data(): UserRole+1 (B7) ==============================

class ProcessTableModelUserRolePlus1ParamTest : public ::testing::TestWithParam<UserRoleCase> {
protected:
    void SetUp() override
    {
        m_model = new ProcessTableModel();
        injectRow(m_model, makeValidProcess(kPidA));
    }
    void TearDown() override { delete m_model; }
    ProcessTableModel *m_model = nullptr;
};

TEST_P(ProcessTableModelUserRolePlus1ParamTest, Data_UserRolePlus1_ReturnsSentRecvBps)
{
    const UserRoleCase &c = GetParam();
    // Act
    const QVariant v = m_model->data(m_model->createIndex(0, c.column), Qt::UserRole + 1);
    // Assert
    EXPECT_EQ(v, c.expected);
}

INSTANTIATE_TEST_SUITE_P(UserRolePlus1Matrix, ProcessTableModelUserRolePlus1ParamTest,
    ::testing::Values(
        // upload 列返回发送方向 sentBps，download 列返回接收方向 recvBps（与源码一致）
        UserRoleCase{ProcessTableModel::kProcessUploadColumn, 200.0, false},
        UserRoleCase{ProcessTableModel::kProcessDownloadColumn, 100.0, false},
        UserRoleCase{ProcessTableModel::kProcessNameColumn, QVariant(), false}
    ));

// ============================== data(): 其余 role (B8~B12) ==============================

TEST_F(ProcessTableModelTest, Data_TextAlignmentRole_ReturnsLeftAndVCenter)
{
    // Arrange
    injectRow(m_tester.data(), makeValidProcess(kPidA));

    // branch B8
    // Act + Assert
    const QVariant v = m_tester->data(m_tester->createIndex(0, ProcessTableModel::kProcessMemoryColumn), Qt::TextAlignmentRole);
    EXPECT_EQ(v.toInt(), int(Qt::AlignLeft | Qt::AlignVCenter));
}

TEST_F(ProcessTableModelTest, Data_UserRolePlus2_ZombieState_ReturnsTextWarning)
{
    // Arrange
    Process p = makeValidProcess(kPidA);
    p.setState('Z');
    injectRow(m_tester.data(), p);

    // branch B9 true
    // Act + Assert
    const QVariant v = m_tester->data(m_tester->createIndex(0, ProcessTableModel::kProcessNameColumn), Qt::UserRole + 2);
    EXPECT_EQ(v.toInt(), int(DPalette::TextWarning));
}

TEST_F(ProcessTableModelTest, Data_UserRolePlus2_RunningState_ReturnsEmptyVariant)
{
    // Arrange
    injectRow(m_tester.data(), makeValidProcess(kPidA));   // state 'R'

    // branch B9 false
    // Act + Assert
    EXPECT_EQ(m_tester->data(m_tester->createIndex(0, ProcessTableModel::kProcessNameColumn), Qt::UserRole + 2), QVariant());
}

TEST_F(ProcessTableModelTest, Data_UserRolePlus3_ReturnsAppType)
{
    // Arrange
    Process p = makeValidProcess(kPidA);
    p.setAppType(1);
    injectRow(m_tester.data(), p);

    // branch B10
    // Act + Assert
    EXPECT_EQ(m_tester->data(m_tester->createIndex(0, ProcessTableModel::kProcessPIDColumn), Qt::UserRole + 3).toInt(), 1);
}

TEST_F(ProcessTableModelTest, Data_UserRolePlus4_NonEmptyCmdline_ReturnsCmdline)
{
    // Arrange
    Process p = makeValidProcess(kPidA);
    p.d->cmdline = QByteArrayList { QByteArray("utapp -v") };
    injectRow(m_tester.data(), p);

    // branch B11 true
    // Act + Assert
    EXPECT_EQ(m_tester->data(m_tester->createIndex(0, ProcessTableModel::kProcessNameColumn), Qt::UserRole + 4).toString(), "utapp -v");
}

TEST_F(ProcessTableModelTest, Data_UserRolePlus4_EmptyCmdline_FallsBackToName)
{
    // Arrange: cmdline 为空（默认）
    injectRow(m_tester.data(), makeValidProcess(kPidA));

    // branch B11 false
    // Act + Assert
    EXPECT_EQ(m_tester->data(m_tester->createIndex(0, ProcessTableModel::kProcessNameColumn), Qt::UserRole + 4).toString(), "utproc");
}

TEST_F(ProcessTableModelTest, Data_ToolTipRole_ReturnsEmptyVariant)
{
    // Arrange
    injectRow(m_tester.data(), makeValidProcess(kPidA));

    // branch B12: 其他 role
    // Act + Assert
    EXPECT_EQ(m_tester->data(m_tester->createIndex(0, ProcessTableModel::kProcessNameColumn), Qt::ToolTipRole), QVariant());
    EXPECT_EQ(m_tester->data(m_tester->createIndex(0, ProcessTableModel::kProcessNameColumn), Qt::EditRole), QVariant());
}

// ============================== headerData (H1~H4) ==============================

class ProcessTableModelHeaderParamTest : public ::testing::TestWithParam<HeaderCase> {
protected:
    void SetUp() override { m_model = new ProcessTableModel(); }
    void TearDown() override { delete m_model; }
    ProcessTableModel *m_model = nullptr;
};

TEST_P(ProcessTableModelHeaderParamTest, HeaderData_DisplayRole_ReturnsTranslatedHeader)
{
    // branch H1: DisplayRole 按 column 逐个映射
    // Act
    const QVariant v = m_model->headerData(GetParam().column, Qt::Horizontal, Qt::DisplayRole);
    // Assert: 与源码同一 translate 调用 -> 精确字符串
    EXPECT_EQ(v.toString(), QApplication::translate("Process.Table.Header", GetParam().headerKey));
}

INSTANTIATE_TEST_SUITE_P(HeaderColumnMatrix, ProcessTableModelHeaderParamTest,
    ::testing::Values(
        HeaderCase{ProcessTableModel::kProcessNameColumn, kProcessName},
        HeaderCase{ProcessTableModel::kProcessCPUColumn, kProcessCPU},
        HeaderCase{ProcessTableModel::kProcessUserColumn, kProcessUser},
        HeaderCase{ProcessTableModel::kProcessMemoryColumn, kProcessMemory},
        HeaderCase{ProcessTableModel::kProcessShareMemoryColumn, kProcessShareMemory},
        HeaderCase{ProcessTableModel::kProcessVTRMemoryColumn, kProcessVtrMemory},
        HeaderCase{ProcessTableModel::kProcessUploadColumn, kProcessUpload},
        HeaderCase{ProcessTableModel::kProcessDownloadColumn, kProcessDownload},
        HeaderCase{ProcessTableModel::kProcessDiskReadColumn, kProcessDiskRead},
        HeaderCase{ProcessTableModel::kProcessDiskWriteColumn, kProcessDiskWrite},
        HeaderCase{ProcessTableModel::kProcessPIDColumn, kProcessPID},
        HeaderCase{ProcessTableModel::kProcessNiceColumn, kProcessNice},
        HeaderCase{ProcessTableModel::kProcessPriorityColumn, kProcessPriority}
    ));

TEST_F(ProcessTableModelTest, HeaderData_OutOfRangeSection_FallsBackToBaseImplementation)
{
    // branch H1 default: section 超出列范围走基类实现。
    // Qt 6.8 起 QAbstractItemModel::headerData 对 DisplayRole 返回默认文本 section+1。
    // Act + Assert
    EXPECT_EQ(m_tester->headerData(ProcessTableModel::kProcessColumnCount, Qt::Horizontal, Qt::DisplayRole),
              QVariant(ProcessTableModel::kProcessColumnCount + 1));
    EXPECT_EQ(m_tester->headerData(-1, Qt::Horizontal, Qt::DisplayRole), QVariant(0));
    // 非 DisplayRole 无默认文本
    EXPECT_EQ(m_tester->headerData(ProcessTableModel::kProcessColumnCount, Qt::Horizontal, Qt::ToolTipRole), QVariant());
}

TEST_F(ProcessTableModelTest, HeaderData_AccessibleTextRole_MatchesDisplayText)
{
    // branch H1: AccessibleTextRole 与 DisplayRole 同分支
    // Act + Assert
    const QVariant acc = m_tester->headerData(ProcessTableModel::kProcessNameColumn, Qt::Horizontal, Qt::AccessibleTextRole);
    const QVariant disp = m_tester->headerData(ProcessTableModel::kProcessNameColumn, Qt::Horizontal, Qt::DisplayRole);
    EXPECT_EQ(acc, disp);
}

TEST_F(ProcessTableModelTest, HeaderData_TextAlignmentRole_ReturnsLeftAndVCenter)
{
    // branch H2
    // Act + Assert
    const QVariant v = m_tester->headerData(ProcessTableModel::kProcessCPUColumn, Qt::Horizontal, Qt::TextAlignmentRole);
    EXPECT_EQ(v.toInt(), int(Qt::AlignLeft | Qt::AlignVCenter));
}

TEST_F(ProcessTableModelTest, HeaderData_InitialSortOrderRole_ReturnsDescending)
{
    // branch H3: QVariant::fromValue 包装的枚举
    // Act + Assert
    const QVariant v = m_tester->headerData(ProcessTableModel::kProcessNameColumn, Qt::Horizontal, Qt::InitialSortOrderRole);
    EXPECT_EQ(v, QVariant::fromValue(Qt::DescendingOrder));
}

TEST_F(ProcessTableModelTest, HeaderData_OtherRole_FallsBackToBaseImplementation)
{
    // branch H4: 其他 role 走基类实现
    // Act + Assert
    EXPECT_EQ(m_tester->headerData(ProcessTableModel::kProcessNameColumn, Qt::Horizontal, Qt::ToolTipRole), QVariant());
    // 缺陷 #4 已修复: headerData 现判断 orientation，Vertical 方向返回空 QVariant
    EXPECT_EQ(m_tester->headerData(ProcessTableModel::kProcessNameColumn, Qt::Vertical, Qt::DisplayRole),
              QVariant());
}

// ============================== rowCount / columnCount ==============================

TEST_F(ProcessTableModelTest, RowCount_RootParent_ReturnsProcessCount)
{
    // Arrange: 注入两行
    injectRow(m_tester.data(), makeValidProcess(kPidA));
    injectRow(m_tester.data(), makeValidProcess(kPidB));

    // Act + Assert
    EXPECT_EQ(m_tester->rowCount(), 2);
    EXPECT_EQ(m_tester->rowCount(QModelIndex()), 2);
}

TEST_F(ProcessTableModelTest, RowCount_ValidParent_ReturnsZero)
{
    // Arrange
    injectRow(m_tester.data(), makeValidProcess(kPidA));
    const QModelIndex parent = m_tester->createIndex(0, 0);

    // 缺陷 #5 已修复: rowCount 现判断 parent，有效 parent 返回 0
    EXPECT_EQ(m_tester->rowCount(parent), 0);
    EXPECT_EQ(m_tester->rowCount(), 1);
}

TEST_F(ProcessTableModelTest, ColumnCount_RootParent_ReturnsFixedCount)
{
    // Act + Assert
    EXPECT_EQ(m_tester->columnCount(QModelIndex()), ProcessTableModel::kProcessColumnCount);
    EXPECT_EQ(m_tester->columnCount(), ProcessTableModel::kProcessColumnCount);
}

TEST_F(ProcessTableModelTest, ColumnCount_ValidParent_ReturnsZero)
{
    // Arrange
    injectRow(m_tester.data(), makeValidProcess(kPidA));
    const QModelIndex parent = m_tester->createIndex(0, 0);

    // 缺陷 #5 已修复: columnCount 现判断 parent，有效 parent 返回 0
    EXPECT_EQ(m_tester->columnCount(parent), 0);
}

// ============================== updateProcessList (定时器路由) ==============================

TEST_F(ProcessTableModelTest, UpdateProcessList_UserModeNull_SchedulesDelaySlot)
{
    // Arrange: userModeName 为空（构造默认）
    ASSERT_TRUE(m_tester->m_userModeName.isNull());
    Stub stub;
    stub.set(static_cast<void (*)(int, const QObject *, const char *)>(&QTimer::singleShot), stub_singleShot);

    // Act
    m_tester->updateProcessList();

    // Assert: 路由到 updateProcessListDelay 定时器
    EXPECT_EQ(g_singleShotSlot, QString(SLOT(updateProcessListDelay())));
}

TEST_F(ProcessTableModelTest, UpdateProcessList_UserModeSet_SchedulesUserSpecifiedSlot)
{
    // Arrange: 先设置用户过滤名
    m_tester->setUserModeName("utuser");
    Stub stub;
    stub.set(static_cast<void (*)(int, const QObject *, const char *)>(&QTimer::singleShot), stub_singleShot);

    // Act
    m_tester->updateProcessList();

    // Assert: 路由到 updateProcessListWithUserSpecified 定时器
    EXPECT_EQ(g_singleShotSlot, QString(SLOT(updateProcessListWithUserSpecified())));
}

// ============================== updateProcessListDelay ==============================

TEST_F(ProcessTableModelTest, UpdateProcessListDelay_InsertsNewProcessRow)
{
    // Arrange: 打桩 ProcessSet，返回 1 个有效进程
    g_stubPidList = QList<pid_t> { kPidA };
    g_stubProcMap = QMap<pid_t, Process> { { kPidA, makeValidProcess(kPidA) } };
    Stub stub;
    stub.set(ADDR(ProcessSet, getPIDList), stub_getPIDList);
    stub.set(ADDR(ProcessSet, getProcessById), stub_getProcessById);

    // Act
    m_tester->updateProcessListDelay();

    // Assert: 新进程插入 + 名称精确
    EXPECT_EQ(m_tester->rowCount(), 1);
    EXPECT_EQ(m_tester->data(m_tester->createIndex(0, ProcessTableModel::kProcessNameColumn), Qt::DisplayRole).toString(), "utproc");
    EXPECT_EQ(rowIndexByPid(m_tester.data(), kPidA), 0);
}

TEST_F(ProcessTableModelTest, UpdateProcessListDelay_UpdatesExistingProcessRow)
{
    // Arrange: 先插入旧名称进程，桩数据同名 pid 携带新名称
    injectRow(m_tester.data(), makeValidProcess(kPidA, "oldname"));
    g_stubPidList = QList<pid_t> { kPidA };
    g_stubProcMap = QMap<pid_t, Process> { { kPidA, makeValidProcess(kPidA, "newname") } };
    Stub stub;
    stub.set(ADDR(ProcessSet, getPIDList), stub_getPIDList);
    stub.set(ADDR(ProcessSet, getProcessById), stub_getProcessById);
    QSignalSpy dataSpy(m_tester.data(), &ProcessTableModel::dataChanged);

    // Act
    m_tester->updateProcessListDelay();

    // Assert: 原地更新，不重复插行
    EXPECT_EQ(m_tester->rowCount(), 1);
    EXPECT_EQ(m_tester->data(m_tester->createIndex(0, ProcessTableModel::kProcessNameColumn), Qt::DisplayRole).toString(), "newname");
    EXPECT_EQ(dataSpy.count(), 1);
}

TEST_F(ProcessTableModelTest, UpdateProcessListDelay_SkipsInvalidProcess)
{
    // Arrange: 桩返回 valid=false 的进程
    g_stubPidList = QList<pid_t> { kPidA };
    Process invalid(kPidA);
    invalid.d->valid = false;
    g_stubProcMap = QMap<pid_t, Process> { { kPidA, invalid } };
    Stub stub;
    stub.set(ADDR(ProcessSet, getPIDList), stub_getPIDList);
    stub.set(ADDR(ProcessSet, getProcessById), stub_getProcessById);

    // Act
    m_tester->updateProcessListDelay();

    // Assert: 无效进程不进模型
    EXPECT_EQ(m_tester->rowCount(), 0);
    EXPECT_TRUE(m_tester->m_procIdList.isEmpty());
}

TEST_F(ProcessTableModelTest, UpdateProcessListDelay_RemovesStaleRows)
{
    // Arrange: 模型内已有两行，桩数据只保留一行
    injectRow(m_tester.data(), makeValidProcess(kPidA));
    injectRow(m_tester.data(), makeValidProcess(kPidB));
    g_stubPidList = QList<pid_t> { kPidB };
    g_stubProcMap = QMap<pid_t, Process> { { kPidB, makeValidProcess(kPidB) } };
    Stub stub;
    stub.set(ADDR(ProcessSet, getPIDList), stub_getPIDList);
    stub.set(ADDR(ProcessSet, getProcessById), stub_getProcessById);

    // Act
    m_tester->updateProcessListDelay();

    // Assert: 失联行被移除
    EXPECT_EQ(m_tester->rowCount(), 1);
    EXPECT_EQ(rowIndexByPid(m_tester.data(), kPidA), -1);
    EXPECT_EQ(rowIndexByPid(m_tester.data(), kPidB), 0);
}

TEST_F(ProcessTableModelTest, UpdateProcessListDelay_EmitsModelUpdated)
{
    // Arrange
    g_stubPidList = QList<pid_t> { kPidA };
    g_stubProcMap = QMap<pid_t, Process> { { kPidA, makeValidProcess(kPidA) } };
    Stub stub;
    stub.set(ADDR(ProcessSet, getPIDList), stub_getPIDList);
    stub.set(ADDR(ProcessSet, getProcessById), stub_getProcessById);
    QSignalSpy spy(m_tester.data(), &ProcessTableModel::modelUpdated);

    // Act
    m_tester->updateProcessListDelay();

    // Assert
    EXPECT_EQ(spy.count(), 1);
}

// ============================== updateProcessListWithUserSpecified ==============================

TEST_F(ProcessTableModelTest, UpdateProcessListWithUserSpecified_KeepsMatchingUser)
{
    // Arrange: 两个进程，仅一个属于目标用户
    Process userProc = makeValidProcess(kPidA, "procA", "utuser");
    Process rootProc = makeValidProcess(kPidB, "procB", "root");
    g_stubPidList = QList<pid_t> { kPidA, kPidB };
    g_stubProcMap = QMap<pid_t, Process> { { kPidA, userProc }, { kPidB, rootProc } };
    Stub stub;
    stub.set(ADDR(ProcessSet, getPIDList), stub_getPIDList);
    stub.set(ADDR(ProcessSet, getProcessById), stub_getProcessById);
    m_tester->setUserModeName("utuser");

    // Act
    m_tester->updateProcessListWithUserSpecified();

    // Assert: 仅目标用户行保留
    EXPECT_EQ(m_tester->rowCount(), 1);
    EXPECT_EQ(m_tester->data(m_tester->createIndex(0, ProcessTableModel::kProcessNameColumn), Qt::DisplayRole).toString(), "procA");
    EXPECT_EQ(rowIndexByPid(m_tester.data(), kPidB), -1);
}

TEST_F(ProcessTableModelTest, UpdateProcessListWithUserSpecified_SkipsInvalidProcess)
{
    // Arrange
    Process invalid(kPidA);
    invalid.d->valid = false;
    g_stubPidList = QList<pid_t> { kPidA };
    g_stubProcMap = QMap<pid_t, Process> { { kPidA, invalid } };
    Stub stub;
    stub.set(ADDR(ProcessSet, getPIDList), stub_getPIDList);
    stub.set(ADDR(ProcessSet, getProcessById), stub_getProcessById);
    m_tester->setUserModeName("utuser");

    // Act
    m_tester->updateProcessListWithUserSpecified();

    // Assert
    EXPECT_EQ(m_tester->rowCount(), 0);
}

TEST_F(ProcessTableModelTest, UpdateProcessListWithUserSpecified_ClearsStaleRows)
{
    // Arrange: 模型内已有行，桩数据为空
    injectRow(m_tester.data(), makeValidProcess(kPidA));
    g_stubPidList = QList<pid_t> {};
    Stub stub;
    stub.set(ADDR(ProcessSet, getPIDList), stub_getPIDList);
    stub.set(ADDR(ProcessSet, getProcessById), stub_getProcessById);
    m_tester->setUserModeName("utuser");

    // Act
    m_tester->updateProcessListWithUserSpecified();

    // Assert: 陈旧行清空
    EXPECT_EQ(m_tester->rowCount(), 0);
    EXPECT_TRUE(m_tester->m_procIdList.isEmpty());
}

// ============================== updateProcessState / updateProcessPriority ==============================

TEST_F(ProcessTableModelTest, UpdateProcessState_ExistingPid_UpdatesState)
{
    // Arrange
    injectRow(m_tester.data(), makeValidProcess(kPidA));
    QSignalSpy spy(m_tester.data(), &ProcessTableModel::dataChanged);

    // Act
    m_tester->updateProcessState(kPidA, 'S');

    // Assert: 缓存行状态更新 + 发出行变更信号
    EXPECT_EQ(m_tester->m_processList[0].state(), 'S');
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(ProcessTableModelTest, UpdateProcessState_UnknownPid_KeepsState)
{
    // Arrange
    injectRow(m_tester.data(), makeValidProcess(kPidA));   // state 'R'
    QSignalSpy spy(m_tester.data(), &ProcessTableModel::dataChanged);

    // Act: 未知 pid
    m_tester->updateProcessState(kPidB, 'S');

    // Assert: 状态与信号保持不变（负面场景状态保持）
    EXPECT_EQ(m_tester->m_processList[0].state(), 'R');
    EXPECT_EQ(spy.count(), 0);
}

TEST_F(ProcessTableModelTest, UpdateProcessPriority_ExistingPid_UpdatesPriority)
{
    // Arrange
    injectRow(m_tester.data(), makeValidProcess(kPidA));   // priority 0
    QSignalSpy spy(m_tester.data(), &ProcessTableModel::dataChanged);

    // Act
    m_tester->updateProcessPriority(kPidA, 10);

    // Assert: 优先级更新为 Low(10) 并发出信号
    EXPECT_EQ(m_tester->m_processList[0].priority(), 10);
    EXPECT_EQ(m_tester->data(m_tester->createIndex(0, ProcessTableModel::kProcessPriorityColumn), Qt::DisplayRole).toString(),
              QApplication::translate("Process.Priority", "Low"));
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(ProcessTableModelTest, UpdateProcessPriority_UnknownPid_KeepsPriority)
{
    // Arrange
    injectRow(m_tester.data(), makeValidProcess(kPidA));
    QSignalSpy spy(m_tester.data(), &ProcessTableModel::dataChanged);

    // Act
    m_tester->updateProcessPriority(kPidB, 10);

    // Assert
    EXPECT_EQ(m_tester->m_processList[0].priority(), 0);
    EXPECT_EQ(spy.count(), 0);
}

// ============================== removeProcess ==============================

TEST_F(ProcessTableModelTest, RemoveProcess_ExistingPid_RemovesRows)
{
    // Arrange
    injectRow(m_tester.data(), makeValidProcess(kPidA));
    injectRow(m_tester.data(), makeValidProcess(kPidB));

    // Act
    m_tester->removeProcess(kPidA);

    // Assert: 目标行被移除，其余保留
    EXPECT_EQ(m_tester->rowCount(), 1);
    EXPECT_EQ(rowIndexByPid(m_tester.data(), kPidA), -1);
    EXPECT_EQ(rowIndexByPid(m_tester.data(), kPidB), 0);
}

TEST_F(ProcessTableModelTest, RemoveProcess_UnknownPid_KeepsRows)
{
    // Arrange
    injectRow(m_tester.data(), makeValidProcess(kPidA));

    // Act: 未知 pid
    m_tester->removeProcess(kPidB);

    // Assert: 行保持不变
    EXPECT_EQ(m_tester->rowCount(), 1);
    EXPECT_EQ(rowIndexByPid(m_tester.data(), kPidA), 0);
}

// ============================== getProcessState / getProcess ==============================

TEST_F(ProcessTableModelTest, GetProcessState_ExistingPid_ReturnsCachedState)
{
    // Arrange: 打桩 ProcessSet::getProcessById（SUT 经 ProcessDB 单例读取）
    Process p = makeValidProcess(kPidA);
    p.setState('S');
    g_getByIdMap.insert(kPidA, p);
    Stub stubDB;
    stubDB.set(ADDR(ProcessSet, getProcessById), stub_getProcessById_map);
    injectRow(m_tester.data(), p);

    // Act + Assert
    EXPECT_EQ(m_tester->getProcessState(kPidA), 'S');
    g_getByIdMap.clear();
}

TEST_F(ProcessTableModelTest, GetProcessState_UnknownPid_ReturnsZero)
{
    // Arrange
    injectRow(m_tester.data(), makeValidProcess(kPidA));

    // Act + Assert: 未知 pid -> 默认 0
    EXPECT_EQ(m_tester->getProcessState(kPidB), 0);
}

TEST_F(ProcessTableModelTest, GetProcess_ExistingPid_ReturnsProcessCopy)
{
    // Arrange: 打桩 ProcessSet::getProcessById
    Process p = makeValidProcess(kPidA);
    g_getByIdMap.insert(kPidA, p);
    Stub stubDB;
    stubDB.set(ADDR(ProcessSet, getProcessById), stub_getProcessById_map);
    injectRow(m_tester.data(), p);

    // Act
    const Process ret = m_tester->getProcess(kPidA);

    // Assert
    EXPECT_EQ(ret.pid(), kPidA);
    EXPECT_EQ(ret.displayName(), QString("utproc"));
    g_getByIdMap.clear();
}

TEST_F(ProcessTableModelTest, GetProcess_UnknownPid_ReturnsDefaultInstance)
{
    // Arrange
    injectRow(m_tester.data(), makeValidProcess(kPidA));

    // Act
    const Process ret = m_tester->getProcess(kPidB);

    // Assert: 默认构造实例（无效）
    EXPECT_EQ(ret.pid(), 0);
    EXPECT_FALSE(ret.isValid());
}

// ============================== getProcessPriority / getProcessPriorityValue ==============================

TEST_F(ProcessTableModelTest, GetProcessPriority_ExistingPid_ReturnsEnumMapping)
{
    // Arrange: priority=0 -> kNormalPriority；打桩 ProcessSet::getProcessById
    Process p = makeValidProcess(kPidA);
    p.setPriority(0);
    g_getByIdMap.insert(kPidA, p);
    Stub stubDB;
    stubDB.set(ADDR(ProcessSet, getProcessById), stub_getProcessById_map);
    injectRow(m_tester.data(), p);

    // Act + Assert
    EXPECT_EQ(m_tester->getProcessPriority(kPidA), kNormalPriority);
    g_getByIdMap.clear();
}

TEST_F(ProcessTableModelTest, GetProcessPriority_UnknownPid_ReturnsInvalidPriority)
{
    // Arrange
    injectRow(m_tester.data(), makeValidProcess(kPidA));

    // Act + Assert: 未知 pid -> kInvalidPriority
    EXPECT_EQ(m_tester->getProcessPriority(kPidB), kInvalidPriority);
}

TEST_F(ProcessTableModelTest, GetProcessPriorityValue_ExistingPid_ReturnsRawPriority)
{
    // Arrange: 打桩 ProcessSet::getProcessById
    Process p = makeValidProcess(kPidA);
    p.setPriority(5);
    g_getByIdMap.insert(kPidA, p);
    Stub stubDB;
    stubDB.set(ADDR(ProcessSet, getProcessById), stub_getProcessById_map);
    injectRow(m_tester.data(), p);

    // Act + Assert
    EXPECT_EQ(m_tester->getProcessPriorityValue(kPidA), 5);
    g_getByIdMap.clear();
}

TEST_F(ProcessTableModelTest, GetProcessPriorityValue_UnknownPid_ReturnsNormalPriority)
{
    // Arrange
    injectRow(m_tester.data(), makeValidProcess(kPidA));

    // Act + Assert: 未知 pid -> kNormalPriority(0)
    EXPECT_EQ(m_tester->getProcessPriorityValue(kPidB), kNormalPriority);
}

// ============================== getTotal* 聚合 ==============================

TEST_F(ProcessTableModelTest, GetTotalCPUUsage_SumsProcessCpu)
{
    // Arrange
    injectRow(m_tester.data(), makeValidProcess(kPidA));                    // cpu 12.5
    Process second = makeValidProcess(kPidB);
    second.setCpu(20.5);
    injectRow(m_tester.data(), second);

    // Act + Assert
    EXPECT_DOUBLE_EQ(m_tester->getTotalCPUUsage(), 33.0);
}

TEST_F(ProcessTableModelTest, GetTotalCPUUsage_EmptyModel_ReturnsZero)
{
    // Act + Assert: 空模型边界
    EXPECT_DOUBLE_EQ(m_tester->getTotalCPUUsage(), 0.0);
}

TEST_F(ProcessTableModelTest, GetTotalMemoryUsage_SumsProcessMemory)
{
    // Arrange: memory = rss - shm = 4096
    injectRow(m_tester.data(), makeValidProcess(kPidA));

    // Act + Assert
    EXPECT_DOUBLE_EQ(m_tester->getTotalMemoryUsage(), 4096.0);
}

TEST_F(ProcessTableModelTest, GetTotalSharedMemoryUsage_SumsProcessShm)
{
    // Arrange
    injectRow(m_tester.data(), makeValidProcess(kPidA));   // shm 512

    // Act + Assert
    EXPECT_DOUBLE_EQ(m_tester->getTotalSharedMemoryUsage(), 512.0);
}

TEST_F(ProcessTableModelTest, GetTotalVirtualMemoryUsage_SumsProcessVmsize)
{
    // Arrange
    injectRow(m_tester.data(), makeValidProcess(kPidA));   // vmsize 8192

    // Act + Assert
    EXPECT_DOUBLE_EQ(m_tester->getTotalVirtualMemoryUsage(), 8192.0);
}

TEST_F(ProcessTableModelTest, GetTotalUpload_SumsSentBps)
{
    // Arrange
    injectRow(m_tester.data(), makeValidProcess(kPidA));   // sentBps 200

    // Act + Assert
    EXPECT_DOUBLE_EQ(m_tester->getTotalUpload(), 200.0);
}

TEST_F(ProcessTableModelTest, GetTotalDownload_SumsRecvBps)
{
    // Arrange
    injectRow(m_tester.data(), makeValidProcess(kPidA));   // recvBps 100

    // Act + Assert
    EXPECT_DOUBLE_EQ(m_tester->getTotalDownload(), 100.0);
}

TEST_F(ProcessTableModelTest, GetTotalDiskRead_SumsReadBps)
{
    // Arrange
    injectRow(m_tester.data(), makeValidProcess(kPidA));   // readBps 1024

    // Act + Assert
    EXPECT_DOUBLE_EQ(m_tester->getTotalDiskRead(), 1024.0);
}

TEST_F(ProcessTableModelTest, GetTotalDiskWrite_SumsWriteBps)
{
    // Arrange
    injectRow(m_tester.data(), makeValidProcess(kPidA));   // writeBps 2048

    // Act + Assert
    EXPECT_DOUBLE_EQ(m_tester->getTotalDiskWrite(), 2048.0);
}

// ============================== setUserModeName ==============================

TEST_F(ProcessTableModelTest, SetUserModeName_DifferentName_EmitsModelUpdated)
{
    // Act
    m_tester->setUserModeName("utuser");

    // Assert: 名称变更触发刷新
    EXPECT_EQ(m_tester->m_userModeName, QString("utuser"));
    EXPECT_EQ(m_modelUpdatedSpy->count(), 1);
}

TEST_F(ProcessTableModelTest, SetUserModeName_SameName_DoesNotEmitModelUpdated)
{
    // Arrange: 预设同名
    m_tester->setUserModeName("utuser");
    ASSERT_EQ(m_modelUpdatedSpy->count(), 1);

    // Act: 再次设置同名
    m_tester->setUserModeName("utuser");

    // Assert: 不重复刷新
    EXPECT_EQ(m_modelUpdatedSpy->count(), 1);
}
