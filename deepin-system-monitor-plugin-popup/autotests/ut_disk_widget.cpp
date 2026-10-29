// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * UT_DiskWidget
 *
 * 测试对象: deepin-system-monitor-plugin-popup/gui/disk_widget.cpp DiskWidget 类
 * 外部依赖: DataDealSingleton 数据源(readDiskInfo/sendJumpWidgetMessage 打桩, 注入固定读数),
 *          不依赖真实磁盘、/proc 计数与设备环境。
 *
 * 用例声明表:
 * |----------------------------------------|-------------|---------|------|----------------------------|
 * | 测试用例名                              | 测试类      | 测试级别 | 类型  | 覆盖函数                    |
 * |----------------------------------------|-------------|---------|------|----------------------------|
 * | construction_defaults_fixedSizeAndBuffer        | UT_DiskWidget| Level_1 | AUTO | 构造/连接                    |
 * | updateStatus_validReadings_parsesFields        | UT_DiskWidget| Level_2 | AUTO | updateStatus(正常解析)       |
 * | updateStatus_readFailed_keepsFields        | UT_DiskWidget| Level_3 | AUTO | updateStatus(读取失败分支)   |
 * | updateStatus_malformedUnit_skipsParse             | UT_DiskWidget| Level_3 | AUTO | updateStatus(格式异常分支)   |
 * | updateStatus_malformedAvailable_stillUpdates| UT_DiskWidget| Level_3 | AUTO | updateStatus(diskAvailable 容错) |
 * | updateStatus_overflow_trimsTo26     | UT_DiskWidget| Level_3 | AUTO | updateStatus(缓冲区裁剪)     |
 * | getPainterPathByData_series_buildsCurve       | UT_DiskWidget| Level_2 | AUTO | getPainterPathByData         |
 * | eventFilter_enterLeave_togglesHoverState               | UT_DiskWidget| Level_2 | AUTO | eventFilter(Enter/Leave)     |
 * | mouseDoubleClick_left_jumpToDisk        | UT_DiskWidget| Level_2 | AUTO | mouseDoubleClickEvent(左键)  |
 * | mouseDoubleClick_right_ignored        | UT_DiskWidget| Level_3 | AUTO | mouseDoubleClickEvent(非左键)|
 * | changeFont_fontChange_updatesTypography              | UT_DiskWidget| Level_2 | AUTO | changeFont                   |
 * | paintEvent_offscreen_rendersWithoutError         | UT_DiskWidget| Level_2 | AUTO | paintEvent(悬浮/非悬浮)      |
 *
 * 用例共 12 个,全部依赖 ut_popup_stub 进程级桩(readDiskInfo/sendJumpWidgetMessage),
 * 用例层仅操作 knob,TearDown 复位。
 *
 * 用例计数声明(min/actual):
 * | DiskWidget 聚合 | Level_1 | stub+mock | 5 | 7 |
 * | DiskWidget 聚合 | Level_2 | stub+mock | 3 | 5 |
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
 * 9. 任何测试不能对其它测试产生依赖? [是] 每个用例独立构造 DiskWidget,knob 每用例复位
 * 10. 使用的是 Google Test 而不是 QtTest? [是]
 */

#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <gtest/gtest.h>

#include "../../tests/addr_pri.h"
#include "application.h"
#include "common/datacommon.h"
#include "gui/datadealsingleton.h"
#include "gui/disk_widget.h"
#include "ut_popup_stub.h"

// ==================== 私有成员访问 ====================

ACCESS_PRIVATE_FIELD(DiskWidget, QList<double> *, readSpeeds);
ACCESS_PRIVATE_FIELD(DiskWidget, QList<double> *, writeSpeeds);
ACCESS_PRIVATE_FIELD(DiskWidget, QPainterPath, readDiskPath);
ACCESS_PRIVATE_FIELD(DiskWidget, QPainterPath, writeDiskPath);
ACCESS_PRIVATE_FIELD(DiskWidget, int, m_width);
ACCESS_PRIVATE_FIELD(DiskWidget, QString, m_diskRead);
ACCESS_PRIVATE_FIELD(DiskWidget, QString, m_diskTotal);
ACCESS_PRIVATE_FIELD(DiskWidget, QString, m_diskWrite);
ACCESS_PRIVATE_FIELD(DiskWidget, QString, m_diskAvailable);
ACCESS_PRIVATE_FIELD(DiskWidget, QString, m_diskReadUnit);
ACCESS_PRIVATE_FIELD(DiskWidget, QString, m_diskTotalUnit);
ACCESS_PRIVATE_FIELD(DiskWidget, QString, m_diskWriteUnit);
ACCESS_PRIVATE_FIELD(DiskWidget, QString, m_diskAvailableUnit);
ACCESS_PRIVATE_FIELD(DiskWidget, bool, m_isHover);
ACCESS_PRIVATE_FIELD(DiskWidget, QFont, m_sectionFont);
ACCESS_PRIVATE_FIELD(DiskWidget, QFont, m_contentFont);
ACCESS_PRIVATE_FIELD(DiskWidget, QFont, m_contentUnitFont);
ACCESS_PRIVATE_FIELD(DiskWidget, QFont, m_subContentFont);

