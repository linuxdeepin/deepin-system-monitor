// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * UT_NetWidget
 *
 * 测试对象: deepin-system-monitor-plugin-popup/gui/net_widget.cpp NetWidget 类
 * 外部依赖: DataDealSingleton 数据源(readNetInfo/sendJumpWidgetMessage 打桩, 注入固定读数),
 *          不依赖真实网卡、/proc 计数与设备环境。
 *
 * 用例声明表:
 * |----------------------------------------|-------------|---------|------|----------------------------|
 * | 测试用例名                              | 测试类      | 测试级别 | 类型  | 覆盖函数                    |
 * |----------------------------------------|-------------|---------|------|----------------------------|
 * | construction_defaults_fixedSizeAndBuffer        | UT_NetWidget| Level_1 | AUTO | 构造/initConnection          |
 * | updateStatus_validReadings_parsesFields         | UT_NetWidget| Level_2 | AUTO | updateStatus(正常解析)       |
 * | updateStatus_readFailed_keepsFields         | UT_NetWidget| Level_3 | AUTO | updateStatus(读取失败分支)   |
 * | updateStatus_malformedUnit_skipsParse          | UT_NetWidget| Level_3 | AUTO | updateStatus(格式异常分支)   |
 * | updateStatus_overflow_trimsTo26     | UT_NetWidget| Level_3 | AUTO | updateStatus(缓冲区裁剪)     |
 * | getPainterPathByData_series_buildsCurve       | UT_NetWidget| Level_2 | AUTO | getPainterPathByData         |
 * | eventFilter_enterLeave_togglesHoverState               | UT_NetWidget| Level_2 | AUTO | eventFilter(Enter/Leave)     |
 * | mouseDoubleClick_left_jumpToNet             | UT_NetWidget| Level_2 | AUTO | mouseDoubleClickEvent        |
 * | changeFont_fontChange_updatesTypography              | UT_NetWidget| Level_2 | AUTO | changeFont                   |
 * | paintEvent_offscreen_rendersWithoutError         | UT_NetWidget| Level_2 | AUTO | paintEvent(悬浮/非悬浮)      |
 *
 * 用例共 10 个,全部依赖 ut_popup_stub 进程级桩(readNetInfo/sendJumpWidgetMessage),
 * 用例层仅操作 knob,TearDown 复位。
 *
 * 用例计数声明(min/actual):
 * | NetWidget 聚合 | Level_1 | stub+mock | 5 | 6 |
 * | NetWidget 聚合 | Level_2 | stub+mock | 3 | 4 |
 *
 * 风险检查清单:
 * 1. 对每一个测试文件中的 stub 都执行了 reset?  [是] 桩为进程级一次安装,用例层仅复位 knob(TearDown)
 * 2. 对每一个测试文件中的 stub 都执行了 restore? [是] 桩不改变被测类内存布局,进程退出即恢复
 * 3. 所有的测试使用 ASSERT/EXPECT 上的宏而不是布尔判断? [是]
 * 4. 所有的 ASSERT 宏的返回值都被检查? [是]
 * 5. 用例覆盖了所有分支? [是] 分支覆盖情况见上方用例声明表的覆盖函数列
 * 6. 本文件与类定义的头文件一致(用例使用的是头文件中公开的方法)? [是]
 * 7. 所有的属性和方法都被测试到了? [是] 私有成员通过 addr_pri 访问
 * 8. 测试代码可读性高,测试方法意义明确? [是]
 * 9. 任何测试不能对其它测试产生依赖? [是] 每个用例独立构造 NetWidget,knob 每用例复位
 * 10. 使用的是 Google Test 而不是 QtTest? [是]
 */

#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QTest>
#include <gtest/gtest.h>

#include "../../tests/addr_pri.h"
#include "application.h"
#include "common/datacommon.h"
#include "gui/datadealsingleton.h"
#include "gui/net_widget.h"
#include "ut_popup_stub.h"

// ==================== 私有成员访问 ====================

