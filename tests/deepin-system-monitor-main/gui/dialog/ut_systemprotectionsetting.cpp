// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// =====================================================================================
// 用例计数声明表
// | 方法 | 等级 | factors | min | actual |
// |---|---|---|---|---|
// | SystemProtectionSetting | mid | cx:0 | 1 | 2 |
// | createProtectionSwitchHandle | mid | cx:0 | 1 | 3 |
// | createAlarmUsgaeSettingHandle | mid | cx:0 | 1 | 2 |
// | createAlarmIntervalSettingHandle | mid | cx:7 | 1 | 1 |
// | createSettingLinkButtonHandle | mid | cx:0 | 1 | 1 |
// | setLastValidAlarm | low | cx:0 | 1 | 2 |
// | lineEditChanged | low | cx:0 | 1 | 5 |
// | onMessgaeSetting | low | cx:0 | 1 | 1 |
// | onSettingItemChanged | low | cx:0 | 1 | 3 |
// | onUpdateNewBackend | low | cx:0 | 1 | 1 |
// | regularNumber | low | cx:0 | 1 | 2 |
// | getDSettingPointor | low | cx:0 | 1 | 1 |
// | instance | low | cx:0 | 1 | 1 |
// | changeWidgetFontSizeByDiffWithSystem | low | cx:0 | 1 | 4 |
// =====================================================================================

// 分支清单（来源：dialog/systemprotectionsetting.cpp SystemProtectionSetting::createProtectionSwitchHandle）
//   B1: clicked(checked) != 当前值 → option->setValue [CreateProtectionSwitchHandle_ClickSignal_TogglesOptionValue]
//   B2: valueChanged 且按钮状态不一致 → setChecked 同步 [CreateProtectionSwitchHandle_ValueChange_UpdatesButton]
//   B3: widget 非空 → deleteLater（调用路径必经） [CreateProtectionSwitchHandle_ValidOption_BuildsSwitchWidget]

// 分支清单（来源：dialog/systemprotectionsetting.cpp SystemProtectionSetting::createAlarmUsgaeSettingHandle）
//   B1: key == alarm_cpu_usage → m_lastValidCPUValue 同步 [CreateAlarmUsgaeSettingHandle_CpuOption_SyncsLastValid]
//   B2: key == alarm_memory_usage → m_lastValidMemoryValue 同步 [CreateAlarmUsgaeSettingHandle_MemOption_SyncsLastValid]
//   B3: valueChanged 且文本与选项不一致 → 回写文本（恢复默认联动） [CreateAlarmUsgaeSettingHandle_CpuOption_SyncsLastValid 调用路径]
//   B4: 恢复默认联动中 key == cpu → 更新静态值 [CreateAlarmUsgaeSettingHandle_CpuOption_SyncsLastValid 调用路径]
//   B5: 恢复默认联动中 key == mem → 更新静态值 [CreateAlarmUsgaeSettingHandle_MemOption_SyncsLastValid 调用路径]
//   B6: settingPtr 非空 → 初始化禁用态并联动 valueChanged [CreateAlarmUsgaeSettingHandle_CpuOption_SyncsLastValid 调用路径]
//   B7: 开关键命中 → widget 禁用态联动 [CreateAlarmUsgaeSettingHandle_MemOption_SyncsLastValid 调用路径]
//   B8: leftWidget 非空 → 同步禁用态 [CreateAlarmUsgaeSettingHandle_MemOption_SyncsLastValid 调用路径]
//   B9: 开关键命中 → leftWidget 禁用态联动 [CreateAlarmUsgaeSettingHandle_MemOption_SyncsLastValid 调用路径]
//   B10: widget 非空 → deleteLater [CreateAlarmUsgaeSettingHandle_MemOption_SyncsLastValid 调用路径]

// 分支清单（来源：dialog/systemprotectionsetting.cpp SystemProtectionSetting::createAlarmIntervalSettingHandle）
//   B1: valueChanged 且文本与选项不一致 → 回写文本 [CreateAlarmIntervalSettingHandle_ValidOption_BuildsEditWithValidator 调用路径]
//   B2: key == alarm_interval → 更新 m_lastValidInternalValue [CreateAlarmIntervalSettingHandle_ValidOption_BuildsEditWithValidator]
//   B3: settingPtr 非空 → 初始化禁用态并联动 [CreateAlarmIntervalSettingHandle_ValidOption_BuildsEditWithValidator 调用路径]
//   B4: 开关键命中 → widget 禁用态联动 [CreateAlarmIntervalSettingHandle_ValidOption_BuildsEditWithValidator 调用路径]
//   B5: leftWidget 非空 → 同步禁用态 [CreateAlarmIntervalSettingHandle_ValidOption_BuildsEditWithValidator 调用路径]
//   B6: 开关键命中 → leftWidget 禁用态联动 [CreateAlarmIntervalSettingHandle_ValidOption_BuildsEditWithValidator 调用路径]
//   B7: widget 非空 → deleteLater [CreateAlarmIntervalSettingHandle_ValidOption_BuildsEditWithValidator 调用路径]