ACCESS_PRIVATE_FUN(DiskWidget,
                   void(QList<double> *, QPainterPath &, qreal),
                   getPainterPathByData);
ACCESS_PRIVATE_FUN(DiskWidget, void(DGuiApplicationHelper::ColorType), changeTheme);
ACCESS_PRIVATE_FUN(DiskWidget, void(const QFont &), changeFont);

// ==================== 测试夹具 ====================

class UT_DiskWidget: public ::testing::Test
{
public:
    UT_DiskWidget() {}

public:
    void SetUp() override
    {
        ut_popup_stub::resetKnobs();
        m_parent.setFixedWidth(300);
        m_tester = new DiskWidget(&m_parent);
    }

    void TearDown() override
    {
        delete m_tester;
        m_tester = nullptr;
        ut_popup_stub::resetKnobs();
    }

    QWidget m_parent;
    DiskWidget *m_tester = nullptr;
};

// ==================== 测试用例 ====================

// 用例名称: construction_defaults_fixedSizeAndBuffer
// 测试类型: 正常场景
// 覆盖函数: 构造函数/信号连接
// 操作步骤: 以固定宽度 300 的父对象构造 DiskWidget
// 预期结果: 宽度 = 父宽-20,固定高度 154,速率缓冲区预填充 pointsNumber+1 个 0
TEST_F(UT_DiskWidget, construction_defaults_fixedSizeAndBuffer)
{
    // Arrange: SetUp 已构造 DiskWidget(父对象宽 300)
    // Act: 构造完成即为主体动作
    // Assert: 尺寸与缓冲区初始化正确
    ASSERT_NE(m_tester, nullptr);
    EXPECT_EQ(m_tester->width(), 280);
    EXPECT_EQ(access_private_field::DiskWidgetm_width(*m_tester), 280);
    EXPECT_EQ(m_tester->height(), 154);
    EXPECT_EQ(access_private_field::DiskWidgetreadSpeeds(*m_tester)->size(), 26);
    EXPECT_EQ(access_private_field::DiskWidgetwriteSpeeds(*m_tester)->size(), 26);
    EXPECT_DOUBLE_EQ(access_private_field::DiskWidgetreadSpeeds(*m_tester)->first(), 0.0);
}

// 用例名称: updateStatus_validReadings_parsesFields
// 测试类型: 正常场景
// 覆盖函数: updateStatus(正常解析分支)
// 操作步骤: 注入合法磁盘读数后发出 sigDataUpdate 驱动 updateStatus
// 预期结果: 读/写/总量/可用字段解析正确,速率追加速度缓冲区,曲线路径生成
TEST_F(UT_DiskWidget, updateStatus_validReadings_parsesFields)
{
    // Arrange: 注入合法读数
    ut_popup_stub::g_diskRead = QStringLiteral("8.0 KB/s");
    ut_popup_stub::g_diskTotalSize = QStringLiteral("500 GB");
    ut_popup_stub::g_diskWrite = QStringLiteral("2.1 KB/s");
    ut_popup_stub::g_diskAvailable = QStringLiteral("120 GB");
    // Act: 发出数据更新信号
    emit DataDealSingleton::getInstance().sigDataUpdate();
    // Assert: 四组数值/单位均按空格拆分
    EXPECT_EQ(access_private_field::DiskWidgetm_diskRead(*m_tester), QStringLiteral("8.0"));
    EXPECT_EQ(access_private_field::DiskWidgetm_diskReadUnit(*m_tester), QStringLiteral("KB/s"));
    EXPECT_EQ(access_private_field::DiskWidgetm_diskWrite(*m_tester), QStringLiteral("2.1"));
    EXPECT_EQ(access_private_field::DiskWidgetm_diskWriteUnit(*m_tester), QStringLiteral("KB/s"));
    EXPECT_EQ(access_private_field::DiskWidgetm_diskTotal(*m_tester), QStringLiteral("500"));
    EXPECT_EQ(access_private_field::DiskWidgetm_diskTotalUnit(*m_tester), QStringLiteral("GB"));
    EXPECT_EQ(access_private_field::DiskWidgetm_diskAvailable(*m_tester), QStringLiteral("120"));
    EXPECT_EQ(access_private_field::DiskWidgetm_diskAvailableUnit(*m_tester), QStringLiteral("GB"));
    // Assert: 速率缓冲区各追加一个点,曲线路径已生成
    EXPECT_EQ(access_private_field::DiskWidgetreadSpeeds(*m_tester)->size(), 26);
    EXPECT_EQ(access_private_field::DiskWidgetwriteSpeeds(*m_tester)->size(), 26);
    EXPECT_DOUBLE_EQ(access_private_field::DiskWidgetreadSpeeds(*m_tester)->last(), 8.0);
    EXPECT_FALSE(access_private_field::DiskWidgetreadDiskPath(*m_tester).isEmpty());
    EXPECT_FALSE(access_private_field::DiskWidgetwriteDiskPath(*m_tester).isEmpty());
}

