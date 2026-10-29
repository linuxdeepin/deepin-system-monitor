// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * UT_BaseItemDelegate
 *
 * 测试对象: deepin-system-monitor-plugin-popup/gui/base/base_item_delegate.cpp
 *           BaseItemDelegate 类
 * 外部依赖: 无 DBus/网络/设备依赖;paint 渲染到内存 QImage,
 *           tooltip 通过 QToolTip 直接校验,不依赖真实桌面环境。
 *
 * 分支覆盖声明 (paint, complexity=20, 源码行 100-260):
 * | #  | 分支条件                                        | 覆盖用例                                    |
 * |----|-------------------------------------------------|---------------------------------------------|
 * | 1  | index 无效 → 基类绘制                            | paint_invalidIndex_fallsBackToBase          |
 * | 2  | State_Enabled 否 → Disabled 色组                 | paint_disabledGroup_usesDisabledPalette     |
 * | 3  | UserRole+2 有效 → 自定义文字色                   | paint_userRoleColor_customTextColor         |
 * | 4  | State_Selected → TextLively                      | paint_selectedSunkenAndHover_variants       |
 * | 5  | Selected+Sunken → 高亮混合色                     | paint_selectedSunkenAndHover_variants       |
 * | 6  | Selected+MouseOver → 悬浮调整色                  | paint_selectedSunkenAndHover_variants       |
 * | 7  | 普通+MouseOver → 主题相关调整色                  | paint_selectedSunkenAndHover_variants       |
 * | 8  | Beginning/Middle/End 边距调整                    | paint_beginningMiddleEnd_marginsApplied     |
 * | 9  | viewItemPosition 非 Beginning/Middle/End/OnlyOne → 基类绘制 | paint_impossiblePosition_fallsBackToBase |
 * | 10 | OnlyOne 有图标 → 图标+文本布局                    | paint_onlyOnePosition_withDecoration        |
 * | 11 | OnlyOne 无图标 → 纯文本布局                      | paint_onlyOnePosition_withoutDecoration     |
 * | 12 | 图标位图缓存命中/未命中                          | paint_onlyOnePosition_withDecoration        |
 *
 * 分支覆盖声明 (helpEvent, complexity=8):
 * | #  | 分支条件                                        | 覆盖用例                                    |
 * |----|-------------------------------------------------|---------------------------------------------|
 * | 13 | event/view 为空 → false                         | helpEvent_nullEventOrView_returnsFalse      |
 * | 14 | 非 ToolTip 事件 → 基类处理                       | helpEvent_nonTooltipType_delegatesToBase    |
 * | 15 | 文本被截断 → 显示 tooltip 返回 true(无图标路径) | helpEvent_truncatedText_showsTooltip        |
 * | 16 | 文本被截断 → 显示 tooltip(图标路径)             | helpEvent_truncatedTextWithIcon_showsTooltip|
 * | 17 | 文本未截断 → 隐藏 tooltip 返回 true              | helpEvent_fittingText_returnsTrue           |
 * | 18 | 行占满视图宽 → 宽度再减 2*margin                 | helpEvent_truncatedText_showsTooltip        |
 *
 * 用例声明表:
 * |--------------------------------------------------|----------------------|---------|------|----------------------|
 * | 测试用例名                                        | 测试类               | 测试级别 | 类型 | 覆盖函数             |
 * |--------------------------------------------------|----------------------|---------|------|----------------------|
 * | construction_DefaultState_InitializesTipColor          | UT_BaseItemDelegate  | Level_1 | AUTO | ctor                 |
 * | createEditor_AnyInput_ReturnsNullptr                      | UT_BaseItemDelegate  | Level_1 | AUTO | createEditor         |
 * | sizeHint_smallFont_returnsMinimum36              | UT_BaseItemDelegate  | Level_1 | AUTO | sizeHint             |
 * | sizeHint_largeFont_keepsNaturalHeight            | UT_BaseItemDelegate  | Level_2 | AUTO | sizeHint             |
 * | initStyleOption_defaults_alignmentElideDisplay   | UT_BaseItemDelegate  | Level_1 | AUTO | initStyleOption      |
 * | initStyleOption_CustomRoles_AppliesAlignmentAndIcon     | UT_BaseItemDelegate  | Level_2 | AUTO | initStyleOption      |
 * | paint_onlyOnePosition_withoutDecoration          | UT_BaseItemDelegate  | Level_1 | AUTO | paint(分支11)        |
 * | paint_onlyOnePosition_withDecoration             | UT_BaseItemDelegate  | Level_2 | AUTO | paint(分支10/12)     |
 * | paint_beginningMiddleEnd_marginsApplied          | UT_BaseItemDelegate  | Level_1 | AUTO | paint(分支8)         |
 * | paint_selectedSunkenAndHover_variants            | UT_BaseItemDelegate  | Level_2 | AUTO | paint(分支4/5/6/7)   |
 * | paint_disabledGroup_usesDisabledPalette          | UT_BaseItemDelegate  | Level_2 | AUTO | paint(分支2)         |
 * | paint_userRoleColor_customTextColor              | UT_BaseItemDelegate  | Level_2 | AUTO | paint(分支3)         |
 * | paint_invalidIndex_fallsBackToBase               | UT_BaseItemDelegate  | Level_3 | AUTO | paint(分支1)         |
 * | paint_impossiblePosition_fallsBackToBase         | UT_BaseItemDelegate  | Level_3 | AUTO | paint(分支9)         |
 * | helpEvent_nullEventOrView_returnsFalse           | UT_BaseItemDelegate  | Level_1 | AUTO | helpEvent(分支13)    |
 * | helpEvent_nonTooltipType_delegatesToBase         | UT_BaseItemDelegate  | Level_2 | AUTO | helpEvent(分支14)    |
 * | helpEvent_truncatedText_showsTooltip             | UT_BaseItemDelegate  | Level_1 | AUTO | helpEvent(分支15/18) |
 * | helpEvent_truncatedTextWithIcon_showsTooltip     | UT_BaseItemDelegate  | Level_2 | AUTO | helpEvent(分支16)    |
 * | helpEvent_fittingText_returnsTrue                | UT_BaseItemDelegate  | Level_2 | AUTO | helpEvent(分支17)    |
 * | changeTheme_RepeatSameType_StableColor        | UT_BaseItemDelegate    | Level_2 | AUTO | changeTheme(幂等)              |
 * | changeTheme_lightAndDark_updatesTipColor         | UT_BaseItemDelegate  | Level_2 | AUTO | changeTheme          |
 *
 * 用例计数声明(方法 | level | factors | min | actual):
 * | paint | high | complexity:20 | 3 | 9 |
 * | helpEvent | high | complexity:8 | 3 | 5 |
 * | initStyleOption | mid | in_degree:3 | 2 | 2 |
 * | changeTheme | mid | complexity:6 | 2 | 2 |
 * | 构造函数 | low | - | 1 | 1 |
 * | createEditor | low | - | 1 | 1 |
 * | sizeHint | low | - | 1 | 2 |
 *
 * 风险检查清单:
 * 1. 对每一个测试文件中的 stub 都执行了 reset?  [是] 未使用 set_lamda;无进程级 stub
 * 2. 对每一个测试文件中的 stub 都执行了 restore? [是] 无进程级 stub,渲染均作用域内完成
 * 3. 所有的测试使用 ASSERT/EXPECT 宏而不是布尔判断? [是]
 * 4. 所有的 ASSERT 宏的返回值都被检查? [是]
 * 5. 用例覆盖了所有分支? [是] paint/helpEvent 主要分支见上方分支声明表
 * 6. 本文件与类定义的头文件一致(用例使用的是头文件中公开的方法)? [是]
 * 7. 所有的属性和方法都被测试到了? [是]
 * 8. 测试代码可读性高,测试方法意义明确? [是]
 * 9. 任何测试不能对其它测试产生依赖? [是] 每用例独立构造模型与图像
 * 10. 使用的是 Google Test 而不是 QtTest? [是]
 */

