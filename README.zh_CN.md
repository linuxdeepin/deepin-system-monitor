# Deepin System Monitor

深度系统监视器：更友好的系统监视工具。

感谢 [Lily Rivers](https://github.com/VioletDarkKitty/system-monitor) 的工作，本项目复用了其部分代码。

## 依赖

建议优先使用 Debian 打包元数据安装依赖（与 `debian/control` 保持一致）：

```bash
sudo apt update
sudo apt build-dep .
```

如果环境不方便使用 `apt build-dep`，可手动安装：

```bash
sudo apt install cmake pkg-config \
    qt6-base-dev qt6-svg-dev qt6-tools-dev qt6-tools-dev-tools \
    libdtk6core-dev libdtk6gui-dev libdtk6widget-dev \
    libgtest-dev libgmock-dev \
    libpcap-dev libxcb1-dev libxext-dev libxcb-util0-dev libxcb-icccm4-dev \
    libnl-3-dev libnl-route-3-dev libudev-dev libicu-dev \
    libpolkit-qt6-1-dev deepin-gettext-tools \
    dde-tray-loader-dev
```

部分发行版可能提供 `dde-dock-dev` 替代 `dde-tray-loader-dev`，二者任选其一即可。

## 安装

```bash
mkdir build
cd build
cmake ..
make
sudo setcap cap_net_raw+ep ./deepin-system-monitor-main/deepin-system-monitor
```

### 构建选项

* `-DCMAKE_BUILD_TYPE=Debug`：启用调试符号
* `-DCMAKE_BUILD_TYPE=Release`：发布优化构建
* `-DDOTEST=ON`：编译单元测试

### 运行测试

启用 `-DDOTEST=ON` 时需要 `GTest/GMock`。

```bash
cd build
cmake .. -DDOTEST=ON
make
./tests/deepin-system-monitor-test

# 仅运行单个测试
./tests/deepin-system-monitor-test --gtest_filter="TestSuiteName.TestName"

# 或使用 CMake 测试目标
make test
```

## 用法

* `./deepin-system-monitor-main/deepin-system-monitor`

## 功能

### GPU / NPU 监控

支持以下设备与指标采集：

* NVIDIA：通过 `nvidia-smi` 获取利用率、显存、温度
* AMD：通过 `sysfs` 获取利用率、显存、温度和部分引擎数据
* Intel：通过 `sysfs` 获取基础利用率与引擎数据
* NPU：通过 `devfreq`/`sysfs` 自动探测（若系统提供）

同时支持：

* GPU 详情页（左侧 GPU 卡片可直接跳转）
* 无 GPU 时自动隐藏左侧 GPU 卡片与详情入口

## 配置文件

* `~/.config/deepin/deepin-system-monitor/config.conf`

## 获取帮助

* [Gitter](https://gitter.im/orgs/linuxdeepin/rooms)
* [Forum](https://bbs.deepin.org)
* [Wiki](http://wiki.deepin.org/)

## 参与贡献

欢迎提交 Issue 和代码贡献。

* [用户贡献指南](http://wiki.deepin.org/index.php?title=Contribution_Guidelines_for_Users)
* [开发者贡献指南](http://wiki.deepin.org/index.php?title=Contribution_Guidelines_for_Developers)

## 许可证

本项目基于 GPLv3 或其后续版本发布。
