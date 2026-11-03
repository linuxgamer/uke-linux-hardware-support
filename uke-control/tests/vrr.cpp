// SPDX-License-Identifier: MIT
#include "../src/vrr.cpp"
#include <cassert>
#include <fstream>
#include <iostream>
using namespace uke::vrr;
template<class Call> static void refused(Call call) {
    bool caught=false;try{call();}catch(const std::exception&){caught=true;}assert(caught);
}
int main() {
    for(const char* value:{"30","48","60","90","120","143"})assert(minimum(value)>=30);
    for(const char* value:{"1","0","29","144","-30","+30"," 90","90 ","90\n","90;id","4294967296",""})
        refused([&]{minimum(value);});
    assert(policy(30)=="30\n");
    Driver current{90,144,{}};validate(30,current);validate(120,current);validate(143,current);
    current.upper=120;refused([&]{validate(120,current);});validate(90,current);
    current.reason="unsupported";refused([&]{validate(90,current);});
    current.reason.clear();current.loaded.reset();refused([&]{validate(90,current);});
    char pattern[]="/tmp/uke-vrr-policy-XXXXXX";const auto name=mkdtemp(pattern);assert(name);
    const fs::path directory=name,file=directory/policy_name;
    try {
        Fd parent{open(name,O_DIRECTORY|O_RDONLY|O_CLOEXEC)};
        assert(trusted(parent.value,true)==(geteuid()==0));
        if(geteuid()==0) {
            assert(!configured(directory));
            auto put=[&](const std::string& value){std::ofstream out(file,std::ios::binary);out<<value;out.close();assert(out.good());};
            put(policy(30));assert(configured(directory)==30);
            put(policy(143));assert(configured(directory)==143);
            for(const auto& invalid:{std::string(""),std::string(97,'x'),std::string("1\n"),std::string("30"),
                    policy(90)+policy(30),std::string("30 extra=1\n"),std::string("options msm o82_vrr_test_min_hz=30\n")}) {
                put(invalid);refused([&]{configured(directory);});
            }
            put(policy(90));assert(chmod(file.c_str(),0666)==0);refused([&]{configured(directory);});
            assert(chmod(file.c_str(),0600)==0);
            assert(link(file.c_str(),(directory/"hardlink").c_str())==0);refused([&]{configured(directory);});
            assert(unlink((directory/"hardlink").c_str())==0);
            assert(chown(file.c_str(),1,0)==0);refused([&]{configured(directory);});assert(chown(file.c_str(),0,0)==0);
            assert(unlink(file.c_str())==0);assert(symlink("hardlink",file.c_str())==0);refused([&]{configured(directory);});
            assert(unlink(file.c_str())==0);assert(mkfifo(file.c_str(),0600)==0);refused([&]{configured(directory);});
            assert(unlink(file.c_str())==0);put(policy(90));
            assert(chmod(name,0777)==0);refused([&]{configured(directory);});assert(chmod(name,0700)==0);
        } else refused([&]{configured(directory);});
        fs::remove_all(directory);
    } catch(...) {fs::remove_all(directory);throw;}
    std::cout<<"VRR minimum bounds, active-mode validation, strict policy parsing, ownership, links, FIFO and directory admission: PASS\n";
}
