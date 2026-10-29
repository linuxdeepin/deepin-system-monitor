// Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "model/process_table_model.h"
#include "process/process.h"
#include "process/private/process_p.h"
#include "process/process_db.h"
#include "common/common.h"
//gtest
#include "stub.h"
#include <gtest/gtest.h>
//Qt
#include <QTimer>
#include <DApplication>

static QString m_Sresult;
/***************************************STUB begin*********************************************/
QList<pid_t> stub_getPIDList()
{
    QList<pid_t> list;
    list.append(1);
    list.append(2);
    list.append(3);
    list.append(4);
    list.append(5);
    list.append(6);
    return list;
}
bool stub_process_data_isValid(){
    m_Sresult = "index is valid";
    return true;
}
int stub_process_data_row(){
    return 0;
}

int stub_process_data_size(){
    return 1;
}

int stub_process_data_column1(){
    m_Sresult = "kProcessNameColumn";
    return ProcessTableModel::kProcessNameColumn;
}

int stub_process_data_column2(){
    m_Sresult = "kProcessCPUColumn";
    return ProcessTableModel::kProcessCPUColumn;
}

int stub_process_data_column3(){
    m_Sresult = "kProcessUserColumn";
    return ProcessTableModel::kProcessUserColumn;
}

int stub_process_data_column4(){
    m_Sresult = "kProcessMemoryColumn";
    return ProcessTableModel::kProcessMemoryColumn;
}

int stub_process_data_column5(){
    m_Sresult = "kProcessShareMemoryColumn";
    return ProcessTableModel::kProcessShareMemoryColumn;
}

int stub_process_data_column6(){
    m_Sresult = "kProcessVTRMemoryColumn";
    return ProcessTableModel::kProcessVTRMemoryColumn;
}

int stub_process_data_column7(){
    m_Sresult = "kProcessUploadColumn";
    return ProcessTableModel::kProcessUploadColumn;
}

int stub_process_data_column8(){
    m_Sresult = "kProcessDownloadColumn";
    return ProcessTableModel::kProcessDownloadColumn;
}

int stub_process_data_column9(){
    m_Sresult = "kProcessDiskReadColumn";
    return ProcessTableModel::kProcessDiskReadColumn;
}

int stub_process_data_column10(){
    m_Sresult = "kProcessDiskWriteColumn";
    return ProcessTableModel::kProcessDiskWriteColumn;
}

int stub_process_data_column11(){
    m_Sresult = "kProcessPIDColumn";
    return ProcessTableModel::kProcessPIDColumn;
}

int stub_process_data_column12(){
    m_Sresult = "kProcessNiceColumn";
    return ProcessTableModel::kProcessNiceColumn;
}

int stub_process_data_column13(){
    m_Sresult = "kProcessPriorityColumn";
    return ProcessTableModel::kProcessPriorityColumn;
}

char stub_process_data_state1(){
    return 'Z';
}