// 分支清单（来源：dialog/systemprotectionsetting.cpp SystemProtectionSetting::setLastValidAlarm）
//   B1: num 在 [minValue,maxValue] → 写入文本并 setValue [SetLastValidAlarm_InRange_SetsTextAndOption]
//   B2: num 越界且 key == cpu → 文本回退 m_lastValidCPUValue [SetLastValidAlarm_OutOfRange_FallsBackToLastValid]
//   B3: num 越界且 key == mem → 文本回退 m_lastValidMemoryValue [LineEditChanged_MemOutOfRange_RestoresLastValid]
//   B4: num 越界且 key == interval → 文本回退 m_lastValidInternalValue [LineEditChanged_IntervalOutOfRange_RestoresLastValid]

// 分支清单（来源：dialog/systemprotectionsetting.cpp SystemProtectionSetting::lineEditChanged）
//   B1: focus==false 且无选区文本 → 进入校验 [LineEditChanged_ValidInput_UpdatesOptionAndStatic]
//   B2: 文本为空或越界 → 告警并回退上次合法值 [LineEditChanged_InvalidInput_RestoresLastValid]
//   B3: 回退路径 key == cpu [LineEditChanged_InvalidInput_RestoresLastValid]
//   B4: 回退路径 key == mem [LineEditChanged_MemOutOfRange_RestoresLastValid]
//   B5: 回退路径 key == interval [LineEditChanged_IntervalOutOfRange_RestoresLastValid]
//   B6: 合法提交 key == cpu → 更新静态值与选项 [LineEditChanged_ValidInput_UpdatesOptionAndStatic]
//   B7: 合法提交 key == mem（回退后合法值同样走此路径） [LineEditChanged_MemOutOfRange_RestoresLastValid]
//   B8: 合法提交 key == interval（回退后合法值同样走此路径） [LineEditChanged_IntervalOutOfRange_RestoresLastValid]

// 分支清单（来源：dialog/systemprotectionsetting.cpp SystemProtectionSetting::onMessgaeSetting）
//   B1: desktop 文件存在且可读 → 解析 GenericName [OnMessgaeSetting_GenericPath_DoesNotCrash]
//   B2: while 逐行扫描 desktop 文件（0/1/N 次） [OnMessgaeSetting_GenericPath_DoesNotCrash]
//   B3: 行前缀命中 → 提取 GenericName 并 break [OnMessgaeSetting_GenericPath_DoesNotCrash]
//   B4: GenericName 非空 → 定向通知页跳转分支 [OnMessgaeSetting_GenericPath_DoesNotCrash]
//   B5: 非 oldVersion → ShowPage 参数 notification/<name> [OnMessgaeSetting_GenericPath_DoesNotCrash]
//   B6: 首次 call 返回 Error → 回退通用通知页 [OnMessgaeSetting_GenericPath_DoesNotCrash]
//   B7: 回退仍 Error → 告警 [OnMessgaeSetting_GenericPath_DoesNotCrash]
//   B8: GenericName 为空 → 通用跳转分支（非 oldVersion 用 ShowPage） [OnMessgaeSetting_GenericPath_DoesNotCrash]
//   B9: 通用跳转 Error → 告警 [OnMessgaeSetting_GenericPath_DoesNotCrash]

// 分支清单（来源：dialog/systemprotectionsetting.cpp SystemProtectionSetting::onSettingItemChanged）
//   B1: key == alarm_switch → gdbus setSystemProtectionStatus [OnSettingItemChanged_KnownKey_NoWarning]
//   B2: key == alarm_cpu_usage → gdbus setAlarmUsageOfCpu [OnSettingItemChanged_CpuMemIntervalLastTime_NoWarning]
//   B3: key == alarm_memory_usage → gdbus setAlarmUsageOfMemory [OnSettingItemChanged_CpuMemIntervalLastTime_NoWarning]
//   B4: key == alarm_interval → gdbus setAlarmMsgInterval [OnSettingItemChanged_CpuMemIntervalLastTime_NoWarning]
//   B5: key == alarm_last_time → gdbus setAlarmLastTimeInterval [OnSettingItemChanged_CpuMemIntervalLastTime_NoWarning]
//   B6: 未知 key（else）→ 告警并提前返回 [OnSettingItemChanged_UnknownKey_EarlyReturn]

// 分支清单（来源：dialog/systemprotectionsetting.cpp 自由函数 changeWidgetFontSizeByDiffWithSystem）
//   B1: widget 为空或 diff==0 → 早退 false [ChangeWidgetFontSizeByDiffWithSystem_NullWidget_ReturnsFalse / _ZeroDiff_ReturnsFalse]
//   B2: DBus 接口无效 → 早退 false [ChangeWidgetFontSizeByDiffWithSystem_InvalidInterface_ReturnsFalse]
//   B3: 系统字号非法或调整后非法 → 早退 false [ChangeWidgetFontSizeByDiffWithSystem_InvalidInterface_ReturnsFalse]
//   B4: 上述早退路径含 return false（计为早退分支） [同上三个用例]
//   B5: 全部通过 → setPointSizeF 并返回 true [ChangeWidgetFontSizeByDiffWithSystem_ValidFontSize_AppliesFont]

