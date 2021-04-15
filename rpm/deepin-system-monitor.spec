%global debug_package   %{nil}
%define pkgrelease  1
%if 0%{?openeuler}
%define specrelease %{pkgrelease}
%else
## allow specrelease to have configurable %%{?dist} tag in other distribution
%define specrelease %{pkgrelease}%{?dist}
%endif

Name:           deepin-system-monitor
Version:        5.8.0.30

Release:        %{specrelease}
Summary:        A more user-friendly system monitor
License:        GPLv3
URL:            https://github.com/linuxdeepin/deepin-system-monitor
Source0:        %{url}/archive/%{version}/%{name}-%{version}.tar.gz	

BuildRequires:  qt5-devel
BuildRequires:  cmake

BuildRequires:  pkgconfig(dtkwidget)
BuildRequires:  pkgconfig(dtkcore)
BuildRequires:  pkgconfig(dtkgui)
BuildRequires:  pkgconfig(dtkwm)

BuildRequires:  pkgconfig(libprocps)
BuildRequires:  pkgconfig(xcb)
BuildRequires:  pkgconfig(xcb-util)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(xext)
BuildRequires:  pkgconfig(xtst)
BuildRequires:  xcb-util-wm-devel
# BuildRequires:  pkgconfig(Qt5DBus)
# BuildRequires:  pkgconfig(Qt5Gui)
# BuildRequires:  pkgconfig(Qt5Network)
# BuildRequires:  pkgconfig(Qt5Widgets)
# BuildRequires:  pkgconfig(Qt5X11Extras)
# BuildRequires:  qt5-linguist
BuildRequires:  libpcap-devel
BuildRequires:  libcap-devel
# BuildRequires:  ncurses-devel
BuildRequires:  desktop-file-utils
BuildRequires:  libappstream-glib
BuildRequires:  deepin-gettext-tools 
# BuildRequires:  dtkwm-devel
# BuildRequires:  qt5-qtmultimedia-devel
# BuildRequires:  dtkcore-devel >= 5.1
BuildRequires:  libicu-devel
BuildRequires:  libnl3-devel
BuildRequires:  systemd-devel
BuildRequires:  pkgconfig(libnl-route-3)
#Requires:       hicolor-icon-theme
Recommends:     deepin-manual

%description
%{summary}.

%prep
%autosetup
# %setup -q
####sed -i 's|lrelease|lrelease-qt5|' translations/translate_generation.sh

# %patch0 -p1

%build
export PATH=%{_qt5_bindir}:$PATH
sed -i "s|^cmake_minimum_required.*|cmake_minimum_required(VERSION 3.0)|" $(find . -name "CMakeLists.txt")
mkdir build && pushd build 
%cmake -DCMAKE_BUILD_TYPE=Release ../  -DAPP_VERSION=%{version} -DVERSION=%{version} 
%make_build  
popd

%install
%make_install -C build INSTALL_ROOT="%buildroot"

# %check
# desktop-file-validate %{buildroot}%{_datadir}/applications/%{name}.desktop ||:
#appstream-util validate-relax --nonet %{buildroot}%{_datadir}/appdata/*.appdata.xml

%files
%doc README.md
%license LICENSE
%caps(cap_kill,cap_net_raw,cap_dac_read_search,cap_sys_ptrace=+ep) %{_bindir}/%{name}
%{_bindir}/%{name} 
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/scalable/apps/%{name}.svg
%{_datadir}/%{name}/translations/*.qm
%{_datadir}/polkit-1/actions/com.deepin.pkexec.deepin-system-monitor.policy

%changelog
* Thu Apr 15 2021 Zhang Ding <zhangdingwen@uniontech.com> - 5.8.0.30-1
- Initial release for OpenEuler