char stub_process_data_state2(){
    return 'T';
}
/***************************************STUB end**********************************************/
class UT_ProcessTableModel: public ::testing::Test
{
public:
    UT_ProcessTableModel() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new ProcessTableModel();
    }

    virtual void TearDown()
    {
        if(m_tester){
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    ProcessTableModel *m_tester;
};

TEST_F(UT_ProcessTableModel, initTest)
{

}

TEST_F(UT_ProcessTableModel, test_getProcessState_001)
{
    pid_t pid = getpid();
    m_tester->getProcessState(pid);

}

TEST_F(UT_ProcessTableModel, test_getProcessState_002)
{
    pid_t pid = 1000;
    m_tester->getProcessState(pid);

}

TEST_F(UT_ProcessTableModel, test_getProcess_001)
{
    pid_t pid = getpid();
    m_tester->getProcess(pid);

}

TEST_F(UT_ProcessTableModel, test_getProcess_002)
{
    pid_t pid = 1000;
    m_tester->getProcessState(pid);
}

TEST_F(UT_ProcessTableModel, test_updateProcessList_001)
{
    m_tester->updateProcessList();
}


TEST_F(UT_ProcessTableModel, test_updateProcessListDelay_001)
{
    m_tester->updateProcessListDelay();
}

TEST_F(UT_ProcessTableModel, test_updateProcessListDelay_002)
{
    Stub stub;
    stub.set(ADDR(ProcessSet, getPIDList), stub_getPIDList);
    m_tester->updateProcessListDelay();
}

TEST_F(UT_ProcessTableModel, test_updateProcessListDelay_003)
{
    Stub stub;
    stub.set(ADDR(ProcessSet, getPIDList), stub_getPIDList);
    m_tester->m_procIdList.append(1);
    Process proc;
    m_tester->m_processList.append(proc);
    m_tester->updateProcessListDelay();
}

TEST_F(UT_ProcessTableModel, test_rowCount_001)
{
    m_tester->rowCount();
}

TEST_F(UT_ProcessTableModel, test_columnCount_001)
{
    int expect = m_tester->columnCount();
    EXPECT_EQ(expect,ProcessTableModel::kProcessColumnCount);
}

TEST_F(UT_ProcessTableModel, test_headerData_001)
{
    int section = ProcessTableModel::kProcessNameColumn;
    int role = Qt::DisplayRole;
    Qt::Orientation orientation = Qt::Horizontal;
    QVariant expect = m_tester->headerData(section,orientation,role);
    EXPECT_EQ(expect,QApplication::translate("Process.Table.Header", kProcessName));
}

TEST_F(UT_ProcessTableModel, test_headerData_002)
{
    int section = ProcessTableModel::kProcessCPUColumn;
    int role = Qt::DisplayRole;
    Qt::Orientation orientation = Qt::Horizontal;
    QVariant expect = m_tester->headerData(section,orientation,role);
    EXPECT_EQ(expect,QApplication::translate("Process.Table.Header", kProcessCPU));
}

TEST_F(UT_ProcessTableModel, test_headerData_003)
{
    int section = ProcessTableModel::kProcessUserColumn;
    int role = Qt::DisplayRole;
    Qt::Orientation orientation = Qt::Horizontal;
    QVariant expect = m_tester->headerData(section,orientation,role);
    EXPECT_EQ(expect,QApplication::translate("Process.Table.Header", kProcessUser));
}

TEST_F(UT_ProcessTableModel, test_headerData_004)
{
    int section = ProcessTableModel::kProcessMemoryColumn;
    int role = Qt::DisplayRole;
    Qt::Orientation orientation = Qt::Horizontal;
    QVariant expect = m_tester->headerData(section,orientation,role);
    EXPECT_EQ(expect,QApplication::translate("Process.Table.Header", kProcessMemory));
}

TEST_F(UT_ProcessTableModel, test_headerData_005)
{
    int section = ProcessTableModel::kProcessShareMemoryColumn;
    int role = Qt::DisplayRole;
    Qt::Orientation orientation = Qt::Horizontal;
    QVariant expect = m_tester->headerData(section,orientation,role);
    EXPECT_EQ(expect,QApplication::translate("Process.Table.Header", kProcessShareMemory));
}

TEST_F(UT_ProcessTableModel, test_headerData_006)
{
    int section = ProcessTableModel::kProcessVTRMemoryColumn;
    int role = Qt::DisplayRole;
    Qt::Orientation orientation = Qt::Horizontal;
    QVariant expect = m_tester->headerData(section,orientation,role);
    EXPECT_EQ(expect,QApplication::translate("Process.Table.Header", kProcessVtrMemory));
}

TEST_F(UT_ProcessTableModel, test_headerData_007)
{
    int section = ProcessTableModel::kProcessUploadColumn;
    int role = Qt::DisplayRole;
    Qt::Orientation orientation = Qt::Horizontal;
    QVariant expect = m_tester->headerData(section,orientation,role);
    EXPECT_EQ(expect,QApplication::translate("Process.Table.Header", kProcessUpload));
}

TEST_F(UT_ProcessTableModel, test_headerData_008)
{
    int section = ProcessTableModel::kProcessDownloadColumn;
    int role = Qt::DisplayRole;
    Qt::Orientation orientation = Qt::Horizontal;
    QVariant expect = m_tester->headerData(section,orientation,role);
    EXPECT_EQ(expect,QApplication::translate("Process.Table.Header", kProcessDownload));
}


TEST_F(UT_ProcessTableModel, test_headerData_009)
{
    int section = ProcessTableModel::kProcessDiskReadColumn;
    int role = Qt::DisplayRole;
    Qt::Orientation orientation = Qt::Horizontal;
    QVariant expect = m_tester->headerData(section,orientation,role);
    EXPECT_EQ(expect,QApplication::translate("Process.Table.Header", kProcessDiskRead));
}

TEST_F(UT_ProcessTableModel, test_headerData_010)
{
    int section = ProcessTableModel::kProcessDiskWriteColumn;
    int role = Qt::DisplayRole;
    Qt::Orientation orientation = Qt::Horizontal;
    QVariant expect = m_tester->headerData(section,orientation,role);
    EXPECT_EQ(expect,QApplication::translate("Process.Table.Header", kProcessDiskWrite));
}

TEST_F(UT_ProcessTableModel, test_headerData_011)
{
    int section = ProcessTableModel::kProcessPIDColumn;
    int role = Qt::DisplayRole;
    Qt::Orientation orientation = Qt::Horizontal;
    QVariant expect = m_tester->headerData(section,orientation,role);
    EXPECT_EQ(expect,QApplication::translate("Process.Table.Header", kProcessPID));
}

TEST_F(UT_ProcessTableModel, test_headerData_012)
{
    int section = ProcessTableModel::kProcessNiceColumn;
    int role = Qt::DisplayRole;
    Qt::Orientation orientation = Qt::Horizontal;
    QVariant expect = m_tester->headerData(section,orientation,role);
    EXPECT_EQ(expect,QApplication::translate("Process.Table.Header", kProcessNice));
}

TEST_F(UT_ProcessTableModel, test_headerData_013)
{
    int section = ProcessTableModel::kProcessPriorityColumn;
    int role = Qt::DisplayRole;
    Qt::Orientation orientation = Qt::Horizontal;
    QVariant expect = m_tester->headerData(section,orientation,role);
    EXPECT_EQ(expect,QApplication::translate("Process.Table.Header", kProcessPriority));
}

TEST_F(UT_ProcessTableModel, test_headerData_014){
    int section = ProcessTableModel::kProcessPriorityColumn;
    int role = Qt::TextAlignmentRole;
    Qt::Orientation orientation = Qt::Horizontal;
    QVariant expect = m_tester->headerData(section,orientation,role);
    EXPECT_EQ(expect,QVariant(Qt::AlignLeft | Qt::AlignVCenter));
}

TEST_F(UT_ProcessTableModel, test_headerData_015)
{
    int section = ProcessTableModel::kProcessPriorityColumn;
    int role = Qt::InitialSortOrderRole;
    Qt::Orientation orientation = Qt::Horizontal;
    QVariant expect = m_tester->headerData(section,orientation,role);
    EXPECT_EQ(expect,QVariant::fromValue(Qt::DescendingOrder));
}

TEST_F(UT_ProcessTableModel, test_data_001)
{
     QModelIndex *index = new QModelIndex();
     int role = Qt::DisplayRole;
     m_tester->data(*index,role);
     delete index;
}

TEST_F(UT_ProcessTableModel, test_data_002)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);
     QModelIndex *index = new QModelIndex();
     int role = Qt::DisplayRole;
     m_tester->data(*index,role);
     delete index;
}