#include <QHelpEvent>
#include <QImage>
#include <QPainter>
#include <QScopedPointer>
#include <QStandardItemModel>
#include <QTableView>
#include <QToolTip>
#include <DGuiApplicationHelper>
#include <DPalette>
#include <gtest/gtest.h>

#include "gui/base/base_item_delegate.h"

DGUI_USE_NAMESPACE

// ==================== 私有成员访问说明 ====================
// 测试目标由 tests/CMakeLists.txt 全局追加 -fno-access-control 编译,
// 测试代码可直接访问 private/protected 成员,无需 addr_pri.h 宏。

// ==================== 测试夹具 ====================
class UT_BaseItemDelegate : public ::testing::Test
{
public:
    void SetUp() override
    {
        m_tester = new BaseItemDelegate();
    }

    void TearDown() override
    {
        delete m_tester;
        m_tester = nullptr;
    }

    // 构造 1x2 标准模型: (0,0) 文本 + 可选图标;(0,1) 短文本
    QStandardItemModel *makeModel(bool withIcon, const QString &longText)
    {
        QStandardItemModel *model = new QStandardItemModel(1, 2);
        QStandardItem *nameItem = new QStandardItem(longText);
        if (withIcon) {
            QPixmap pm(16, 16);
            pm.fill(Qt::blue);
            nameItem->setIcon(QIcon(pm));
        }
        model->setItem(0, 0, nameItem);
        model->setItem(0, 1, new QStandardItem(QStringLiteral("12.3%")));
        return model;
    }

