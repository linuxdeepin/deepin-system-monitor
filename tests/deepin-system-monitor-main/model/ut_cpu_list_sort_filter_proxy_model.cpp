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
#include "model/cpu_list_sort_filter_proxy_model.h"
//gtest
#include "stub.h"
#include <gtest/gtest.h>

/***************************************STUB begin*********************************************/

/***************************************STUB end**********************************************/

class UT_CPUListSortFilterProxyModel: public ::testing::Test
{
public:
    UT_CPUListSortFilterProxyModel() : m_tester(nullptr) {}

public:
    virtual void SetUp()
    {
        m_tester = new CPUListSortFilterProxyModel(nullptr);
    }

    virtual void TearDown()
    {
        if(m_tester){
            delete m_tester;
            m_tester = nullptr;
        }
    }

protected:
    CPUListSortFilterProxyModel *m_tester;
};

TEST_F(UT_CPUListSortFilterProxyModel, initTest)
{

}

TEST_F(UT_CPUListSortFilterProxyModel, test_setSortKey_001)
{
    CPUListSortFilterProxyModel::SortKey key = CPUListSortFilterProxyModel::kSortByIndex;
    m_tester->setSortKey(key);
    EXPECT_EQ(m_tester->m_sortKey,key);

}

TEST_F(UT_CPUListSortFilterProxyModel, test_filterAcceptsRow_001)
{

}

TEST_F(UT_CPUListSortFilterProxyModel, test_filterAcceptsColumn_001)
{
    QModelIndex *index = new QModelIndex;
    m_tester->filterAcceptsColumn(1,*index);
    delete index;

}

TEST_F(UT_CPUListSortFilterProxyModel, test_lessThan_001)
{

}