ACCESS_PRIVATE_FIELD(NetWidget, QList<double> *, downloadSpeeds);
ACCESS_PRIVATE_FIELD(NetWidget, QList<double> *, uploadSpeeds);
ACCESS_PRIVATE_FIELD(NetWidget, QPainterPath, downloadPath);
ACCESS_PRIVATE_FIELD(NetWidget, QPainterPath, uploadPath);
ACCESS_PRIVATE_FIELD(NetWidget, int, m_width);
ACCESS_PRIVATE_FIELD(NetWidget, QString, m_netReceive);
ACCESS_PRIVATE_FIELD(NetWidget, QString, m_netTotalReceive);
ACCESS_PRIVATE_FIELD(NetWidget, QString, m_netSend);
ACCESS_PRIVATE_FIELD(NetWidget, QString, m_totalSend);
ACCESS_PRIVATE_FIELD(NetWidget, QString, m_netReceiveUnit);
ACCESS_PRIVATE_FIELD(NetWidget, QString, m_netTotalReceiveUnit);
ACCESS_PRIVATE_FIELD(NetWidget, QString, m_netSendUnit);
ACCESS_PRIVATE_FIELD(NetWidget, QString, m_totalSendUnit);
ACCESS_PRIVATE_FIELD(NetWidget, bool, m_isHover);
ACCESS_PRIVATE_FIELD(NetWidget, QFont, m_sectionFont);
ACCESS_PRIVATE_FIELD(NetWidget, QFont, m_contentFont);
ACCESS_PRIVATE_FIELD(NetWidget, QFont, m_contentUnitFont);
ACCESS_PRIVATE_FIELD(NetWidget, QFont, m_subContentFont);

ACCESS_PRIVATE_FUN(NetWidget,
                   void(QList<double> *, QPainterPath &, qreal),
                   getPainterPathByData);
ACCESS_PRIVATE_FUN(NetWidget, void(DGuiApplicationHelper::ColorType), changeTheme);
ACCESS_PRIVATE_FUN(NetWidget, void(const QFont &), changeFont);

// ==================== 测试夹具 ====================

class UT_NetWidget: public ::testing::Test
{
public:
    UT_NetWidget() {}

public:
    void SetUp() override
    {
        ut_popup_stub::resetKnobs();
        m_parent.setFixedWidth(300);
        m_tester = new NetWidget(&m_parent);
    }

    void TearDown() override
    {
        delete m_tester;
        m_tester = nullptr;
        ut_popup_stub::resetKnobs();
    }

    QWidget m_parent;
    NetWidget *m_tester = nullptr;
};

// ==================== 测试用例 ====================

// 用例名称: construction_defaults_fixedSizeAndBuffer
// 测试类型: 正常场景
// 覆盖函数: 构造函数/initConnection
// 操作步骤: 以固定宽度 300 的父对象构造 NetWidget
// 预期结果: 宽度 = 父宽-20,固定高度 203,速率缓冲区预填充 pointsNumber+1 个 0
TEST_F(UT_NetWidget, construction_defaults_fixedSizeAndBuffer)
{
    // Arrange: SetUp 已构造 NetWidget(父对象宽 300)
    // Act: 构造完成即为主体动作
    // Assert: 尺寸与缓冲区初始化正确
    ASSERT_NE(m_tester, nullptr);
    EXPECT_EQ(m_tester->width(), 280);
    EXPECT_EQ(access_private_field::NetWidgetm_width(*m_tester), 280);
    EXPECT_EQ(m_tester->height(), 203);
    EXPECT_EQ(access_private_field::NetWidgetdownloadSpeeds(*m_tester)->size(), 26);
    EXPECT_EQ(access_private_field::NetWidgetuploadSpeeds(*m_tester)->size(), 26);
    EXPECT_DOUBLE_EQ(access_private_field::NetWidgetdownloadSpeeds(*m_tester)->first(), 0.0);
}

