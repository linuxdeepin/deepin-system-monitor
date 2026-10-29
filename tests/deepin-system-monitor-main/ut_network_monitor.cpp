// Copyright (C) 2019-2026 Uniontech Software Technology Co.,Ltd
// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//self
#include "network_monitor.h"

//dependence
#include "system/net_info.h"
#include "system/device_db.h"
#include "system/system_monitor.h"

//gtest
#include "stub.h"
#include <gtest/gtest.h>

//qt
#include <QPainterPath>
#include <QMouseEvent>
#include <QSignalSpy>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <DApplicationHelper>
DWIDGET_USE_NAMESPACE
using ThemeHelper = DApplicationHelper;
#else
#include <DGuiApplicationHelper>
DGUI_USE_NAMESPACE
using ThemeHelper = DGuiApplicationHelper;
#endif

/***************************************STUB begin*********************************************/
using namespace core::system;

namespace {
// NetworkMonitor::updateStatus() 依赖 DeviceDB::instance()->netInfo()，
// 用打桩隔离真实设备/proc 数据源，测试数据完全由本文件控制
static qreal s_recvBps = 0.0;
static qreal s_sentBps = 0.0;
static qulonglong s_totalRecvBytes = 0;
static qulonglong s_totalSentBytes = 0;

static SystemMonitor *stub_SystemMonitor_instance()
{
    // 返回 nullptr，阻止构造重型 SystemMonitor 单例（其内部会创建 SysInfo/DeviceDB 等真实数据源）
    return nullptr;
}

static DeviceDB *stub_DeviceDB_instance()
{
    // 返回 nullptr；netInfo() 同样被打桩，不会解引用 this
    return nullptr;
}

static NetInfo *stub_DeviceDB_netInfo(DeviceDB *)
{
    static NetInfo fakeNetInfo;
    return &fakeNetInfo;
}

static qreal stub_NetInfo_recvBps(NetInfo *)
{
    return s_recvBps;
}

static qreal stub_NetInfo_sentBps(NetInfo *)
{
    return s_sentBps;
}

static qulonglong stub_NetInfo_totalRecvBytes(NetInfo *)
{
    return s_totalRecvBytes;
}

static qulonglong stub_NetInfo_totalSentBytes(NetInfo *)
{
    return s_totalSentBytes;
}
}  // namespace
/***************************************STUB end***********************************************/

static const int kPointsNumber = 30;  // network_monitor.cpp 内 const int pointsNumber = 30

// 分支清单（源自 network_monitor.cpp）:
// 1. ctor: 固定宽 common::getStatusBarMaxWidth()=280、固定高 180、双速率列表各 pointsNumber+1 项
// 2. changeTheme: LightType -> light 图标 | DarkType -> dark 图标 | default -> 不改图标；公共分支设置 m_frameColor.alphaF=0.3
// 3. changeFont: sectionFont=+12ptF、contentFont=Medium/-1ptF、subContentFont=-1ptF
// 4. updateStatus: 追加速率 -> size>pointsNumber+1 时 pop_front -> 构建 download/uploadPath -> update()
// 5. getPainterPathByData: for i<dataCount-1 -> y=renderMaxHeight*v/maxVlaue（maxVlaue=0 极值分支）
// 6. mouseReleaseEvent: LeftButton -> emit clicked("MSG_NET") | 其它按键不发射
// 7. mouseMoveEvent: 直接返回（不发射、不崩溃）
// 8. paintEvent: 完整绘制路径（grab 触发），每帧 setFixedHeight

class NetworkMonitorTest : public ::testing::Test
{
public:
    NetworkMonitorTest() : m_tester(nullptr) {}

