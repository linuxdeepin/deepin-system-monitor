// Copyright (C) 2019-2026 UnionTech Software Technology Co., Ltd.
// SPDX-FileCopyrightText: 2019-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// rewritten unit tests for SystemServiceTableModel (deepin-system-monitor-main/model/system_service_table_model.cpp)
// based on upstream/master @ 3df42339004d7aa3e2cf2cc7b09de81fb8c981c2
//
// 用例计数声明（满足 test-code-gen.md §4 下限表，level/factors 取自 .ut inventory t18）:
//   data()               high (cx=13, factors: columns=8, role=3) -> min 3 -> 13 (Display 矩阵 9 + role 3 + AccessibleText 1)
//   headerData()         high (cx=13, factors: columns=8, role=3) -> min 3 -> 10 (矩阵 8 + 对齐/排序/回退 3)
//   updateServiceEntry   mid  (cx=7, loop)                        -> min 2 -> 3
//   updateServiceList    mid  (cx=4, loop)                        -> min 2 -> 4
//   fetchMore            mid  (cx=5)                              -> min 2 -> 4
//   canFetchMore         low  (cx=3)                              -> min 1 -> 3
//   rowCount/columnCount low  (cx=2)                              -> min 1 -> 4
//   flags                low  (cx=2)                              -> min 1 -> 2
//   reset                low  (cx=2)                              -> min 1 -> 1
//   getUnitFile*         low  (cx=2)                              -> min 1 -> 6
//
// 最小清单（test-types.md §8）自检:
//   [x] 1. 公开方法已列出，每个方法 >= 1 用例
//   [x] 2. 输入维度等价类划分（column 8 类 / role 4 类 / entry 名称空/非空 / pid 0/非0 / index 有效/无效）
//   [x] 3. 边界值显式覆盖（row=0 / row=size / section=列数 / pid=0 / fetchMore 边界 more=0,100 / 第 2 次抓取）
//   [x] 4. 同质多组输入 TEST_P 参数化（Display 列矩阵 / headerData 列矩阵）
//   [x] 5. 分支清单已列出并映射到用例名（见 BRANCH MAP）
//   [x] 6. 每条 if/switch/early-return 分支有触发用例
//   [x] 7. 源码无显式 throw；负面路径用无效 index/空名称构造
//   [x] 8. 负面场景（无效 index / 越界 row / 空 sname / 父 index）有专门用例
//   [x] 9. 负面用例验证状态保持（未知 entry 操作后 rowCount/内容不变）
//   [x] 10. 无外部 DBus 依赖注入点，ServiceManager 单例按旧用例约定直接构造（offscreen 安全）
//
// ============================ BRANCH MAP ============================
// 来源: get_code_snippet SystemServiceTableModel::data @
//       deepin-system-monitor-main/model/system_service_table_model.cpp:70-131
// SD1 : !index.isValid() || !(0<=row<m_svcList.size()) -> {}
// SD2a: Display/Accessible, Name        -> getSName()
// SD2b: Display/Accessible, LoadState   -> translate("DBus.Unit.Load.State", loadState)
// SD2c: Display/Accessible, ActiveState -> translate("DBus.Unit.Active.State", activeState)
// SD2d: Display/Accessible, SubState    -> getSubState()
// SD2e: Display/Accessible, State       -> translate("DBus.Unit.State", state)
// SD2f: Display/Accessible, StartupMode -> translate("DBus.Unit.Startup.Mode", startupType)
// SD2g: Display/Accessible, Description -> getDescription()
// SD2h: Display/Accessible, PID         -> pid==0 ? QVariant() : QVariant(pid)
// SD2i: Display/Accessible, default     -> {}
// SD3 : TextAlignmentRole               -> AlignLeft|AlignVCenter
// SD4 : 其他 role                       -> {}
// headerData @ system_service_table_model.cpp:155-202:
// SVH1: Display/Accessible 8 列矩阵 + default -> 基类
// SVH2: TextAlignmentRole -> AlignLeft|AlignVCenter
// SVH3: InitialSortOrderRole -> DescendingOrder
// SVH4: 其他 -> 基类实现
// updateServiceEntry @ system_service_table_model.cpp:36-67:
// UE1 : sname 已存在 -> 原位替换 + dataChanged + break
// UE2 : sname 为空 -> return（不插入）
// UE3 : 新名称 -> beginInsertRows + ++m_nr + currentRowChanged(row)
// updateServiceList @ system_service_table_model.cpp:253-272:
// UL1 : 空 sname 条目跳过
// UL2 : 有效条目写入 m_svcList + m_svcMap
// UL3 : beginResetModel/endResetModel（m_nr 归零且不再累加 -> 已知缺陷 #1，见 Mode 5 报告）
// fetchMore @ system_service_table_model.cpp:216-237:
// FM1 : parent 有效 -> return
// FM2 : more=min(100, size-m_nr) <= 0 -> return
// FM3 : more>0 -> 插入 m_nr..m_nr+more-1
// canFetchMore @ system_service_table_model.cpp:240-250:
// CF1 : parent 有效 -> false；CF2: m_nr < size；CF3: m_nr >= size
// flags: 无效 -> NoItemFlags；有效 -> Enabled|Selectable
// 用例映射:
//   Data_InvalidIndex_ReturnsEmptyVariant          -> SD1
//   Data_RowOutOfBounds_ReturnsEmptyVariant        -> SD1
//   Data_DisplayRole_ReturnsExpectedText (TEST_P)  -> SD2a~SD2i
//   Data_DisplayRole_PidZero_ReturnsInvalidVariant -> SD2h (pid=0)
//   Data_TextAlignmentRole_ReturnsLeftAndVCenter   -> SD3
//   Data_AccessibleTextRole_MatchesDisplayText     -> SD2 (Accessible)
//   Data_ToolTipRole_ReturnsEmptyVariant           -> SD4
//   HeaderData_DisplayRole_ReturnsTranslatedHeader (TEST_P) -> SVH1
//   HeaderData_OutOfRangeSection_FallsBackToBaseImplementation -> SVH1 default
//   HeaderData_TextAlignmentRole_ReturnsLeftAndVCenter -> SVH2
//   HeaderData_InitialSortOrderRole_ReturnsDescending  -> SVH3
//   HeaderData_OtherRole_FallsBackToBaseImplementation -> SVH4
//   UpdateServiceEntry_NewEntry_InsertsRowAndEmitsRowChanged -> UE3
//   UpdateServiceEntry_ExistingEntry_ReplacesAndEmitsDataChanged -> UE1
//   UpdateServiceEntry_EmptyName_Ignored -> UE2
//   UpdateServiceList_PopulatesEntriesAndResetsModel -> UL2/UL3
//   UpdateServiceList_SkipsEmptyEntries -> UL1
//   UpdateServiceList_ReplacesPreviousContent -> UL2
//   UpdateServiceList_ResetsLoadedCount -> UL3 (缺陷注记)
//   FetchMore_RootParent_LoadsUpToHundredRows -> FM3
//   FetchMore_SecondCall_LoadsRemainingRows -> FM3 (第二次)
//   FetchMore_NothingLeft_NoInsert -> FM2
//   FetchMore_ValidParent_NoInsert -> FM1
//   CanFetchMore_RootParent_PendingRows_ReturnsTrue  -> CF2
//   CanFetchMore_RootParent_AllLoaded_ReturnsFalse   -> CF3
//   CanFetchMore_ValidParent_ReturnsFalse            -> CF1
//   RowCount_RootParent_ReturnsLoadedCount / RowCount_ValidParent_ReturnsZero
//   ColumnCount_RootParent_ReturnsFixedCount / ColumnCount_ValidParent_ReturnsZero
//   Flags_InvalidIndex_ReturnsNoItemFlags / Flags_ValidIndex_ReturnsEnabledSelectable
//   Reset_PopulatedModel_ClearsEntries (缺陷注记: m_nr 未清零)
//   GetUnitFileName_ValidIndex_ReturnsName / GetUnitFileName_InvalidIndex_ReturnsEmpty
//   GetUnitActiveState_ValidIndex_ReturnsState / GetUnitActiveState_InvalidIndex_ReturnsEmpty
//   GetUnitFileState_ValidIndex_ReturnsState / GetUnitFileState_InvalidIndex_ReturnsEmpty
// ====================================================================

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include <QApplication>
#include <QSignalSpy>
#include <QStandardItemModel>
#include <QStandardItem>
#include <DApplication>

