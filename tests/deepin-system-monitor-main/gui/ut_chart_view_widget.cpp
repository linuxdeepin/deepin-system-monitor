// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// =====================================================================================
// 用例计数声明表
// | 方法 | 等级 | factors | min | actual |
// |---|---|---|---|---|
// | ChartViewWidget | low | cx:0 | 1 | 1 |
// | setData1Color | low | cx:0 | 1 | 1 |
// | setData2Color | low | cx:0 | 1 | 1 |
// | addData1 | mid | cx:6 | 1 | 4 |
// | addData2 | mid | cx:6 | 1 | 4 |
// | setSpeedAxis | low | cx:0 | 1 | 2 |
// | paintEvent | low | cx:0 | 1 | 1 |
// | resizeEvent | low | cx:0 | 1 | 1 |
// | drawBackPixmap | mid | cx:0 | 1 | 2 |
// | drawData1 | low | cx:0 | 1 | 2 |
// | drawData2 | low | cx:0 | 1 | 2 |
// | drawAxisText | low | cx:0 | 1 | 2 |
// | getPainterPathByData | mid | cx:6 | 3 | 4 |
// | changeFont | mid | cx:0 | 1 | 1 |
// | changeTheme | low | cx:0 | 1 | 1 |
// =====================================================================================

// 分支清单（来源：chart_view_widget.cpp ChartViewWidget::addData1）
//   B1: m_listData1.size() > allDatacount+1(=31) → pop_front 淘汰最旧样本 [AddData1_FullList_PopsOldestData]
//   B2: maxdata.toLongLong() > 0 → 进入正数最大值更新分支 [AddData1_FirstPositiveData_UpdatesMax]
//   B3: maxdata != m_maxData1 → 更新 m_maxData1 并同步 m_maxData [AddData1_FirstPositiveData_UpdatesMax]
//   B4: m_speedAxis 且 viewType != NET_CHART → formatUnit_memory_disk 标题 [AddData1_SpeedAxisMemChart_UpdatesAxisTitle]
//   B5: m_speedAxis 且 viewType == NET_CHART → formatUnit_net 标题 [SetSpeedAxis_NetChart_UsesNetUnitTitle]
//   B6: maxdata == 0（else 分支）且 m_speedAxis → 零值标题回退 [AddData1_ZeroData_SpeedAxisMemChart_ShowsZeroTitle]

// 分支清单（来源：chart_view_widget.cpp ChartViewWidget::addData2）
//   B1: m_listData2.size() > allDatacount+1(=31) → pop_front 淘汰最旧样本 [AddData2_FullList_PopsOldestData]
//   B2: maxdata.toLongLong() > 0 → 进入正数最大值更新分支 [AddData2_FirstPositiveData_UpdatesMax]
//   B3: maxdata != m_maxData2 → 更新 m_maxData2 并同步 m_maxData [AddData2_FirstPositiveData_UpdatesMax]
//   B4: m_speedAxis → 进入速度标题分支 [AddData2_SpeedAxisMemChart_UpdatesAxisTitle]
//   B5: B4 内层 viewType != NET_CHART → formatUnit_memory_disk [AddData2_SpeedAxisMemChart_UpdatesAxisTitle]
//   B6: B4 的 else 内层 if(m_speedAxis) 为不可达死分支（疑似源码缺陷, 只标红不改源码, 观察标题保持默认） [AddData2_ZeroData_KeepsAxisTitleDefault]

// 分支清单（来源：chart_view_widget.cpp ChartViewWidget::drawBackPixmap）
//   B1: width/height 为 0 → 早退 return, 背景与网格区不重建 [DrawBackPixmap_ZeroSize_LeavesPixmapNull]
//   B2: while gridLineX 竖向网格线循环（0/1/N 次） [DrawBackPixmap_NewSize_CutsGridRect]
//   B3: while gridLineY 横向网格线循环（0/1/N 次） [DrawBackPixmap_NewSize_CutsGridRect]

// 分支清单（来源：chart_view_widget.cpp ChartViewWidget::getPainterPathByData）
//   B1: 首点 canConvert(Double) → toDouble 取值路径 [GetPainterPathByData_DataVariants_BuildsExpectedPath/Doubles4]
//   B2: 首点不可转 Double → toLongLong 取值路径（空 QVariant） [GetPainterPathByData_DataVariants_BuildsExpectedPath/EmptyData]
//   B3: for 循环 0 次（单点仅 moveTo）或 N 次（moveTo + (n-1)×cubicTo） [GetPainterPathByData_DataVariants_BuildsExpectedPath/Single 与 /Ints3]
//   B4: 循环体内 toDouble/toLongLong 双分支（与 B1/B2 同构, 数值型样本走 toDouble） [GetPainterPathByData_DataVariants_BuildsExpectedPath/Ints3]