TEST_F(UT_ProcessTableModel, test_data_003)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);
     Stub b2;
     b2.set(ADDR(QModelIndex,row),stub_process_data_row);
     Process *proc = new Process;
     QModelIndex *index = new QModelIndex();

     m_tester->m_processList  << *proc;
     int role = Qt::DisplayRole;
     m_tester->data(*index,role);
     delete proc;
     delete index;

}

TEST_F(UT_ProcessTableModel, test_data_004)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);
     Stub b2;
     b2.set(ADDR(QModelIndex,row),stub_process_data_row);
     Stub b3;
     b3.set(ADDR(Process,isValid),stub_process_data_isValid);
     Process *proc = new Process;
     QModelIndex *index = new QModelIndex();

     m_tester->m_processList  << *proc;
     int role = Qt::DisplayRole;
     m_tester->data(*index,role);

     delete proc;
     delete index;

}

TEST_F(UT_ProcessTableModel, test_data_005)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);
     Stub b2;
     b2.set(ADDR(QModelIndex,row),stub_process_data_row);
     Stub b3;
     b3.set(ADDR(Process,isValid),stub_process_data_isValid);
     Process *proc = new Process;
     QModelIndex *index = new QModelIndex();
     Stub b4;
     b4.set(ADDR(QModelIndex,column),stub_process_data_column1);
     Stub b5;
     b5.set(ADDR(Process,state),stub_process_data_state1);
     m_tester->m_processList  << *proc;
     int role = Qt::DisplayRole;
     m_tester->data(*index,role);
     delete proc;
     delete index;
}

TEST_F(UT_ProcessTableModel, test_data_006)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);
     Stub b2;
     b2.set(ADDR(QModelIndex,row),stub_process_data_row);
     Stub b3;
     b3.set(ADDR(Process,isValid),stub_process_data_isValid);
     Process *proc = new Process;
     QModelIndex *index = new QModelIndex();
     Stub b4;
     b4.set(ADDR(QModelIndex,column),stub_process_data_column1);
     Stub b5;
     b5.set(ADDR(Process,state),stub_process_data_state2);
     m_tester->m_processList  << *proc;
     int role = Qt::DisplayRole;
     m_tester->data(*index,role);
     delete proc;
     delete index;
}


TEST_F(UT_ProcessTableModel, test_data_007)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);
     Stub b2;
     b2.set(ADDR(QModelIndex,row),stub_process_data_row);
     Stub b3;
     b3.set(ADDR(Process,isValid),stub_process_data_isValid);
     Process *proc = new Process;
     QModelIndex *index = new QModelIndex();
     Stub b4;
     b4.set(ADDR(QModelIndex,column),stub_process_data_column2);

     m_tester->m_processList  << *proc;
     int role = Qt::DisplayRole;
     m_tester->data(*index,role);
     delete proc;
     delete index;
}

TEST_F(UT_ProcessTableModel, test_data_008)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);
     Stub b2;
     b2.set(ADDR(QModelIndex,row),stub_process_data_row);
     Stub b3;
     b3.set(ADDR(Process,isValid),stub_process_data_isValid);
     Process *proc = new Process;
     QModelIndex *index = new QModelIndex();
     Stub b4;
     b4.set(ADDR(QModelIndex,column),stub_process_data_column3);

     m_tester->m_processList  << *proc;
     int role = Qt::DisplayRole;
     m_tester->data(*index,role);
     delete proc;
     delete index;
}

TEST_F(UT_ProcessTableModel, test_data_009)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);
     Stub b2;
     b2.set(ADDR(QModelIndex,row),stub_process_data_row);
     Stub b3;
     b3.set(ADDR(Process,isValid),stub_process_data_isValid);
     Process *proc = new Process;
     QModelIndex *index = new QModelIndex();
     Stub b4;
     b4.set(ADDR(QModelIndex,column),stub_process_data_column4);

     m_tester->m_processList  << *proc;
     int role = Qt::DisplayRole;
     m_tester->data(*index,role);
     delete proc;
     delete index;
}

TEST_F(UT_ProcessTableModel, test_data_010)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);
     Stub b2;
     b2.set(ADDR(QModelIndex,row),stub_process_data_row);
     Stub b3;
     b3.set(ADDR(Process,isValid),stub_process_data_isValid);
     Process *proc = new Process;
     QModelIndex *index = new QModelIndex();
     Stub b4;
     b4.set(ADDR(QModelIndex,column),stub_process_data_column5);

     m_tester->m_processList  << *proc;
     int role = Qt::DisplayRole;
     m_tester->data(*index,role);
     delete proc;
     delete index;
}

TEST_F(UT_ProcessTableModel, test_data_011)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);
     Stub b2;
     b2.set(ADDR(QModelIndex,row),stub_process_data_row);
     Stub b3;
     b3.set(ADDR(Process,isValid),stub_process_data_isValid);
     Process *proc = new Process;
     QModelIndex *index = new QModelIndex();
     Stub b4;
     b4.set(ADDR(QModelIndex,column),stub_process_data_column6);

     m_tester->m_processList  << *proc;
     int role = Qt::DisplayRole;
     m_tester->data(*index,role);
     delete proc;
     delete index;
}