#include "model/system_service_table_model.h"
#include "service/system_service_entry.h"

DWIDGET_USE_NAMESPACE

namespace {

constexpr const char *kSvcName = "utsvc.service";

SystemServiceEntry makeEntry(const QString &sname = kSvcName,
                             const QString &loadState = "loaded",
                             const QString &activeState = "active",
                             const QString &subState = "running",
                             const QString &state = "enabled",
                             const QString &startupType = "enabled",
                             const QString &description = "ut service",
                             quint32 mainPID = 1234)
{
    return SystemServiceEntry("ut-id", sname, loadState, activeState, subState, state,
                              startupType, "/org/freedesktop/systemd1/unit/utservice",
                              description, mainPID, true, true, true);
}

// headerData 列矩阵参数
struct HeaderCase {
    int column;
    const char *headerKey;
};

// DisplayRole 列矩阵参数
struct DisplayCase {
    int column;
    QString expected;
};

}   // namespace

// ============================== 构造与规模 ==============================

class SystemServiceTableModelTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_tester.reset(new SystemServiceTableModel());
    }
    void TearDown() override
    {
        m_tester.reset();
    }
    QScopedPointer<SystemServiceTableModel> m_tester;
};

TEST_F(SystemServiceTableModelTest, Ctor_InitialState_HasZeroRowsAndFixedColumns)
{
    // Assert: 初始空模型
    EXPECT_EQ(m_tester->rowCount(), 0);
    EXPECT_EQ(m_tester->columnCount(), SystemServiceTableModel::kSystemServiceTableColumnCount);
    EXPECT_EQ(SystemServiceTableModel::kSystemServiceTableColumnCount, 8);
    EXPECT_FALSE(m_tester->canFetchMore(QModelIndex()));
}