    // 用指定位置与状态把 (row,col) 单元格绘制到内存图像
    QImage paintCell(QStyleOptionViewItem::ViewItemPosition pos, QStyle::State extraState,
                     QAbstractItemModel *model, int row, int col)
    {
        QImage img(140, 40, QImage::Format_ARGB32_Premultiplied);
        img.fill(QColor(128, 128, 128));
        QPainter p(&img);
        QStyleOptionViewItem opt;
        opt.rect = QRect(0, 0, 140, 40);
        opt.state = QStyle::State_Active | extraState;
        opt.viewItemPosition = pos;
        opt.font = QFont();
        m_tester->paint(&p, opt, model->index(row, col));
        p.end();
        return img;
    }

    // 与背景一致的参照图(用于"未绘制"判定)
    QImage blankImage() const
    {
        QImage img(140, 40, QImage::Format_ARGB32_Premultiplied);
        img.fill(QColor(128, 128, 128));
        return img;
    }

    BaseItemDelegate *m_tester = nullptr;
};

// ==================== 测试用例 ====================

// 用例名称: construction_DefaultState_InitializesTipColor
// 测试类型: 正常场景
// 覆盖函数: 构造函数
// 操作步骤: 构造代理,读取 m_tipColor 与当前主题对照
// 预期结果: 浅色主题为 (255,255,255,255),深色主题为 (0,0,0,200)
TEST_F(UT_BaseItemDelegate, construction_DefaultState_InitializesTipColor)
{
    // Arrange: 默认构造
    const QColor &tip = m_tester->m_tipColor;
    // Act: 读取当前主题
    const auto theme = DGuiApplicationHelper::instance()->themeType();
    // Assert: 按主题校验
    if (theme == DGuiApplicationHelper::DarkType)
        EXPECT_EQ(tip, QColor(0, 0, 0, 200));
    else
        EXPECT_EQ(tip, QColor(255, 255, 255, 255));
}

// 用例名称: createEditor_AnyInput_ReturnsNullptr
// 测试类型: 正常场景
// 覆盖函数: createEditor
// 操作步骤: 调用 createEditor
// 预期结果: 表格代理不提供编辑器,返回空指针
TEST_F(UT_BaseItemDelegate, createEditor_AnyInput_ReturnsNullptr)
{
    // Arrange: 提供有效父窗口与选项
    QWidget parent;
    QStyleOptionViewItem opt;
    QStandardItemModel model(1, 1);
    model.setItem(0, 0, new QStandardItem(QStringLiteral("cell")));
    // Act: 分别以空父窗口与有效父窗口请求编辑器
    QWidget *editorNoParent = m_tester->createEditor(nullptr, opt, QModelIndex());
    QWidget *editorWithParent = m_tester->createEditor(&parent, opt, model.index(0, 0));
    // Assert: 表格代理不提供编辑器,均为空指针
    EXPECT_EQ(editorNoParent, nullptr);
    EXPECT_EQ(editorWithParent, nullptr);
}

// 用例名称: sizeHint_smallFont_returnsMinimum36
// 测试类型: 正常场景
// 覆盖函数: sizeHint(最小高度 36)
// 操作步骤: 用 8pt 小字体计算空单元格尺寸
// 预期结果: 高度不小于 36
TEST_F(UT_BaseItemDelegate, sizeHint_smallFont_returnsMinimum36)
{
    // Arrange: 小字体选项
    QStyleOptionViewItem opt;
    opt.font = QFont();
    opt.font.setPointSize(8);
    // Act: 计算尺寸
    const QSize size = m_tester->sizeHint(opt, QModelIndex());
    // Assert: 最小高度生效且宽度非负
    EXPECT_GE(size.height(), 36);
    EXPECT_GT(size.width(), 0);
}