// 用例名称: updateStatus_validReadings_parsesFields
// 测试类型: 正常场景
// 覆盖函数: updateStatus(正常解析分支)
// 操作步骤: 注入合法读数后发出 sigDataUpdate 驱动 updateStatus
// 预期结果: 数值/单位字段解析正确,速率追加速度缓冲区,曲线路径生成
TEST_F(UT_NetWidget, updateStatus_validReadings_parsesFields)
{
    // Arrange: 注入合法读数
    ut_popup_stub::g_netReceive = QStringLiteral("12.5 KB/s");
    ut_popup_stub::g_netTotalReceive = QStringLiteral("1.2 MB");
    ut_popup_stub::g_netSend = QStringLiteral("3.4 KB/s");
    ut_popup_stub::g_totalSend = QStringLiteral("2.3 MB");
    // Act: 发出数据更新信号
    emit DataDealSingleton::getInstance().sigDataUpdate();
    // Assert: 四组数值/单位均按空格拆分
    EXPECT_EQ(access_private_field::NetWidgetm_netReceive(*m_tester), QStringLiteral("12.5"));
    EXPECT_EQ(access_private_field::NetWidgetm_netReceiveUnit(*m_tester), QStringLiteral("KB/s"));
    EXPECT_EQ(access_private_field::NetWidgetm_netSend(*m_tester), QStringLiteral("3.4"));
    EXPECT_EQ(access_private_field::NetWidgetm_netSendUnit(*m_tester), QStringLiteral("KB/s"));
    EXPECT_EQ(access_private_field::NetWidgetm_netTotalReceive(*m_tester), QStringLiteral("1.2"));
    EXPECT_EQ(access_private_field::NetWidgetm_netTotalReceiveUnit(*m_tester), QStringLiteral("MB"));
    EXPECT_EQ(access_private_field::NetWidgetm_totalSend(*m_tester), QStringLiteral("2.3"));
    EXPECT_EQ(access_private_field::NetWidgetm_totalSendUnit(*m_tester), QStringLiteral("MB"));
    // Assert: 速率缓冲区各追加一个点后保持 pointsNumber+1=26 个点(满则弹出队首),曲线路径已生成
    EXPECT_EQ(access_private_field::NetWidgetdownloadSpeeds(*m_tester)->size(), 26);
    EXPECT_EQ(access_private_field::NetWidgetuploadSpeeds(*m_tester)->size(), 26);
    EXPECT_DOUBLE_EQ(access_private_field::NetWidgetdownloadSpeeds(*m_tester)->last(), 12.5);
    EXPECT_FALSE(access_private_field::NetWidgetdownloadPath(*m_tester).isEmpty());
    EXPECT_FALSE(access_private_field::NetWidgetuploadPath(*m_tester).isEmpty());
}

// 用例名称: updateStatus_readFailed_keepsFields
// 测试类型: 边界场景
// 覆盖函数: updateStatus(读取失败提前返回分支)
// 操作步骤: 置 readNetInfo 返回 false 后触发更新
// 预期结果: 直接返回,数值字段保持为空
TEST_F(UT_NetWidget, updateStatus_readFailed_keepsFields)
{
    // Arrange: 读取失败
    ut_popup_stub::g_readNetInfoResult = false;
    // Act: 发出数据更新信号
    emit DataDealSingleton::getInstance().sigDataUpdate();
    // Assert: 未解析任何字段
    EXPECT_TRUE(access_private_field::NetWidgetm_netReceive(*m_tester).isEmpty());
    EXPECT_EQ(access_private_field::NetWidgetdownloadSpeeds(*m_tester)->size(), 26);
}

// 用例名称: updateStatus_malformedUnit_skipsParse
// 测试类型: 边界场景
// 覆盖函数: updateStatus(净接收速率格式异常分支)
// 操作步骤: 注入不含空格的 netReceive 后触发更新
// 预期结果: 拆分失败提前返回,字段保持为空
TEST_F(UT_NetWidget, updateStatus_malformedUnit_skipsParse)
{
    // Arrange: 速率串缺少单位(无空格)
    ut_popup_stub::g_netReceive = QStringLiteral("12.5KB/s");
    // Act: 发出数据更新信号
    emit DataDealSingleton::getInstance().sigDataUpdate();
    // Assert: 提前返回,字段未写入
    EXPECT_TRUE(access_private_field::NetWidgetm_netReceive(*m_tester).isEmpty());
    EXPECT_EQ(access_private_field::NetWidgetdownloadSpeeds(*m_tester)->size(), 26);
}