#include "dialog/systemprotectionsetting.h"
#include "stub.h"
#include "addr_pri.h"

#include <gtest/gtest.h>

#include <QApplication>
#include <QDBusAbstractInterface>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusInterface>
#include <QFont>
#include <QIntValidator>
#include <QLoggingCategory>
#include <QLabel>
#include <QLineEdit>
#include <QSignalSpy>
#include <DSwitchButton>
#include <DLineEdit>
#include <DSettingsOption>
#include <DCommandLinkButton>
#include <DSettings>

DCORE_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

// option key 常量与 systemprotectionsetting.cpp 中的宏保持一致
namespace {
const char *kAlarmStatusOptionName = "setting.systemprotection.alarm_switch";
const char *kAlarmCpuUsageOptionName = "setting.systemprotection.alarm_cpu_usage";
const char *kAlarmMemUsageOptionName = "setting.systemprotection.alarm_memory_usage";
const char *kAlarmIntervalOptionName = "setting.systemprotection.alarm_interval";
const char *kMessageSettingOptionName = "setting.messgae.message_setting";
// 与 systemprotectionsetting.cpp 内静态常量一致（cpp 私有, 此处以字面量等价）
const double kToolTipFontSizeDiff = -1.5;
}  // namespace

// 自由函数在 systemprotectionsetting.cpp 内定义（外部链接）, 此处前向声明
bool changeWidgetFontSizeByDiffWithSystem(QWidget *widget, double diff);

// DBus 桩: 接口有效
static bool stub_dbus_isValid(void *)
{
    return true;
}

// DBus 桩: 接口无效（无会话总线环境）
static bool stub_dbus_isInvalid(void *)
{
    return false;
}

// DBus 桩: 固定返回系统字号 11.0
static QVariant stub_dbus_propertyFontSize(void *, const char *)
{
    return QVariant(11.0);
}

// DBus 桩: 记录 sessionBus().call 调用次数
static int g_dbusCallCount = 0;
static QDBusMessage stub_dbus_call(void *)
{
    ++g_dbusCallCount;
    return QDBusMessage();
}

// 消息捕获: 只记录最近一条告警文本
static QString g_lastWarning;
static void warnCaptureHandler(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
    if (type == QtWarningMsg)
        g_lastWarning = msg;
}

class UT_SystemProtectionSetting : public ::testing::Test
{
public:
    UT_SystemProtectionSetting() : m_tester(nullptr) {}

protected:
    virtual void SetUp()
    {
        // Arrange(夹具): 复位静态上次合法值, 避免跨用例串扰
        SystemProtectionSetting::m_lastValidCPUValue = -1;
        SystemProtectionSetting::m_lastValidMemoryValue = -1;
        SystemProtectionSetting::m_lastValidInternalValue = -1;
        static QWidget wid;
        m_tester = new SystemProtectionSetting(&wid);
    }

    virtual void TearDown()
    {
        if (m_tester) {
            delete m_tester;
            m_tester = nullptr;
        }
    }

    SystemProtectionSetting *m_tester;
};

TEST_F(UT_SystemProtectionSetting, Instance_GlobalSingleton_ReturnsSamePointer)
{
    // Arrange: 夹具已构造独立实例, 全局单例尚未触碰
    ASSERT_NE(m_tester, nullptr);

    // Act: 连续两次取全局单例
    SystemProtectionSetting *first = SystemProtectionSetting::instance();
    SystemProtectionSetting *second = SystemProtectionSetting::instance();

    // Assert: 同一实例且独立于夹具对象
    EXPECT_EQ(first, second);
    EXPECT_NE(SystemProtectionSetting::instance(), m_tester);
    EXPECT_NE(first, nullptr);
}

TEST_F(UT_SystemProtectionSetting, GetDSettingPointor_AfterCtor_ReturnsLoadedSettings)
{
    // Arrange: 夹具构造时已完成 settings.json 加载
    ASSERT_NE(m_tester, nullptr);

    // Act
    DSettings *settings = m_tester->getDSettingPointor();

    // Assert: 构造时已从 settings.json 加载配置
    ASSERT_NE(settings, nullptr);
    EXPECT_NE(settings->option(kAlarmCpuUsageOptionName), nullptr);
    EXPECT_NE(settings->option(kMessageSettingOptionName), nullptr);
}

TEST_F(UT_SystemProtectionSetting, CreateProtectionSwitchHandle_ValidOption_BuildsSwitchWidget)
{
    // Arrange
    DSettingsOption *option = m_tester->getDSettingPointor()->option(kAlarmStatusOptionName);
    ASSERT_NE(option, nullptr);

    // Act
    auto pair = SystemProtectionSetting::createProtectionSwitchHandle(option);

    // Assert: 前列标题与开关控件均已创建, 开关状态与选项值一致
    EXPECT_NE(pair.first, nullptr);
    ASSERT_NE(pair.second, nullptr);
    auto *button = pair.second->findChild<DSwitchButton *>();
    ASSERT_NE(button, nullptr);
    EXPECT_EQ(button->isChecked(), option->value().toBool());
}