    virtual void SetUp() override
    {
        // Arrange: 全量打桩，隔离 SystemMonitor/DeviceDB 真实数据源
        m_stub.set(ADDR(SystemMonitor, instance), stub_SystemMonitor_instance);
        m_stub.set(ADDR(DeviceDB, instance), stub_DeviceDB_instance);
        m_stub.set(ADDR(DeviceDB, netInfo), stub_DeviceDB_netInfo);
        m_stub.set(ADDR(NetInfo, recvBps), stub_NetInfo_recvBps);
        m_stub.set(ADDR(NetInfo, sentBps), stub_NetInfo_sentBps);
        m_stub.set(ADDR(NetInfo, totalRecvBytes), stub_NetInfo_totalRecvBytes);
        m_stub.set(ADDR(NetInfo, totalSentBytes), stub_NetInfo_totalSentBytes);

        s_recvBps = 0.0;
        s_sentBps = 0.0;
        s_totalRecvBytes = 0;
        s_totalSentBytes = 0;

        // Act
        m_tester = new NetworkMonitor();
        // Assert: 构造不崩溃
        ASSERT_NE(m_tester, nullptr);
    }

    virtual void TearDown() override
    {
        delete m_tester;
        m_tester = nullptr;
        // m_stub 为成员变量，析构时恢复全部函数补丁，保证用例间隔离
    }

protected:
    NetworkMonitor *m_tester;
    Stub m_stub;
};

// ───────────────────────── ctor ─────────────────────────
TEST_F(NetworkMonitorTest, Construction_DefaultInit_FixedSizeAndSpeedsListInitialized)
{
    // Arrange: SetUp 中已完成打桩（隔离重型单例）并构造控件
    // Act: 读取构造后的固定尺寸与速率列表状态
    const int fixedWidth = m_tester->width();
    const int fixedHeight = m_tester->height();
    const int dlSize = m_tester->downloadSpeeds->size();
    const int ulSize = m_tester->uploadSpeeds->size();
    const double dlFirst = m_tester->downloadSpeeds->first();
    const double ulFirst = m_tester->uploadSpeeds->first();
    // Assert: 宽度 = getStatusBarMaxWidth() 返回的固定值 280，高度 = 180
    EXPECT_EQ(fixedWidth, 280);
    EXPECT_EQ(fixedHeight, 180);
    // Assert: 两个速率列表均初始化为 pointsNumber+1 个 0 值
    EXPECT_NE(m_tester->downloadSpeeds, nullptr);
    EXPECT_NE(m_tester->uploadSpeeds, nullptr);
    EXPECT_EQ(dlSize, kPointsNumber + 1);
    EXPECT_EQ(ulSize, kPointsNumber + 1);
    EXPECT_DOUBLE_EQ(dlFirst, 0.0);
    EXPECT_DOUBLE_EQ(ulFirst, 0.0);
}

// ───────────────────────── updateStatus ─────────────────────────
TEST_F(NetworkMonitorTest, UpdateStatus_WhenInvoked_ReadsNetInfoAndAppendsSpeeds)
{
    // Arrange: 打桩返回受控网络数据
    s_recvBps = 1024.0;
    s_sentBps = 2048.0;
    s_totalRecvBytes = 100000;
    s_totalSentBytes = 200000;

    // Act
    m_tester->updateStatus();

    // Assert: 私有状态被刷新为打桩值
    EXPECT_DOUBLE_EQ(m_tester->m_recvBps, 1024.0);
    EXPECT_DOUBLE_EQ(m_tester->m_sentBps, 2048.0);
    EXPECT_EQ(m_tester->m_totalRecvBytes, 100000u);
    EXPECT_EQ(m_tester->m_totalSentBytes, 200000u);
    // Assert: 列表从初始 31 项 -> 追加 1 项(32) 超界 -> pop 1 项 = 31 项
    EXPECT_EQ(m_tester->downloadSpeeds->size(), kPointsNumber + 1);
    EXPECT_EQ(m_tester->uploadSpeeds->size(), kPointsNumber + 1);
    // Assert: 最新速率被追加到列表尾部
    EXPECT_DOUBLE_EQ(m_tester->downloadSpeeds->last(), 1024.0);
    EXPECT_DOUBLE_EQ(m_tester->uploadSpeeds->last(), 2048.0);
}

