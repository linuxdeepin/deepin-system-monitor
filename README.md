# Deepin System Monitor

Deepin system monitor: a more user-friendly system monitor.

Thanks a lot for [Lily Rivers](https://github.com/VioletDarkKitty/system-monitor)'s working, this project used a lot of her code.

## Dependencies

Prefer installing build dependencies directly from Debian metadata (keeps docs aligned with `debian/control`):

```bash
sudo apt update
sudo apt build-dep .
```

If your environment cannot use `apt build-dep`, install them manually:

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

Some distributions provide `dde-dock-dev` instead of `dde-tray-loader-dev`; either one is acceptable.

## Installation

```bash
mkdir build
cd build
cmake ..
make
sudo setcap cap_net_raw+ep ./deepin-system-monitor-main/deepin-system-monitor
```

### Build Options

* `-DCMAKE_BUILD_TYPE=Debug` — Build with debug symbols
* `-DCMAKE_BUILD_TYPE=Release` — Build optimized release version
* `-DDOTEST=ON` — Enable unit tests compilation

### Running Tests

`GTest/GMock` are required when building with `-DDOTEST=ON`. Install them with:

```bash
sudo apt install libgtest-dev libgmock-dev
```

```bash
cd build
cmake .. -DDOTEST=ON
make
./tests/deepin-system-monitor-test

# Run a single test
./tests/deepin-system-monitor-test --gtest_filter="TestSuiteName.TestName"

# Or use the test target
make test
```

## Usage

* ./deepin-system-monitor-main/deepin-system-monitor

## Features

### GPU Monitoring

The system monitor supports GPU metrics monitoring for:
- **NVIDIA GPUs** — Via nvidia-smi (GPU load, memory usage, temperature)
- **AMD GPUs** — Via sysfs (GPU load, VRAM usage, temperature)
- **Intel GPUs** — Basic detection via sysfs

GPU information is automatically detected and displayed in the system monitor.

## Config file

* ~/.config/deepin/deepin-system-monitor/config.conf

## Getting help

Any usage issues can ask for help via

* [Gitter](https://gitter.im/orgs/linuxdeepin/rooms)
* [IRC channel](https://webchat.freenode.net/?channels=deepin)
* [Forum](https://bbs.deepin.org)
* [WiKi](http://wiki.deepin.org/)

## Getting involved

We encourage you to report issues and contribute changes

* [Contribution guide for users](http://wiki.deepin.org/index.php?title=Contribution_Guidelines_for_Users)
* [Contribution guide for developers](http://wiki.deepin.org/index.php?title=Contribution_Guidelines_for_Developers).

## License

This project is licensed under GPLv3 or any later version.