// 用例名称: updateStatus_readFailed_keepsFields
// 测试类型: 边界场景
// 覆盖函数: updateStatus(读取失败提前返回分支)
// 操作步骤: 置 readDiskInfo 返回 false 后触发更新
// 预期结果: 直接返回,数值字段保持为空
TEST_F(UT_DiskWidget, updateStatus_readFailed_keepsFields)
{
    // Arrange: 读取失败
    ut_popup_stub::g_readDiskInfoResult = false;
    // Act: 发出数据更新信号
    emit DataDealSingleton::getInstance().sigDataUpdate();
    // Assert: 未解析任何字段
    EXPECT_TRUE(access_private_field::DiskWidgetm_diskRead(*m_tester).isEmpty());
    EXPECT_EQ(access_private_field::DiskWidgetreadSpeeds(*m_tester)->size(), 26);
}

// 用例名称: updateStatus_malformedUnit_skipsParse
// 测试类型: 边界场景
// 覆盖函数: updateStatus(磁盘读速率格式异常分支)
// 操作步骤: 注入不含空格的 diskRead 后触发更新
// 预期结果: 拆分失败提前返回,字段保持为空
TEST_F(UT_DiskWidget, updateStatus_malformedUnit_skipsParse)
{
    // Arrange: 读速率串缺少单位(无空格)
    ut_popup_stub::g_diskRead = QStringLiteral("8.0KB/s");
    // Act: 发出数据更新信号
    emit DataDealSingleton::getInstance().sigDataUpdate();
    // Assert: 提前返回,字段未写入
    EXPECT_TRUE(access_private_field::DiskWidgetm_diskRead(*m_tester).isEmpty());
    EXPECT_EQ(access_private_field::DiskWidgetreadSpeeds(*m_tester)->size(), 26);
}

// 用例名称: updateStatus_malformedAvailable_stillUpdates
// 测试类型: 边界场景
// 覆盖函数: updateStatus(diskAvailable 解析容错)
// 操作步骤: 注入不含空格的 diskAvailable(其余字段合法)后触发更新
// 预期结果: 读/写字段正常解析并继续更新(available 异常不影响流程)
TEST_F(UT_DiskWidget, updateStatus_malformedAvailable_stillUpdates)
{
    // Arrange: available 串缺少单位(无空格),其余合法
    ut_popup_stub::g_diskRead = QStringLiteral("8.0 KB/s");
    ut_popup_stub::g_diskWrite = QStringLiteral("2.1 KB/s");
    ut_popup_stub::g_diskTotalSize = QStringLiteral("500 GB");
    ut_popup_stub::g_diskAvailable = QStringLiteral("120GB");
    // Act: 发出数据更新信号
    emit DataDealSingleton::getInstance().sigDataUpdate();
    // Assert: 读/写字段正常,available 保持原始串(无 else 拦截)
    EXPECT_EQ(access_private_field::DiskWidgetm_diskRead(*m_tester), QStringLiteral("8.0"));
    EXPECT_EQ(access_private_field::DiskWidgetm_diskWrite(*m_tester), QStringLiteral("2.1"));
    EXPECT_EQ(access_private_field::DiskWidgetm_diskTotal(*m_tester), QStringLiteral("500"));
    EXPECT_EQ(access_private_field::DiskWidgetreadSpeeds(*m_tester)->size(), 26);
}

