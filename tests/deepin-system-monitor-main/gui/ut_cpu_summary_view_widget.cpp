/*
* Copyright (C) 2019 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     yukuan  <yukuan@uniontech.com>
*
* Maintainer: yukuan  <yukuan@uniontech.com>
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

//Self
#include "cpu_summary_view_widget.h"
#include "model/cpu_info_model.h"
#include "system/cpu_set.h"
//gtest
#include "stub.h"
#include <gtest/gtest.h>
#include "addr_pri.h"

//Qt
#include <QSignalSpy>
#include <QApplication>
#include <DApplicationHelper>

/***************************************STUB begin*********************************************/
bool stub_CPUSummaryTableModel_data_isValid_true()
{
    return true;
}

int stub_CPUSummaryTableModel_data_index_row_0()
{
    return 0;
}

int stub_CPUSummaryTableModel_data_index_row_1()
{
    return 1;
}

int stub_CPUSummaryTableModel_data_index_row_2()
{
    return 2;
}

int stub_CPUSummaryTableModel_data_index_row_3()
{
    return 3;
}

int stub_CPUSummaryTableModel_data_index_row_4()
{
    return 4;
}

int stub_CPUSummaryTableModel_data_index_row_5()
{
    return 5;
}

int stub_CPUSummaryTableModel_data_index_row_6()
{
    return 6;
}

int stub_CPUSummaryTableModel_data_index_column_0()
{
    return 0;
}

int stub_CPUSummaryTableModel_data_index_column_1()
{
    return 1;
}
/***************************************STUB end**********************************************/

class UT_CPUDetailSummaryTable : public ::testing::Test
{
public:
    UT_CPUDetailSummaryTable() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        static CPUInfoModel model;
        m_tester = new CPUDetailSummaryTable(&model, nullptr);
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    CPUDetailSummaryTable *m_tester;
};

TEST_F(UT_CPUDetailSummaryTable, initTest)
{

}

TEST_F(UT_CPUDetailSummaryTable, test_fontChanged_01)
{
    QFont font;
    font.setBold(true);
    m_tester->fontChanged(font);

    EXPECT_EQ(m_tester->m_font.bold(), font.bold());
}

TEST_F(UT_CPUDetailSummaryTable, test_onModelUpdate_01)
{
    m_tester->onModelUpdate();
}

TEST_F(UT_CPUDetailSummaryTable, test_paintEvent_01)
{
    EXPECT_FALSE(m_tester->grab().isNull());
}


///////////////////////////CLASS MODEL//////////////////////////////////
using namespace core::system;
using namespace common::format;

class CPUSummaryTableModel : public QAbstractTableModel
{
public:
    explicit CPUSummaryTableModel(CPUInfoModel *m_model, QObject *parent = nullptr);

    virtual ~CPUSummaryTableModel();

private:
    CPUInfoModel *m_model;

protected:
    int rowCount(const QModelIndex &) const
    {
        return 10;
    }

    int columnCount(const QModelIndex &) const
    {
        return 2;
    }

