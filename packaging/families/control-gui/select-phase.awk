# SPDX-License-Identifier: MIT
# The pinned spec has one top-level shell group per original source component.
# Retain complete groups and package sections, never trim commands inside one.
function fail(message) {print message > "/dev/stderr"; failed=1; exit 1}
function finish_body() {
    if(body && (depth!=0 || group!=3)) fail("Unexpected published source phase structure: " body)
    body="";depth=0;group=0
}
function tail_package( i) {
    for(i=2;i<=NF;i++) if($i=="-n") return $(i+1)
    return "uke-control"
}
BEGIN {
    if(build_phase=="backend") {
        allowed["uke-control"]=allowed["uke-control-tui"]=allowed["uke-control-tests"]=1
        allowed["uke-control-client"]=allowed["uke-control-client-devel"]=1
        selected_group=1; root_package="uke-control"
    } else if(build_phase=="metas") {
        allowed["uke-desktop-metas"]=allowed["uke-kde-plasma-meta"]=1
        allowed["uke-cutefish-meta"]=allowed["uke-plasma-controls"]=allowed["uke-gnome-meta"]=1
        selected_group=2; root_package="uke-desktop-metas"
    } else fail("Unknown publication phase")
    keep=1
}
/^Name:/ {current=$2;keep=(current in allowed);if(keep)print;next}
/^BuildRequires:/ {
    if(!requirements_printed) {
        while((getline requirement < requirements)>0) print requirement
        close(requirements); requirements_printed=1
    }
    next
}
/^%package -n / {
    current=$3;keep=(current in allowed)
    if(keep) {
        if(current==root_package) print "Name: " root_package
        else print
    }
    next
}
/^%description/ {keep=(tail_package() in allowed);if(keep)print;next}
/^%(prep|build|install|check)([[:space:]]|$)/ {
    finish_body();body=substr($1,2);keep=1;print;next
}
/^%(pre|post|preun|postun|pretrans|posttrans|files|changelog)([[:space:]]|$)/ {
    finish_body()
    keep=($1=="%changelog" || (tail_package() in allowed))
    if(keep) print
    next
}
body {
    text=$0;gsub(/^[[:space:]]+|[[:space:]]+$/, "", text)
    if(text=="(") {if(depth==0)group++;depth++;if(group==selected_group)print;next}
    if(text==")") {if(depth<=0)fail("Unbalanced source group");if(group==selected_group)print;depth--;next}
    if(depth==0 || group==selected_group) {
        if(build_phase=="metas" && body=="prep" && $0=="%setup -q") print "%setup -q -n uke-control-2.8.1"
        else print
    }
    next
}
keep {print}
END {if(!failed)finish_body()}