// ============================== data(): SD1 无效输入 ==============================

TEST_F(SystemServiceTableModelTest, Data_InvalidIndex_ReturnsEmptyVariant)
{
    // branch SD1
    // Act + Assert
    EXPECT_EQ(m_tester->data(QModelIndex(), Qt::DisplayRole), QVariant());
}

TEST_F(SystemServiceTableModelTest, Data_RowOutOfBounds_ReturnsEmptyVariant)
{
    // Arrange: 注入一条记录
    m_tester->updateServiceEntry(makeEntry());
    ASSERT_EQ(m_tester->rowCount(), 1);

    // branch SD1: row 越界
    // Act + Assert
    EXPECT_EQ(m_tester->data(m_tester->index(-1, 0), Qt::DisplayRole), QVariant());
    EXPECT_EQ(m_tester->data(m_tester->index(1, 0), Qt::DisplayRole), QVariant());
}

// ============================== data(): DisplayRole 列矩阵 (SD2) ==============================

class SystemServiceTableModelDisplayParamTest : public SystemServiceTableModelTest,
                                                public ::testing::WithParamInterface<DisplayCase> {
};

TEST_P(SystemServiceTableModelDisplayParamTest, Data_DisplayRole_ReturnsExpectedText)
{
    // Arrange: 注入一条完整记录
    m_tester->updateServiceEntry(makeEntry());
    ASSERT_EQ(m_tester->rowCount(), 1);
    const DisplayCase &c = GetParam();

    // Act
    const QVariant v = m_tester->data(m_tester->index(0, c.column), Qt::DisplayRole);

    // Assert: 精确字符串
    EXPECT_EQ(v.toString(), c.expected);
}

INSTANTIATE_TEST_SUITE_P(DisplayColumnMatrix, SystemServiceTableModelDisplayParamTest,
    ::testing::Values(
        DisplayCase{SystemServiceTableModel::kSystemServiceNameColumn, "utsvc.service"},
        DisplayCase{SystemServiceTableModel::kSystemServiceLoadStateColumn,
                    DApplication::translate("DBus.Unit.Load.State", "loaded")},
        DisplayCase{SystemServiceTableModel::kSystemServiceActiveStateColumn,
                    DApplication::translate("DBus.Unit.Active.State", "active")},
        DisplayCase{SystemServiceTableModel::kSystemServiceSubStateColumn, "running"},
        DisplayCase{SystemServiceTableModel::kSystemServiceStateColumn,
                    DApplication::translate("DBus.Unit.State", "enabled")},
        DisplayCase{SystemServiceTableModel::kSystemServiceStartupModeColumn,
                    DApplication::translate("DBus.Unit.Startup.Mode", "enabled")},
        DisplayCase{SystemServiceTableModel::kSystemServiceDescriptionColumn, "ut service"},
        // 等价类: 枚举之外列 -> 空串（SD2i）
        DisplayCase{SystemServiceTableModel::kSystemServiceTableColumnCount, QString()}
    ));