    QVariant data(const QModelIndex &index, int role) const
    {
        if (!index.isValid())
            return QVariant();

        int row = index.row();
        int column = index.column();

        if (role == Qt::DisplayRole) {
            switch (row) {
            case 0:
                if (column == 0)
                    return QApplication::translate("CPUSummaryTableModel", "Utilization"); //百分比显示，为CPU的总体利用率，显示精度为1%，2秒刷新1次；
                else if (column == 1)
                    return QApplication::translate("CPUSummaryTableModel", "Current frequency");//显示当前CPU的实际运行速度，单位说明：如果当前CPU速度大于1GHz，单位为GHz；如果当前CPU速度小于1GHz，显示单位为MHz；
                break;

            case 1:
                if (column == 0)    //m_model->cpuSet()->modelName()); //CPU属于的名字及其编号、标称主频；
                    return QApplication::translate("CPUSummaryTableModel", "Frequency");//最小频率  ~ 最大频率；
                else if (column == 1)
                    return QApplication::translate("CPUSummaryTableModel", "Vendor");//显示制造商名称。格式：字串
                break;
            case 2:
                if (column == 0)    //m_model->cpuSet()->coreId(0));   //处理器ID
                    return QApplication::translate("CPUSummaryTableModel", "Sockets"); //插槽
                else if (column == 1)
                    return QApplication::translate("CPUSummaryTableModel", "Logical processors"); //逻辑处理器数量；（格式：数字）
                break;
            case 3:
                if (column == 0)
                    return QApplication::translate("CPUSummaryTableModel", "Virtualization"); //虚拟化机制；（Intel 的VT-x，AMD的AMD-v），如果当前CPU不支持虚拟化，显示“不支持”；
                else if (column == 1)
                    return QApplication::translate("CPUSummaryTableModel", "L1i cache");//L1缓存（指令）：1级缓存大小；（单位：小于1K，用B；小于1M，用KB；大于1M：用MB；）
                break;
            case 4:
                if (column == 0)
                    return QApplication::translate("CPUSummaryTableModel", "L1d cache");//L1缓存（数据）：1级缓存大小；（单位：小于1K，用B；小于1M，用KB；大于1M：用MB；）
                else if (column == 1)
                    return QApplication::translate("CPUSummaryTableModel", "L2 cache");//L2缓存：2级缓存大小；（单位：小于1K，用B；小于1M，用KB；大于1M：用MB；）
                break;
            case 5:
                if (column == 0)
                    return QApplication::translate("CPUSummaryTableModel", "L3 cache");//L3缓存：3级缓存大小；（单位：小于1K，用B；小于1M，用KB；大于1M：用MB；）
                else if (column == 1)
                    return QApplication::translate("CPUSummaryTableModel", "Load average"); //负载均衡：Load Average 就是一段时间 (1 分钟、5分钟、15分钟) 内平均 Load；
                break;
            case 6:
                if (column == 0)
                    return QApplication::translate("CPUSummaryTableModel", "File descriptors");//文件描述符数
                else if (column == 1)
                    return QApplication::translate("CPUSummaryTableModel", "Processes"); //进程数量（格式：数字）
                break;
            case 7:
                if (column == 0)
                    return QApplication::translate("CPUSummaryTableModel", "Threads");//线程数量（格式：数字）
                else if (column == 1)
                    return QApplication::translate("CPUSummaryTableModel", "Host name");//显示主机名称。（格式：字符串）
                break;
            case 8:
                if (column == 0)
                    return QApplication::translate("CPUSummaryTableModel", "OS type");//系统类型
                else if (column == 1)
                    return QApplication::translate("CPUSummaryTableModel", "Version"); //版本号
                break;
            case 9:
                if (column == 0)
                    return QApplication::translate("CPUSummaryTableModel", "Up time");//最近一次开机到目前的运行时间。格式 天（DDDD）：小时（HH）：分钟（MM），60分自动进位到1小时；24小时自动进位为1天；最大支持 9999天；
                break;
            default:
                break;
            }
        } else if (role == Qt::UserRole) {
            switch (row) {
            case 0:
                if (column == 0)
                    return  QString::number(m_model->cpuAllPercent(), 'f', 0) + "%";
                else if (column == 1)
                    return m_model->cpuSet()->curFreq();
                break;
            case 1:
                if (column == 0)
                    return m_model->cpuSet()->minFreq() + " ~ " + m_model->cpuSet()->maxFreq();
                else if (column == 1)
                    return m_model->cpuSet()->vendor();
                break;
            case 2:
                if (column == 0)
                    return QString::number(m_model->cpuSet()->socketCount());
                else if (column == 1)
                    return QString::number(m_model->cpuSet()->cpuCount());
                break;
            case 3:
                if (column == 0)
                    return m_model->cpuSet()->virtualization();
                else if (column == 1)
                    return m_model->cpuSet()->l1iCache();
                break;
            case 4:
                if (column == 0)
                    return m_model->cpuSet()->l1dCache();
                else if (column == 1)
                    return m_model->cpuSet()->l2Cache();
                break;
            case 5:
                if (column == 0)
                    return m_model->cpuSet()->l3Cache();
                else if (column == 1)
                    return m_model->loadavg();
                break;
            case 6:
                if (column == 0)
                    return QString::number(m_model->nFileDescriptors());
                else if (column == 1)
                    return  QString::number(m_model->nProcesses());
                break;
            case 7:
                if (column == 0)
                    return QString::number(m_model->nThreads());
                else if (column == 1)
                    return  m_model->hostname();
                break;
            case 8:
                if (column == 0)
                    return m_model->osType();
                else if (column == 1)
                    return m_model->osVersion();
                break;
            case 9:
                if (column == 0)
                    return m_model->uptime();
                break;
            default:
                break;
            }
        } else if (role == Qt::TextColorRole) {
            const auto &palette = DApplicationHelper::instance()->applicationPalette();
            return palette.color(DPalette::Text);
        }

        return QVariant();
    }