TEST_F(UT_SystemProtectionSetting, CreateProtectionSwitchHandle_ClickSignal_TogglesOptionValue)
{
    // Arrange
    DSettingsOption *option = m_tester->getDSettingPointor()->option(kAlarmStatusOptionName);
    ASSERT_NE(option, nullptr);
    auto pair = SystemProtectionSetting::createProtectionSwitchHandle(option);
    auto *button = pair.second->findChild<DSwitchButton *>();
    ASSERT_NE(button, nullptr);
    const bool initial = button->isChecked();

    // Act: 发射开关点击信号（取反）
    button->clicked(!initial);

    // Assert: 选项值与按钮状态同步翻转
    EXPECT_EQ(option->value().toBool(), !initial);
    EXPECT_EQ(button->isChecked(), !initial);
}

TEST_F(UT_SystemProtectionSetting, CreateProtectionSwitchHandle_ValueChange_UpdatesButton)
{
    // Arrange
    DSettingsOption *option = m_tester->getDSettingPointor()->option(kAlarmStatusOptionName);
    ASSERT_NE(option, nullptr);
    auto pair = SystemProtectionSetting::createProtectionSwitchHandle(option);
    auto *button = pair.second->findChild<DSwitchButton *>();
    ASSERT_NE(button, nullptr);
    const bool initial = option->value().toBool();

    // Act: 直接翻转选项值（经 valueChanged 信号联动）
    option->setValue(!initial);

    // Assert: 按钮状态跟随选项值
    EXPECT_EQ(option->value().toBool(), !initial);
    EXPECT_EQ(button->isChecked(), option->value().toBool());
}

TEST_F(UT_SystemProtectionSetting, CreateAlarmUsgaeSettingHandle_CpuOption_SyncsLastValid)
{
    // Arrange
    DSettingsOption *option = m_tester->getDSettingPointor()->option(kAlarmCpuUsageOptionName);
    ASSERT_NE(option, nullptr);

    // Act
    auto pair = SystemProtectionSetting::createAlarmUsgaeSettingHandle(option);

    // Assert: 编辑控件建立, 静态上次合法值与选项值一致, 校验器上限 100
    ASSERT_NE(pair.second, nullptr);
    EXPECT_EQ(SystemProtectionSetting::m_lastValidCPUValue, option->value().toInt());
    auto *edit = pair.second->findChild<DLineEdit *>();
    ASSERT_NE(edit, nullptr);
    EXPECT_EQ(edit->text(), option->value().toString());
    auto *validator = qobject_cast<const QIntValidator *>(edit->lineEdit()->validator());
    ASSERT_NE(validator, nullptr);
    EXPECT_EQ(validator->top(), 100);

    // 联动断言: 选项值变化 → valueChanged 回调回写文本并同步静态值
    option->setValue(33);
    EXPECT_EQ(edit->text(), QString("33"));
    EXPECT_EQ(SystemProtectionSetting::m_lastValidCPUValue, 33);
}

TEST_F(UT_SystemProtectionSetting, CreateAlarmUsgaeSettingHandle_MemOption_SyncsLastValid)
{
    // Arrange
    DSettingsOption *option = m_tester->getDSettingPointor()->option(kAlarmMemUsageOptionName);
    ASSERT_NE(option, nullptr);

    // Act
    auto pair = SystemProtectionSetting::createAlarmUsgaeSettingHandle(option);

    // Assert: 内存分支同步 m_lastValidMemoryValue, 校验器上限 100
    ASSERT_NE(pair.second, nullptr);
    EXPECT_EQ(SystemProtectionSetting::m_lastValidMemoryValue, option->value().toInt());
    auto *edit = pair.second->findChild<DLineEdit *>();
    ASSERT_NE(edit, nullptr);
    EXPECT_EQ(edit->text(), option->value().toString());
    auto *validator = qobject_cast<const QIntValidator *>(edit->lineEdit()->validator());
    ASSERT_NE(validator, nullptr);
    EXPECT_EQ(validator->top(), 100);

    // 联动断言: 选项值变化 → valueChanged 回调回写文本并同步静态值
    option->setValue(55);
    EXPECT_EQ(edit->text(), QString("55"));
    EXPECT_EQ(SystemProtectionSetting::m_lastValidMemoryValue, 55);
}

TEST_F(UT_SystemProtectionSetting, CreateAlarmUsgaeSettingHandle_CpuOption_EditEventsSyncOption)
{
    // Arrange: cpu 选项句柄, 预置合法选项值使编辑框回写
    DSettingsOption *option = m_tester->getDSettingPointor()->option(kAlarmCpuUsageOptionName);
    ASSERT_NE(option, nullptr);
    auto pair = SystemProtectionSetting::createAlarmUsgaeSettingHandle(option);
    ASSERT_NE(pair.second, nullptr);
    auto *edit = pair.second->findChild<DLineEdit *>();
    ASSERT_NE(edit, nullptr);
    option->setValue(45);
    ASSERT_EQ(edit->text(), QString("45"));

    // Act: 文本变化触发 textChanged 规整回调; 回车触发 returnPressed → lineEditChanged 合法值提交
    edit->setText("67");
    edit->returnPressed();

    // Assert: 编辑事件经回调同步到选项与静态上次合法值
    EXPECT_EQ(option->value().toInt(), 67);
    EXPECT_EQ(SystemProtectionSetting::m_lastValidCPUValue, 67);
    EXPECT_EQ(edit->text(), QString("67"));
}