TEST_F(SystemServiceTableModelTest, Data_DisplayRole_PidZero_ReturnsInvalidVariant)
{
    // Arrange: mainPID=0 的服务
    m_tester->updateServiceEntry(makeEntry(kSvcName, "loaded", "active", "running", "enabled", "enabled", "ut service", 0));
    ASSERT_EQ(m_tester->rowCount(), 1);

    // branch SD2h: pid==0 -> QVariant()
    // Act + Assert
    EXPECT_EQ(m_tester->data(m_tester->index(0, SystemServiceTableModel::kSystemServicePIDColumn), Qt::DisplayRole), QVariant());
}

TEST_F(SystemServiceTableModelTest, Data_DisplayRole_PidNonZero_ReturnsUintVariant)
{
    // Arrange: mainPID=1234
    m_tester->updateServiceEntry(makeEntry());
    ASSERT_EQ(m_tester->rowCount(), 1);

    // branch SD2h: pid!=0 -> QVariant(pid)
    // Act + Assert
    const QVariant v = m_tester->data(m_tester->index(0, SystemServiceTableModel::kSystemServicePIDColumn), Qt::DisplayRole);
    EXPECT_EQ(v.toUInt(), 1234u);
}

// ============================== data(): 其余 role (SD3/SD4) ==============================

TEST_F(SystemServiceTableModelTest, Data_TextAlignmentRole_ReturnsLeftAndVCenter)
{
    // Arrange
    m_tester->updateServiceEntry(makeEntry());
    ASSERT_EQ(m_tester->rowCount(), 1);

    // branch SD3
    // Act + Assert
    const QVariant v = m_tester->data(m_tester->index(0, SystemServiceTableModel::kSystemServiceNameColumn), Qt::TextAlignmentRole);
    EXPECT_EQ(v.toInt(), int(Qt::AlignLeft | Qt::AlignVCenter));
}

TEST_F(SystemServiceTableModelTest, Data_AccessibleTextRole_MatchesDisplayText)
{
    // Arrange
    m_tester->updateServiceEntry(makeEntry());
    ASSERT_EQ(m_tester->rowCount(), 1);

    // branch SD2 (AccessibleTextRole 同分支)
    // Act + Assert
    const QVariant acc = m_tester->data(m_tester->index(0, SystemServiceTableModel::kSystemServiceNameColumn), Qt::AccessibleTextRole);
    const QVariant disp = m_tester->data(m_tester->index(0, SystemServiceTableModel::kSystemServiceNameColumn), Qt::DisplayRole);
    EXPECT_EQ(acc, disp);
}

TEST_F(SystemServiceTableModelTest, Data_ToolTipRole_ReturnsEmptyVariant)
{
    // Arrange
    m_tester->updateServiceEntry(makeEntry());
    ASSERT_EQ(m_tester->rowCount(), 1);

    // branch SD4: 其他 role
    // Act + Assert
    EXPECT_EQ(m_tester->data(m_tester->index(0, SystemServiceTableModel::kSystemServiceNameColumn), Qt::ToolTipRole), QVariant());
    EXPECT_EQ(m_tester->data(m_tester->index(0, SystemServiceTableModel::kSystemServiceNameColumn), Qt::EditRole), QVariant());
}

// ============================== headerData (SVH) ==============================

class SystemServiceTableModelHeaderParamTest : public SystemServiceTableModelTest,
                                               public ::testing::WithParamInterface<HeaderCase> {
};

TEST_P(SystemServiceTableModelHeaderParamTest, HeaderData_DisplayRole_ReturnsTranslatedHeader)
{
    // branch SVH1
    // Act + Assert: 与源码同一 translate 调用
    const QVariant v = m_tester->headerData(GetParam().column, Qt::Horizontal, Qt::DisplayRole);
    EXPECT_EQ(v.toString(), DApplication::translate("Service.Table.Header", GetParam().headerKey));
}

