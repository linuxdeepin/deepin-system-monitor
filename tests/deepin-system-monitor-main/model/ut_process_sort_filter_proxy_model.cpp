// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self

#include "model/process_sort_filter_proxy_model.h"
#include "process/process_db.h"
#include "model/process_table_model.h"
#include "common/han_latin.h"
#include "common/common.h"


//gtest
#include "stub.h"
#include <gtest/gtest.h>
// Qt
#include <QStandardItemModel>
#include <QStandardItem>
#include "process/process_set.h"  // FilterType 枚举（kFilterApps/kFilterCurrentUser/kNoFilter）
/***************************************STUB begin*********************************************/
int stub_proclessThan_sortColumn1(){
    return ProcessTableModel::kProcessNameColumn;
}

int stub_proclessThan_sortColumn2(){
    return ProcessTableModel::kProcessUserColumn;
}

int stub_proclessThan_sortColumn3(){
    return ProcessTableModel::kProcessMemoryColumn;
}

int stub_proclessThan_sortColumn4(){
    return ProcessTableModel::kProcessShareMemoryColumn;
}

int stub_proclessThan_sortColumn5(){
    return ProcessTableModel::kProcessVTRMemoryColumn;
}

int stub_proclessThan_sortColumn6(){
    return ProcessTableModel::kProcessCPUColumn;
}

int stub_proclessThan_sortColumn7(){
    return ProcessTableModel::kProcessUploadColumn;
}

int stub_proclessThan_sortColumn8(){
    return ProcessTableModel::kProcessDownloadColumn;
}

int stub_proclessThan_sortColumn9(){
    return ProcessTableModel::kProcessPIDColumn;
}

int stub_proclessThan_sortColumn10(){
    return ProcessTableModel::kProcessDiskReadColumn;
}

int stub_proclessThan_sortColumn11(){
    return ProcessTableModel::kProcessDiskWriteColumn;
}

int stub_proclessThan_sortColumn12(){
    return ProcessTableModel::kProcessNiceColumn;
}

int stub_proclessThan_sortColumn13(){
    return ProcessTableModel::kProcessPriorityColumn;
}

int stub_proclessThan_sortColumn14(){
    return 1000;
}