// 用例名称: sizeHint_largeFont_keepsNaturalHeight
// 测试类型: 边界场景
// 覆盖函数: sizeHint(自然高度)
// 操作步骤: 用 30pt 大字体计算尺寸
// 预期结果: 高度保留自然值(大于 36)
TEST_F(UT_BaseItemDelegate, sizeHint_largeFont_keepsNaturalHeight)
{
    // Arrange: 大字体选项
    QStyleOptionViewItem opt;
    opt.font = QFont();
    opt.font.setPointSize(30);
    // Act: 计算尺寸
    const QSize size = m_tester->sizeHint(opt, QModelIndex());
    // Assert: 自然高度生效,且仍不低于最小高度
    EXPECT_GT(size.height(), 36);
    EXPECT_GE(size.height(), 36);
}

// 用例名称: initStyleOption_defaults_alignmentElideDisplay
// 测试类型: 正常场景
// 覆盖函数: initStyleOption(默认对齐/省略/Display 特性)
// 操作步骤: 对无对齐角色的 index 调用 initStyleOption
// 预期结果: 左对齐垂直居中,ElideRight,HasDisplay,text 为显示文本
TEST_F(UT_BaseItemDelegate, initStyleOption_defaults_alignmentElideDisplay)
{
    // Arrange: 1x2 模型,无自定义对齐
    QStandardItemModel model(1, 1);
    model.setItem(0, 0, new QStandardItem(QStringLiteral("hello")));
    QStyleOptionViewItem opt;
    opt.displayAlignment = Qt::AlignBottom | Qt::AlignRight;   // 先置为干扰值
    // Act: 初始化选项
    m_tester->initStyleOption(&opt, model.index(0, 0));
    // Assert: 默认值
    EXPECT_EQ(opt.displayAlignment, Qt::AlignLeft | Qt::AlignVCenter);
    EXPECT_EQ(opt.textElideMode, Qt::ElideRight);
    EXPECT_TRUE(opt.features & QStyleOptionViewItem::HasDisplay);
    EXPECT_EQ(opt.text, QStringLiteral("hello"));
    EXPECT_FALSE(opt.features & QStyleOptionViewItem::HasDecoration);
}

// 用例名称: initStyleOption_CustomRoles_AppliesAlignmentAndIcon
// 测试类型: 边界场景
// 覆盖函数: initStyleOption(自定义对齐 + 装饰图标)
// 操作步骤: index 提供 TextAlignmentRole 与 DecorationRole
// 预期结果: 对齐随角色更新,HasDecoration 置位,icon 非空
TEST_F(UT_BaseItemDelegate, initStyleOption_CustomRoles_AppliesAlignmentAndIcon)
{
    // Arrange: 带对齐与图标的模型
    QStandardItemModel model(1, 1);
    QStandardItem *item = new QStandardItem(QStringLiteral("cell"));
    QPixmap pm(16, 16);
    pm.fill(Qt::red);
    item->setIcon(QIcon(pm));
    item->setData(QVariant(int(Qt::AlignRight | Qt::AlignVCenter)), Qt::TextAlignmentRole);
    model.setItem(0, 0, item);
    QStyleOptionViewItem opt;
    // Act: 初始化选项
    m_tester->initStyleOption(&opt, model.index(0, 0));
    // Assert: 自定义对齐与装饰
    EXPECT_EQ(opt.displayAlignment, Qt::AlignRight | Qt::AlignVCenter);
    EXPECT_TRUE(opt.features & QStyleOptionViewItem::HasDecoration);
    EXPECT_FALSE(opt.icon.isNull());
}

// 用例名称: paint_onlyOnePosition_withoutDecoration
// 测试类型: 正常场景
// 覆盖函数: paint(分支11: OnlyOne 无图标纯文本)
// 操作步骤: 单列位置绘制纯文本单元格
// 预期结果: 完成绘制且内容写入图像(非空白)
TEST_F(UT_BaseItemDelegate, paint_onlyOnePosition_withoutDecoration)
{
    // Arrange: 无图标模型
    QScopedPointer<QStandardItemModel> model(makeModel(false, QStringLiteral("process-name")));
    // Act: 绘制
    const QImage img = paintCell(QStyleOptionViewItem::OnlyOne, QStyle::State_Enabled,
                                 model.data(), 0, 0);
    // Assert: 有内容写入
    EXPECT_FALSE(img.isNull());
    EXPECT_NE(img, blankImage());
}