TEST_F(NetworkMonitorTest, UpdateStatus_WhenListAtBoundary_PopsOldestEntry)
{
    // Arrange: 清空列表后填满 pointsNumber+1 个可辨识的旧值（31 项，处于阈值边界）
    m_tester->downloadSpeeds->clear();
    for (int i = 0; i <= kPointsNumber; ++i)
        m_tester->downloadSpeeds->append(i * 1.0);
    const double oldestExpectedAfterPop = m_tester->downloadSpeeds->at(1);
    s_recvBps = 512.0;

    // Act: 追加后变为 32 > pointsNumber+1，应 pop 掉最老一项
    m_tester->updateStatus();

    // Assert: 窗口长度维持 pointsNumber+1，且最老一项（值为 0）已被移除
    EXPECT_EQ(m_tester->downloadSpeeds->size(), kPointsNumber + 1);
    EXPECT_DOUBLE_EQ(m_tester->downloadSpeeds->first(), oldestExpectedAfterPop);
    EXPECT_DOUBLE_EQ(m_tester->downloadSpeeds->last(), 512.0);
}

TEST_F(NetworkMonitorTest, UpdateStatus_WhenListBelowBoundary_KeepsAllEntries)
{
    // Arrange: 列表长度(5)小于 pointsNumber+1，不触发 pop 分支
    m_tester->downloadSpeeds->clear();
    m_tester->downloadSpeeds->append(1.0);
    m_tester->downloadSpeeds->append(2.0);
    m_tester->downloadSpeeds->append(3.0);
    m_tester->downloadSpeeds->append(4.0);
    m_tester->downloadSpeeds->append(5.0);
    m_tester->uploadSpeeds->clear();
    m_tester->uploadSpeeds->append(1.0);
    m_tester->uploadSpeeds->append(2.0);
    m_tester->uploadSpeeds->append(3.0);
    m_tester->uploadSpeeds->append(4.0);
    m_tester->uploadSpeeds->append(5.0);

    // Act
    m_tester->updateStatus();

    // Assert: 全部保留，仅各自追加 1 项
    EXPECT_EQ(m_tester->downloadSpeeds->size(), 6);
    EXPECT_EQ(m_tester->uploadSpeeds->size(), 6);
}

TEST_F(NetworkMonitorTest, UpdateStatus_WhenInvoked_BuildsDownloadAndUploadPaths)
{
    // Arrange
    s_recvBps = 640.0;
    s_sentBps = 320.0;

    // Act
    m_tester->updateStatus();

    // Assert: 路径由 31 个数据点生成，每相邻两点一段 cubicTo；空路径首次 cubicTo
    // 会隐式插入 moveTo(0,0)，元素总数 = 1 + 3*30 = 91
    EXPECT_EQ(m_tester->downloadPath.elementCount(), 1 + 3 * kPointsNumber);
    EXPECT_EQ(m_tester->uploadPath.elementCount(), 1 + 3 * kPointsNumber);
}

// ───────────────────────── getPainterPathByData ─────────────────────────
TEST_F(NetworkMonitorTest, GetPainterPathByData_WithEmptyList_ReturnsEmptyPath)
{
    // Arrange: 空数据列表
    QList<double> emptyList;

    // Act
    QPainterPath path;
    m_tester->getPainterPathByData(&emptyList, path, 20.0);

    // Assert: 循环体不执行，路径为空
    EXPECT_TRUE(path.isEmpty());
    EXPECT_EQ(path.elementCount(), 0);
}

TEST_F(NetworkMonitorTest, GetPainterPathByData_WithSinglePoint_ReturnsEmptyPath)
{
    // Arrange: 单点数据（dataCount-1 = 0，不进入循环）
    QList<double> single {42.0};

    // Act
    QPainterPath path;
    m_tester->getPainterPathByData(&single, path, 20.0);

    // Assert
    EXPECT_TRUE(path.isEmpty());
    EXPECT_EQ(path.elementCount(), 0);
}

TEST_F(NetworkMonitorTest, GetPainterPathByData_WithFivePoints_GeneratesTwelveElements)
{
    // Arrange: 5 个点 -> 隐式 moveTo + 4 段 cubicTo
    QList<double> data {0.0, 0.25, 0.5, 0.75, 1.0};

    // Act
    QPainterPath path;
    m_tester->getPainterPathByData(&data, path, 1.0);

    // Assert: 元素数 = 1 + 3*4 = 13
    EXPECT_EQ(path.elementCount(), 13);
    // Assert: 隐式 moveTo 落在原点
    EXPECT_DOUBLE_EQ(path.elementAt(0).y, 0.0);
}