/***************************************STUB end**********************************************/
class UT_ProcessSortFilterProxyModel: public ::testing::Test
{
public:
    UT_ProcessSortFilterProxyModel() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new ProcessSortFilterProxyModel();
    }

    virtual void TearDown()
    {
        if(m_tester){
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    ProcessSortFilterProxyModel *m_tester;
};

TEST_F(UT_ProcessSortFilterProxyModel, initTest)
{

}

TEST_F(UT_ProcessSortFilterProxyModel, test_setSortFilterString_001)
{
    m_tester->setSortFilterString("aaa");
}

TEST_F(UT_ProcessSortFilterProxyModel, test_setFilterType_001)
{

}

TEST_F(UT_ProcessSortFilterProxyModel, test_filterAcceptsRow_001)
{
    m_tester->setFilterType(0);
}

TEST_F(UT_ProcessSortFilterProxyModel, test_lessThan_001)
{
    Stub b;
    b.set(ADDR(QSortFilterProxyModel,sortColumn),stub_proclessThan_sortColumn1);
    QModelIndex *lindex = new QModelIndex;
    QModelIndex *rindex = new QModelIndex;

    m_tester->lessThan(*lindex,*rindex);

    delete rindex;
    delete lindex;
}

TEST_F(UT_ProcessSortFilterProxyModel, test_lessThan_002)
{
    Stub b;
    b.set(ADDR(QSortFilterProxyModel,sortColumn),stub_proclessThan_sortColumn2);
    QModelIndex *lindex = new QModelIndex;
    QModelIndex *rindex = new QModelIndex;

    m_tester->lessThan(*lindex,*rindex);

    delete rindex;
    delete lindex;
}

TEST_F(UT_ProcessSortFilterProxyModel, test_lessThan_003)
{
    Stub b;
    b.set(ADDR(QSortFilterProxyModel,sortColumn),stub_proclessThan_sortColumn3);
    QModelIndex *lindex = new QModelIndex;
    QModelIndex *rindex = new QModelIndex;

    m_tester->lessThan(*lindex,*rindex);

    delete rindex;
    delete lindex;
}

TEST_F(UT_ProcessSortFilterProxyModel, test_lessThan_004)
{
    Stub b;
    b.set(ADDR(QSortFilterProxyModel,sortColumn),stub_proclessThan_sortColumn4);
    QModelIndex *lindex = new QModelIndex;
    QModelIndex *rindex = new QModelIndex;

    m_tester->lessThan(*lindex,*rindex);

    delete rindex;
    delete lindex;
}

TEST_F(UT_ProcessSortFilterProxyModel, test_lessThan_005)
{
    Stub b;
    b.set(ADDR(QSortFilterProxyModel,sortColumn),stub_proclessThan_sortColumn5);
    QModelIndex *lindex = new QModelIndex;
    QModelIndex *rindex = new QModelIndex;

    m_tester->lessThan(*lindex,*rindex);

    delete rindex;
    delete lindex;
}

TEST_F(UT_ProcessSortFilterProxyModel, test_lessThan_006)
{
    Stub b;
    b.set(ADDR(QSortFilterProxyModel,sortColumn),stub_proclessThan_sortColumn6);
    QModelIndex *lindex = new QModelIndex;
    QModelIndex *rindex = new QModelIndex;

    m_tester->lessThan(*lindex,*rindex);

    delete rindex;
    delete lindex;
}

TEST_F(UT_ProcessSortFilterProxyModel, test_lessThan_007)
{
    Stub b;
    b.set(ADDR(QSortFilterProxyModel,sortColumn),stub_proclessThan_sortColumn7);
    QModelIndex *lindex = new QModelIndex;
    QModelIndex *rindex = new QModelIndex;

    m_tester->lessThan(*lindex,*rindex);

    delete rindex;
    delete lindex;
}

TEST_F(UT_ProcessSortFilterProxyModel, test_lessThan_008)
{
    Stub b;
    b.set(ADDR(QSortFilterProxyModel,sortColumn),stub_proclessThan_sortColumn8);
    QModelIndex *lindex = new QModelIndex;
    QModelIndex *rindex = new QModelIndex;

    m_tester->lessThan(*lindex,*rindex);

    delete rindex;
    delete lindex;
}

TEST_F(UT_ProcessSortFilterProxyModel, test_lessThan_009)
{
    Stub b;
    b.set(ADDR(QSortFilterProxyModel,sortColumn),stub_proclessThan_sortColumn9);
    QModelIndex *lindex = new QModelIndex;
    QModelIndex *rindex = new QModelIndex;

    m_tester->lessThan(*lindex,*rindex);

    delete rindex;
    delete lindex;
}

TEST_F(UT_ProcessSortFilterProxyModel, test_lessThan_010)
{
    Stub b;
    b.set(ADDR(QSortFilterProxyModel,sortColumn),stub_proclessThan_sortColumn10);
    QModelIndex *lindex = new QModelIndex;
    QModelIndex *rindex = new QModelIndex;

    m_tester->lessThan(*lindex,*rindex);

    delete rindex;
    delete lindex;
}

TEST_F(UT_ProcessSortFilterProxyModel, test_lessThan_011)
{
    Stub b;
    b.set(ADDR(QSortFilterProxyModel,sortColumn),stub_proclessThan_sortColumn11);
    QModelIndex *lindex = new QModelIndex;
    QModelIndex *rindex = new QModelIndex;

    m_tester->lessThan(*lindex,*rindex);

    delete rindex;
    delete lindex;
}

TEST_F(UT_ProcessSortFilterProxyModel, test_lessThan_012)
{
    Stub b;
    b.set(ADDR(QSortFilterProxyModel,sortColumn),stub_proclessThan_sortColumn12);
    QModelIndex *lindex = new QModelIndex;
    QModelIndex *rindex = new QModelIndex;

    m_tester->lessThan(*lindex,*rindex);

    delete rindex;
    delete lindex;
}

TEST_F(UT_ProcessSortFilterProxyModel, test_lessThan_013)
{
    Stub b;
    b.set(ADDR(QSortFilterProxyModel,sortColumn),stub_proclessThan_sortColumn13);
    QModelIndex *lindex = new QModelIndex;
    QModelIndex *rindex = new QModelIndex;

    m_tester->lessThan(*lindex,*rindex);

    delete rindex;
    delete lindex;
}

TEST_F(UT_ProcessSortFilterProxyModel, test_lessThan_014)
{
    Stub b;
    b.set(ADDR(QSortFilterProxyModel,sortColumn),stub_proclessThan_sortColumn14);
    QModelIndex *lindex = new QModelIndex;
    QModelIndex *rindex = new QModelIndex;

    m_tester->lessThan(*lindex,*rindex);

    delete rindex;
    delete lindex;
}

/***************************************真实源模型 begin*********************************************/
// 构建带 2 行真实数据的 QStandardItemModel 源模型，使 lessThan/filterAcceptsRow 走入实际分支
// （既有用例传入非法 QModelIndex，在 lessThan 起始的 !isValid() 处即返回，无法覆盖排序逻辑）
static QStandardItemModel *makeProcSourceModel(QObject *parent)
{
    auto *m = new QStandardItemModel(2, ProcessTableModel::kProcessColumnCount, parent);
    // row 0: name "bash" pid 1000 cpu 1.0 mem 100 nice 0 upload 1.0/bytes 100 user "root"
    // row 1: name "zsh"  pid 2000 cpu 2.0 mem 200 nice 5 upload 2.0/bytes 200 user "uos"
    auto setCell = [&](int row, int col, const QVariant &display, const QVariant &user) {
        QStandardItem *item = m->item(row, col);
        if (!item) {
            item = new QStandardItem;
            m->setItem(row, col, item);
        }
        if (display.isValid()) item->setData(display, Qt::DisplayRole);
        if (user.isValid()) item->setData(user, Qt::UserRole);
    };

    setCell(0, ProcessTableModel::kProcessNameColumn, "bash", "bash");
    setCell(1, ProcessTableModel::kProcessNameColumn, "zsh", "zsh");
    setCell(0, ProcessTableModel::kProcessCPUColumn, "1.0", 1.0);
    setCell(1, ProcessTableModel::kProcessCPUColumn, "2.0", 2.0);
    setCell(0, ProcessTableModel::kProcessUserColumn, "root", QVariant());
    setCell(1, ProcessTableModel::kProcessUserColumn, "uos", QVariant());
    setCell(0, ProcessTableModel::kProcessMemoryColumn, QVariant(), (qulonglong)100);
    setCell(1, ProcessTableModel::kProcessMemoryColumn, QVariant(), (qulonglong)200);
    setCell(0, ProcessTableModel::kProcessShareMemoryColumn, QVariant(), (qulonglong)100);
    setCell(1, ProcessTableModel::kProcessShareMemoryColumn, QVariant(), (qulonglong)200);
    setCell(0, ProcessTableModel::kProcessVTRMemoryColumn, QVariant(), (qulonglong)100);
    setCell(1, ProcessTableModel::kProcessVTRMemoryColumn, QVariant(), (qulonglong)200);
    setCell(0, ProcessTableModel::kProcessPIDColumn, "1000", 1000);
    setCell(1, ProcessTableModel::kProcessPIDColumn, "2000", 2000);
    // apptype 存于 PID 列 UserRole+3，过滤逻辑据此判断
    m->item(0, ProcessTableModel::kProcessPIDColumn)->setData(kFilterApps, Qt::UserRole + 3);
    m->item(1, ProcessTableModel::kProcessPIDColumn)->setData(kFilterApps, Qt::UserRole + 3);
    setCell(0, ProcessTableModel::kProcessNiceColumn, QVariant(), 0);
    setCell(1, ProcessTableModel::kProcessNiceColumn, QVariant(), 5);
    setCell(0, ProcessTableModel::kProcessUploadColumn, QVariant(), 1.0);
    setCell(1, ProcessTableModel::kProcessUploadColumn, QVariant(), 2.0);
    m->item(0, ProcessTableModel::kProcessUploadColumn)->setData((qulonglong)100, Qt::UserRole + 1);
    m->item(1, ProcessTableModel::kProcessUploadColumn)->setData((qulonglong)200, Qt::UserRole + 1);
    setCell(0, ProcessTableModel::kProcessDownloadColumn, QVariant(), 1.0);
    setCell(1, ProcessTableModel::kProcessDownloadColumn, QVariant(), 2.0);
    m->item(0, ProcessTableModel::kProcessDownloadColumn)->setData((qulonglong)100, Qt::UserRole + 1);
    m->item(1, ProcessTableModel::kProcessDownloadColumn)->setData((qulonglong)200, Qt::UserRole + 1);
    setCell(0, ProcessTableModel::kProcessDiskReadColumn, QVariant(), 1.0);
    setCell(1, ProcessTableModel::kProcessDiskReadColumn, QVariant(), 2.0);
    setCell(0, ProcessTableModel::kProcessDiskWriteColumn, QVariant(), 1.0);
    setCell(1, ProcessTableModel::kProcessDiskWriteColumn, QVariant(), 2.0);
    return m;
}

// lessThan 按 PID 列：1000 < 2000 → true
TEST_F(UT_ProcessSortFilterProxyModel, test_lessThan_real_pid)
{
    QStandardItemModel *src = makeProcSourceModel(m_tester);
    m_tester->setSourceModel(src);
    Stub b;
    b.set(ADDR(QSortFilterProxyModel, sortColumn), stub_proclessThan_sortColumn9);
    QModelIndex l = src->index(0, ProcessTableModel::kProcessPIDColumn);
    QModelIndex r = src->index(1, ProcessTableModel::kProcessPIDColumn);
    EXPECT_TRUE(m_tester->lessThan(l, r));
}

// lessThan 按 CPU 列：1.0 < 2.0 → true
TEST_F(UT_ProcessSortFilterProxyModel, test_lessThan_real_cpu)
{
    QStandardItemModel *src = makeProcSourceModel(m_tester);
    m_tester->setSourceModel(src);
    Stub b;
    b.set(ADDR(QSortFilterProxyModel, sortColumn), stub_proclessThan_sortColumn6);
    QModelIndex l = src->index(0, ProcessTableModel::kProcessCPUColumn);
    QModelIndex r = src->index(1, ProcessTableModel::kProcessCPUColumn);
    EXPECT_TRUE(m_tester->lessThan(l, r));
}

// lessThan 按内存列：100 < 200 → true
TEST_F(UT_ProcessSortFilterProxyModel, test_lessThan_real_memory)
{
    QStandardItemModel *src = makeProcSourceModel(m_tester);
    m_tester->setSourceModel(src);
    Stub b;
    b.set(ADDR(QSortFilterProxyModel, sortColumn), stub_proclessThan_sortColumn3);
    QModelIndex l = src->index(0, ProcessTableModel::kProcessMemoryColumn);
    QModelIndex r = src->index(1, ProcessTableModel::kProcessMemoryColumn);
    EXPECT_TRUE(m_tester->lessThan(l, r));
}

// lessThan 按名称列：bash < zsh → true（localeAwareCompare）
TEST_F(UT_ProcessSortFilterProxyModel, test_lessThan_real_name)
{
    QStandardItemModel *src = makeProcSourceModel(m_tester);
    m_tester->setSourceModel(src);
    Stub b;
    b.set(ADDR(QSortFilterProxyModel, sortColumn), stub_proclessThan_sortColumn1);
    QModelIndex l = src->index(0, ProcessTableModel::kProcessNameColumn);
    QModelIndex r = src->index(1, ProcessTableModel::kProcessNameColumn);
    EXPECT_TRUE(m_tester->lessThan(l, r));
}

// lessThan 按用户名列：root < uos → true
TEST_F(UT_ProcessSortFilterProxyModel, test_lessThan_real_user)
{
    QStandardItemModel *src = makeProcSourceModel(m_tester);
    m_tester->setSourceModel(src);
    Stub b;
    b.set(ADDR(QSortFilterProxyModel, sortColumn), stub_proclessThan_sortColumn2);
    QModelIndex l = src->index(0, ProcessTableModel::kProcessUserColumn);
    QModelIndex r = src->index(1, ProcessTableModel::kProcessUserColumn);
    EXPECT_TRUE(m_tester->lessThan(l, r));
}

// lessThan 按上传列：1.0 < 2.0 → true
TEST_F(UT_ProcessSortFilterProxyModel, test_lessThan_real_upload)
{
    QStandardItemModel *src = makeProcSourceModel(m_tester);
    m_tester->setSourceModel(src);
    Stub b;
    b.set(ADDR(QSortFilterProxyModel, sortColumn), stub_proclessThan_sortColumn7);
    QModelIndex l = src->index(0, ProcessTableModel::kProcessUploadColumn);
    QModelIndex r = src->index(1, ProcessTableModel::kProcessUploadColumn);
    EXPECT_TRUE(m_tester->lessThan(l, r));
}

// lessThan 按下载列：1.0 < 2.0 → true
TEST_F(UT_ProcessSortFilterProxyModel, test_lessThan_real_download)
{
    QStandardItemModel *src = makeProcSourceModel(m_tester);
    m_tester->setSourceModel(src);
    Stub b;
    b.set(ADDR(QSortFilterProxyModel, sortColumn), stub_proclessThan_sortColumn8);
    QModelIndex l = src->index(0, ProcessTableModel::kProcessDownloadColumn);
    QModelIndex r = src->index(1, ProcessTableModel::kProcessDownloadColumn);
    EXPECT_TRUE(m_tester->lessThan(l, r));
}

// lessThan 按 nice 列：nice 越大优先级越低，0 < 5 但取反 → 返回 false
TEST_F(UT_ProcessSortFilterProxyModel, test_lessThan_real_nice)
{
    QStandardItemModel *src = makeProcSourceModel(m_tester);
    m_tester->setSourceModel(src);
    Stub b;
    b.set(ADDR(QSortFilterProxyModel, sortColumn), stub_proclessThan_sortColumn12);
    QModelIndex l = src->index(0, ProcessTableModel::kProcessNiceColumn);
    QModelIndex r = src->index(1, ProcessTableModel::kProcessNiceColumn);
    m_tester->lessThan(l, r);  // 仅验证不崩溃（取反逻辑）
}

// filterAcceptsRow：kNoFilter 类型，无搜索词 → 两行均接受
TEST_F(UT_ProcessSortFilterProxyModel, test_filterAcceptsRow_real_nofilter)
{
    QStandardItemModel *src = makeProcSourceModel(m_tester);
    m_tester->setSourceModel(src);
    m_tester->setFilterType(kNoFilter);
    EXPECT_TRUE(m_tester->filterAcceptsRow(0, {}));
    EXPECT_TRUE(m_tester->filterAcceptsRow(1, {}));
}

// filterAcceptsRow：kFilterApps 类型 + apptype=kFilterApps → 接受
TEST_F(UT_ProcessSortFilterProxyModel, test_filterAcceptsRow_real_filterapps)
{
    QStandardItemModel *src = makeProcSourceModel(m_tester);
    m_tester->setSourceModel(src);
    m_tester->setFilterType(kFilterApps);
    EXPECT_TRUE(m_tester->filterAcceptsRow(0, {}));
}

// filterAcceptsRow：设置搜索词 "bash" → 名称匹配 row0，row1("zsh") 不匹配
TEST_F(UT_ProcessSortFilterProxyModel, test_filterAcceptsRow_real_search_match)
{
    QStandardItemModel *src = makeProcSourceModel(m_tester);
    m_tester->setSourceModel(src);
    m_tester->setFilterType(kNoFilter);
    m_tester->setSortFilterString("bash");
    EXPECT_TRUE(m_tester->filterAcceptsRow(0, {}));   // 名称含 "bash"
    EXPECT_FALSE(m_tester->filterAcceptsRow(1, {}));  // "zsh"/"uos"/"2000" 均不含 "bash"
}

// filterAcceptsRow：row 越界 / 无源模型 → 返回 false（边界分支）
TEST_F(UT_ProcessSortFilterProxyModel, test_filterAcceptsRow_real_outofrange)
{
    QStandardItemModel *src = makeProcSourceModel(m_tester);
    m_tester->setSourceModel(src);
    m_tester->setFilterType(kNoFilter);
    EXPECT_FALSE(m_tester->filterAcceptsRow(999, {}));  // row 越界
}
/***************************************真实源模型 end**********************************************/