TEST_F(UT_ProcessTableModel, test_data_012)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);
     Stub b2;
     b2.set(ADDR(QModelIndex,row),stub_process_data_row);
     Stub b3;
     b3.set(ADDR(Process,isValid),stub_process_data_isValid);
     Process *proc = new Process;
     QModelIndex *index = new QModelIndex();
     Stub b4;
     b4.set(ADDR(QModelIndex,column),stub_process_data_column7);

     m_tester->m_processList  << *proc;
     int role = Qt::DisplayRole;
     m_tester->data(*index,role);
     delete proc;
     delete index;
}

TEST_F(UT_ProcessTableModel, test_data_013)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);
     Stub b2;
     b2.set(ADDR(QModelIndex,row),stub_process_data_row);
     Stub b3;
     b3.set(ADDR(Process,isValid),stub_process_data_isValid);
     Process *proc = new Process;
     QModelIndex *index = new QModelIndex();
     Stub b4;
     b4.set(ADDR(QModelIndex,column),stub_process_data_column8);

     m_tester->m_processList  << *proc;
     int role = Qt::DisplayRole;
     m_tester->data(*index,role);
     delete proc;
     delete index;
}

TEST_F(UT_ProcessTableModel, test_data_014)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);
     Stub b2;
     b2.set(ADDR(QModelIndex,row),stub_process_data_row);
     Stub b3;
     b3.set(ADDR(Process,isValid),stub_process_data_isValid);
     Process *proc = new Process;
     QModelIndex *index = new QModelIndex();
     Stub b4;
     b4.set(ADDR(QModelIndex,column),stub_process_data_column9);

     m_tester->m_processList  << *proc;
     int role = Qt::DisplayRole;
     m_tester->data(*index,role);
     delete proc;
     delete index;
}

TEST_F(UT_ProcessTableModel, test_data_015)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);
     Stub b2;
     b2.set(ADDR(QModelIndex,row),stub_process_data_row);
     Stub b3;
     b3.set(ADDR(Process,isValid),stub_process_data_isValid);
     Process *proc = new Process;
     QModelIndex *index = new QModelIndex();
     Stub b4;
     b4.set(ADDR(QModelIndex,column),stub_process_data_column10);

     m_tester->m_processList  << *proc;
     int role = Qt::DisplayRole;
     m_tester->data(*index,role);
     delete proc;
     delete index;
}

TEST_F(UT_ProcessTableModel, test_data_016)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);
     Stub b2;
     b2.set(ADDR(QModelIndex,row),stub_process_data_row);
     Stub b3;
     b3.set(ADDR(Process,isValid),stub_process_data_isValid);
     Process *proc = new Process;
     QModelIndex *index = new QModelIndex();
     Stub b4;
     b4.set(ADDR(QModelIndex,column),stub_process_data_column11);

     m_tester->m_processList  << *proc;
     int role = Qt::DisplayRole;
     m_tester->data(*index,role);
     delete proc;
     delete index;
}

TEST_F(UT_ProcessTableModel, test_data_017)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);
     Stub b2;
     b2.set(ADDR(QModelIndex,row),stub_process_data_row);
     Stub b3;
     b3.set(ADDR(Process,isValid),stub_process_data_isValid);
     Process *proc = new Process;
     QModelIndex *index = new QModelIndex();
     Stub b4;
     b4.set(ADDR(QModelIndex,column),stub_process_data_column12);

     m_tester->m_processList  << *proc;
     int role = Qt::DisplayRole;
     m_tester->data(*index,role);
     delete proc;
     delete index;
}

TEST_F(UT_ProcessTableModel, test_data_018)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);
     Stub b2;
     b2.set(ADDR(QModelIndex,row),stub_process_data_row);
     Stub b3;
     b3.set(ADDR(Process,isValid),stub_process_data_isValid);
     Process *proc = new Process;
     QModelIndex *index = new QModelIndex();
     Stub b4;
     b4.set(ADDR(QModelIndex,column),stub_process_data_column13);

     m_tester->m_processList  << *proc;
     int role = Qt::DisplayRole;
     m_tester->data(*index,role);
     delete proc;
     delete index;
}

TEST_F(UT_ProcessTableModel, test_data_019)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);
     Stub b2;
     b2.set(ADDR(QModelIndex,row),stub_process_data_row);
     Stub b3;
     b3.set(ADDR(Process,isValid),stub_process_data_isValid);
     Process *proc = new Process;
     QModelIndex *index = new QModelIndex();
     Stub b4;
     b4.set(ADDR(QModelIndex,column),stub_process_data_column1);

     m_tester->m_processList  << *proc;
     int role = Qt::DecorationRole;
     m_tester->data(*index,role);
     delete proc;
     delete index;
}

TEST_F(UT_ProcessTableModel, test_data_020)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);
     Stub b2;
     b2.set(ADDR(QModelIndex,row),stub_process_data_row);
     Stub b3;
     b3.set(ADDR(Process,isValid),stub_process_data_isValid);
     Process *proc = new Process;
     QModelIndex *index = new QModelIndex();
     Stub b4;
     b4.set(ADDR(QModelIndex,column),stub_process_data_column1);

     m_tester->m_processList  << *proc;
     int role = Qt::UserRole;
     m_tester->data(*index,role);
     delete proc;
     delete index;
}