INSTANTIATE_TEST_SUITE_P(HeaderColumnMatrix, SystemServiceTableModelHeaderParamTest,
    ::testing::Values(
        HeaderCase{SystemServiceTableModel::kSystemServiceNameColumn, kSystemServiceName},
        HeaderCase{SystemServiceTableModel::kSystemServiceLoadStateColumn, kSystemServiceLoadState},
        HeaderCase{SystemServiceTableModel::kSystemServiceActiveStateColumn, kSystemServiceActiveState},
        HeaderCase{SystemServiceTableModel::kSystemServiceSubStateColumn, kSystemServiceSubState},
        HeaderCase{SystemServiceTableModel::kSystemServiceStateColumn, kSystemServiceState},
        HeaderCase{SystemServiceTableModel::kSystemServiceDescriptionColumn, kSystemServiceDescription},
        HeaderCase{SystemServiceTableModel::kSystemServicePIDColumn, kSystemServicePID},
        HeaderCase{SystemServiceTableModel::kSystemServiceStartupModeColumn, kSystemServiceStartupMode}
    ));

TEST_F(SystemServiceTableModelTest, HeaderData_OutOfRangeSection_FallsBackToBaseImplementation)
{
    // branch SVH1 default: section 超出列范围走基类实现。
    // Qt 6.8 起 QAbstractItemModel::headerData 对 DisplayRole 返回默认文本 section+1。
    // Act + Assert
    EXPECT_EQ(m_tester->headerData(SystemServiceTableModel::kSystemServiceTableColumnCount, Qt::Horizontal, Qt::DisplayRole),
              QVariant(SystemServiceTableModel::kSystemServiceTableColumnCount + 1));
    EXPECT_EQ(m_tester->headerData(-1, Qt::Horizontal, Qt::DisplayRole), QVariant(0));
    // 非 DisplayRole 无默认文本
    EXPECT_EQ(m_tester->headerData(SystemServiceTableModel::kSystemServiceTableColumnCount, Qt::Horizontal, Qt::ToolTipRole), QVariant());
}

TEST_F(SystemServiceTableModelTest, HeaderData_TextAlignmentRole_ReturnsLeftAndVCenter)
{
    // branch SVH2
    // Act + Assert
    const QVariant v = m_tester->headerData(SystemServiceTableModel::kSystemServiceNameColumn, Qt::Horizontal, Qt::TextAlignmentRole);
    EXPECT_EQ(v.toInt(), int(Qt::AlignLeft | Qt::AlignVCenter));
}

TEST_F(SystemServiceTableModelTest, HeaderData_InitialSortOrderRole_ReturnsDescending)
{
    // branch SVH3
    // Act + Assert
    const QVariant v = m_tester->headerData(SystemServiceTableModel::kSystemServiceNameColumn, Qt::Horizontal, Qt::InitialSortOrderRole);
    EXPECT_EQ(v, QVariant::fromValue(Qt::DescendingOrder));
}

TEST_F(SystemServiceTableModelTest, HeaderData_OtherRole_FallsBackToBaseImplementation)
{
    // branch SVH4
    // Act + Assert
    EXPECT_EQ(m_tester->headerData(SystemServiceTableModel::kSystemServiceNameColumn, Qt::Horizontal, Qt::ToolTipRole), QVariant());
    // 缺陷 #4 已修复: headerData 现判断 orientation，Vertical 方向返回空 QVariant
    EXPECT_EQ(m_tester->headerData(SystemServiceTableModel::kSystemServiceNameColumn, Qt::Vertical, Qt::DisplayRole),
              QVariant());
}

// ============================== updateServiceEntry (UE1~UE3) ==============================

TEST_F(SystemServiceTableModelTest, UpdateServiceEntry_NewEntry_InsertsRowAndEmitsRowChanged)
{
    // Arrange
    QSignalSpy rowSpy(m_tester.data(), &SystemServiceTableModel::currentRowChanged);
    QSignalSpy insertSpy(m_tester.data(), &SystemServiceTableModel::rowsInserted);

    // branch UE3: 新名称插入
    // Act
    m_tester->updateServiceEntry(makeEntry());

    // Assert: 行可见（m_nr 递增）+ 信号
    EXPECT_EQ(m_tester->rowCount(), 1);
    EXPECT_EQ(m_tester->data(m_tester->index(0, SystemServiceTableModel::kSystemServiceNameColumn), Qt::DisplayRole).toString(),
              QString("utsvc.service"));
    EXPECT_EQ(rowSpy.count(), 1);
    EXPECT_EQ(rowSpy.at(0).at(0).toInt(), 0);
    EXPECT_EQ(insertSpy.count(), 1);
}