#include "chart_view_widget.h"

#include <DFontSizeManager>
#include <gtest/gtest.h>

#include <QColor>
#include <DFontSizeManager>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>

DWIDGET_USE_NAMESPACE
#include <QPixmap>
#include <QVariant>

// 像素扫描辅助：画布中是否存在接近指定颜色的像素（抗锯齿容差）
static bool hasPixelNear(const QImage &image, const QColor &color, int tolerance = 60)
{
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor pixel = image.pixelColor(x, y);
            if (qAbs(pixel.red() - color.red()) <= tolerance
                    && qAbs(pixel.green() - color.green()) <= tolerance
                    && qAbs(pixel.blue() - color.blue()) <= tolerance)
                return true;
        }
    }
    return false;
}

// 像素扫描辅助：画布中是否存在非纯白像素（文字/线条已绘制）
static bool hasNonWhitePixel(const QImage &image)
{
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor pixel = image.pixelColor(x, y);
            if (pixel.red() < 250 || pixel.green() < 250 || pixel.blue() < 250)
                return true;
        }
    }
    return false;
}

class UT_ChartViewWidget : public ::testing::Test
{
public:
    UT_ChartViewWidget() : m_tester(nullptr) {}

protected:
    virtual void SetUp()
    {
        // Arrange(夹具): 每个用例创建独立的内存监控图表
        m_tester = new ChartViewWidget(ChartViewWidget::ChartViewTypes::MEM_CHART, nullptr);
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

    ChartViewWidget *m_tester;
};

TEST_F(UT_ChartViewWidget, Constructor_DefaultState_InitializesMembers)
{
    // Arrange: 夹具已构造默认 MEM 图表
    ASSERT_NE(m_tester, nullptr);

    // Act: 以 MEM_CHART 视图类型构造被测对象
    delete m_tester;
    m_tester = new ChartViewWidget(ChartViewWidget::ChartViewTypes::MEM_CHART, nullptr);

    // Assert: 默认标题/数据列表/最大值与视图类型均为初始状态
    EXPECT_EQ(m_tester->m_axisTitle, QString("100%"));
    EXPECT_EQ(m_tester->m_listData1.size(), 0);
    EXPECT_EQ(m_tester->m_listData2.size(), 0);
    EXPECT_EQ(m_tester->m_maxData.toLongLong(), qlonglong(1));
    EXPECT_EQ(m_tester->m_viewType, ChartViewWidget::ChartViewTypes::MEM_CHART);
}

TEST_F(UT_ChartViewWidget, SetData1Color_CustomColor_SetsMemberColor)
{
    // Arrange: 夹具已构造默认图表
    ASSERT_NE(m_tester, nullptr);

    // Act: 先后设置两种颜色
    m_tester->setData1Color(QColor(255, 0, 0));
    // Assert: 成员颜色与设置值一致
    EXPECT_EQ(m_tester->m_data1Color, QColor(255, 0, 0));

    m_tester->setData1Color(QColor(0, 0, 255));
    // Assert: 二次设置覆盖前值
    EXPECT_EQ(m_tester->m_data1Color, QColor(0, 0, 255));
}

TEST_F(UT_ChartViewWidget, SetData2Color_CustomColor_SetsMemberColor)
{
    // Arrange: 夹具已构造默认图表
    ASSERT_NE(m_tester, nullptr);

    // Act: 先后设置两种颜色
    m_tester->setData2Color(QColor(255, 215, 0));
    // Assert: 成员颜色与设置值一致
    EXPECT_EQ(m_tester->m_data2Color, QColor(255, 215, 0));

    m_tester->setData2Color(QColor(0, 128, 0));
    // Assert: 二次设置覆盖前值
    EXPECT_EQ(m_tester->m_data2Color, QColor(0, 128, 0));
}

TEST_F(UT_ChartViewWidget, AddData1_FirstPositiveData_UpdatesMax)
{
    // Arrange: 夹具已构造默认图表
    ASSERT_NE(m_tester, nullptr);

    // Act: 写入首个正值数据
    m_tester->addData1(QVariant(20));

    // Assert: 最大值按 1.1 倍放大, 总最大值同步, 列表记录样本
    EXPECT_EQ(m_tester->m_maxData1.toLongLong(), qlonglong(22));
    EXPECT_EQ(m_tester->m_maxData.toLongLong(), qlonglong(22));
    EXPECT_EQ(m_tester->m_listData1.size(), 1);
}

TEST_F(UT_ChartViewWidget, AddData1_FullList_PopsOldestData)
{
    // Arrange: 填满窗口（allDatacount=30，容量上限 31）
    for (int i = 1; i <= 40; ++i)
        m_tester->addData1(QVariant(i));

    // Act: 超限后再写入一个较小值
    m_tester->addData1(QVariant(20));

    // Assert: 最旧样本被淘汰, 列表维持上限容量, 最大值仍来自历史峰值 40
    EXPECT_EQ(m_tester->m_listData1.size(), 31);
    EXPECT_EQ(m_tester->m_listData1.first().toLongLong(), qlonglong(11));
    EXPECT_EQ(m_tester->m_maxData1.toLongLong(), qlonglong(44));
}

TEST_F(UT_ChartViewWidget, AddData1_SpeedAxisMemChart_UpdatesAxisTitle)
{
    // Arrange: 打开速度轴（MEM 图表 → formatUnit_memory_disk 文案）
    m_tester->setSpeedAxis(true);

    // Act: 写入正值数据
    m_tester->addData1(QVariant(20));

    // Assert: 标题由默认 "100%" 更新为容量速度文案
    EXPECT_EQ(m_tester->m_maxData1.toLongLong(), qlonglong(22));
    EXPECT_NE(m_tester->m_axisTitle, QString("100%"));
    EXPECT_FALSE(m_tester->m_axisTitle.isEmpty());
}

TEST_F(UT_ChartViewWidget, AddData1_ZeroData_SpeedAxisMemChart_ShowsZeroTitle)
{
    // Arrange
    m_tester->setSpeedAxis(true);

    // Act: 写入零值 → else 分支把标题置零值文案
    m_tester->addData1(QVariant(0));

    // Assert: 最大值保持初始 1, 标题被置为零值文案
    EXPECT_EQ(m_tester->m_maxData1.toLongLong(), qlonglong(1));
    EXPECT_NE(m_tester->m_axisTitle, QString("100%"));
    EXPECT_FALSE(m_tester->m_axisTitle.isEmpty());
}

TEST_F(UT_ChartViewWidget, AddData2_FirstPositiveData_UpdatesMax)
{
    // Arrange: 夹具已构造默认图表
    ASSERT_NE(m_tester, nullptr);

    // Act: 写入首个正值数据
    m_tester->addData2(QVariant(20));

    // Assert: 最大值按 1.1 倍放大, 总最大值同步, 列表记录样本
    EXPECT_EQ(m_tester->m_maxData2.toLongLong(), qlonglong(22));
    EXPECT_EQ(m_tester->m_maxData.toLongLong(), qlonglong(22));
    EXPECT_EQ(m_tester->m_listData2.size(), 1);
}

TEST_F(UT_ChartViewWidget, AddData2_FullList_PopsOldestData)
{
    // Arrange: 填满窗口（容量上限 31）
    for (int i = 1; i <= 40; ++i)
        m_tester->addData2(QVariant(i));

    // Act: 超限后再写入一个较小值
    m_tester->addData2(QVariant(20));

    // Assert: 最旧样本被淘汰, 最大值仍来自历史峰值 40
    EXPECT_EQ(m_tester->m_listData2.size(), 31);
    EXPECT_EQ(m_tester->m_listData2.first().toLongLong(), qlonglong(11));
    EXPECT_EQ(m_tester->m_maxData2.toLongLong(), qlonglong(44));
}

TEST_F(UT_ChartViewWidget, AddData2_SpeedAxisMemChart_UpdatesAxisTitle)
{
    // Arrange
    m_tester->setSpeedAxis(true);

    // Act: 写入正值数据
    m_tester->addData2(QVariant(20));

    // Assert: 标题由默认 "100%" 更新为容量速度文案
    EXPECT_EQ(m_tester->m_maxData2.toLongLong(), qlonglong(22));
    EXPECT_NE(m_tester->m_axisTitle, QString("100%"));
    EXPECT_FALSE(m_tester->m_axisTitle.isEmpty());
}

TEST_F(UT_ChartViewWidget, AddData2_ZeroData_KeepsAxisTitleDefault)
{
    // Arrange: 先预置零值样本, 避免空列表 max_element 未定义行为
    m_tester->setSpeedAxis(true);
    m_tester->m_listData2 << QVariant(0);

    // Act: 写入零值; addData2 的零值标题回退位于 else(m_speedAxis) 内层,
    // 实际不可达（疑似源码缺陷, 只标红不改源码）; 零值样本不触发外层 max 分支
    QString titleBefore = m_tester->m_axisTitle;
    m_tester->addData2(QVariant(0));

    // Assert: 最大值保持初始 1, 标题维持 setSpeedAxis 生成的当前值不变
    EXPECT_EQ(m_tester->m_maxData2.toLongLong(), qlonglong(1));
    EXPECT_EQ(m_tester->m_axisTitle, titleBefore);
}

TEST_F(UT_ChartViewWidget, SetSpeedAxis_MemChart_UsesMemoryUnitTitle)
{
    // Arrange: 夹具已构造默认 MEM 图表
    ASSERT_NE(m_tester, nullptr);

    // Act: 打开速度轴
    m_tester->setSpeedAxis(true);

    // Assert: 依据 m_maxData1/m_maxData2 默认值 1 生成速度文案
    EXPECT_TRUE(m_tester->m_speedAxis);
    EXPECT_NE(m_tester->m_axisTitle, QString("100%"));
    EXPECT_FALSE(m_tester->m_axisTitle.isEmpty());
}

TEST_F(UT_ChartViewWidget, SetSpeedAxis_NetChart_UsesNetUnitTitle)
{
    // Arrange: 网络视图走 formatUnit_net 分支
    delete m_tester;
    m_tester = new ChartViewWidget(ChartViewWidget::ChartViewTypes::NET_CHART, nullptr);

    // Act
    m_tester->setSpeedAxis(true);

    // Assert: 网络图表标题同样被更新为速度文案
    EXPECT_TRUE(m_tester->m_speedAxis);
    EXPECT_NE(m_tester->m_axisTitle, QString("100%"));
    EXPECT_FALSE(m_tester->m_axisTitle.isEmpty());
}

TEST_F(UT_ChartViewWidget, PaintEvent_WithData_GrabsValidPixmap)
{
    // Arrange: 给定尺寸并注入两条数据序列
    m_tester->resize(200, 100);
    m_tester->addData1(QVariant(20));
    m_tester->addData2(QVariant(15));

    // Act: grab() 触发 paintEvent 渲染
    const QPixmap grabbed = m_tester->grab();

    // Assert: 渲染产物有效且与控件同尺寸
    EXPECT_FALSE(grabbed.isNull());
    EXPECT_EQ(grabbed.size(), QSize(200, 100));
}

TEST_F(UT_ChartViewWidget, ResizeEvent_NewSize_RerendersBackPixmap)
{
    // Arrange: 夹具已构造默认图表; 未 show 的控件 resize() 只改几何不投递事件,
    // 因此先设定实际几何, 再手动投递 resizeEvent（drawBackPixmap 读取 this->size()）
    ASSERT_NE(m_tester, nullptr);
    m_tester->resize(300, 200);

    // Act: 直接投递新尺寸的 resizeEvent
    QResizeEvent re(QSize(300, 200), QSize(0, 0));
    m_tester->resizeEvent(&re);

    // Assert: resizeEvent 内部重建背景图, 网格区宽度 = 控件宽 - 2
    EXPECT_EQ(m_tester->m_backPixmap.size(), QSize(300, 200));
    EXPECT_EQ(m_tester->m_chartRect.width(), 298);
}

TEST_F(UT_ChartViewWidget, DrawBackPixmap_NewSize_CutsGridRect)
{
    // Arrange: 给定控件尺寸
    m_tester->resize(150, 120);

    // Act: 直接重绘背景
    m_tester->drawBackPixmap();

    // Assert: 背景尺寸一致, 网格区从 x=1 开始且宽为 w-2, 顶部留出字号高度
    EXPECT_EQ(m_tester->m_backPixmap.size(), QSize(150, 120));
    EXPECT_EQ(m_tester->m_chartRect.width(), 148);
    EXPECT_EQ(m_tester->m_chartRect.y(), 1 + QFontMetrics(m_tester->m_textfont).height());
}

TEST_F(UT_ChartViewWidget, DrawBackPixmap_ZeroSize_LeavesPixmapNull)
{
    // Arrange: 零尺寸控件触发早退分支
    m_tester->setGeometry(0, 0, 0, 0);

    // Act
    m_tester->drawBackPixmap();

    // Assert: 背景图未被创建, 网格区未更新
    EXPECT_TRUE(m_tester->m_backPixmap.isNull());
    EXPECT_EQ(m_tester->m_backPixmap.size(), QSize(0, 0));
}

TEST_F(UT_ChartViewWidget, DrawData1_WithData_DrawsRedLine)
{
    // Arrange: 100x100 白底画布, 构造裁剪区与两条数据点（最大值 100 保证坐标有限）
    QPixmap pixmap(100, 100);
    pixmap.fill(Qt::white);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    m_tester->m_chartRect = QRect(0, 0, 100, 100);
    m_tester->setData1Color(QColor(255, 0, 0));
    m_tester->m_listData1 << QVariant(20) << QVariant(50);
    m_tester->m_maxData = QVariant(100);

    // Act
    m_tester->drawData1(&painter);

    // Assert: 画布出现红色系像素（曲线已绘制）, 数据列表保持两条
    EXPECT_TRUE(hasPixelNear(pixmap.toImage(), QColor(255, 0, 0)));
    EXPECT_EQ(m_tester->m_listData1.size(), 2);
}

TEST_F(UT_ChartViewWidget, DrawData1_NoData_SkipsDrawing)
{
    // Arrange: 空数据列表与白底画布
    QPixmap pixmap(100, 100);
    pixmap.fill(Qt::white);
    QPainter painter(&pixmap);

    // Act: 空列表走早退分支
    m_tester->drawData1(&painter);

    // Assert: 画笔未被销毁, 画布无线色像素, 列表仍为空
    EXPECT_TRUE(painter.isActive());
    EXPECT_EQ(m_tester->m_listData1.size(), 0);
    EXPECT_FALSE(hasPixelNear(pixmap.toImage(), QColor(0, 197, 192)));
}

TEST_F(UT_ChartViewWidget, DrawData2_WithData_DrawsYellowLine)
{
    // Arrange: 白底画布 + 裁剪区 + 两条数据点（最大值 120 保证坐标有限）
    QPixmap pixmap(100, 100);
    pixmap.fill(Qt::white);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    m_tester->m_chartRect = QRect(0, 0, 100, 100);
    m_tester->setData2Color(QColor(255, 215, 0));
    m_tester->m_listData2 << QVariant(30) << QVariant(60);
    m_tester->m_maxData = QVariant(120);

    // Act
    m_tester->drawData2(&painter);

    // Assert: 画布出现黄色系像素（曲线已绘制）, 数据列表保持两条
    EXPECT_TRUE(hasPixelNear(pixmap.toImage(), QColor(255, 215, 0)));
    EXPECT_EQ(m_tester->m_listData2.size(), 2);
}

TEST_F(UT_ChartViewWidget, DrawData2_NoData_SkipsDrawing)
{
    // Arrange: 空数据列表与白底画布
    QPixmap pixmap(100, 100);
    pixmap.fill(Qt::white);
    QPainter painter(&pixmap);

    // Act: 空列表走早退分支
    m_tester->drawData2(&painter);

    // Assert: 画笔未被销毁, 列表仍为空, 画布无线色像素
    EXPECT_TRUE(painter.isActive());
    EXPECT_EQ(m_tester->m_listData2.size(), 0);
    EXPECT_FALSE(hasPixelNear(pixmap.toImage(), QColor(255, 215, 0)));
}

TEST_F(UT_ChartViewWidget, DrawAxisText_WithTitle_DrawsTexts)
{
    // Arrange: 白底画布与标题文本
    QPixmap pixmap(200, 100);
    pixmap.fill(Qt::white);
    QPainter painter(&pixmap);
    m_tester->resize(200, 100);
    m_tester->setAxisTitle("1.5MB");

    // Act
    m_tester->drawAxisText(&painter);

    // Assert: 标题/底部 "0"/"60 seconds" 以 0.3 透明度灰字绘制在白底上
    EXPECT_TRUE(hasNonWhitePixel(pixmap.toImage()));
    EXPECT_NE(painter.device(), nullptr);
}

TEST_F(UT_ChartViewWidget, DrawAxisText_NoTitle_StillDrawsBottomTexts)
{
    // Arrange: 空标题时底部文本仍应绘制
    QPixmap pixmap(200, 100);
    pixmap.fill(Qt::white);
    QPainter painter(&pixmap);
    m_tester->resize(200, 100);
    m_tester->setAxisTitle(QString());

    // Act
    m_tester->drawAxisText(&painter);

    // Assert: 底部文本已绘制, 标题保持为空
    EXPECT_TRUE(hasNonWhitePixel(pixmap.toImage()));
    EXPECT_EQ(m_tester->m_axisTitle, QString());
}

// getPainterPathByData 参数化用例: 元素数 = 1(moveTo) + 3×cubicTo 数
struct PathCaseParam {
    const char *caseName;
    QList<QVariant> data;
    qlonglong maxY;
    int expectedElements;
    double expectedFirstY;
    double expectedLastX;
};

class UT_ChartViewWidgetPath : public ::testing::TestWithParam<PathCaseParam>
{
public:
    UT_ChartViewWidgetPath() : m_tester(nullptr) {}

protected:
    virtual void SetUp()
    {
        // Arrange(夹具): 固定网格区 120x100, distance = 120/30 = 4.0
        m_tester = new ChartViewWidget(ChartViewWidget::ChartViewTypes::MEM_CHART, nullptr);
        m_tester->m_chartRect = QRect(0, 0, 120, 100);
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

    ChartViewWidget *m_tester;
};

TEST_P(UT_ChartViewWidgetPath, GetPainterPathByData_DataVariants_BuildsExpectedPath)
{
    const PathCaseParam &param = GetParam();

    // Arrange: 注入参数化数据序列与最大值
    QPainterPath path;

    // Act: 按数据序列构建曲线路径
    m_tester->getPainterPathByData(param.data, path, QVariant(param.maxY));

    // Assert: 元素数/首点 Y（-h×last/max）/末点 X（-(n-1)×distance）均符合公式
    ASSERT_NE(m_tester, nullptr);
    EXPECT_EQ(path.elementCount(), param.expectedElements);
    EXPECT_DOUBLE_EQ(path.elementAt(0).y, param.expectedFirstY);
    EXPECT_DOUBLE_EQ(path.elementAt(path.elementCount() - 1).x, param.expectedLastX);
}

INSTANTIATE_TEST_SUITE_P(GetPainterPathByData, UT_ChartViewWidgetPath,
                         ::testing::Values(
                                 // [10,20,30], max=30: moveTo(0,-100) + 2×cubicTo = 7 元素, 末点 x=-8
                                 PathCaseParam { "Ints3", QList<QVariant>() << QVariant(10) << QVariant(20) << QVariant(30), 30, 7, -100.0, -8.0 },
                                 // 单点: 仅 moveTo(0,-100), 无 cubicTo
                                 PathCaseParam { "Single", QList<QVariant>() << QVariant(15), 15, 1, -100.0, 0.0 },
                                 // [1.5,2.5,3.5,4.5], max=5: 走 toDouble 分支, 10 元素, 末点 x=-12
                                 PathCaseParam { "Doubles4", QList<QVariant>() << QVariant(1.5) << QVariant(2.5) << QVariant(3.5) << QVariant(4.5), 5, 10, -90.0, -12.0 },
                                 // 空 QVariant 不可转 Double → toLongLong 分支, y=0
                                 PathCaseParam { "EmptyData", QList<QVariant>() << QVariant(), 30, 1, 0.0, 0.0 }),
                         [](const ::testing::TestParamInfo<PathCaseParam> &info) { return info.param.caseName; });

TEST_F(UT_ChartViewWidget, ChangeFont_T8Font_AppliesToMember)
{
    // Arrange: 预置一个自定义字体（家族不同, 确保可与 T8 区分）
    QFont custom("serif", 10);
    m_tester->m_textfont = custom;

    // Act
    m_tester->changeFont(custom);

    // Assert: 成员字体被替换为字号管理器 T8 字号
    EXPECT_EQ(m_tester->m_textfont, DFontSizeManager::instance()->get(DFontSizeManager::T8));
    EXPECT_NE(m_tester->m_textfont, custom);
}

TEST_F(UT_ChartViewWidget, ChangeTheme_ClearsState_RerendersBackPixmap)
{
    // Arrange: 先给一个尺寸并清空背景图, 使 changeTheme 的重绘效果可观察
    m_tester->resize(120, 90);
    m_tester->m_backPixmap = QPixmap();

    // Act: 主题变化 → 重建背景图并请求重绘
    m_tester->changeTheme();

    // Assert: 背景图按当前尺寸重建, 网格区宽度恢复
    EXPECT_EQ(m_tester->m_backPixmap.size(), QSize(120, 90));
    EXPECT_EQ(m_tester->m_chartRect.width(), 118);
}