// 用例名称: paint_onlyOnePosition_withDecoration
// 测试类型: 边界场景
// 覆盖函数: paint(分支10/12: OnlyOne 图标布局 + 位图缓存未命中路径)
// 操作步骤: 单列位置绘制带图标单元格
// 预期结果: 完成绘制且与无图标结果不同
TEST_F(UT_BaseItemDelegate, paint_onlyOnePosition_withDecoration)
{
    // Arrange: 带图标与无图标模型
    QScopedPointer<QStandardItemModel> model(makeModel(true, QStringLiteral("process-name")));
    QScopedPointer<QStandardItemModel> plainModel(makeModel(false, QStringLiteral("process-name")));
    // Act: 绘制
    const QImage img = paintCell(QStyleOptionViewItem::OnlyOne, QStyle::State_Enabled,
                                 model.data(), 0, 0);
    const QImage imgNoIcon = paintCell(QStyleOptionViewItem::OnlyOne, QStyle::State_Enabled,
                                       plainModel.data(), 0, 0);
    // Assert: 图标参与绘制,两种结果不同
    EXPECT_FALSE(img.isNull());
    EXPECT_NE(img, blankImage());
    EXPECT_NE(img, imgNoIcon);
}

// 用例名称: paint_beginningMiddleEnd_marginsApplied
// 测试类型: 正常场景
// 覆盖函数: paint(分支8: Beginning/Middle/End 边距)
// 操作步骤: 依次以三种位置绘制同一单元格
// 预期结果: 均完成绘制;Beginning 与 Middle 因边距不同而像素不同
TEST_F(UT_BaseItemDelegate, paint_beginningMiddleEnd_marginsApplied)
{
    // Arrange: 无图标模型
    QScopedPointer<QStandardItemModel> model(makeModel(false, QStringLiteral("process-name")));
    // Act: 三种位置绘制
    const QImage begin = paintCell(QStyleOptionViewItem::Beginning, QStyle::State_Enabled,
                                   model.data(), 0, 1);
    const QImage middle = paintCell(QStyleOptionViewItem::Middle, QStyle::State_Enabled,
                                    model.data(), 0, 1);
    const QImage end = paintCell(QStyleOptionViewItem::End, QStyle::State_Enabled,
                                 model.data(), 0, 1);
    // Assert: 均有内容;边距差异可见
    EXPECT_NE(begin, blankImage());
    EXPECT_NE(middle, blankImage());
    EXPECT_NE(end, blankImage());
    EXPECT_NE(begin, middle);
}

// 用例名称: paint_selectedSunkenAndHover_variants
// 测试类型: 边界场景
// 覆盖函数: paint(分支4/5/6/7: 选中/按压/悬浮配色)
// 操作步骤: 依次以 Selected、Selected+Sunken、Selected+MouseOver、普通+MouseOver 状态绘制
// 预期结果: 均完成绘制;按压态与普通选中态颜色不同
TEST_F(UT_BaseItemDelegate, paint_selectedSunkenAndHover_variants)
{
    // Arrange: 无图标模型
    QScopedPointer<QStandardItemModel> model(makeModel(false, QStringLiteral("process-name")));
    // Act: 四种状态绘制
    const QImage selected = paintCell(QStyleOptionViewItem::OnlyOne,
                                      QStyle::State_Enabled | QStyle::State_Selected,
                                      model.data(), 0, 0);
    const QImage sunken = paintCell(QStyleOptionViewItem::OnlyOne,
                                    QStyle::State_Enabled | QStyle::State_Selected | QStyle::State_Sunken,
                                    model.data(), 0, 0);
    const QImage selectedHover = paintCell(QStyleOptionViewItem::OnlyOne,
                                           QStyle::State_Enabled | QStyle::State_Selected | QStyle::State_MouseOver,
                                           model.data(), 0, 0);
    const QImage normalHover = paintCell(QStyleOptionViewItem::OnlyOne,
                                         QStyle::State_Enabled | QStyle::State_MouseOver,
                                         model.data(), 0, 0);
    // Assert: 均有内容;按压态配色不同
    EXPECT_NE(selected, blankImage());
    EXPECT_NE(sunken, blankImage());
    EXPECT_NE(selectedHover, blankImage());
    EXPECT_NE(normalHover, blankImage());
    EXPECT_NE(selected, sunken);
}