TEST_F(SystemServiceTableModelTest, UpdateServiceEntry_ExistingEntry_ReplacesAndEmitsDataChanged)
{
    // Arrange: 先插入一条
    m_tester->updateServiceEntry(makeEntry());
    ASSERT_EQ(m_tester->rowCount(), 1);
    QSignalSpy dataSpy(m_tester.data(), &SystemServiceTableModel::dataChanged);
    QSignalSpy rowSpy(m_tester.data(), &SystemServiceTableModel::currentRowChanged);

    // branch UE1: 同名替换
    // Act
    m_tester->updateServiceEntry(makeEntry(kSvcName, "loaded", "active", "running", "enabled", "enabled", "updated desc"));

    // Assert: 行数不变、描述更新、发 dataChanged 不发 currentRowChanged
    EXPECT_EQ(m_tester->rowCount(), 1);
    EXPECT_EQ(m_tester->data(m_tester->index(0, SystemServiceTableModel::kSystemServiceDescriptionColumn), Qt::DisplayRole).toString(),
              QString("updated desc"));
    EXPECT_EQ(dataSpy.count(), 1);
    EXPECT_EQ(rowSpy.count(), 0);
}

TEST_F(SystemServiceTableModelTest, UpdateServiceEntry_EmptyName_Ignored)
{
    // Arrange
    QSignalSpy rowSpy(m_tester.data(), &SystemServiceTableModel::currentRowChanged);

    // branch UE2: 空 sname 忽略（负面场景状态保持）
    // Act
    m_tester->updateServiceEntry(makeEntry(""));

    // Assert: 无插入、无信号
    EXPECT_EQ(m_tester->rowCount(), 0);
    EXPECT_EQ(rowSpy.count(), 0);
}

// ============================== updateServiceList (UL1~UL3) ==============================

TEST_F(SystemServiceTableModelTest, UpdateServiceList_PopulatesEntriesAndResetsModel)
{
    // Arrange
    const QList<SystemServiceEntry> list { makeEntry("a.service"), makeEntry("b.service"), makeEntry("c.service") };
    QSignalSpy resetSpy(m_tester.data(), &SystemServiceTableModel::modelReset);

    // branch UL2/UL3
    // Act
    m_tester->updateServiceList(list);

    // Assert: 内部容器已填充；去重表 3 条
    EXPECT_EQ(m_tester->m_svcList.size(), 3);
    EXPECT_EQ(m_tester->m_svcMap.size(), 3);
    EXPECT_EQ(m_tester->m_svcMap.value("b.service").getMainPID(), 1234u);
    EXPECT_EQ(resetSpy.count(), 1);
}

TEST_F(SystemServiceTableModelTest, UpdateServiceList_SkipsEmptyEntries)
{
    // Arrange: 第二条 sname 为空
    const QList<SystemServiceEntry> list { makeEntry("a.service"), makeEntry(""), makeEntry("c.service") };

    // branch UL1
    // Act
    m_tester->updateServiceList(list);

    // Assert: 仅 2 条有效条目
    EXPECT_EQ(m_tester->m_svcList.size(), 2);
    EXPECT_FALSE(m_tester->m_svcMap.contains(""));
}

TEST_F(SystemServiceTableModelTest, UpdateServiceList_ReplacesPreviousContent)
{
    // Arrange: 先批量导入 2 条，再导入 1 条
    m_tester->updateServiceList(QList<SystemServiceEntry> { makeEntry("old1.service"), makeEntry("old2.service") });
    ASSERT_EQ(m_tester->m_svcList.size(), 2);

    // branch UL2: 全量替换语义
    // Act
    m_tester->updateServiceList(QList<SystemServiceEntry> { makeEntry("new.service") });

    // Assert: 旧条目清除
    EXPECT_EQ(m_tester->m_svcList.size(), 1);
    EXPECT_TRUE(m_tester->m_svcMap.contains("new.service"));
    EXPECT_FALSE(m_tester->m_svcMap.contains("old1.service"));
}

// 已知缺陷 #1（详见 Mode 5 缺陷清单）: updateServiceList 将 m_nr 归零后不再累加，
// bulk 导入后 rowCount() 恒为 0（与 updateServiceEntry 的 ++m_nr 行为不一致）。
// 此用例锁定当前实际行为并在缺陷清单中跟进，修复后应改为 EXPECT_EQ(m_tester->rowCount(), 2)。
TEST_F(SystemServiceTableModelTest, UpdateServiceList_ResetsLoadedCount)
{
    // Arrange
    m_tester->updateServiceList(QList<SystemServiceEntry> { makeEntry("a.service"), makeEntry("b.service") });

    // branch UL3 (m_nr = 0)
    // Act + Assert: 锁定缺陷现状
    EXPECT_EQ(m_tester->rowCount(), 0);
    EXPECT_EQ(m_tester->m_nr, 0);
    // 数据仍在模型内，fetchMore 消费的是 m_svcList
    EXPECT_TRUE(m_tester->canFetchMore(QModelIndex()));
}