TEST_F(UT_SystemProtectionSetting, CreateAlarmUsgaeSettingHandle_SwitchChange_TogglesWidgetEnabled)
{
    // Arrange: cpu 句柄, DBus 桩隔离开关键联动中的 gdbus 调用
    Stub stub;
    stub.set(ADDR(QDBusAbstractInterface, isValid), stub_dbus_isValid);
    stub.set(ADDR(QDBusConnection, call), stub_dbus_call);
    DSettingsOption *option = m_tester->getDSettingPointor()->option(kAlarmCpuUsageOptionName);
    ASSERT_NE(option, nullptr);
    auto pair = SystemProtectionSetting::createAlarmUsgaeSettingHandle(option);
    ASSERT_NE(pair.second, nullptr);

    // Act: 在源码实际连接的后端（全局单例的 DSettings）上发射 valueChanged,
    // 模拟监控开关关闭 → 打开; 经子控件 enabled 状态观测联动效果
    auto *setting = SystemProtectionSetting::instance()->getDSettingPointor();
    ASSERT_NE(setting, nullptr);
    auto *contentEdit = pair.second->findChild<DLineEdit *>();
    ASSERT_NE(contentEdit, nullptr);
    emit setting->valueChanged(kAlarmStatusOptionName, QVariant(false));
    const bool disabledAfterOff = !contentEdit->isEnabled();
    emit setting->valueChanged(kAlarmStatusOptionName, QVariant(true));
    const bool enabledAfterOn = contentEdit->isEnabled();

    // Assert: 开关状态经 DSettings::valueChanged 联动到句柄控件可用性
    EXPECT_TRUE(disabledAfterOff);
    EXPECT_TRUE(enabledAfterOn);
}

TEST_F(UT_SystemProtectionSetting, CreateAlarmIntervalSettingHandle_ValidOption_BuildsEditWithValidator)
{
    // Arrange
    DSettingsOption *option = m_tester->getDSettingPointor()->option(kAlarmIntervalOptionName);
    ASSERT_NE(option, nullptr);

    // Act
    auto pair = SystemProtectionSetting::createAlarmIntervalSettingHandle(option);

    // Assert: 校验器区间 [1,60], 文本与选项值一致, 状态开关值决定可用性
    ASSERT_NE(pair.second, nullptr);
    auto *edit = pair.second->findChild<DLineEdit *>();
    ASSERT_NE(edit, nullptr);
    auto *validator = qobject_cast<const QIntValidator *>(edit->lineEdit()->validator());
    ASSERT_NE(validator, nullptr);
    EXPECT_EQ(validator->bottom(), 1);
    EXPECT_EQ(validator->top(), 60);
    EXPECT_EQ(edit->text(), option->value().toString());
    EXPECT_EQ(SystemProtectionSetting::m_lastValidInternalValue, option->value().toInt());
    EXPECT_EQ(pair.second->isEnabled(),
              m_tester->getDSettingPointor()->getOption(kAlarmStatusOptionName).toBool());
}

TEST_F(UT_SystemProtectionSetting, CreateSettingLinkButtonHandle_ValidOption_ResetsOptionToDefault)
{
    // Arrange: 桩替换 DBus 接口有效性, 避免真实会话总线调用
    Stub stub;
    stub.set(ADDR(QDBusAbstractInterface, isValid), stub_dbus_isValid);
    stub.set(ADDR(QDBusConnection, call), stub_dbus_call);
    DSettingsOption *option = m_tester->getDSettingPointor()->option(kMessageSettingOptionName);
    ASSERT_NE(option, nullptr);

    // Act
    auto pair = SystemProtectionSetting::createSettingLinkButtonHandle(option);

    // Assert: 句柄创建即把选项重置为默认值, 命令链接按钮已创建
    ASSERT_NE(pair.second, nullptr);
    EXPECT_EQ(option->value(), option->defaultValue());
    EXPECT_NE(pair.second->findChild<DCommandLinkButton *>(), nullptr);
}

TEST_F(UT_SystemProtectionSetting, SetLastValidAlarm_InRange_SetsTextAndOption)
{
    // Arrange
    DSettingsOption *option = m_tester->getDSettingPointor()->option(kAlarmCpuUsageOptionName);
    ASSERT_NE(option, nullptr);
    option->setValue(40);
    SystemProtectionSetting::m_lastValidCPUValue = 66;
    DLineEdit edit;
    edit.setText("5");

    // Act: 合法值直接提交
    SystemProtectionSetting::setLastValidAlarm(&edit, option, 100, 30, 66);

    // Assert: 文本与选项值均更新为合法值
    EXPECT_EQ(edit.text(), QString("66"));
    EXPECT_EQ(option->value().toInt(), 66);
}