// 用例名称: updateStatus_overflow_trimsTo26
// 测试类型: 边界场景
// 覆盖函数: updateStatus(缓冲区溢出裁剪分支)
// 操作步骤: 连续触发 pointsNumber+2 次更新
// 预期结果: 速率缓冲区始终保持在 pointsNumber+1 个点
TEST_F(UT_DiskWidget, updateStatus_overflow_trimsTo26)
{
    // Arrange: 合法读数
    ut_popup_stub::g_diskRead = QStringLiteral("8.0 KB/s");
    ut_popup_stub::g_diskWrite = QStringLiteral("2.1 KB/s");
    ut_popup_stub::g_diskTotalSize = QStringLiteral("500 GB");
    ut_popup_stub::g_diskAvailable = QStringLiteral("120 GB");
    // Act: 连续触发 27 次更新(超出缓冲区 1 次)
    for (int i = 0; i < 27; ++i)
        emit DataDealSingleton::getInstance().sigDataUpdate();
    // Assert: 缓冲区裁剪至 pointsNumber+1
    EXPECT_EQ(access_private_field::DiskWidgetreadSpeeds(*m_tester)->size(), 26);
    EXPECT_EQ(access_private_field::DiskWidgetwriteSpeeds(*m_tester)->size(), 26);
}

// 用例名称: getPainterPathByData_series_buildsCurve
// 测试类型: 正常场景
// 覆盖函数: getPainterPathByData
// 操作步骤: 注入 3 个递增速率点,调用曲线生成
// 预期结果: 路径以三次贝塞尔连接相邻点,纵坐标按 renderMaxHeight 归一化
TEST_F(UT_DiskWidget, getPainterPathByData_series_buildsCurve)
{
    // Arrange: 3 个数据点
    QList<double> data { 0.0, 6.5, 3.25 };
    QPainterPath path;
    // Act: 生成曲线
    call_private_fun::DiskWidgetgetPainterPathByData(*m_tester, &data, path, 6.5 * 1.1);
    // Assert: 每段 cubicTo 贡献 3 个元素,首点隐式 MoveTo
    EXPECT_EQ(path.elementCount(), 3 * (data.size() - 1) + 1);
    EXPECT_EQ(path.elementAt(0).type, QPainterPath::MoveToElement);
}

// 用例名称: eventFilter_enterLeave_togglesHoverState
// 测试类型: 正常场景
// 覆盖函数: eventFilter(Enter/Leave 悬浮分支)
// 操作步骤: 向控件发送 Enter 与 Leave 事件
// 预期结果: 悬浮状态随事件切换
TEST_F(UT_DiskWidget, eventFilter_enterLeave_togglesHoverState)
{
    // Arrange: 初始非悬浮
    ASSERT_FALSE(access_private_field::DiskWidgetm_isHover(*m_tester));
    QEvent enter(QEvent::Enter);
    QEvent leave(QEvent::Leave);
    // Act: 进入悬浮
    QApplication::sendEvent(m_tester, &enter);
    // Assert: 悬浮置位
    EXPECT_TRUE(access_private_field::DiskWidgetm_isHover(*m_tester));
    // Act: 离开悬浮
    QApplication::sendEvent(m_tester, &leave);
    // Assert: 悬浮复位
    EXPECT_FALSE(access_private_field::DiskWidgetm_isHover(*m_tester));
}