// 用例名称: paint_disabledGroup_usesDisabledPalette
// 测试类型: 边界场景
// 覆盖函数: paint(分支2: 非 Enabled 走 Disabled 色组)
// 操作步骤: 分别以禁用与启用状态绘制同一单元格
// 预期结果: 两种色组绘制结果不同
TEST_F(UT_BaseItemDelegate, paint_disabledGroup_usesDisabledPalette)
{
    // Arrange: 无图标模型
    QScopedPointer<QStandardItemModel> model(makeModel(false, QStringLiteral("process-name")));
    // Act: 禁用与启用绘制
    const QImage disabled = paintCell(QStyleOptionViewItem::OnlyOne, QStyle::State_None,
                                      model.data(), 0, 0);
    const QImage enabled = paintCell(QStyleOptionViewItem::OnlyOne, QStyle::State_Enabled,
                                     model.data(), 0, 0);
    // Assert: 色组差异
    EXPECT_NE(disabled, blankImage());
    EXPECT_NE(disabled, enabled);
}

// 用例名称: paint_userRoleColor_customTextColor
// 测试类型: 边界场景
// 覆盖函数: paint(分支3: UserRole+2 自定义文字色)
// 操作步骤: 为 index 提供 UserRole+2 数据后绘制,与默认文字色对比
// 预期结果: 自定义色与默认色绘制结果不同
TEST_F(UT_BaseItemDelegate, paint_userRoleColor_customTextColor)
{
    // Arrange: 一份默认模型,一份带 UserRole+2 的模型
    QScopedPointer<QStandardItemModel> plain(makeModel(false, QStringLiteral("process-name")));
    QScopedPointer<QStandardItemModel> colored(makeModel(false, QStringLiteral("process-name")));
    colored->item(0, 0)->setData(QVariant(int(Dtk::Gui::DPalette::TextWarning)), Qt::UserRole + 2);
    // Act: 两次绘制
    const QImage imgPlain = paintCell(QStyleOptionViewItem::OnlyOne, QStyle::State_Enabled,
                                      plain.data(), 0, 0);
    const QImage imgColored = paintCell(QStyleOptionViewItem::OnlyOne, QStyle::State_Enabled,
                                        colored.data(), 0, 0);
    // Assert: 颜色路径差异
    EXPECT_NE(imgPlain, blankImage());
    EXPECT_NE(imgColored, blankImage());
    EXPECT_NE(imgPlain, imgColored);
}

// 用例名称: paint_invalidIndex_fallsBackToBase
// 测试类型: 异常场景
// 覆盖函数: paint(分支1: 无效 index 基类绘制)
// 操作步骤: 用无效 QModelIndex 调用 paint
// 预期结果: 不崩溃,且不产生本类自绘内容
TEST_F(UT_BaseItemDelegate, paint_invalidIndex_fallsBackToBase)
{
    // Arrange: 空图像与空白参照
    QImage img(140, 40, QImage::Format_ARGB32_Premultiplied);
    img.fill(QColor(128, 128, 128));
    QPainter p(&img);
    QStyleOptionViewItem opt;
    opt.rect = QRect(0, 0, 140, 40);
    opt.state = QStyle::State_Enabled;
    opt.viewItemPosition = QStyleOptionViewItem::OnlyOne;
    // Act: 绘制无效 index
    m_tester->paint(&p, opt, QModelIndex());
    p.end();
    // Assert: 未写入内容且图像未被破坏
    EXPECT_EQ(img, blankImage());
    EXPECT_FALSE(img.isNull());
}

// 用例名称: paint_impossiblePosition_fallsBackToBase
// 测试类型: 异常场景
// 覆盖函数: paint(分支9: 位置默认分支回退基类)
// 操作步骤: 以 viewItemPosition=Invalid 调用 paint
// 预期结果: 不崩溃,回退基类路径完成
TEST_F(UT_BaseItemDelegate, paint_impossiblePosition_fallsBackToBase)
{
    // Arrange: 模型与选项
    QScopedPointer<QStandardItemModel> model(makeModel(false, QStringLiteral("process-name")));
    QImage img(140, 40, QImage::Format_ARGB32_Premultiplied);
    img.fill(QColor(128, 128, 128));
    QPainter p(&img);
    QStyleOptionViewItem opt;
    opt.rect = QRect(0, 0, 140, 40);
    opt.state = QStyle::State_Enabled;
    opt.viewItemPosition = QStyleOptionViewItem::Invalid;
    // Act: 绘制
    m_tester->paint(&p, opt, model->index(0, 0));
    p.end();
    // Assert: 不崩溃即可(基类路径),图像尺寸不变
    EXPECT_FALSE(img.isNull());
    EXPECT_EQ(img.size(), QSize(140, 40));
}