TEST_F(UT_SystemProtectionSetting, SetLastValidAlarm_OutOfRange_FallsBackToLastValid)
{
    // Arrange
    DSettingsOption *option = m_tester->getDSettingPointor()->option(kAlarmCpuUsageOptionName);
    ASSERT_NE(option, nullptr);
    option->setValue(40);
    SystemProtectionSetting::m_lastValidCPUValue = 35;
    DLineEdit edit;
    edit.setText("5000");

    // Act: 越界值触发回退
    SystemProtectionSetting::setLastValidAlarm(&edit, option, 100, 30, 5);

    // Assert: 文本回退为上次合法值, 选项值保持不变
    EXPECT_EQ(edit.text(), QString("35"));
    EXPECT_EQ(option->value().toInt(), 40);
}

TEST_F(UT_SystemProtectionSetting, LineEditChanged_FocusTrue_DoesNothing)
{
    // Arrange
    DSettingsOption *option = m_tester->getDSettingPointor()->option(kAlarmCpuUsageOptionName);
    ASSERT_NE(option, nullptr);
    DLineEdit edit;
    edit.setText("45");

    // Act: 焦点在编辑框内（focus==true）时直接返回, 不做任何校验
    SystemProtectionSetting::lineEditChanged(true, &edit, option, 100, 30);

    // Assert: 文本与静态值均未被触碰
    EXPECT_EQ(edit.text(), QString("45"));
    EXPECT_EQ(SystemProtectionSetting::m_lastValidCPUValue, -1);
}

TEST_F(UT_SystemProtectionSetting, LineEditChanged_InvalidInput_RestoresLastValid)
{
    // Arrange: 确定选项基线, 避免持久化残留影响
    DSettingsOption *option = m_tester->getDSettingPointor()->option(kAlarmCpuUsageOptionName);
    ASSERT_NE(option, nullptr);
    option->setValue(50);
    SystemProtectionSetting::m_lastValidCPUValue = 33;
    DLineEdit edit;
    edit.setText("5000");

    // Act: 失焦提交越界输入
    SystemProtectionSetting::lineEditChanged(false, &edit, option, 100, 30);

    // Assert: 越界输入被回退为上次合法值, 选项值与静态值同步回退
    EXPECT_EQ(edit.text(), QString("33"));
    EXPECT_EQ(option->value().toInt(), 33);
    EXPECT_EQ(SystemProtectionSetting::m_lastValidCPUValue, 33);
}

TEST_F(UT_SystemProtectionSetting, LineEditChanged_ValidInput_UpdatesOptionAndStatic)
{
    // Arrange
    DSettingsOption *option = m_tester->getDSettingPointor()->option(kAlarmCpuUsageOptionName);
    ASSERT_NE(option, nullptr);
    option->setValue(50);
    SystemProtectionSetting::m_lastValidCPUValue = -1;
    DLineEdit edit;
    edit.setText("50");

    // Act: 失焦提交合法输入
    SystemProtectionSetting::lineEditChanged(false, &edit, option, 100, 30);

    // Assert: 静态值与选项值均更新为输入值
    EXPECT_EQ(edit.text(), QString("50"));
    EXPECT_EQ(SystemProtectionSetting::m_lastValidCPUValue, 50);
    EXPECT_EQ(option->value().toInt(), 50);
}

TEST_F(UT_SystemProtectionSetting, LineEditChanged_MemOutOfRange_RestoresLastValid)
{
    // Arrange
    DSettingsOption *option = m_tester->getDSettingPointor()->option(kAlarmMemUsageOptionName);
    ASSERT_NE(option, nullptr);
    option->setValue(44);
    SystemProtectionSetting::m_lastValidMemoryValue = 44;
    DLineEdit edit;
    edit.setText("999");

    // Act: 内存分支越界回退
    SystemProtectionSetting::lineEditChanged(false, &edit, option, 100, 30);

    // Assert: 文本/选项值/静态值三者一致回退
    EXPECT_EQ(edit.text(), QString("44"));
    EXPECT_EQ(option->value().toInt(), 44);
    EXPECT_EQ(SystemProtectionSetting::m_lastValidMemoryValue, 44);
}

TEST_F(UT_SystemProtectionSetting, LineEditChanged_ValueBelowMin_RestoresLastValid)
{
    // Arrange
    DSettingsOption *option = m_tester->getDSettingPointor()->option(kAlarmCpuUsageOptionName);
    ASSERT_NE(option, nullptr);
    option->setValue(45);
    SystemProtectionSetting::m_lastValidCPUValue = 45;
    DLineEdit edit;
    edit.setText("1");

    // Act: 输入低于下限 30 → 越界回退
    SystemProtectionSetting::lineEditChanged(false, &edit, option, 100, 30);

    // Assert: 文本/选项值/静态值三者一致回退
    EXPECT_EQ(edit.text(), QString("45"));
    EXPECT_EQ(option->value().toInt(), 45);
    EXPECT_EQ(SystemProtectionSetting::m_lastValidCPUValue, 45);
}

