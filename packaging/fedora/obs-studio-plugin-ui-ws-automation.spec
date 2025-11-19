Name: obs-studio-plugin-ui-ws-automation
Version: @VERSION@
Release: @RELEASE@%{?dist}
Summary: UI WS Automation Plugin for OBS Studio
License: GPLv2+

Source0: %{name}-%{version}.tar.bz2
BuildRequires: cmake, gcc, gcc-c++
BuildRequires: obs-studio-devel
BuildRequires: qt6-qtbase-devel qt6-qtbase-private-devel

%description
Helps to test plugins on OBS Studio by delegating user-interface operation
through obs-websocket.

%prep
%autosetup -p1

%build
%{cmake} -DLINUX_PORTABLE=OFF -DLINUX_RPATH=OFF -DQT_VERSION=6 -DINSTALL_LICENSE_FILES:BOOL=OFF
%{cmake_build}

%install
%{cmake_install}

%files
%{_libdir}/obs-plugins/*.so
%{_datadir}/obs/obs-plugins/*/
%license LICENSE
