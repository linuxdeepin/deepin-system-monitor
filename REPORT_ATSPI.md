# deepin-system-monitor AT-SPI 补全报告

## 扫描情况

### 扫描范围与方法
- 仓库：https://github.com/linuxdeepin/deepin-system-monitor (master 分支)
- 扫描方法：源码静态分析，检查 accessible factory 注册、accessibleName/objectName 设置
- 扫描产出：见下方详细分析

### 扫描结论：以下三个子项目分别评估

| 子项目 | 现有 AT-SPI 覆盖 | 待补全项 |
|--------|:--------:|:--------:|
| `deepin-system-monitor-main` (主应用) | ✅ 完整 | 0 |
| `deepin-system-monitor-plugin` (Dock 插件) | ⚠️ 缺少 objectName | 4 |
| `deepin-system-monitor-plugin-popup` (插件弹窗) | ❌ 完全缺失 | 1 套基础设施 + 8 个控件 |

---

### 1. deepin-system-monitor-main（主应用）— 已有完善覆盖

主应用已具备完整的 AT-SPI 基础设施：
- `accessibledefine.h` — 定义了完整的 accessible wrapper 宏（SET_FORM_ACCESSIBLE、SET_BUTTON_ACCESSIBLE 等）
- `accessible.h` — 注册了所有自定义控件（MainWindow、Toolbar、ProcessPageWidget 等 30+ 类）和通用 Qt/DTK 控件（QWidget、QPushButton、DFrame 等）
- `main.cpp` — 调用 `QAccessible::installFactory(accessibleFactory)`

**覆盖的控件清单：** MainWindow, ErrorDialog, Toolbar, BaseHeaderView, ProcessPageWidget, ServiceNameSubInputDialog, SystemServicePageWidget, MonitorExpandView, MonitorCompactView, PrioritySlider, KillProcessConfirmDialog, ProcessAttributeDialog, XWinKillPreviewWidget, XWinKillPreviewBackgroundWidget, CPUDetailWidget, MemDetailViewWidget, MemSummaryViewWidget, BlockDevDetailViewWidget, BlockDevSummaryViewWidget, NetifDetailViewWidget, NetifSummaryViewWidget, DetailViewStackedWidget, CompactCpuMonitor, CompactDiskMonitor, CompactMemoryMonitor, CompactNetworkMonitor, CpuMonitor, MemoryMonitor, NetworkMonitor, UserPageWidget

**结论：主应用无需补全。**

---

### 2. deepin-system-monitor-plugin（Dock 插件）— 缺少 objectName

Dock 插件运行在 dde-dock 进程内，4 个自定义 QWidget 控件缺少 `setObjectName()` / `setAccessibleName()`：

| 控件 | 文件 | 状态 |
|------|------|:----:|
| `MonitorPluginButtonWidget` | `gui/monitorpluginbuttonwidget.cpp` | ⚠️ 无 name |
| `QuickPanelWidget` | `gui/quickpanelwidget.cpp` | ⚠️ 无 name |
| `SystemMonitorTipsWidget` | `gui/systemmonitortipswidget.cpp` | ⚠️ 无 name |
| `CommonIconButton` | `gui/commoniconbutton.cpp` | ✅ 已有 setAccessibleName |

---

### 3. deepin-system-monitor-plugin-popup（插件弹窗）— 完全缺失 AT-SPI

插件弹窗是一个**独立进程**，其 `main.cpp` **未安装** AT-SPI factory，所有 Widget 没有 accessibility 信息。

**缺失的基础设施：**
- 缺失 `accessible.h` / `QAccessible::installFactory()`
- 以下 8 个自定义控件均无 AT-SPI 注册：

| 控件 | 类 | 父类 |
|------|:---|:----:|
| `MainWindow` | 弹窗主窗口 | DBlurEffectWidget |
| `CpuWidget` | CPU 监控面板 | QWidget |
| `DiskWidget` | 磁盘监控面板 | QWidget |
| `MemoryWidget` | 内存监控面板 | QWidget |
| `NetWidget` | 网络监控面板 | QWidget |
| `ProcessWidget` | 进程列表面板 | QWidget |
| `ProcessTableView` | 进程表格视图 | BaseTableView → DTreeView |
| `BaseTableView` | 基础表格视图 | DTreeView |

---

## 补全详情

### 修改 1: plugin-popup 新增 accessible.h（AT-SPI 基础设施）

**文件：** `deepin-system-monitor-plugin-popup/accessible.h`（新文件）

为弹窗独立进程创建完整的 accessible 工厂注册，包括：
- 8 个自定义控件的 SET_FORM_ACCESSIBLE 注册
- 通用 Qt 控件回退（QWidget、QFrame、QPushButton）
- 通用 DTK 控件回退（DWidget、DFrame、DScrollArea、DPushButton）
- `accessibleFactory` 函数用于 `QAccessible::installFactory`

### 修改 2: plugin-popup main.cpp 安装 AT-SPI factory

**文件：** `deepin-system-monitor-plugin-popup/main.cpp`

- 新增 `#include "accessible.h"`
- 在 `ac.loadTranslator()` 前添加 `QAccessible::installFactory(accessibleFactory)`

**原因：** 使弹窗进程的所有控件在 AT-SPI 层面可识别。

### 修改 3-5: Plugin 控件添加 objectName/accessibleName

**文件：** 
- `deepin-system-monitor-plugin/gui/monitorpluginbuttonwidget.cpp`
- `deepin-system-monitor-plugin/gui/quickpanelwidget.cpp`
- `deepin-system-monitor-plugin/gui/systemmonitortipswidget.cpp`

在每个控件构造函数中添加 `setObjectName()` 和 `setAccessibleName()`。

**原因：** 这 3 个 Dock 插件控件自定义绘制且无 objectName，设置后可在 dde-dock 的通用 QWidget accessible 中通过有意义名称被识别。

---

## 覆盖率对比

| 组件 | 补全前 AT-SPI | 补全后 AT-SPI |
|------|:------------:|:------------:|
| deepin-system-monitor-main | ✅ 完整 | ✅ 完整（无变化） |
| deepin-system-monitor-plugin | ⚠️ 4 控件缺 3 个 objectName | ✅ 所有控件均有名称 |
| deepin-system-monitor-plugin-popup | ❌ 0/8 控件可识别 | ✅ 8/8 控件有 AT-SPI 注册 |

**说明：** 由于缺少 `libpcap-dev` 等构建依赖（无 root 权限），无法运行 libclang AST 扫描自动化覆盖率计算。上述覆盖率基于源码静态分析。

---

## 补全文件清单

```
新增:  deepin-system-monitor-plugin-popup/accessible.h                      (AT-SPI 工厂注册)
修改:  deepin-system-monitor-plugin-popup/main.cpp                          (安装 factory)
修改:  deepin-system-monitor-plugin/gui/monitorpluginbuttonwidget.cpp       (添加 objectName)
修改:  deepin-system-monitor-plugin/gui/quickpanelwidget.cpp                (添加 objectName)
修改:  deepin-system-monitor-plugin/gui/systemmonitortipswidget.cpp         (添加 objectName)
```

---

## PR 链接

- Draft PR: https://github.com/linuxdeepin/deepin-system-monitor/pull/560
- 分支：`fix/at-spi-completion-20260805`

## 备注

- 无改动覆盖已有用户代码
- 所有操作均可通过 git 追溯