TEST_F(NetworkMonitorTest, GetPainterPathByData_WithZeroMaxValue_GeneratesExtremePath)
{
    // Arrange: maxVlaue = 0（非法边界值），y 坐标将出现 inf/NaN，验证不崩溃
    QList<double> data {0.0, 1.0, 2.0};

    // Act
    QPainterPath path;
    m_tester->getPainterPathByData(&data, path, 0.0);

    // Assert: 除零分支产生 inf/NaN 坐标，Qt6 忽略无效坐标并保持路径为空（不崩溃）
    EXPECT_EQ(path.elementCount(), 0);
    EXPECT_TRUE(path.isEmpty());
}

// ───────────────────────── changeTheme ─────────────────────────
TEST_F(NetworkMonitorTest, ChangeTheme_LightType_SetsLightIconAndFrameColor)
{
    // Arrange: 先切到暗色主题，记录其图标渲染结果作为比对基线
    m_tester->changeTheme(ThemeHelper::DarkType);
    const QImage iconBefore = m_tester->m_icon.pixmap(24, 24).toImage();

    // Act
    m_tester->changeTheme(ThemeHelper::LightType);

    // Assert: 主题图标来自内置 qrc 资源，不应为空图标且与暗色主题图标不同
    EXPECT_FALSE(m_tester->m_icon.isNull());
    EXPECT_NE(m_tester->m_icon.pixmap(24, 24).toImage(), iconBefore);
    // Assert: 公共分支 m_frameColor = TextTips 且 alphaF ≈ 0.3（QColor 内部 16 位精度）
    EXPECT_NEAR(m_tester->m_frameColor.alphaF(), 0.3, 1e-3);
    // Assert: 文本颜色取自调色板，应为不透明有效颜色
    EXPECT_TRUE(m_tester->textColor.isValid());
    EXPECT_TRUE(m_tester->ltextColor.isValid());
}

TEST_F(NetworkMonitorTest, ChangeTheme_DarkType_SetsDarkIconAndFrameColor)
{
    // Arrange: 记录构造时默认主题初始化后的图标状态
    const QImage iconBefore = m_tester->m_icon.pixmap(24, 24).toImage();

    // Act
    m_tester->changeTheme(ThemeHelper::DarkType);

    // Assert: 记录构造时默认主题（light）初始化后的图标，切暗色后应不同
    EXPECT_FALSE(m_tester->m_icon.isNull());
    EXPECT_NE(m_tester->m_icon.pixmap(24, 24).toImage(), iconBefore);
    EXPECT_NEAR(m_tester->m_frameColor.alphaF(), 0.3, 1e-3);
}

TEST_F(NetworkMonitorTest, ChangeTheme_UnknownType_KeepsIconUnchanged)
{
    // Arrange: 先设为暗色主题，记录当前图标渲染结果
    m_tester->changeTheme(ThemeHelper::DarkType);
    const QImage iconBefore = m_tester->m_icon.pixmap(24, 24).toImage();

    // Act: UnknownType 走 default 分支，不重设图标
    m_tester->changeTheme(ThemeHelper::UnknownType);

    // Assert: 图标保持不变，但公共颜色分支仍然生效
    EXPECT_EQ(m_tester->m_icon.pixmap(24, 24).toImage(), iconBefore);
    EXPECT_NEAR(m_tester->m_frameColor.alphaF(), 0.3, 1e-3);
}

// ───────────────────────── changeFont ─────────────────────────
TEST_F(NetworkMonitorTest, ChangeFont_WithCustomFont_UpdatesAllThreeFonts)
{
    // Arrange
    QFont font;
    font.setPointSizeF(9.0);

    // Act
    m_tester->changeFont(font);

    // Assert: section 字体 +12ptF
    EXPECT_DOUBLE_EQ(m_tester->m_sectionFont.pointSizeF(), 21.0);
    // Assert: content 字体 Medium 字重且 -1ptF
    EXPECT_EQ(m_tester->m_contentFont.weight(), QFont::Medium);
    EXPECT_DOUBLE_EQ(m_tester->m_contentFont.pointSizeF(), 8.0);
    // Assert: subContent 字体 -1ptF
    EXPECT_DOUBLE_EQ(m_tester->m_subContentFont.pointSizeF(), 8.0);
}