TEST_F(UT_ProcessTableModel, test_data_021)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);
     Stub b2;
     b2.set(ADDR(QModelIndex,row),stub_process_data_row);
     Stub b3;
     b3.set(ADDR(Process,isValid),stub_process_data_isValid);
     Process *proc = new Process;
     QModelIndex *index = new QModelIndex();
     Stub b4;
     b4.set(ADDR(QModelIndex,column),stub_process_data_column4);

     m_tester->m_processList  << *proc;
     int role = Qt::UserRole;
     m_tester->data(*index,role);
     delete proc;
     delete index;
}

TEST_F(UT_ProcessTableModel, test_data_022)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);
     Stub b2;
     b2.set(ADDR(QModelIndex,row),stub_process_data_row);
     Stub b3;
     b3.set(ADDR(Process,isValid),stub_process_data_isValid);
     Process *proc = new Process;
     QModelIndex *index = new QModelIndex();
     Stub b4;
     b4.set(ADDR(QModelIndex,column),stub_process_data_column5);

     m_tester->m_processList  << *proc;
     int role = Qt::UserRole;
     m_tester->data(*index,role);
     delete proc;
     delete index;
}

TEST_F(UT_ProcessTableModel, test_data_023)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);
     Stub b2;
     b2.set(ADDR(QModelIndex,row),stub_process_data_row);
     Stub b3;
     b3.set(ADDR(Process,isValid),stub_process_data_isValid);
     Process *proc = new Process;
     QModelIndex *index = new QModelIndex();
     Stub b4;
     b4.set(ADDR(QModelIndex,column),stub_process_data_column6);

     m_tester->m_processList  << *proc;
     int role = Qt::UserRole;

     delete proc;
     delete index;
}

TEST_F(UT_ProcessTableModel, test_data_024)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);
     Stub b2;
     b2.set(ADDR(QModelIndex,row),stub_process_data_row);
     Stub b3;
     b3.set(ADDR(Process,isValid),stub_process_data_isValid);
     Process *proc = new Process;
     QModelIndex *index = new QModelIndex();
     Stub b4;
     b4.set(ADDR(QModelIndex,column),stub_process_data_column2);

     m_tester->m_processList  << *proc;
     int role = Qt::UserRole;
     m_tester->data(*index,role);

     delete proc;
     delete index;
}

TEST_F(UT_ProcessTableModel, test_data_025)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);
     Stub b2;
     b2.set(ADDR(QModelIndex,row),stub_process_data_row);
     Stub b3;
     b3.set(ADDR(Process,isValid),stub_process_data_isValid);
     Process *proc = new Process;
     QModelIndex *index = new QModelIndex();
     Stub b4;
     b4.set(ADDR(QModelIndex,column),stub_process_data_column7);

     m_tester->m_processList  << *proc;
     int role = Qt::UserRole;
     m_tester->data(*index,role);

     delete proc;
     delete index;
}

TEST_F(UT_ProcessTableModel, test_data_026)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);
     Stub b2;
     b2.set(ADDR(QModelIndex,row),stub_process_data_row);
     Stub b3;
     b3.set(ADDR(Process,isValid),stub_process_data_isValid);
     Process *proc = new Process;
     QModelIndex *index = new QModelIndex();
     Stub b4;
     b4.set(ADDR(QModelIndex,column),stub_process_data_column8);

     m_tester->m_processList  << *proc;
     int role = Qt::UserRole;
     m_tester->data(*index,role);

     delete proc;
     delete index;
}

TEST_F(UT_ProcessTableModel, test_data_027)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);
     Stub b2;
     b2.set(ADDR(QModelIndex,row),stub_process_data_row);
     Stub b3;
     b3.set(ADDR(Process,isValid),stub_process_data_isValid);
     Process *proc = new Process;
     QModelIndex *index = new QModelIndex();
     Stub b4;
     b4.set(ADDR(QModelIndex,column),stub_process_data_column11);

     m_tester->m_processList  << *proc;
     int role = Qt::UserRole;
     m_tester->data(*index,role);

     delete proc;
     delete index;
}

TEST_F(UT_ProcessTableModel, test_data_028)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);
     Stub b2;
     b2.set(ADDR(QModelIndex,row),stub_process_data_row);
     Stub b3;
     b3.set(ADDR(Process,isValid),stub_process_data_isValid);
     Process *proc = new Process;
     QModelIndex *index = new QModelIndex();
     Stub b4;
     b4.set(ADDR(QModelIndex,column),stub_process_data_column9);

     m_tester->m_processList  << *proc;
     int role = Qt::UserRole;
     m_tester->data(*index,role);

     delete proc;
     delete index;
}

TEST_F(UT_ProcessTableModel, test_data_029)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);
     Stub b2;
     b2.set(ADDR(QModelIndex,row),stub_process_data_row);
     Stub b3;
     b3.set(ADDR(Process,isValid),stub_process_data_isValid);
     Process *proc = new Process;
     QModelIndex *index = new QModelIndex();
     Stub b4;
     b4.set(ADDR(QModelIndex,column),stub_process_data_column10);

     m_tester->m_processList  << *proc;
     int role = Qt::UserRole;
     m_tester->data(*index,role);

     delete proc;
     delete index;
}

TEST_F(UT_ProcessTableModel, test_data_030)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);
     Stub b2;
     b2.set(ADDR(QModelIndex,row),stub_process_data_row);
     Stub b3;
     b3.set(ADDR(Process,isValid),stub_process_data_isValid);
     Process *proc = new Process;
     QModelIndex *index = new QModelIndex();
     Stub b4;
     b4.set(ADDR(QModelIndex,column),stub_process_data_column12);

     m_tester->m_processList  << *proc;
     int role = Qt::UserRole;
     m_tester->data(*index,role);

     delete proc;
     delete index;
}