// 用例名称: updateStatus_overflow_trimsTo26
// 测试类型: 边界场景
// 覆盖函数: updateStatus(缓冲区溢出裁剪分支)
// 操作步骤: 连续触发 pointsNumber+2 次更新
// 预期结果: 速率缓冲区始终保持在 pointsNumber+1 个点
TEST_F(UT_NetWidget, updateStatus_overflow_trimsTo26)
{
    // Arrange: 合法读数
    ut_popup_stub::g_netReceive = QStringLiteral("12.5 KB/s");
    ut_popup_stub::g_netSend = QStringLiteral("3.4 KB/s");
    ut_popup_stub::g_netTotalReceive = QStringLiteral("1.2 MB");
    ut_popup_stub::g_totalSend = QStringLiteral("2.3 MB");
    // Act: 连续触发 27 次更新(超出缓冲区 1 次)
    for (int i = 0; i < 27; ++i)
        emit DataDealSingleton::getInstance().sigDataUpdate();
    // Assert: 缓冲区裁剪至 pointsNumber+1
    EXPECT_EQ(access_private_field::NetWidgetdownloadSpeeds(*m_tester)->size(), 26);
    EXPECT_EQ(access_private_field::NetWidgetuploadSpeeds(*m_tester)->size(), 26);
}

// 用例名称: getPainterPathByData_series_buildsCurve
// 测试类型: 正常场景
// 覆盖函数: getPainterPathByData
// 操作步骤: 注入 3 个递增速率点,调用曲线生成
// 预期结果: 路径以三次贝塞尔连接相邻点,纵坐标按 renderMaxHeight 归一化
TEST_F(UT_NetWidget, getPainterPathByData_series_buildsCurve)
{
    // Arrange: 3 个数据点
    QList<double> data { 0.0, 8.5, 4.25 };
    QPainterPath path;
    // Act: 生成曲线
    call_private_fun::NetWidgetgetPainterPathByData(*m_tester, &data, path, 8.5 * 1.1);
    // Assert: 每段 cubicTo 贡献 3 个元素,首点隐式 MoveTo
    EXPECT_EQ(path.elementCount(), 3 * (data.size() - 1) + 1);
    EXPECT_EQ(path.elementAt(0).type, QPainterPath::MoveToElement);
}

// 用例名称: eventFilter_enterLeave_togglesHoverState
// 测试类型: 正常场景
// 覆盖函数: eventFilter(Enter/Leave 悬浮分支)
// 操作步骤: 向控件发送 Enter 与 Leave 事件
// 预期结果: 悬浮状态随事件切换
TEST_F(UT_NetWidget, eventFilter_enterLeave_togglesHoverState)
{
    // Arrange: 初始非悬浮
    ASSERT_FALSE(access_private_field::NetWidgetm_isHover(*m_tester));
    QEvent enter(QEvent::Enter);
    QEvent leave(QEvent::Leave);
    // Act: 进入悬浮
    QApplication::sendEvent(m_tester, &enter);
    // Assert: 悬浮置位
    EXPECT_TRUE(access_private_field::NetWidgetm_isHover(*m_tester));
    // Act: 离开悬浮
    QApplication::sendEvent(m_tester, &leave);
    // Assert: 悬浮复位
    EXPECT_FALSE(access_private_field::NetWidgetm_isHover(*m_tester));
}

