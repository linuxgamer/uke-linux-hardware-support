// SPDX-License-Identifier: MIT
#define main resize_helper_main
#include "../src/resize-system.cpp"
#undef main
#include <cassert>
#include <functional>

using namespace uke::resize;
static void refuse(const std::function<void()>& action) {
    bool rejected=false;try{action();}catch(const std::exception&){rejected=true;}assert(rejected);
}
static void put(const fs::path& path,const std::string& value="fixture") {
    fs::create_directories(path.parent_path());std::ofstream(path)<<value;
}
int main(int argc,char** argv) {
    std::ostringstream prompts;auto* output=std::cout.rdbuf(prompts.rdbuf());
    for(const auto* answer:{"no\n","\n","y\n","YES\n","yes please\n",""}) {
        std::istringstream input(answer);assert(!confirm_reboot(input,true));
    }
    std::istringstream approved("yes\n");assert(confirm_reboot(approved,true));
    std::istringstream piped("yes\n");refuse([&]{confirm_reboot(piped,false);});assert(piped.peek()=='y');
    std::cout.rdbuf(output);assert(prompts.str().find("reboot now?")!=std::string::npos);
    struct stat node{};node.st_mode=S_IFCHR|0600;node.st_uid=0;node.st_nlink=1;
    for(const auto& item:std::map<std::string,dev_t>{{"dev/null",makedev(1,3)},{"dev/random",makedev(1,8)},{"dev/urandom",makedev(1,9)},{"dev/kmsg",makedev(1,11)},{"dev/console",makedev(5,1)}}) {
        node.st_rdev=item.second;assert(dracut_device(item.first,node));
    }
    node.st_rdev=makedev(1,3);assert(!dracut_device("dev/sda",node));assert(!dracut_device("other/null",node));
    node.st_rdev=makedev(8,0);assert(!dracut_device("dev/null",node));node.st_rdev=makedev(1,3);
    node.st_mode=S_IFBLK|0600;assert(!dracut_device("dev/null",node));node.st_mode=S_IFCHR|0600;
    node.st_uid=1000;assert(!dracut_device("dev/null",node));node.st_uid=0;
    node.st_nlink=2;assert(!dracut_device("dev/null",node));
    const Request request{"8:42","00112233445566778899aabbccddeeff","01234567-89ab-cdef-0123-456789abcdef",1024ULL*1024*1024,512ULL*1024*1024,100};
    const auto text=encode(request);const auto decoded=decode(text);assert(encode(decoded)==text);
    request_matches(decoded,request,700);
    refuse([&]{request_matches(decoded,request,701);});
    refuse([&]{request_matches(decoded,request,99);});
    for(const auto& changed:{"bytes","before","prepared"})refuse([&]{decode(text+changed+"=1\n");});
    refuse([&]{decode(text+"extra=1\n");});
    refuse([&]{decode("format=1\n");});
    refuse([&]{number("18446744073709551616");});
    for(const auto* value:{"-1","+1","1;id","1\n",""})refuse([&]{number(value);});
    auto changed=request;changed.uuid[0]='f';refuse([&]{request_matches(changed,request,100);});
    changed=request;changed.device="8:41";refuse([&]{request_matches(changed,request,100);});
    changed=request;changed.bytes+=4096;refuse([&]{request_matches(changed,request,100);});
    changed=request;changed.boot[0]='f';refuse([&]{request_matches(changed,request,100);});
    changed=request;changed.before+=4096;refuse([&]{request_matches(changed,request,100);});
    assert(compatible(std::string("xiaomi,uke\0qcom,sm7675\0",23)));
    assert(!compatible(std::string("xxiaomi,uke\0qcom,sm7675\0",24)));
    assert(!compatible("xiaomi,uke"));
    const std::string mount="20 1 8:42 / / rw,relatime - f2fs /dev/sda42 rw\n";
    root_device(mount,"8:42");unmounted("20 1 0:1 / / rw - tmpfs tmpfs rw\n","8:42");
    refuse([&]{unmounted(mount,"8:42");});
    refuse([&]{root_device(mount,"8:43");});
    refuse([&]{root_device(mount+mount,"8:42");});
    refuse([&]{root_device("20 1 8:42 / / ro - f2fs /dev/sda42 ro\n","8:42");});
    refuse([&]{root_device("20 1 8:42 /subvol / rw - f2fs /dev/sda42 rw\n","8:42");});
    refuse([&]{root_device("20 1 8:42 / / rw - ext4 /dev/sda42 rw\n","8:42");});
    refuse([&]{unmounted("broken\n","8:42");});
    char pattern[]="/tmp/uke-resize-fixture-XXXXXX";const auto made=mkdtemp(pattern);assert(made);const fs::path root=made;
    try {
        save(root/"private",text);struct stat st{};assert(stat((root/"private").c_str(),&st)==0&&(st.st_mode&0777)==0600);
        if(geteuid()==0)assert(private_request(root/"private")==text);else refuse([&]{private_request(root/"private");});
        assert(chmod((root/"private").c_str(),0644)==0);refuse([&]{private_request(root/"private");});assert(chmod((root/"private").c_str(),0600)==0);
        refuse([&]{save(root/"private","replace");});assert(read(root/"private")==text);
        fs::create_symlink("private",root/"link");refuse([&]{save(root/"link","replace");});
        refuse([&]{private_request(root/"link");});
        fs::create_hard_link(root/"private",root/"hard-link");refuse([&]{private_request(root/"private");});fs::remove(root/"hard-link");
        const auto supplies=root/"supplies";put(supplies/"battery/type","Battery\n");put(supplies/"battery/capacity","50\n");power(supplies);
        put(supplies/"battery/capacity","49\n");refuse([&]{power(supplies);});
        put(supplies/"usb/online","1\n");put(supplies/"battery/capacity","30\n");power(supplies);
        put(supplies/"battery/capacity","29\n");refuse([&]{power(supplies);});
        put(supplies/"battery/capacity","101\n");refuse([&]{power(supplies);});
        put(supplies/"battery/capacity","100\n");put(supplies/"battery2/type","Battery\n");put(supplies/"battery2/capacity","100\n");refuse([&]{power(supplies);});
        for(const auto* name:{"shutdown","bin/sh","usr/libexec/uke-control/resize-system","usr/sbin/fsck.f2fs","usr/sbin/resize.f2fs"}){put(root/"tree"/name);assert(chmod((root/"tree"/name).c_str(),0755)==0);}
        put(root/"tree/usr/lib/initrd-release");
        audit(root/"tree");put(root/"tree/usr/bin/python3");refuse([&]{audit(root/"tree");});fs::remove(root/"tree/usr/bin/python3");
        assert(mkfifo((root/"tree/fifo").c_str(),0600)==0);refuse([&]{audit(root/"tree");});fs::remove(root/"tree/fifo");
        put(root/"tree/etc/shadow");refuse([&]{audit(root/"tree");});fs::remove(root/"tree/etc/shadow");
        fs::create_directories(root/"stage/tree");put(root/"target/log","preserved");put(root/"stage/tree/shutdown","prepared");
        activate(root/"stage",root/"target");assert(read(root/"target/shutdown")=="prepared");assert(read(root/"stage/previous/log")=="preserved");
        fs::create_directories(root/"next/tree");refuse([&]{activate(root/"next",root/"target");});assert(read(root/"target/shutdown")=="prepared");
        put(root/"rollback-target/log","rollback");fs::create_directories(root/"rollback-stage");
        refuse([&]{activate(root/"rollback-stage",root/"rollback-target");});assert(read(root/"rollback-target/log")=="rollback");
        fs::remove_all(root);
    } catch(...) {fs::remove_all(root);throw;}
    if(argc==3&&std::string(argv[1])=="--audit-fixture") {
        audit(fs::canonical(argv[2]));std::cout<<"Existing dracut payload audit passed\n";
    } else if(argc==3&&std::string(argv[1])=="--grow-fixture") {
        const auto image=fs::canonical(argv[2]);struct stat st{};assert(stat(image.c_str(),&st)==0&&S_ISREG(st.st_mode));
        Fd fd(open(image.c_str(),O_RDWR|O_CLOEXEC|O_NOFOLLOW));expand(fd.value,image.string(),st.st_size);
    } else if(argc==2) {
        const auto stage=fs::absolute(argv[1]);assert(!fs::exists(stage));fs::create_directories(stage);
        build_environment(stage,request);assert(encode(decode(read(stage/"tree/etc/uke-resize-system.request")))==text);
        std::cout<<"Actual Fedora dracut shutdown environment built and payload audit passed\n";
    } else assert(argc==1);
    std::cout<<"Resize identity, freshness, mount, private-file, payload, no-clobber and activation rollback tests passed\n";
}