TEST_F(UT_ProcessTableModel, test_data_031)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);
     Stub b2;
     b2.set(ADDR(QModelIndex,row),stub_process_data_row);
     Stub b3;
     b3.set(ADDR(Process,isValid),stub_process_data_isValid);
     Process *proc = new Process;
     QModelIndex *index = new QModelIndex();
     Stub b4;
     b4.set(ADDR(QModelIndex,column),stub_process_data_column7);

     m_tester->m_processList  << *proc;
     int role = (Qt::UserRole + 1);
     m_tester->data(*index,role);

     delete proc;
     delete index;
}

TEST_F(UT_ProcessTableModel, test_data_032)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);
     Stub b2;
     b2.set(ADDR(QModelIndex,row),stub_process_data_row);
     Stub b3;
     b3.set(ADDR(Process,isValid),stub_process_data_isValid);
     Process *proc = new Process;
     QModelIndex *index = new QModelIndex();
     Stub b4;
     b4.set(ADDR(QModelIndex,column),stub_process_data_column8);

     m_tester->m_processList  << *proc;
     int role = (Qt::UserRole + 1);
     m_tester->data(*index,role);

     delete proc;
     delete index;
}

TEST_F(UT_ProcessTableModel, test_data_033)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);
     Stub b2;
     b2.set(ADDR(QModelIndex,row),stub_process_data_row);
     Stub b3;
     b3.set(ADDR(Process,isValid),stub_process_data_isValid);
     Process *proc = new Process;
     QModelIndex *index = new QModelIndex();
     Stub b4;
     b4.set(ADDR(QModelIndex,column),stub_process_data_column8);

     m_tester->m_processList  << *proc;
     int role = Qt::TextAlignmentRole;
     QVariant expect = m_tester->data(*index,role);

     delete proc;
     delete index;
}

TEST_F(UT_ProcessTableModel, test_data_034)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);
     Stub b2;
     b2.set(ADDR(QModelIndex,row),stub_process_data_row);
     Stub b3;
     b3.set(ADDR(Process,isValid),stub_process_data_isValid);
     Process *proc = new Process;
     QModelIndex *index = new QModelIndex();
     Stub b4;
     b4.set(ADDR(QModelIndex,column),stub_process_data_column1);
     Stub b5;
     b5.set(ADDR(Process,state),stub_process_data_state1);
     m_tester->m_processList  << *proc;
     int role = Qt::UserRole + 2;
     QVariant expect = m_tester->data(*index,role);

     delete proc;
     delete index;
}

TEST_F(UT_ProcessTableModel, test_data_035)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);
     Stub b2;
     b2.set(ADDR(QModelIndex,row),stub_process_data_row);
     Stub b3;
     b3.set(ADDR(Process,isValid),stub_process_data_isValid);
     Process *proc = new Process;
     QModelIndex *index = new QModelIndex();
     Stub b4;
     b4.set(ADDR(QModelIndex,column),stub_process_data_column1);

     m_tester->m_processList  << *proc;
     int role = Qt::UserRole + 4;
     m_tester->data(*index,role);

     delete proc;
     delete index;
}

TEST_F(UT_ProcessTableModel, test_flags_001)
{
     Stub b1;
     b1.set(ADDR(QModelIndex,isValid),stub_process_data_isValid);

     QModelIndex *index = new QModelIndex();

     m_tester->flags(*index);

     delete index;
}

TEST_F(UT_ProcessTableModel, test_flags_002)
{
     QModelIndex *index = new QModelIndex();
     m_tester->flags(*index);

     delete index;
}

TEST_F(UT_ProcessTableModel, test_getProcessPriority_001)
{
     pid_t pid = getpid();
     m_tester->m_procIdList << pid;
     m_tester->getProcessPriority(pid);
}

TEST_F(UT_ProcessTableModel, test_getProcessPriority_002)
{
     pid_t pid = -1;
     ProcessPriority expect = m_tester->getProcessPriority(pid);
     EXPECT_EQ(expect,kInvalidPriority);
}

TEST_F(UT_ProcessTableModel, test_removeProcess_001)
{
     pid_t pid = getpid();
     Process proc(pid);
     m_tester->m_procIdList << pid;
     m_tester->m_processList << proc;
     m_tester->removeProcess(pid);
}

TEST_F(UT_ProcessTableModel, test_updateProcessState_001)
{
     pid_t pid = getpid();
     Process proc(pid);
     char state = 'Z';
     m_tester->m_procIdList << pid;
     m_tester->m_processList << proc;

     m_tester->updateProcessState(pid,state);

}

TEST_F(UT_ProcessTableModel, test_updateProcessPriority_001)
{
     pid_t pid = getpid();
     Process proc(pid);
     int priority = 0;
     m_tester->m_procIdList << pid;
     m_tester->m_processList << proc;

     m_tester->updateProcessPriority(pid,priority);

}

// getProcessPriorityValue：pid 不在列表 → 兜底 kNormalPriority(0)
TEST_F(UT_ProcessTableModel, test_getProcessPriorityValue_001)
{
    pid_t pid = 999999; // 不在 m_procIdList 中
    EXPECT_EQ(m_tester->getProcessPriorityValue(pid), kNormalPriority);
}