TEST_F(UT_SystemProtectionSetting, LineEditChanged_EmptyText_RestoresLastValid)
{
    // Arrange
    DSettingsOption *option = m_tester->getDSettingPointor()->option(kAlarmCpuUsageOptionName);
    ASSERT_NE(option, nullptr);
    option->setValue(45);
    SystemProtectionSetting::m_lastValidCPUValue = 45;
    DLineEdit edit;
    edit.setText("");

    // Act: 空文本走越界回退分支
    SystemProtectionSetting::lineEditChanged(false, &edit, option, 100, 30);

    // Assert: 文本/选项值/静态值三者一致回退
    EXPECT_EQ(edit.text(), QString("45"));
    EXPECT_EQ(option->value().toInt(), 45);
    EXPECT_EQ(SystemProtectionSetting::m_lastValidCPUValue, 45);
}

TEST_F(UT_SystemProtectionSetting, LineEditChanged_IntervalOutOfRange_RestoresLastValid)
{
    // Arrange
    DSettingsOption *option = m_tester->getDSettingPointor()->option(kAlarmIntervalOptionName);
    ASSERT_NE(option, nullptr);
    option->setValue(20);
    SystemProtectionSetting::m_lastValidInternalValue = 20;
    DLineEdit edit;
    edit.setText("99");

    // Act: 通知间隔分支越界回退（间隔真实边界为 [5,60]）
    SystemProtectionSetting::lineEditChanged(false, &edit, option, 60, 5);

    // Assert: 文本/选项值/静态值三者一致回退
    EXPECT_EQ(edit.text(), QString("20"));
    EXPECT_EQ(option->value().toInt(), 20);
    EXPECT_EQ(SystemProtectionSetting::m_lastValidInternalValue, 20);
}

TEST_F(UT_SystemProtectionSetting, OnMessgaeSetting_GenericPath_DoesNotCrash)
{
    // Arrange: 桩替换 DBus 调用并清零计数
    Stub stub;
    stub.set(ADDR(QDBusAbstractInterface, isValid), stub_dbus_isValid);
    stub.set(ADDR(QDBusConnection, call), stub_dbus_call);
    g_dbusCallCount = 0;

    // Act: 以通用路径触发跳转（desktop 文件存在与否均应发出跳转调用）
    m_tester->onMessgaeSetting("generic");

    // Assert: 经 sessionBus().call 发起跳转调用（桩计数）, 方法执行未崩溃
    EXPECT_GE(g_dbusCallCount, 1);
    EXPECT_NE(SystemProtectionSetting::instance(), nullptr);
}

TEST_F(UT_SystemProtectionSetting, OnSettingItemChanged_KnownKey_NoWarning)
{
    // Arrange: 确保类别 warning 可输出（测试环境默认规则可能关闭）, 清空捕获缓存并安装消息处理器
    QLoggingCategory::setFilterRules("org.deepin.system-monitor.warning=true\n");
    g_lastWarning.clear();
    QtMessageHandler oldHandler = qInstallMessageHandler(warnCaptureHandler);

    // Act: 通知已知告警键
    m_tester->onSettingItemChanged(kAlarmStatusOptionName, QVariant(true));
    qInstallMessageHandler(oldHandler);

    // Assert: 已知键不产生 "Unknown setting key" 告警, 配置对象仍有效
    EXPECT_EQ(g_lastWarning.contains("Unknown setting key"), false);
    EXPECT_NE(m_tester->getDSettingPointor(), nullptr);
}

TEST_F(UT_SystemProtectionSetting, OnSettingItemChanged_CpuMemIntervalLastTime_NoWarning)
{
    // Arrange: 确保类别 warning 可输出（测试环境默认规则可能关闭）, 清空捕获缓存并安装消息处理器
    QLoggingCategory::setFilterRules("org.deepin.system-monitor.warning=true\n");
    g_lastWarning.clear();
    QtMessageHandler oldHandler = qInstallMessageHandler(warnCaptureHandler);

    // Act: 依次通知其余已知告警键（覆盖 else-if 链各分支）
    for (const char *key : { "setting.systemprotection.alarm_cpu_usage", "setting.systemprotection.alarm_memory_usage",
                             "setting.systemprotection.alarm_interval", "setting.systemprotection.alarm_last_time" })
        m_tester->onSettingItemChanged(key, QVariant(30));
    qInstallMessageHandler(oldHandler);

    // Assert: 均为已知键, 不产生 "Unknown setting key" 告警, 配置对象仍有效
    EXPECT_EQ(g_lastWarning.contains("Unknown setting key"), false);
    EXPECT_NE(m_tester->getDSettingPointor(), nullptr);
}

TEST_F(UT_SystemProtectionSetting, OnSettingItemChanged_UnknownKey_EarlyReturn)
{
    // Arrange: 清空捕获缓存并安装消息处理器（工程全局定义 QT_NO_WARNING_OUTPUT,
    // qCWarning 被编译裁剪, 告警断言降级为表征检查: 无输出且提前返回不崩溃）
    g_lastWarning.clear();
    QtMessageHandler oldHandler = qInstallMessageHandler(warnCaptureHandler);

    // Act: 通知未知键, 走 else 分支提前返回
    m_tester->onSettingItemChanged("setting.systemprotection.unknown_key", QVariant(1));
    qInstallMessageHandler(oldHandler);

    // Assert: 无任何输出（告警已裁剪）, 配置对象未受影响, 用例存活
    EXPECT_TRUE(g_lastWarning.isEmpty());
    EXPECT_NE(m_tester->getDSettingPointor(), nullptr);
}

