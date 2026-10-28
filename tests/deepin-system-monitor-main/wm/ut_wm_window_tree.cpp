// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// UT_WMWindowTree：WMWindowTree 单元测试（qt-autotest-generator 生成）
//
// ============================ 用例计数声明 ============================
// 方法             | level | 维度                     | min | actual
// | WMWindowTree()   | low   | 默认构造（类唯一公开入口） | 1   | 1 |
//
// ============================== 最小清单 ==============================
// [x] 1 唯一公开方法（构造）覆盖 1 用例
// [x] 2 构造无参数 → 仅默认等价类
// [x] 3 无分支/无边界值（空实现构造函数）
// [x] 4 输入维度单一 → 不适用 TEST_P
// [x] 5 无分支清单（函数体为空，仅注释日志）
// [x] 6 早退分支不适用
// [x] 7 无 throw
// [x] 8 负面场景不适用（无外部依赖、无错误路径）
// [x] 9 失败路径不适用
// [x] 10 无外部依赖（QObject 基类，无需 stub）

#include <gtest/gtest.h>

#include "wm/wm_window_tree.h"

namespace {
class WMWindowTreeTest : public ::testing::Test
{
public:
    void SetUp() override {}
    void TearDown() override {}
};
}

TEST_F(WMWindowTreeTest, DefaultConstruct_DefaultWorld_InstantiatesSafely)
{
    // Arrange：类无外部依赖，前置状态仅 QObject 元对象继承关系成立
    ASSERT_TRUE(WMWindowTree::staticMetaObject.inherits(&QObject::staticMetaObject));

    // Act：默认构造并立即析构（唯一公开行为）
    {
        WMWindowTree tree;

        // Assert：直接基类为 QObject（私有继承，经公开元对象验证），可安全析构（无崩溃）
        EXPECT_EQ(WMWindowTree::staticMetaObject.superClass(), &QObject::staticMetaObject);
        EXPECT_STREQ(WMWindowTree::staticMetaObject.className(), "WMWindowTree");
    }
    SUCCEED() << "WMWindowTree default construction and destruction completed";
}
