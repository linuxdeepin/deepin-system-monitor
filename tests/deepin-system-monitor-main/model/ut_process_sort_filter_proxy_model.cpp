/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     lishiqi <lishiqi@uniontech.com>
*
* Maintainer: lishiqi  <lishiqi@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
//self

#include "model/process_sort_filter_proxy_model.h"
#include "process/process_db.h"
#include "model/process_table_model.h"
#include "common/han_latin.h"
#include "common/common.h"


//gtest
#include "stub.h"
#include <gtest/gtest.h>
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
        delete m_tester;
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