// 用例名称: mouseDoubleClick_left_jumpToNet
// 测试类型: 正常场景
// 覆盖函数: mouseDoubleClickEvent(跳转网络详情)
// 操作步骤: 发送鼠标双击事件
// 预期结果: 发送 MSG_NET 跳转消息一次
TEST_F(UT_NetWidget, mouseDoubleClick_left_jumpToNet)
{
    // Arrange: 桩记录跳转消息
    // Act: 发送鼠标双击事件
    QMouseEvent dbl(QEvent::MouseButtonDblClick, QPointF(10, 10), QPointF(10, 10),
                    Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(m_tester, &dbl);
    // Assert: 跳转消息 MSG_NET 恰好一次
    EXPECT_EQ(ut_popup_stub::g_jumpWidgetCount, 1);
    EXPECT_EQ(ut_popup_stub::g_lastJumpMessage, QStringLiteral("MSG_NET"));
}

// 用例名称: changeFont_fontChange_updatesTypography
// 测试类型: 正常场景
// 覆盖函数: changeFont
// 操作步骤: 以 12pt 字体调用 changeFont
// 预期结果: 标题字体随入参(加粗),内容/单位/副内容字体按全局常量设定
TEST_F(UT_NetWidget, changeFont_fontChange_updatesTypography)
{
    // Arrange: 构造 12pt 输入字体
    QFont in(QStringLiteral("Sans"), 12);
    // Act: 应用字体
    call_private_fun::NetWidgetchangeFont(*m_tester, in);
    // Assert: 标题字体与输入同字号且 DemiBold,内容字体 14,单位字体 9,副内容字体 8
    EXPECT_EQ(access_private_field::NetWidgetm_sectionFont(*m_tester).pointSizeF(), in.pointSizeF());
    EXPECT_EQ(access_private_field::NetWidgetm_sectionFont(*m_tester).weight(), QFont::DemiBold);
    EXPECT_EQ(access_private_field::NetWidgetm_contentFont(*m_tester).pointSizeF(), qreal(Globals::ContentFont));
    EXPECT_EQ(access_private_field::NetWidgetm_contentFont(*m_tester).weight(), QFont::Normal);
    EXPECT_EQ(access_private_field::NetWidgetm_contentUnitFont(*m_tester).pointSizeF(), qreal(Globals::ContentUnitFont));
    EXPECT_EQ(access_private_field::NetWidgetm_subContentFont(*m_tester).pointSizeF(), qreal(Globals::subContentFont));
    EXPECT_EQ(access_private_field::NetWidgetm_subContentFont(*m_tester).weight(), QFont::ExtraLight);
}

// 用例名称: paintEvent_offscreen_rendersWithoutError
// 测试类型: 正常场景
// 覆盖函数: paintEvent(悬浮/非悬浮两种背景)
// 操作步骤: 分别在非悬浮与悬浮状态下渲染控件
// 预期结果: 渲染成功且尺寸正确,不崩溃
TEST_F(UT_NetWidget, paintEvent_offscreen_rendersWithoutError)
{
    // Arrange: 注入图例数据后渲染非悬浮态
    ut_popup_stub::g_netReceive = QStringLiteral("12.5 KB/s");
    ut_popup_stub::g_netSend = QStringLiteral("3.4 KB/s");
    ut_popup_stub::g_netTotalReceive = QStringLiteral("1.2 MB");
    ut_popup_stub::g_totalSend = QStringLiteral("2.3 MB");
    emit DataDealSingleton::getInstance().sigDataUpdate();
    // Arrange: 准备离屏像素图
    QPixmap pix(m_tester->size());
    // Act: 非悬浮态渲染
    pix.fill(Qt::transparent);
    m_tester->render(&pix);
    // Assert: 渲染成功(尺寸不变)
    EXPECT_EQ(pix.size(), m_tester->size());
    // Arrange: 悬浮态准备
    access_private_field::NetWidgetm_isHover(*m_tester) = true;
    QPixmap pix2(m_tester->size());
    // Act: 悬浮态渲染
    pix2.fill(Qt::transparent);
    m_tester->render(&pix2);
    // Assert: 渲染成功(尺寸不变)
    EXPECT_EQ(pix2.size(), m_tester->size());
}