// ───────────────────────── mouseReleaseEvent ─────────────────────────
TEST_F(NetworkMonitorTest, MouseReleaseEvent_LeftButton_EmitsClickedSignal)
{
    // Arrange
    QSignalSpy spy(m_tester, &NetworkMonitor::clicked);
    QMouseEvent ev(QEvent::MouseButtonRelease, QPointF(10, 10),
                   Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    // Act
    m_tester->mouseReleaseEvent(&ev);

    // Assert: 左键 -> 发射 clicked("MSG_NET")
    EXPECT_EQ(spy.count(), 1);
    ASSERT_EQ(spy.first().size(), 1);
    EXPECT_EQ(spy.first().first().toString(), QString("MSG_NET"));
}

TEST_F(NetworkMonitorTest, MouseReleaseEvent_RightButton_DoesNotEmitSignal)
{
    // Arrange
    QSignalSpy spy(m_tester, &NetworkMonitor::clicked);
    QMouseEvent ev(QEvent::MouseButtonRelease, QPointF(10, 10),
                   Qt::RightButton, Qt::RightButton, Qt::NoModifier);

    // Act: 右键释放
    m_tester->mouseReleaseEvent(&ev);

    // Assert: 非左键 -> 不发射
    EXPECT_EQ(spy.count(), 0);

    // Act: 随后左键释放，验证信号连接本身有效、过滤仅针对按键类型
    QMouseEvent leftEv(QEvent::MouseButtonRelease, QPointF(10, 10),
                       Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    m_tester->mouseReleaseEvent(&leftEv);

    // Assert
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.first().first().toString(), QString("MSG_NET"));
}

// ───────────────────────── mouseMoveEvent ─────────────────────────
TEST_F(NetworkMonitorTest, MouseMoveEvent_AnyMovement_IsIgnoredAndSilent)
{
    // Arrange: mouseMoveEvent 按设计屏蔽鼠标移动响应；随后以左键释放验证事件管线完好
    QSignalSpy spy(m_tester, &NetworkMonitor::clicked);
    QMouseEvent ev(QEvent::MouseMove, QPointF(5, 5),
                   Qt::NoButton, Qt::LeftButton, Qt::NoModifier);

    // Act: 两次不同位置的移动均不应产生任何点击
    m_tester->mouseMoveEvent(&ev);
    m_tester->mouseMoveEvent(&ev);

    // Assert: 不崩溃且不产生任何点击信号
    EXPECT_EQ(spy.count(), 0);

    // Act: 左键释放
    QMouseEvent relEv(QEvent::MouseButtonRelease, QPointF(5, 5),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    m_tester->mouseReleaseEvent(&relEv);

    // Assert: 点击事件管线未被移动事件破坏
    EXPECT_EQ(spy.count(), 1);
}

// ───────────────────────── paintEvent ─────────────────────────
TEST_F(NetworkMonitorTest, PaintEvent_AfterGrab_RendersPixmapWithWidgetSize)
{
    // Arrange: 注入受控数据后触发绘制
    s_recvBps = 1024.0;
    s_sentBps = 2048.0;
    s_totalRecvBytes = 12345;
    s_totalSentBytes = 6789;
    m_tester->updateStatus();

    // Act: 第一次 grab 触发 paintEvent（内部会按内容重设固定高度）
    const QPixmap firstPix = m_tester->grab();
    // Act: 尺寸稳定后再次 grab
    const QPixmap pix = m_tester->grab();

    // Assert: 渲染非空且尺寸与控件一致
    EXPECT_FALSE(firstPix.isNull());
    EXPECT_FALSE(pix.isNull());
    EXPECT_EQ(pix.size(), m_tester->size());
}