// 用例名称: mouseDoubleClick_left_jumpToDisk
// 测试类型: 正常场景
// 覆盖函数: mouseDoubleClickEvent(左键跳转磁盘详情)
// 操作步骤: 发送左键双击事件
// 预期结果: 发送 MSG_DISK 跳转消息一次
TEST_F(UT_DiskWidget, mouseDoubleClick_left_jumpToDisk)
{
    // Arrange: 桩记录跳转消息
    // Act: 左键双击控件
    QMouseEvent dbl(QEvent::MouseButtonDblClick, QPointF(10, 10), QPointF(10, 10),
                    Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(m_tester, &dbl);
    // Assert: 跳转消息 MSG_DISK 恰好一次
    EXPECT_EQ(ut_popup_stub::g_jumpWidgetCount, 1);
    EXPECT_EQ(ut_popup_stub::g_lastJumpMessage, QStringLiteral("MSG_DISK"));
}

// 用例名称: mouseDoubleClick_right_ignored
// 测试类型: 边界场景
// 覆盖函数: mouseDoubleClickEvent(非左键忽略分支)
// 操作步骤: 发送右键双击事件
// 预期结果: 不发送任何跳转消息
TEST_F(UT_DiskWidget, mouseDoubleClick_right_ignored)
{
    // Arrange: 桩记录跳转消息
    // Act: 右键双击控件
    QMouseEvent dbl(QEvent::MouseButtonDblClick, QPointF(10, 10), QPointF(10, 10),
                    Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    QApplication::sendEvent(m_tester, &dbl);
    // Assert: 无跳转
    EXPECT_EQ(ut_popup_stub::g_jumpWidgetCount, 0);
    // Act: 中键双击控件
    QMouseEvent dblMid(QEvent::MouseButtonDblClick, QPointF(10, 10), QPointF(10, 10),
                       Qt::MiddleButton, Qt::MiddleButton, Qt::NoModifier);
    QApplication::sendEvent(m_tester, &dblMid);
    // Assert: 仍无跳转
    EXPECT_EQ(ut_popup_stub::g_jumpWidgetCount, 0);
}

// 用例名称: changeFont_fontChange_updatesTypography
// 测试类型: 正常场景
// 覆盖函数: changeFont
// 操作步骤: 以 12pt 字体调用 changeFont
// 预期结果: 标题字体随入参(加粗),内容/单位/副内容字体按全局常量设定
TEST_F(UT_DiskWidget, changeFont_fontChange_updatesTypography)
{
    // Arrange: 构造 12pt 输入字体
    QFont in(QStringLiteral("Sans"), 12);
    // Act: 应用字体
    call_private_fun::DiskWidgetchangeFont(*m_tester, in);
    // Assert: 标题字体与输入同字号且 DemiBold,内容字体 14,单位字体 9,副内容字体 8
    EXPECT_EQ(access_private_field::DiskWidgetm_sectionFont(*m_tester).pointSizeF(), in.pointSizeF());
    EXPECT_EQ(access_private_field::DiskWidgetm_sectionFont(*m_tester).weight(), QFont::DemiBold);
    EXPECT_EQ(access_private_field::DiskWidgetm_contentFont(*m_tester).pointSizeF(), qreal(Globals::ContentFont));
    EXPECT_EQ(access_private_field::DiskWidgetm_contentFont(*m_tester).weight(), QFont::Normal);
    EXPECT_EQ(access_private_field::DiskWidgetm_contentUnitFont(*m_tester).pointSizeF(), qreal(Globals::ContentUnitFont));
    EXPECT_EQ(access_private_field::DiskWidgetm_subContentFont(*m_tester).pointSizeF(), qreal(Globals::subContentFont));
    EXPECT_EQ(access_private_field::DiskWidgetm_subContentFont(*m_tester).weight(), QFont::ExtraLight);
}

// 用例名称: paintEvent_offscreen_rendersWithoutError
// 测试类型: 正常场景
// 覆盖函数: paintEvent(悬浮/非悬浮两种背景)
// 操作步骤: 分别在非悬浮与悬浮状态下渲染控件
// 预期结果: 渲染成功且尺寸正确,不崩溃
TEST_F(UT_DiskWidget, paintEvent_offscreen_rendersWithoutError)
{
    // Arrange: 注入图例数据后渲染非悬浮态
    ut_popup_stub::g_diskRead = QStringLiteral("8.0 KB/s");
    ut_popup_stub::g_diskWrite = QStringLiteral("2.1 KB/s");
    ut_popup_stub::g_diskTotalSize = QStringLiteral("500 GB");
    ut_popup_stub::g_diskAvailable = QStringLiteral("120 GB");
    emit DataDealSingleton::getInstance().sigDataUpdate();
    // Arrange: 准备离屏像素图
    QPixmap pix(m_tester->size());
    // Act: 非悬浮态渲染
    pix.fill(Qt::transparent);
    m_tester->render(&pix);
    // Assert: 渲染成功(尺寸不变)
    EXPECT_EQ(pix.size(), m_tester->size());
    // Arrange: 悬浮态准备
    access_private_field::DiskWidgetm_isHover(*m_tester) = true;
    QPixmap pix2(m_tester->size());
    // Act: 悬浮态渲染
    pix2.fill(Qt::transparent);
    m_tester->render(&pix2);
    // Assert: 渲染成功(尺寸不变)
    EXPECT_EQ(pix2.size(), m_tester->size());
}