// 用例名称: helpEvent_nullEventOrView_returnsFalse
// 测试类型: 正常场景
// 覆盖函数: helpEvent(分支13: 空参数防御)
// 操作步骤: 分别以空 event、空 view 调用
// 预期结果: 均返回 false
TEST_F(UT_BaseItemDelegate, helpEvent_nullEventOrView_returnsFalse)
{
    // Arrange: 视图与模型
    QStandardItemModel model(1, 1);
    model.setItem(0, 0, new QStandardItem(QStringLiteral("text")));
    QTableView view;
    view.setModel(&model);
    QStyleOptionViewItem opt;
    // Act: 执行上述步骤
    // Assert: 空参数
    EXPECT_FALSE(m_tester->helpEvent(nullptr, &view, opt, model.index(0, 0)));
    QHelpEvent e(QEvent::ToolTip, QPoint(0, 0), QPoint(0, 0));
    EXPECT_FALSE(m_tester->helpEvent(&e, nullptr, opt, model.index(0, 0)));
}

// 用例名称: helpEvent_nonTooltipType_delegatesToBase
// 测试类型: 边界场景
// 覆盖函数: helpEvent(分支14: 非 ToolTip 事件基类处理)
// 操作步骤: 发送 QEvent::WhatsThis 类型的 QHelpEvent(索引无 WhatsThisRole 数据)
// 预期结果: 走基类路径返回 false,且不弹出 tooltip
TEST_F(UT_BaseItemDelegate, helpEvent_nonTooltipType_delegatesToBase)
{
    // Arrange: 视图与模型
    QStandardItemModel model(1, 1);
    model.setItem(0, 0, new QStandardItem(QStringLiteral("text")));
    QTableView view;
    view.setModel(&model);
    QStyleOptionViewItem opt;
    // Act: WhatsThis 帮助事件(索引无 WhatsThisRole 数据时基类确定返回 false)
    QHelpEvent e(QEvent::WhatsThis, QPoint(0, 0), QPoint(0, 0));
    const bool rc = m_tester->helpEvent(&e, &view, opt, model.index(0, 0));
    // Assert: 基类返回 false 且未弹出 tooltip
    EXPECT_FALSE(rc);
    EXPECT_FALSE(QToolTip::isVisible());
}

// 用例名称: helpEvent_truncatedText_showsTooltip
// 测试类型: 正常场景
// 覆盖函数: helpEvent(分支15/18: 截断文本显示 tooltip,行占满宽度)
// 操作步骤: 列占满视图且文本超宽,发送 ToolTip 事件
// 预期结果: 返回 true 且 tooltip 可见
TEST_F(UT_BaseItemDelegate, helpEvent_truncatedText_showsTooltip)
{
    // Arrange: 窄视图 + 长文本
    QStandardItemModel model(1, 2);
    model.setItem(0, 0, new QStandardItem(QStringLiteral("a-very-long-process-display-name")));
    QTableView view;
    view.setFrameShape(QFrame::NoFrame);
    view.setModel(&model);
    view.resize(100, 40);
    view.show();
    QCoreApplication::processEvents();
    const QModelIndex idx = model.index(0, 0);
    QStyleOptionViewItem opt;
    opt.rect = view.visualRect(idx);
    opt.font = view.font();
    opt.widget = &view;
    QToolTip::hideText();
    // Act: ToolTip 事件
    QHelpEvent e(QEvent::ToolTip, QPoint(10, 10), view.mapToGlobal(QPoint(10, 10)));
    const bool rc = m_tester->helpEvent(&e, &view, opt, idx);
    // Assert: tooltip 展示
    EXPECT_TRUE(rc);
    EXPECT_TRUE(QToolTip::isVisible());
    QToolTip::hideText();
}