    Qt::ItemFlags flags(const QModelIndex &) const
    {
        return Qt::NoItemFlags;
    }

private slots:
    void onModelUpdated();

};

class UT_CPUSummaryTableModel : public ::testing::Test
{
public:
    UT_CPUSummaryTableModel() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        static CPUInfoModel model;
        m_tester = new CPUSummaryTableModel(&model);
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    CPUSummaryTableModel *m_tester;
};

TEST_F(UT_CPUSummaryTableModel, initTest)
{

}


TEST_F(UT_CPUSummaryTableModel, test_data_01)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_CPUSummaryTableModel_data_isValid_true);
    stub.set(ADDR(QModelIndex, row), stub_CPUSummaryTableModel_data_index_row_0);
    stub.set(ADDR(QModelIndex, column), stub_CPUSummaryTableModel_data_index_column_0);
    QModelIndex index;
    m_tester->data(index, Qt::DisplayRole);
}


TEST_F(UT_CPUSummaryTableModel, test_data_02)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_CPUSummaryTableModel_data_isValid_true);
    stub.set(ADDR(QModelIndex, row), stub_CPUSummaryTableModel_data_index_row_0);
    stub.set(ADDR(QModelIndex, column), stub_CPUSummaryTableModel_data_index_column_1);
    QModelIndex index;
    m_tester->data(index, Qt::DisplayRole);
}

TEST_F(UT_CPUSummaryTableModel, test_data_03)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_CPUSummaryTableModel_data_isValid_true);
    stub.set(ADDR(QModelIndex, row), stub_CPUSummaryTableModel_data_index_row_1);
    stub.set(ADDR(QModelIndex, column), stub_CPUSummaryTableModel_data_index_column_0);
    QModelIndex index;
    m_tester->data(index, Qt::DisplayRole);
}


TEST_F(UT_CPUSummaryTableModel, test_data_04)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_CPUSummaryTableModel_data_isValid_true);
    stub.set(ADDR(QModelIndex, row), stub_CPUSummaryTableModel_data_index_row_1);
    stub.set(ADDR(QModelIndex, column), stub_CPUSummaryTableModel_data_index_column_1);
    QModelIndex index;
    m_tester->data(index, Qt::DisplayRole);
}

TEST_F(UT_CPUSummaryTableModel, test_data_05)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_CPUSummaryTableModel_data_isValid_true);
    stub.set(ADDR(QModelIndex, row), stub_CPUSummaryTableModel_data_index_row_2);
    stub.set(ADDR(QModelIndex, column), stub_CPUSummaryTableModel_data_index_column_0);
    QModelIndex index;
    m_tester->data(index, Qt::DisplayRole);
}

TEST_F(UT_CPUSummaryTableModel, test_data_06)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_CPUSummaryTableModel_data_isValid_true);
    stub.set(ADDR(QModelIndex, row), stub_CPUSummaryTableModel_data_index_row_2);
    stub.set(ADDR(QModelIndex, column), stub_CPUSummaryTableModel_data_index_column_1);
    QModelIndex index;
    m_tester->data(index, Qt::DisplayRole);
}

TEST_F(UT_CPUSummaryTableModel, test_data_07)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_CPUSummaryTableModel_data_isValid_true);
    stub.set(ADDR(QModelIndex, row), stub_CPUSummaryTableModel_data_index_row_3);
    stub.set(ADDR(QModelIndex, column), stub_CPUSummaryTableModel_data_index_column_0);
    QModelIndex index;
    m_tester->data(index, Qt::DisplayRole);
}

TEST_F(UT_CPUSummaryTableModel, test_data_08)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_CPUSummaryTableModel_data_isValid_true);
    stub.set(ADDR(QModelIndex, row), stub_CPUSummaryTableModel_data_index_row_3);
    stub.set(ADDR(QModelIndex, column), stub_CPUSummaryTableModel_data_index_column_1);
    QModelIndex index;
    m_tester->data(index, Qt::DisplayRole);
}

TEST_F(UT_CPUSummaryTableModel, test_data_09)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_CPUSummaryTableModel_data_isValid_true);
    stub.set(ADDR(QModelIndex, row), stub_CPUSummaryTableModel_data_index_row_4);
    stub.set(ADDR(QModelIndex, column), stub_CPUSummaryTableModel_data_index_column_0);
    QModelIndex index;
    m_tester->data(index, Qt::DisplayRole);
}