// ============================== fetchMore (FM1~FM3) ==============================

TEST_F(SystemServiceTableModelTest, FetchMore_RootParent_LoadsUpToHundredRows)
{
    // Arrange: 直接注入 150 条名称（跳过信号开销）
    for (int i = 0; i < 150; ++i)
        m_tester->m_svcList << QString("svc%1.service").arg(i);

    // branch FM3: 首次最多加载 100
    // Act
    m_tester->fetchMore(QModelIndex());

    // Assert
    EXPECT_EQ(m_tester->rowCount(), 100);
    EXPECT_TRUE(m_tester->canFetchMore(QModelIndex()));
}

TEST_F(SystemServiceTableModelTest, FetchMore_SecondCall_LoadsRemainingRows)
{
    // Arrange: 注入 150 条并先抓取 100
    for (int i = 0; i < 150; ++i)
        m_tester->m_svcList << QString("svc%1.service").arg(i);
    m_tester->fetchMore(QModelIndex());
    ASSERT_EQ(m_tester->rowCount(), 100);

    // branch FM3: 第二次抓取剩余 50
    // Act
    m_tester->fetchMore(QModelIndex());

    // Assert
    EXPECT_EQ(m_tester->rowCount(), 150);
    EXPECT_FALSE(m_tester->canFetchMore(QModelIndex()));
}

TEST_F(SystemServiceTableModelTest, FetchMore_NothingLeft_NoInsert)
{
    // Arrange: 空模型（left = 0 - 0 = 0）
    QSignalSpy insertSpy(m_tester.data(), &SystemServiceTableModel::rowsInserted);

    // branch FM2: more <= 0
    // Act
    m_tester->fetchMore(QModelIndex());

    // Assert
    EXPECT_EQ(m_tester->rowCount(), 0);
    EXPECT_EQ(insertSpy.count(), 0);
}

TEST_F(SystemServiceTableModelTest, FetchMore_ValidParent_NoInsert)
{
    // Arrange: 至少一条可见行以获得有效 parent index
    m_tester->updateServiceEntry(makeEntry("a.service"));
    ASSERT_EQ(m_tester->rowCount(), 1);
    QSignalSpy insertSpy(m_tester.data(), &SystemServiceTableModel::rowsInserted);

    // branch FM1: parent 有效直接返回
    // Act + Assert
    m_tester->fetchMore(m_tester->index(0, 0));
    EXPECT_EQ(m_tester->rowCount(), 1);
    EXPECT_EQ(insertSpy.count(), 0);
}

// ============================== canFetchMore (CF1~CF3) ==============================

TEST_F(SystemServiceTableModelTest, CanFetchMore_RootParent_PendingRows_ReturnsTrue)
{
    // Arrange: m_nr=0 < size=5
    for (int i = 0; i < 5; ++i)
        m_tester->m_svcList << QString("svc%1.service").arg(i);

    // branch CF2
    // Act + Assert
    EXPECT_TRUE(m_tester->canFetchMore(QModelIndex()));
}

TEST_F(SystemServiceTableModelTest, CanFetchMore_RootParent_AllLoaded_ReturnsFalse)
{
    // Arrange: m_nr == size
    m_tester->updateServiceEntry(makeEntry());
    ASSERT_EQ(m_tester->rowCount(), 1);

    // branch CF3
    // Act + Assert
    EXPECT_FALSE(m_tester->canFetchMore(QModelIndex()));
}

TEST_F(SystemServiceTableModelTest, CanFetchMore_ValidParent_ReturnsFalse)
{
    // Arrange
    m_tester->updateServiceEntry(makeEntry());

    // branch CF1
    // Act + Assert
    EXPECT_FALSE(m_tester->canFetchMore(m_tester->index(0, 0)));
}

// ============================== rowCount / columnCount ==============================

TEST_F(SystemServiceTableModelTest, RowCount_RootParent_ReturnsLoadedCount)
{
    // Arrange
    m_tester->updateServiceEntry(makeEntry("a.service"));
    m_tester->updateServiceEntry(makeEntry("b.service"));

    // Act + Assert
    EXPECT_EQ(m_tester->rowCount(), 2);
    EXPECT_EQ(m_tester->rowCount(QModelIndex()), 2);
}