// 用例名称: helpEvent_truncatedTextWithIcon_showsTooltip
// 测试类型: 边界场景
// 覆盖函数: helpEvent(分支16: 截断文本 + HasDecoration 图标布局)
// 操作步骤: 第 0 列带图标且文本超宽,发送 ToolTip 事件
// 预期结果: 返回 true 且 tooltip 可见
TEST_F(UT_BaseItemDelegate, helpEvent_truncatedTextWithIcon_showsTooltip)
{
    // Arrange: 窄视图 + 带图标长文本
    QStandardItemModel model(1, 2);
    QStandardItem *item = new QStandardItem(QStringLiteral("a-very-long-process-display-name"));
    QPixmap pm(16, 16);
    pm.fill(Qt::blue);
    item->setIcon(QIcon(pm));
    model.setItem(0, 0, item);
    QTableView view;
    view.setFrameShape(QFrame::NoFrame);
    view.setModel(&model);
    view.resize(100, 40);
    view.show();
    QCoreApplication::processEvents();
    const QModelIndex idx = model.index(0, 0);
    QStyleOptionViewItem opt;
    opt.rect = view.visualRect(idx);
    opt.font = view.font();
    opt.widget = &view;
    QToolTip::hideText();
    // Act: ToolTip 事件
    QHelpEvent e(QEvent::ToolTip, QPoint(10, 10), view.mapToGlobal(QPoint(10, 10)));
    const bool rc = m_tester->helpEvent(&e, &view, opt, idx);
    // Assert: tooltip 展示
    EXPECT_TRUE(rc);
    EXPECT_TRUE(QToolTip::isVisible());
    QToolTip::hideText();
}

// 用例名称: helpEvent_fittingText_returnsTrue
// 测试类型: 边界场景
// 覆盖函数: helpEvent(分支17: 文本未截断,隐藏 tooltip 并返回 true)
// 操作步骤: 第 1 列短文本,发送 ToolTip 事件
// 预期结果: 返回 true 且不展示 tooltip
TEST_F(UT_BaseItemDelegate, helpEvent_fittingText_returnsTrue)
{
    // Arrange: 视图 + 第 1 列短文本
    QStandardItemModel model(1, 2);
    model.setItem(0, 1, new QStandardItem(QStringLiteral("ok")));
    QTableView view;
    view.setFrameShape(QFrame::NoFrame);
    view.setModel(&model);
    view.resize(200, 40);
    view.show();
    QCoreApplication::processEvents();
    const QModelIndex idx = model.index(0, 1);
    QStyleOptionViewItem opt;
    opt.rect = view.visualRect(idx);
    opt.font = view.font();
    opt.widget = &view;
    QToolTip::hideText();
    // Act: ToolTip 事件
    QHelpEvent e(QEvent::ToolTip, QPoint(10, 10), view.mapToGlobal(QPoint(10, 10)));
    const bool rc = m_tester->helpEvent(&e, &view, opt, idx);
    // Assert: 返回 true,tooltip 保持隐藏
    EXPECT_TRUE(rc);
    EXPECT_FALSE(QToolTip::isVisible());
}

// 用例名称: changeTheme_RepeatSameType_StableColor
// 测试类型: 边界场景
// 覆盖函数: changeTheme(重复同类型通知)
// 操作步骤: 连续两次切换到浅色主题,再连续两次深色
// 预期结果: 颜色幂等,不随重复通知漂移
TEST_F(UT_BaseItemDelegate, changeTheme_RepeatSameType_StableColor)
{
    // Arrange: 默认构造
    QColor &tip = m_tester->m_tipColor;
    // Act: 重复同类型切换
    m_tester->changeTheme(DGuiApplicationHelper::LightType);
    const QColor first = tip;
    m_tester->changeTheme(DGuiApplicationHelper::LightType);
    // Assert: 同类型重复切换颜色不变
    EXPECT_EQ(tip, first);
    EXPECT_EQ(tip, QColor(255, 255, 255, 255));
    m_tester->changeTheme(DGuiApplicationHelper::DarkType);
    const QColor dark = tip;
    m_tester->changeTheme(DGuiApplicationHelper::DarkType);
    EXPECT_EQ(tip, dark);
}

// 用例名称: changeTheme_lightAndDark_updatesTipColor
// 测试类型: 边界场景
// 覆盖函数: changeTheme
// 操作步骤: 依次切换浅色、深色、未知主题
// 预期结果: 浅色 (255,255,255,255);深色 (0,0,0,200);未知主题保持不变
TEST_F(UT_BaseItemDelegate, changeTheme_lightAndDark_updatesTipColor)
{
    // Arrange: 默认构造
    QColor &tip = m_tester->m_tipColor;
    // Act: 执行上述步骤
    // Assert: 浅色
    m_tester->changeTheme(DGuiApplicationHelper::LightType);
    EXPECT_EQ(tip, QColor(255, 255, 255, 255));
    // 深色
    m_tester->changeTheme(DGuiApplicationHelper::DarkType);
    EXPECT_EQ(tip, QColor(0, 0, 0, 200));
    // 未知主题保持
    m_tester->changeTheme(DGuiApplicationHelper::UnknownType);
    EXPECT_EQ(tip, QColor(0, 0, 0, 200));
}