TEST_F(UT_CPUSummaryTableModel, test_data_10)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_CPUSummaryTableModel_data_isValid_true);
    stub.set(ADDR(QModelIndex, row), stub_CPUSummaryTableModel_data_index_row_4);
    stub.set(ADDR(QModelIndex, column), stub_CPUSummaryTableModel_data_index_column_1);
    QModelIndex index;
    m_tester->data(index, Qt::DisplayRole);
}

TEST_F(UT_CPUSummaryTableModel, test_data_11)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_CPUSummaryTableModel_data_isValid_true);
    stub.set(ADDR(QModelIndex, row), stub_CPUSummaryTableModel_data_index_row_5);
    stub.set(ADDR(QModelIndex, column), stub_CPUSummaryTableModel_data_index_column_0);
    QModelIndex index;
    m_tester->data(index, Qt::DisplayRole);
}

TEST_F(UT_CPUSummaryTableModel, test_data_12)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_CPUSummaryTableModel_data_isValid_true);
    stub.set(ADDR(QModelIndex, row), stub_CPUSummaryTableModel_data_index_row_5);
    stub.set(ADDR(QModelIndex, column), stub_CPUSummaryTableModel_data_index_column_1);
    QModelIndex index;
    m_tester->data(index, Qt::DisplayRole);
}

TEST_F(UT_CPUSummaryTableModel, test_data_13)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_CPUSummaryTableModel_data_isValid_true);
    stub.set(ADDR(QModelIndex, row), stub_CPUSummaryTableModel_data_index_row_6);
    stub.set(ADDR(QModelIndex, column), stub_CPUSummaryTableModel_data_index_column_0);
    QModelIndex index;
    m_tester->data(index, Qt::DisplayRole);
}

TEST_F(UT_CPUSummaryTableModel, test_data_14)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_CPUSummaryTableModel_data_isValid_true);
    stub.set(ADDR(QModelIndex, row), stub_CPUSummaryTableModel_data_index_row_6);
    stub.set(ADDR(QModelIndex, column), stub_CPUSummaryTableModel_data_index_column_1);
    QModelIndex index;
    m_tester->data(index, Qt::DisplayRole);
}

TEST_F(UT_CPUSummaryTableModel, test_data_15)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_CPUSummaryTableModel_data_isValid_true);
    stub.set(ADDR(QModelIndex, row), stub_CPUSummaryTableModel_data_index_row_0);
    stub.set(ADDR(QModelIndex, column), stub_CPUSummaryTableModel_data_index_column_0);
    QModelIndex index;
    m_tester->data(index, Qt::UserRole);
}


TEST_F(UT_CPUSummaryTableModel, test_data_16)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_CPUSummaryTableModel_data_isValid_true);
    stub.set(ADDR(QModelIndex, row), stub_CPUSummaryTableModel_data_index_row_0);
    stub.set(ADDR(QModelIndex, column), stub_CPUSummaryTableModel_data_index_column_1);
    QModelIndex index;
    m_tester->data(index, Qt::UserRole);
}

TEST_F(UT_CPUSummaryTableModel, test_data_17)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_CPUSummaryTableModel_data_isValid_true);
    stub.set(ADDR(QModelIndex, row), stub_CPUSummaryTableModel_data_index_row_1);
    stub.set(ADDR(QModelIndex, column), stub_CPUSummaryTableModel_data_index_column_0);
    QModelIndex index;
    m_tester->data(index, Qt::UserRole);
}


TEST_F(UT_CPUSummaryTableModel, test_data_18)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_CPUSummaryTableModel_data_isValid_true);
    stub.set(ADDR(QModelIndex, row), stub_CPUSummaryTableModel_data_index_row_1);
    stub.set(ADDR(QModelIndex, column), stub_CPUSummaryTableModel_data_index_column_1);
    QModelIndex index;
    m_tester->data(index, Qt::UserRole);
}

TEST_F(UT_CPUSummaryTableModel, test_data_19)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_CPUSummaryTableModel_data_isValid_true);
    stub.set(ADDR(QModelIndex, row), stub_CPUSummaryTableModel_data_index_row_2);
    stub.set(ADDR(QModelIndex, column), stub_CPUSummaryTableModel_data_index_column_0);
    QModelIndex index;
    m_tester->data(index, Qt::UserRole);
}

