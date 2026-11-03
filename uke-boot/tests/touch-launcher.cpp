// SPDX-License-Identifier: MIT
#define main touch_launcher_main
#include "../src/touch-launcher.cpp"
#undef main
#include <cassert>
static void put(const fs::path &path,const std::string &value) {
    fs::create_directories(path.parent_path());std::ofstream(path,std::ios::binary)<<value;
}
template<class Call>static void refuses(Call call) {
    bool refused=false;try{call();}catch(const std::exception&){refused=true;}assert(refused);
}
int main() {
    char directory[]="/tmp/uke-touch-launcher-XXXXXX";
    const auto created=mkdtemp(directory);assert(created);const fs::path root=created;
    const auto board=root/"sys/firmware/devicetree/base/compatible";
    const auto leaf=root/"sys/bus/spi/devices/spi0.0/of_node/compatible";
    const auto legacy=root/"usr/libexec/senemos-uke/uke-touch-thp";
    const auto capture=root/"usr/lib/modules/7.2.12-18/uke/uke-touch-capture";
    const auto token=[](const std::string &value){return value+'\0';};
    try {
        refuses([&]{touch_client(root,"7.2.12-18");});
        put(board,token("xiaomi,uke")+token("qcom,sm7675"));
        put(leaf,token("novatek,NVT-ts-spi"));
        refuses([&]{touch_client(root,"7.2.12-18");});
        put(legacy,"fixture\n");assert(chmod(legacy.c_str(),0755)==0);
        if(geteuid()==0)assert(touch_client(root,"7.2.12-14")==legacy);
        else refuses([&]{touch_client(root,"7.2.12-14");});
        put(leaf,token("novatek,nt36532-uke-thp"));
        refuses([&]{touch_client(root,"7.2.12-18");}); // Never fall back to direct SPI.
        put(capture,"fixture\n");assert(chmod(capture.c_str(),0755)==0);
        if(geteuid()==0)assert(touch_client(root,"7.2.12-18")==capture);
        assert(chmod(capture.c_str(),0777)==0);refuses([&]{touch_client(root,"7.2.12-18");});
        fs::remove(capture);fs::create_symlink(legacy,capture);
        refuses([&]{touch_client(root,"7.2.12-18");});
        for(const auto &release:{"","../legacy","7.2.12/18","unmatched"})
            refuses([&]{touch_client(root,release);});
        put(root/"sys/bus/spi/devices/spi1.0/of_node/compatible",token("novatek,NVT-ts-spi"));
        refuses([&]{touch_client(root,"7.2.12-18");});
        fs::remove_all(root/"sys/bus/spi/devices/spi1.0");
        put(leaf,"novatek,nt36532-uke-thp");refuses([&]{touch_client(root,"7.2.12-18");});
        fs::remove_all(root);
    } catch(...) {fs::remove_all(root);throw;}
    std::cout<<"Running-kernel touch selection, legacy rollback, missing-client/owner/symlink, ambiguity and path refusal: PASS\n";
}