// getProcessPriorityValue：pid 在列表中 → 返回该进程的 priority
TEST_F(UT_ProcessTableModel, test_getProcessPriorityValue_002)
{
    pid_t pid = getpid(); // 当前测试进程真实存在
    m_tester->m_procIdList << pid;
    // 仅断言能取到非异常值（覆盖 found 分支），具体 nice 值随环境
    int val = m_tester->getProcessPriorityValue(pid);
    EXPECT_TRUE(val >= -20 && val <= 19);
}

// setUserModeName：设置相同名称走守卫提前返回分支（不触发刷新）
TEST_F(UT_ProcessTableModel, test_setUserModeName_001)
{
    m_tester->setUserModeName(""); // 首次设置
    m_tester->setUserModeName(""); // 相同值 → 守卫提前返回
}

// getTotalCPUUsage：空列表为 0；注入带 cpu 的进程后求和
TEST_F(UT_ProcessTableModel, test_getTotalCPUUsage_001)
{
    EXPECT_NEAR(m_tester->getTotalCPUUsage(), 0.0, 0.001);

    Process p1, p2;
    p1.setCpu(10.5);
    p2.setCpu(20.0);
    m_tester->m_processList << p1 << p2;
    EXPECT_NEAR(m_tester->getTotalCPUUsage(), 30.5, 0.001);
}

// getTotalMemoryUsage
TEST_F(UT_ProcessTableModel, test_getTotalMemoryUsage_001)
{
    EXPECT_NEAR(m_tester->getTotalMemoryUsage(), 0.0, 0.001);

    Process p1, p2;
    p1.setMemory(1024);
    p2.setMemory(2048);
    m_tester->m_processList << p1 << p2;
    EXPECT_NEAR(m_tester->getTotalMemoryUsage(), 3072.0, 0.001);
}

// getTotalDownload / getTotalUpload：setNetIoBps 同时设置 recv/send
TEST_F(UT_ProcessTableModel, test_getTotalDownload_001)
{
    Process p1, p2;
    p1.setNetIoBps(100.0, 50.0); // recv=100, send=50
    p2.setNetIoBps(200.0, 25.0); // recv=200, send=25
    m_tester->m_processList << p1 << p2;
    EXPECT_NEAR(m_tester->getTotalDownload(), 300.0, 0.001);
}

TEST_F(UT_ProcessTableModel, test_getTotalUpload_001)
{
    Process p1, p2;
    p1.setNetIoBps(100.0, 50.0);
    p2.setNetIoBps(200.0, 25.0);
    m_tester->m_processList << p1 << p2;
    EXPECT_NEAR(m_tester->getTotalUpload(), 75.0, 0.001);
}

// vtrmemory/sharememory/readBps/writeBps 无 setter，默认 0；注入进程覆盖循环体
TEST_F(UT_ProcessTableModel, test_getTotalVirtualMemoryUsage_001)
{
    m_tester->m_processList << Process() << Process();
    EXPECT_NEAR(m_tester->getTotalVirtualMemoryUsage(), 0.0, 0.001);
}

TEST_F(UT_ProcessTableModel, test_getTotalSharedMemoryUsage_001)
{
    m_tester->m_processList << Process() << Process();
    EXPECT_NEAR(m_tester->getTotalSharedMemoryUsage(), 0.0, 0.001);
}

TEST_F(UT_ProcessTableModel, test_getTotalDiskRead_001)
{
    m_tester->m_processList << Process() << Process();
    EXPECT_NEAR(m_tester->getTotalDiskRead(), 0.0, 0.001);
}

TEST_F(UT_ProcessTableModel, test_getTotalDiskWrite_001)
{
    m_tester->m_processList << Process() << Process();
    EXPECT_NEAR(m_tester->getTotalDiskWrite(), 0.0, 0.001);
}

// ========== 新增：用真实 index 直接驱动 data() 全部分支（不依赖 stub 顺序，更稳定） ==========
// 这些测试覆盖 DisplayRole/DecorationRole/UserRole/UserRole+1/TextAlignmentRole/UserRole+2
// 以及各 column 的 switch 分支，避免既有 stub 版测试的 flakiness。
namespace {
Process makeValidProcess()
{
    Process p(getpid());
    p.d->valid = true;  // 标记为有效，使 data() 越过 isValid 早退
    p.setCpu(12.5);
    p.setMemory(4096);
    p.setPriority(0);
    p.setState('R');
    p.setName("utproc");
    p.setDisplayName("utproc");
    p.setNetIoBps(100.0, 200.0);
    p.setUserName("utuser");
    return p;
}
} // namespace

TEST_F(UT_ProcessTableModel, test_data_displayRole_allColumns_realIndex)
{
    Process p = makeValidProcess();
    m_tester->m_processList << p;

    int cols[] = {
        ProcessTableModel::kProcessNameColumn,
        ProcessTableModel::kProcessCPUColumn,
        ProcessTableModel::kProcessUserColumn,
        ProcessTableModel::kProcessMemoryColumn,
        ProcessTableModel::kProcessShareMemoryColumn,
        ProcessTableModel::kProcessVTRMemoryColumn,
        ProcessTableModel::kProcessUploadColumn,
        ProcessTableModel::kProcessDownloadColumn,
        ProcessTableModel::kProcessDiskReadColumn,
        ProcessTableModel::kProcessDiskWriteColumn,
        ProcessTableModel::kProcessPIDColumn,
        ProcessTableModel::kProcessNiceColumn,
        ProcessTableModel::kProcessPriorityColumn,
    };
    for (int c : cols) {
        QModelIndex idx = m_tester->createIndex(0, c);
        QVariant v = m_tester->data(idx, Qt::DisplayRole);
        EXPECT_TRUE(v.isValid()) << "DisplayRole col=" << c;
    }
}