TEST_F(UT_CPUSummaryTableModel, test_data_20)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_CPUSummaryTableModel_data_isValid_true);
    stub.set(ADDR(QModelIndex, row), stub_CPUSummaryTableModel_data_index_row_2);
    stub.set(ADDR(QModelIndex, column), stub_CPUSummaryTableModel_data_index_column_1);
    QModelIndex index;
    m_tester->data(index, Qt::UserRole);
}

TEST_F(UT_CPUSummaryTableModel, test_data_21)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_CPUSummaryTableModel_data_isValid_true);
    stub.set(ADDR(QModelIndex, row), stub_CPUSummaryTableModel_data_index_row_3);
    stub.set(ADDR(QModelIndex, column), stub_CPUSummaryTableModel_data_index_column_0);
    QModelIndex index;
    m_tester->data(index, Qt::UserRole);
}

TEST_F(UT_CPUSummaryTableModel, test_data_22)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_CPUSummaryTableModel_data_isValid_true);
    stub.set(ADDR(QModelIndex, row), stub_CPUSummaryTableModel_data_index_row_3);
    stub.set(ADDR(QModelIndex, column), stub_CPUSummaryTableModel_data_index_column_1);
    QModelIndex index;
    m_tester->data(index, Qt::UserRole);
}

TEST_F(UT_CPUSummaryTableModel, test_data_23)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_CPUSummaryTableModel_data_isValid_true);
    stub.set(ADDR(QModelIndex, row), stub_CPUSummaryTableModel_data_index_row_4);
    stub.set(ADDR(QModelIndex, column), stub_CPUSummaryTableModel_data_index_column_0);
    QModelIndex index;
    m_tester->data(index, Qt::UserRole);
}

TEST_F(UT_CPUSummaryTableModel, test_data_24)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_CPUSummaryTableModel_data_isValid_true);
    stub.set(ADDR(QModelIndex, row), stub_CPUSummaryTableModel_data_index_row_4);
    stub.set(ADDR(QModelIndex, column), stub_CPUSummaryTableModel_data_index_column_1);
    QModelIndex index;
    m_tester->data(index, Qt::UserRole);
}

TEST_F(UT_CPUSummaryTableModel, test_data_25)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_CPUSummaryTableModel_data_isValid_true);
    stub.set(ADDR(QModelIndex, row), stub_CPUSummaryTableModel_data_index_row_5);
    stub.set(ADDR(QModelIndex, column), stub_CPUSummaryTableModel_data_index_column_0);
    QModelIndex index;
    m_tester->data(index, Qt::UserRole);
}

TEST_F(UT_CPUSummaryTableModel, test_data_26)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_CPUSummaryTableModel_data_isValid_true);
    stub.set(ADDR(QModelIndex, row), stub_CPUSummaryTableModel_data_index_row_5);
    stub.set(ADDR(QModelIndex, column), stub_CPUSummaryTableModel_data_index_column_1);
    QModelIndex index;
    m_tester->data(index, Qt::UserRole);
}

TEST_F(UT_CPUSummaryTableModel, test_data_27)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_CPUSummaryTableModel_data_isValid_true);
    stub.set(ADDR(QModelIndex, row), stub_CPUSummaryTableModel_data_index_row_6);
    stub.set(ADDR(QModelIndex, column), stub_CPUSummaryTableModel_data_index_column_0);
    QModelIndex index;
    m_tester->data(index, Qt::UserRole);
}

TEST_F(UT_CPUSummaryTableModel, test_data_28)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_CPUSummaryTableModel_data_isValid_true);
    stub.set(ADDR(QModelIndex, row), stub_CPUSummaryTableModel_data_index_row_6);
    stub.set(ADDR(QModelIndex, column), stub_CPUSummaryTableModel_data_index_column_1);
    QModelIndex index;
    m_tester->data(index, Qt::UserRole);
}

TEST_F(UT_CPUSummaryTableModel, test_data_29)
{
    Stub stub;
    stub.set(ADDR(QModelIndex, isValid), stub_CPUSummaryTableModel_data_isValid_true);
    stub.set(ADDR(QModelIndex, row), stub_CPUSummaryTableModel_data_index_row_6);
    stub.set(ADDR(QModelIndex, column), stub_CPUSummaryTableModel_data_index_column_1);
    QModelIndex index;
    m_tester->data(index, Qt::TextColorRole);
}
