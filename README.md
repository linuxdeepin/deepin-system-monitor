# Deepin System Monitor

Deepin system monitor: a more user-friendly system monitor.

Thanks alot [Lily Rivers](https://github.com/VioletDarkKitty/system-monitor), this project used a lot of his code.

## Dependencies

## Build Dependencies

* debhelper (>= 9)
* qt5-qmake
* qt5-default
* qtbase5-dev
* pkg-config
* libdtkbase-dev
* libdtkutil-dev
* libdtkwidget-dev
* libxcb-util0-dev
* libxcb1-dev
* libqt5x11extras5-dev
* libprocps-dev
* libxext-dev
* libxtst-dev
* libncurses-dev
* libpcap-dev

## Installation

* mkdir build
* cd build
* qmake ..
* make
* sudo setcap cap_kill,cap_net_raw,cap_dac_read_search,cap_sys_ptrace+ep ./deepin-system-monitor
* ./deepin-system-monitor

## Usage

## Config file

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

Deepin System Monitor is licensed under [GPLv3](LICENSE).