TEST_F(UT_ProcessTableModel, test_data_displayRole_nameColumn_zombieState)
{
    Process p = makeValidProcess();
    p.setState('Z');
    m_tester->m_processList << p;
    QModelIndex idx = m_tester->createIndex(0, ProcessTableModel::kProcessNameColumn);
    QVariant v = m_tester->data(idx, Qt::DisplayRole);
    EXPECT_TRUE(v.isValid());
    EXPECT_TRUE(v.toString().contains("utproc"));
}

TEST_F(UT_ProcessTableModel, test_data_displayRole_nameColumn_suspendedState)
{
    Process p = makeValidProcess();
    p.setState('T');
    m_tester->m_processList << p;
    QModelIndex idx = m_tester->createIndex(0, ProcessTableModel::kProcessNameColumn);
    QVariant v = m_tester->data(idx, Qt::DisplayRole);
    EXPECT_TRUE(v.isValid());
}

TEST_F(UT_ProcessTableModel, test_data_displayRole_nameColumn_normalState)
{
    Process p = makeValidProcess();
    p.setState('R'); // 非 Z/T，不进入状态修饰分支
    m_tester->m_processList << p;
    QModelIndex idx = m_tester->createIndex(0, ProcessTableModel::kProcessNameColumn);
    QVariant v = m_tester->data(idx, Qt::DisplayRole);
    EXPECT_EQ(v.toString(), QString("utproc"));
}

TEST_F(UT_ProcessTableModel, test_data_decorationRole_nameColumn)
{
    Process p = makeValidProcess();
    m_tester->m_processList << p;
    QModelIndex idx = m_tester->createIndex(0, ProcessTableModel::kProcessNameColumn);
    // decoration 分支，icon 可能为空但不崩
    m_tester->data(idx, Qt::DecorationRole);
}

TEST_F(UT_ProcessTableModel, test_data_decorationRole_otherColumn_returnsInvalid)
{
    Process p = makeValidProcess();
    m_tester->m_processList << p;
    QModelIndex idx = m_tester->createIndex(0, ProcessTableModel::kProcessCPUColumn);
    QVariant v = m_tester->data(idx, Qt::DecorationRole);
    EXPECT_FALSE(v.isValid());
}

TEST_F(UT_ProcessTableModel, test_data_userRole_allColumns)
{
    Process p = makeValidProcess();
    m_tester->m_processList << p;
    int cols[] = {
        ProcessTableModel::kProcessNameColumn,
        ProcessTableModel::kProcessMemoryColumn,
        ProcessTableModel::kProcessShareMemoryColumn,
        ProcessTableModel::kProcessVTRMemoryColumn,
        ProcessTableModel::kProcessCPUColumn,
        ProcessTableModel::kProcessUploadColumn,
        ProcessTableModel::kProcessDownloadColumn,
        ProcessTableModel::kProcessPIDColumn,
        ProcessTableModel::kProcessDiskReadColumn,
        ProcessTableModel::kProcessDiskWriteColumn,
        ProcessTableModel::kProcessNiceColumn,
    };
    for (int c : cols) {
        QModelIndex idx = m_tester->createIndex(0, c);
        QVariant v = m_tester->data(idx, Qt::UserRole);
        EXPECT_TRUE(v.isValid()) << "UserRole col=" << c;
    }
}

TEST_F(UT_ProcessTableModel, test_data_userRolePlus1_uploadDownload)
{
    Process p = makeValidProcess();
    m_tester->m_processList << p;
    QModelIndex idxU = m_tester->createIndex(0, ProcessTableModel::kProcessUploadColumn);
    EXPECT_TRUE(m_tester->data(idxU, Qt::UserRole + 1).isValid());
    QModelIndex idxD = m_tester->createIndex(0, ProcessTableModel::kProcessDownloadColumn);
    EXPECT_TRUE(m_tester->data(idxD, Qt::UserRole + 1).isValid());
    // 其它列返回无效
    QModelIndex idxOther = m_tester->createIndex(0, ProcessTableModel::kProcessCPUColumn);
    EXPECT_FALSE(m_tester->data(idxOther, Qt::UserRole + 1).isValid());
}

TEST_F(UT_ProcessTableModel, test_data_textAlignmentRole)
{
    Process p = makeValidProcess();
    m_tester->m_processList << p;
    QModelIndex idx = m_tester->createIndex(0, ProcessTableModel::kProcessNameColumn);
    QVariant v = m_tester->data(idx, Qt::TextAlignmentRole);
    EXPECT_TRUE(v.isValid());
}

TEST_F(UT_ProcessTableModel, test_data_invalidProcess_returnsInvalid)
{
    // 默认构造的 Process isValid()=false → data 返回空
    Process p;
    m_tester->m_processList << p;
    QModelIndex idx = m_tester->createIndex(0, ProcessTableModel::kProcessNameColumn);
    QVariant v = m_tester->data(idx, Qt::DisplayRole);
    EXPECT_FALSE(v.isValid());
}

TEST_F(UT_ProcessTableModel, test_data_outOfBoundsRow_returnsInvalid)
{
    Process p = makeValidProcess();
    m_tester->m_processList << p;
    QModelIndex idx = m_tester->createIndex(100, 0);
    QVariant v = m_tester->data(idx, Qt::DisplayRole);
    EXPECT_FALSE(v.isValid());
}

