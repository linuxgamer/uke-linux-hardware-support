# Based on Elaine Gibson's Fedora package review, rhbz#2386504.
# Keep the upstream application name and Fedora runtime dependencies.
Name:           freecad
Epoch:          1
Version:        1.1.4
Release:        1%{?dist}
Summary:        A 3D parametric modeler

License:        LGPL-2.1-or-later AND Zlib AND BSD-3-Clause AND Apache-2.0 AND EUPL-1.2 AND BSL-1.0 AND MIT AND Artistic-2.0 AND GPL-2.0-or-later AND CC-BY-SA-4.0
URL:            https://www.freecad.org/
Source0:        https://github.com/FreeCAD/FreeCAD/releases/download/%{version}/freecad_source_%{version}.tar.gz
Source1:        freecad-source-lock.json
Source2:        https://raw.githubusercontent.com/nlohmann/json/v3.11.3/LICENSE.MIT#/nlohmann-json-LICENSE.MIT
# Upstream fixes used by the Fedora review; still absent from the 1.1.4 branch.
Patch0:         https://github.com/FreeCAD/FreeCAD/commit/2f899992a048a8a1011bd59ccb66eeec00d42b29.patch
Patch1:         https://github.com/FreeCAD/FreeCAD/commit/e9b4a1c4bfdb7d08f4f3dc4c7a8df4e3bc2cb284.patch
Patch2:         https://github.com/FreeCAD/FreeCAD/commit/8c8b7c1d8fa34fc3d11d593ce0a6d5f655f37f80.patch

ExclusiveArch:  aarch64

BuildRequires:  cmake
BuildRequires:  gcc-c++
BuildRequires:  cmake(Qt6Core)
BuildRequires:  cmake(Qt6SvgWidgets)
BuildRequires:  cmake(fmt)
BuildRequires:  cmake(yaml-cpp)
BuildRequires:  libicu-devel
BuildRequires:  python3-devel
BuildRequires:  cmake(pybind11)
BuildRequires:  cmake(XercesC)
BuildRequires:  pkgconfig(PyCXX)
BuildRequires:  cmake(OpenCASCADE)
BuildRequires:  cmake(MEDFile)
BuildRequires:  pkgconfig(ompi-cxx)
BuildRequires:  swig
BuildRequires:  Coin4-devel
BuildRequires:  libspnav-devel
BuildRequires:  cmake(Shiboken6)
BuildRequires:  cmake(PySide6)
BuildRequires:  pyside6-tools
BuildRequires:  qt6-qttools-devel
BuildRequires:  python3-matplotlib
BuildRequires:  python3-pivy
BuildRequires:  ondselsolver-devel
BuildRequires:  orocos-kdl-devel
BuildRequires:  cmake(e57format)
BuildRequires:  cmake(Microsoft.GSL)
BuildRequires:  cmake(GTest)
BuildRequires:  gmock-devel
BuildRequires:  hdf5-static
BuildRequires:  vtk-devel
BuildRequires:  desktop-file-utils
BuildRequires:  libappstream-glib
BuildRequires:  python3-rpm-macros

Requires:       %{name}-data = %{epoch}:%{version}-%{release}
Requires:       python3-pivy
Requires:       python3-matplotlib
Requires:       python3-pyside6
Requires:       python3-collada
Requires:       python3-defusedxml
Requires:       python3-typing-extensions
Requires:       hicolor-icon-theme
Requires:       shared-mime-info

Provides:       bundled(zipios++)
Provides:       bundled(SMESH) = 7.7.1.0
Provides:       bundled(nlohmann_json) = 3.11.3
Provides:       bundled(libkdtree++)

%global __provides_exclude_from ^%{_libdir}/freecad/.*$
%global __requires_exclude ^(lib(Driver(|DAT|STL|UNV)|FreeCAD(App|Base|Gui)|MEFISTO2|SMDS|SMESH|SMESHDS|StdMeshers|area-native)[.]so.*)$

%description
FreeCAD is an open-source parametric 3D computer-aided design modeler for
mechanical engineering, product design, architecture and related disciplines.
This package retains FreeCAD's upstream Python workbenches and uses Fedora's
Python, Qt and engineering libraries.

%package data
Summary:        Data files for FreeCAD
License:        LGPL-2.1-or-later AND GPL-2.0-or-later AND CC-BY-3.0 AND CC-BY-4.0 AND CC-BY-SA-4.0
BuildArch:      noarch

%description data
Shared resources, translations and workbench data for FreeCAD.

