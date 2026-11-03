// SPDX-License-Identifier: MIT
#include "lights.hpp"
#include <cassert>
#include <fstream>
#include <functional>
#include <iostream>
#include <unistd.h>
namespace fs=std::filesystem;
using namespace uke::lights;
static void put(const fs::path& path,const std::string& text) {fs::create_directories(path.parent_path());std::ofstream(path,std::ios::binary)<<text;}
static std::string cell(unsigned n){std::string s(4,'\0');for(unsigned i=0;i<4;++i)s[i]=char(n>>(24-8*i));return s;}
static void refuses(const std::function<void()>& f){bool failed=false;try{f();}catch(...){failed=true;}assert(failed);}
int main() {
    char name[]="/tmp/uke-lights-XXXXXX";const fs::path stage=mkdtemp(name),sys=stage/"sys";
    const auto base=sys/"firmware/devicetree/base",pmic=base/"soc/qcom,spmi@c42d000/qcom,pmxr2230@1";
    // Match the exact existing board identity, not a substring.
    put(base/"compatible",std::string("xiaomi,uke\0",11)+std::string("qcom,sm7675\0",12));
    for(bool rgb:{false,true}) {
        const auto label=rgb?"uke:rgb:status":"uke:white:torch";
        const auto controller=pmic/(rgb?"pwm":"led-controller@ee00");
        const auto node=controller/(rgb?"multi-led":"led-0");
        const auto device=sys/"devices/platform"/(rgb?"pwm":"flash");
        const auto led=device/"leds"/label;
        fs::create_directories(led);fs::create_directories(node);fs::create_directories(sys/"class/leds");
        fs::create_directory_symlink(led,sys/"class/leds"/label);
        fs::create_directory_symlink(device,led/"device");
        fs::create_directory_symlink(controller,device/"of_node");
        put(node/"label",std::string(label)+'\0');
        const auto driver=sys/"bus/platform/drivers"/(rgb?"qcom-spmi-lpg":"leds-qcom-flash");
        fs::create_directories(driver);fs::create_directory_symlink(driver,device/"driver");
        const std::string compatible=rgb?"qcom,pm8350c-pwm":"qcom,spmi-flash-led";
        put(controller/"compatible",compatible+'\0');
        put(led/"max_brightness",rgb?"511\n":"255\n");put(led/"brightness","0\n");
        put(led/"trigger","[none] timer\n");
        if(rgb) {
            for(unsigned i=1;i<=3;++i){auto child=node/("led@"+std::to_string(i));put(child/"reg",cell(i));put(child/"color",cell(i));}
            put(led/"multi_index","green blue red\n");
        } else {put(controller/"reg",cell(0xee00));put(node/"led-sources",cell(1)+cell(4));put(node/"led-max-microamp",cell(20000));}
        assert(!fs::exists(led/"of_node") && !discover(rgb,sys).path.empty());
        fs::create_directory(controller/"other-led");assert(discover(rgb,sys).path.empty());fs::remove(controller/"other-led");
        put(led/"trigger","none [timer]\n");assert(discover(rgb,sys).path.empty());put(led/"trigger","[none]\n");
        put(led/"max_brightness","1000\n");assert(discover(rgb,sys).path.empty());put(led/"max_brightness",rgb?"511":"255");
        if(!rgb) {
            for(unsigned current:{0U,20001U,500000U}){put(node/"led-max-microamp",cell(current));assert(discover(false,sys).path.empty());}
            put(node/"led-max-microamp",cell(20000));put(node/"led-sources",cell(1)+cell(2));assert(discover(false,sys).path.empty());
            put(node/"led-sources",cell(1)+cell(4));put(node/"flash-max-microamp",cell(100000));assert(discover(false,sys).path.empty());fs::remove(node/"flash-max-microamp");
        } else {put(node/"led@1/color",cell(2));assert(discover(true,sys).path.empty());put(node/"led@1/color",cell(1));}
        assert(!discover(rgb,sys).path.empty());
        put(node/"label",std::string("other:status\0",13));assert(discover(rgb,sys).path.empty());put(node/"label",std::string(label)+'\0');
        fs::remove(device/"of_node");fs::create_directory_symlink(stage,device/"of_node");assert(discover(rgb,sys).path.empty());
        fs::remove(device/"of_node");fs::create_directory_symlink(controller,device/"of_node");
    }
    assert(snapshot(sys).find("\"max_microamp\":20000")!=std::string::npos);
    put(base/"compatible",std::string("xiaomi,nabu\0qcom,sm7675\0",24));
    assert(discover(false,sys).path.empty() && discover(true,sys).path.empty());
    assert(intensities("green blue red",{255,0,128})=="0 256 511");
    assert(color("255")==255 && color("0")==0);
    for(const std::string value:{"-1","256","1;reboot","+1"," 1","1x",""})refuses([&]{(void)color(value);});
    for(const std::string order:{"red green red","red blue","red green white"})refuses([&]{(void)intensities(order,{1,2,3});});
    refuses([]{(void)intensities("red green blue",{256,0,0});});
    fs::remove_all(stage);
    std::cout<<"Light identity, current ceiling, no-strobe, mapping, trigger ownership, containment and RGB input checks passed; no device writes\n";
}
