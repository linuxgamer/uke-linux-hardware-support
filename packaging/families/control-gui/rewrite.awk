# SPDX-License-Identifier: MIT
# Read the exact published package identities, then alter only this GUI contract.
FNR == NR {
    version[$1]=$2; release[$1]=$3; newer[$1]=$4
    next
}
function finish_phase() {
    if(phase=="prep") {
        print "mkdir -p %{_builddir}/uke-control-frontend-contract"
        print "tar -xJf %{SOURCE1} -C %{_builddir}/uke-control-frontend-contract"
    } else if(phase=="build" && build_phase=="backend") {
        print "contract=%{_builddir}/uke-control-frontend-contract"
        print "mapfile -t client_libraries < <(find %{_builddir}/uke-control-2.8.1/work -type f -path '*/build/libuke-control-client.so.1')"
        print "test ${#client_libraries[@]} = 1"
        print "library=${client_libraries[0]}"
        print "cmp \"$contract/client.hpp\" \"${library%/build/libuke-control-client.so.1}/src/client.hpp\""
        print "g++ %{build_cxxflags} -std=c++20 -Wall -Wextra -I\"$contract\" \"$contract/frontend-request.cpp\" \"$library\" -o \"$contract/frontend-request\""
        print "printf '%s\\n' \"${library%/*}\" > \"$contract/library-directory\""
    } else if(phase=="install") {
        print "contract=%{_builddir}/uke-control-frontend-contract"
        if(build_phase=="backend") {
            print "install -Dm644 \"$contract/frontend.hpp\" %{buildroot}%{_includedir}/uke-control/frontend.hpp"
            print "install -Dm755 \"$contract/frontend-request\" %{buildroot}%{_libexecdir}/uke-control/frontend-request"
        } else {
            print "install -Dm644 \"$contract/gnome.json\" %{buildroot}%{_datadir}/senemos/uke/desktops/gnome.json"
        }
    } else if(phase=="check" && build_phase=="backend") {
        print "contract=%{_builddir}/uke-control-frontend-contract"
        print "export LD_LIBRARY_PATH=$(cat \"$contract/library-directory\")${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
        print "\"$contract/frontend-request\" --help > /dev/null"
        print "set +e"
        print "\"$contract/frontend-request\" status > \"$contract/captured-read-refusal\" 2>&1"
        print "status=$?"
        print "set -e"
        print "test $status = 64"
        print "grep -F 'read-only query interface' \"$contract/captured-read-refusal\""
    } else if(phase=="check" && build_phase=="metas") {
        # The retained metadata check compares the Core profile before pruning
        # that unselected package's data from this buildroot.
        print "rm -f %{buildroot}%{_datadir}/senemos/uke/core/profile.json"
        print "rmdir %{buildroot}%{_datadir}/senemos/uke/core"
    }
}
function gnome_preamble() {
    if(build_phase=="backend") {
    print ""
    print "%package -n uke-control-client"
    print "Version: " version["uke-control"]
    print "Release: " newer["uke-control"]
    print "Summary: UI-independent native Uke Control client library"
    print "License: MIT AND Apache-2.0"
    print "%description -n uke-control-client"
    print "Typed action catalog, validation and owned read-query client."
    print "The graphical applications require the matching secured backend separately."
    print ""
    print "%package -n uke-control-client-devel"
    print "Version: " version["uke-control"]
    print "Release: " newer["uke-control"]
    print "Summary: C++ SDK for the native Uke Control client"
    print "License: MIT AND Apache-2.0"
    print "Requires: uke-control-client%{?_isa} = " version["uke-control"] "-" newer["uke-control"]
    print "Provides: uke-control(frontend-sdk-v1) = 1"
    print "Provides: /usr/include/uke-control/frontend.hpp"
    print "%description -n uke-control-client-devel"
    print "Headers and pkg-config metadata for building unprivileged native frontends."
    return
    }
    print ""
    print "%package -n uke-gnome-meta"
    print "Version: 1.0.0"
    print "Release: " newer["uke-kde-plasma-meta"]
    print "Summary: Original Fedora GNOME applications and native Uke Control"
    print "License: MIT"
    print "BuildArch: noarch"
    print "Requires: (senemos-uke-linux-kernel-mainline >= 7.2.11-16 or senemos-uke-linux-kernel-mainline-singleboot >= 7.2.11-16)"
    print "Requires: uke-control-gtk >= 1.0.0-2"
    print "Requires: gnome-shell gnome-session gnome-control-center gnome-settings-daemon"
    print "Requires: gdm gnome-keyring nautilus xdg-desktop-portal-gnome"
    print "%description -n uke-gnome-meta"
    print "Original Fedora GNOME packages and GTK 4/libadwaita Uke Control."
    print "No display manager activation or device configuration is performed."
    print ""
}
/^Name:/ {current=$2}
/^%package -n / {current=$3}
/^Release:/ {
    if(!(current in newer)) {print "Unreviewed package identity" > "/dev/stderr"; exit 1}
    print "Release: " newer[current]; next
}
/^Source0:/ {
    print
    print "Source1: uke-control-frontend-contract.tar.xz"
    print "Source2: uke-control-published.spec"
    print "Source3: input.json"
    print "Source4: copr-public-key.asc"
    next
}
/^(Requires|Recommends|Suggests):/ {
    # Align strict internal dependencies with the new cohort's exact NEVRAs.
    if($3=="=") {
        key=$2; sub(/\([^)]*\)$/, "", key)
        if(key in newer) $4=version[key] "-" newer[key]
    }
}
/^Summary:/ {
    print
    if(current=="uke-control") {
        print "Provides: uke-control(frontend-v1) = 1"
        print "Provides: /usr/libexec/uke-control/frontend-request"
        print "Requires: uke-control-client%{?_isa} = " version["uke-control"] "-" newer["uke-control"]
    }
    if(current=="uke-kde-plasma-meta") print "Requires: uke-control-qt >= 1.0.0-2"
    next
}
/^%(prep|build|install|check)([[:space:]]|$)/ {
    finish_phase()
    if($1=="%prep") gnome_preamble()
    phase=substr($1,2)
}
/^%(pre|post|preun|postun|pretrans|posttrans|files|changelog)([[:space:]]|$)/ {
    finish_phase(); phase=""
}
/^%files -n uke-control$/ {root_files=1;print; print "%{_libexecdir}/uke-control/frontend-request"; next}
/^%files/ {root_files=0}
root_files && /^\/usr\/lib64\/libuke-control-client[.]so([.]1)?$/ {next}
root_files && /^\/usr\/lib64\/pkgconfig\/uke-control-client[.]pc$/ {next}
root_files && /^\/usr\/include\/uke-control\/\*[.]hpp$/ {next}
/^%changelog/ {skip_changelog=1; next}
!skip_changelog {print}
END {
    if(!skip_changelog) {print "Missing published changelog boundary" > "/dev/stderr"; exit 1}
    if(build_phase=="metas") {
    print "%files -n uke-gnome-meta"
    print "%license LICENSE"
    print "%{_datadir}/senemos/uke/desktops/gnome.json"
    } else {
    print ""
    print "%files -n uke-control-client"
    print "%license LICENSE"
    print "%{_libdir}/libuke-control-client.so.1"
    print ""
    print "%files -n uke-control-client-devel"
    print "%license LICENSE"
    print "%dir %{_includedir}/uke-control"
    print "%{_includedir}/uke-control/*.hpp"
    print "%{_libdir}/libuke-control-client.so"
    print "%{_libdir}/pkgconfig/uke-control-client.pc"
    }
    print ""
    print "%changelog"
    if(build_phase=="backend") print "* Mon Nov 02 2026 Uke contributors - 2.8.1-6"
    else print "* Mon Nov 02 2026 Uke contributors - 1.0.0-36"
    print "- Preserve the published backend and add the typed GUI client contract."
    print "- Require the corresponding native application in each desktop selection."
}