%prep
%autosetup -c -p1
# External libraries come from Fedora. The stable branch does not support the
# newer external JSON/kdtree options, so retain its original bundled headers.
rm -r src/3rdParty/PyCXX src/3rdParty/OndselSolver src/3rdParty/libE57Format \
      src/3rdParty/GSL tests/lib src/3rdParty/3Dconnexion \
      src/3rdParty/OpenGL src/3rdParty/lru-cache
# Workbench modules are imported by FreeCAD, not executed as standalone scripts.
find src -type f -name '*.py' -exec sed -i '1{/^#!/d;}' {} +

%build
%limit_build -m 3000
%cmake \
 -DCMAKE_INSTALL_PREFIX=%{_libdir}/freecad \
 -DCMAKE_INSTALL_BINDIR=%{_libdir}/freecad/bin \
 -DCMAKE_INSTALL_LIBDIR=%{_libdir}/freecad \
 -DCMAKE_INSTALL_DATAROOTDIR=%{_datadir} \
 -DCMAKE_INSTALL_DATADIR=../../share/freecad \
 -DFREECAD_QT_MAJOR_VERSION=6 \
 -DFREECAD_USE_EXTERNAL_ZIPIOS=OFF \
 -DFREECAD_USE_EXTERNAL_SMESH=OFF \
 -DFREECAD_USE_EXTERNAL_KDL=ON \
 -DFREECAD_USE_EXTERNAL_FMT=ON \
 -DFREECAD_USE_EXTERNAL_ONDSELSOLVER=ON \
 -DFREECAD_USE_EXTERNAL_E57FORMAT=ON \
 -DFREECAD_USE_EXTERNAL_GTEST=ON \
 -DFREECAD_USE_EXTERNAL_PYCXX=ON \
 -DBUILD_MATERIAL_EXTERNAL=OFF \
 -DENABLE_DEVELOPER_TESTS=ON \
 -DFETCHCONTENT_FULLY_DISCONNECTED=ON
%cmake_build

%install
%cmake_install
rm %{buildroot}%{_docdir}/FreeCAD/LICENSE.html
rm %{buildroot}%{_docdir}/FreeCAD/ThirdPartyLibraries.html
rmdir %{buildroot}%{_docdir}/FreeCAD
mkdir -p %{buildroot}%{_bindir}
ln -sr %{buildroot}%{_libdir}/freecad/bin/FreeCAD %{buildroot}%{_bindir}/FreeCAD
ln -sr %{buildroot}%{_libdir}/freecad/bin/FreeCADCmd %{buildroot}%{_bindir}/FreeCADCmd
ln -s FreeCAD %{buildroot}%{_bindir}/freecad
%py_byte_compile %{python3} %{buildroot}%{_libdir}/freecad/

%check
export QT_QPA_PLATFORM=offscreen
export HOME=%{_vpath_builddir}/test-home
export XDG_RUNTIME_DIR=%{_vpath_builddir}/test-runtime
mkdir -p "$HOME" "$XDG_RUNTIME_DIR"
chmod 700 "$XDG_RUNTIME_DIR"
%ctest
desktop-file-validate %{buildroot}%{_datadir}/applications/org.freecad.FreeCAD.desktop
appstream-util validate-relax --nonet %{buildroot}%{_metainfodir}/org.freecad.FreeCAD.metainfo.xml

%files
%license LICENSE src/3rdParty/libkdtree/COPYING %{SOURCE2}
%doc README.md
%{_bindir}/FreeCAD
%{_bindir}/FreeCADCmd
%{_bindir}/freecad
%{_libdir}/freecad/
%{python3_sitelib}/freecad/
%{_datadir}/applications/org.freecad.FreeCAD.desktop
%{_datadir}/icons/hicolor/*/apps/org.freecad.FreeCAD.*
%{_datadir}/icons/hicolor/scalable/mimetypes/application-x-extension-fcstd.svg
%{_metainfodir}/org.freecad.FreeCAD.metainfo.xml
%{_datadir}/mime/packages/org.freecad.FreeCAD.xml
%{_datadir}/pixmaps/freecad.svg
%{_datadir}/thumbnailers/FreeCAD.thumbnailer
%dir %{_datadir}/thumbnailers

%files data
%license LICENSE
%{_datadir}/freecad/

%changelog
* Mon Nov 02 2026 MCC45TR <75160848+MCC45TR@users.noreply.github.com> - 1:1.1.4-1
- Build the upstream stable release for Fedora Rawhide AArch64 in unstable.
- Adapt the Fedora review spec and retain its upstream build and test fixes.