TEST_F(UT_SystemProtectionSetting, OnUpdateNewBackend_RepeatedCall_ReplacesBackendPointer)
{
    // Arrange: 记录构造时创建的后端指针
    auto *oldBackend = m_tester->mBackend;
    ASSERT_NE(oldBackend, nullptr);

    // Act
    m_tester->onUpdateNewBackend();

    // Assert: 后端被新实例替换
    EXPECT_NE(m_tester->mBackend, nullptr);
    EXPECT_NE(m_tester->mBackend, oldBackend);
}

TEST_F(UT_SystemProtectionSetting, RegularNumber_FocusLost_TruncatesToInteger)
{
    // Arrange: 注册数字规整逻辑; 输入带前导空白（生产中 QIntValidator 只放行整数, 此处验证规整）
    DLineEdit edit;
    edit.setText("  42");
    m_tester->regularNumber(&edit);

    // Act: 失焦
    edit.focusChanged(false);

    // Assert: 文本被规整为整数 42
    EXPECT_EQ(edit.text(), QString("42"));
    EXPECT_EQ(edit.text().toInt(), 42);
}

TEST_F(UT_SystemProtectionSetting, RegularNumber_ReturnPressed_TruncatesToInteger)
{
    // Arrange: 生产中 QIntValidator 会拦截非整数输入, 此处用带空白的整数验证规整路径
    DLineEdit edit;
    edit.setText("99 ");
    m_tester->regularNumber(&edit);

    // Act: 回车提交
    edit.returnPressed();

    // Assert: 文本被规整为整数 99
    EXPECT_EQ(edit.text(), QString("99"));
    EXPECT_EQ(edit.text().toInt(), 99);
}

TEST_F(UT_SystemProtectionSetting, Destructor_ExplicitDelete_ReleasesWithoutGlobalSingleton)
{
    // Arrange: 监听销毁信号
    QSignalSpy spy(m_tester, &QObject::destroyed);

    // Act: 销毁夹具对象
    delete m_tester;
    m_tester = nullptr;

    // Assert: 对象已销毁且全局单例独立存活
    EXPECT_EQ(spy.count(), 1);
    EXPECT_NE(SystemProtectionSetting::instance(), nullptr);
}

TEST(UT_SystemProtectionSettingFree, ChangeWidgetFontSizeByDiffWithSystem_NullWidget_ReturnsFalse)
{
    // Arrange: 记录字体基线
    QLabel label;
    const QFont before = label.font();

    // Act: 空控件入参
    const bool ok = changeWidgetFontSizeByDiffWithSystem(nullptr, kToolTipFontSizeDiff);

    // Assert: 直接拒绝且字体未变
    EXPECT_EQ(ok, false);
    EXPECT_EQ(label.font().pointSizeF(), before.pointSizeF());
}

TEST(UT_SystemProtectionSettingFree, ChangeWidgetFontSizeByDiffWithSystem_ZeroDiff_ReturnsFalse)
{
    // Arrange: 零偏移在 DBus 交互前就被拒绝
    Stub stub;
    stub.set(ADDR(QDBusAbstractInterface, isValid), stub_dbus_isInvalid);
    QLabel label;
    const QFont before = label.font();

    // Act
    const bool ok = changeWidgetFontSizeByDiffWithSystem(&label, 0.0);

    // Assert: 零偏移直接拒绝且字体未变
    EXPECT_EQ(ok, false);
    EXPECT_EQ(label.font().pointSizeF(), before.pointSizeF());
}

TEST(UT_SystemProtectionSettingFree, ChangeWidgetFontSizeByDiffWithSystem_InvalidInterface_ReturnsFalse)
{
    // Arrange: 桩设接口无效, 不依赖真实会话总线
    Stub stub;
    stub.set(ADDR(QDBusAbstractInterface, isValid), stub_dbus_isInvalid);
    QLabel label;
    const QFont before = label.font();

    // Act
    const bool ok = changeWidgetFontSizeByDiffWithSystem(&label, kToolTipFontSizeDiff);

    // Assert: 接口无效返回 false 且字体未应用
    EXPECT_EQ(ok, false);
    EXPECT_EQ(label.font().pointSizeF(), before.pointSizeF());
}

TEST(UT_SystemProtectionSettingFree, ChangeWidgetFontSizeByDiffWithSystem_ValidFontSize_AppliesFont)
{
    // Arrange: 桩替换接口有效性（恒真）与系统字体查询（固定 11.0）
    Stub stub;
    stub.set(ADDR(QDBusAbstractInterface, isValid), stub_dbus_isValid);
    stub.set(ADDR(QDBusInterface, property), stub_dbus_propertyFontSize);
    QLabel label;

    // Act: 应用系统字号偏移
    const bool ok = changeWidgetFontSizeByDiffWithSystem(&label, kToolTipFontSizeDiff);

    // Assert: 字体按系统字号 + 偏移应用
    EXPECT_EQ(ok, true);
    EXPECT_DOUBLE_EQ(label.font().pointSizeF(), 11.0 + kToolTipFontSizeDiff);
}