TEST_F(SystemServiceTableModelTest, RowCount_ValidParent_ReturnsZero)
{
    // Arrange
    m_tester->updateServiceEntry(makeEntry());

    // Act + Assert
    EXPECT_EQ(m_tester->rowCount(m_tester->index(0, 0)), 0);
}

TEST_F(SystemServiceTableModelTest, ColumnCount_RootParent_ReturnsFixedCount)
{
    // Act + Assert
    EXPECT_EQ(m_tester->columnCount(), SystemServiceTableModel::kSystemServiceTableColumnCount);
    EXPECT_EQ(m_tester->columnCount(QModelIndex()), 8);
}

TEST_F(SystemServiceTableModelTest, ColumnCount_ValidParent_ReturnsZero)
{
    // Arrange
    m_tester->updateServiceEntry(makeEntry());

    // Act + Assert
    EXPECT_EQ(m_tester->columnCount(m_tester->index(0, 0)), 0);
}

// ============================== flags ==============================

TEST_F(SystemServiceTableModelTest, Flags_InvalidIndex_ReturnsNoItemFlags)
{
    // Act + Assert
    EXPECT_EQ(m_tester->flags(QModelIndex()), Qt::NoItemFlags);
}

TEST_F(SystemServiceTableModelTest, Flags_ValidIndex_ReturnsEnabledSelectable)
{
    // Arrange
    m_tester->updateServiceEntry(makeEntry());

    // Act + Assert
    EXPECT_EQ(m_tester->flags(m_tester->index(0, 0)), Qt::ItemIsEnabled | Qt::ItemIsSelectable);
}

// ============================== reset ==============================

// 缺陷 #1/#2 已修复: reset() 现已清零 m_nr 并对空列表做守卫，
// reset 后 rowCount() 正确返回 0。
TEST_F(SystemServiceTableModelTest, Reset_PopulatedModel_ClearsEntries)
{
    // Arrange: 两条可见行
    m_tester->updateServiceEntry(makeEntry("a.service"));
    m_tester->updateServiceEntry(makeEntry("b.service"));
    ASSERT_EQ(m_tester->rowCount(), 2);

    // Act
    m_tester->reset();

    // Assert: 数据已清空，rowCount 正确归零
    EXPECT_TRUE(m_tester->m_svcList.isEmpty());
    EXPECT_TRUE(m_tester->m_svcMap.isEmpty());
    EXPECT_EQ(m_tester->rowCount(), 0);
}

// ============================== getUnitFile* ==============================

TEST_F(SystemServiceTableModelTest, GetUnitFileName_ValidIndex_ReturnsName)
{
    // Arrange
    m_tester->updateServiceEntry(makeEntry());

    // Act + Assert
    EXPECT_EQ(m_tester->getUnitFileName(m_tester->index(0, 0)), QString("utsvc.service"));
}

TEST_F(SystemServiceTableModelTest, GetUnitFileName_InvalidIndex_ReturnsEmpty)
{
    // Act + Assert
    EXPECT_EQ(m_tester->getUnitFileName(QModelIndex()), QString());
}

TEST_F(SystemServiceTableModelTest, GetUnitActiveState_ValidIndex_ReturnsState)
{
    // Arrange
    m_tester->updateServiceEntry(makeEntry());

    // Act + Assert
    EXPECT_EQ(m_tester->getUnitActiveState(m_tester->index(0, 0)), QString("active"));
}

TEST_F(SystemServiceTableModelTest, GetUnitActiveState_InvalidIndex_ReturnsEmpty)
{
    // Act + Assert
    EXPECT_EQ(m_tester->getUnitActiveState(QModelIndex()), QString());
}

TEST_F(SystemServiceTableModelTest, GetUnitFileState_ValidIndex_ReturnsState)
{
    // Arrange
    m_tester->updateServiceEntry(makeEntry());

    // Act + Assert
    EXPECT_EQ(m_tester->getUnitFileState(m_tester->index(0, 0)), QString("enabled"));
}

TEST_F(SystemServiceTableModelTest, GetUnitFileState_InvalidIndex_ReturnsEmpty)
{
    // Act + Assert
    EXPECT_EQ(m_tester->getUnitFileState(QModelIndex()), QString());
}
