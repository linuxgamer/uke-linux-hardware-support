// SPDX-License-Identifier: MIT
#include "../src/diagnostic.cpp"
#include <cassert>
#include <fstream>
#include <sstream>

static unsigned calls=0;
uke::Result uke::execute(const std::vector<std::string>& args,unsigned seconds,std::size_t limit) {
    using namespace std::string_literals;
    assert(seconds==20&&limit==8*1024*1024);
    assert(!args.empty()&&args[0].starts_with("/usr/"));
    assert(args[0]!="/usr/bin/sh"&&args[0]!="/usr/bin/bash");
    if(args[0]=="/usr/bin/od") {
        assert(args.size()==5&&args[1]=="-An"&&args[2]=="-v"&&args[3]=="-tx1");
        assert(args[4].starts_with("/sys/module/")&&args[4].ends_with("/notes/.note.gnu.build-id"));
    }
    ++calls;
    if(args[0]=="/usr/bin/dnf5") {
        assert(args[1]=="--cacheonly"||args[1]=="--version");
        return {127,"fixture missing optional command\n"};
    }
    if(args[0]=="/usr/bin/uname")return {124,"fixture time/output bound\n"};
    return {0,"fixture\x1b[31m\0binary\n"s};
}
static std::string read_file(const std::string& name) {
    std::ifstream file(name);std::ostringstream out;out<<file.rdbuf();return out.str();
}
int main() {
    using namespace uke::diagnostic;
    const auto previous=std::filesystem::current_path();
    char pattern[]="/tmp/uke-diagnostic-fixture-XXXXXX";
    const auto created=mkdtemp(pattern);assert(created);
    const std::filesystem::path root=created;
    try {
        std::filesystem::create_directory(root/"working directory");
        std::filesystem::current_path(root/"working directory");
        const auto catalog=jobs("all");assert(catalog.size()>150);
        const auto display=jobs("display");
        assert(std::count_if(display.begin(),display.end(),[](const auto& job){return job.args[0]=="/usr/bin/od";})==3);
        for(const auto& topic:topics) {
            const auto selected=jobs(topic);assert(!selected.empty());
            for(const auto& job:selected) {
                assert(job.topic==topic);
                assert(job.args!=std::vector<std::string>({"/usr/bin/uke-control","desktop","status"}));
                if(topic!="hardware")assert(job.args!=std::vector<std::string>({"/usr/bin/uke-control","hardware","status"}));
            }
            assert(std::count_if(catalog.begin(),catalog.end(),[&](const auto& job){return job.topic==topic;})==long(selected.size()));
        }
        for(const auto& job:catalog) {
            for(const auto& arg:job.args) {
                assert(arg!="--show-secrets"&&arg!="--rebuilddb"&&arg!="suspend"&&arg!="pair"&&arg!="scan"&&arg!="install"&&arg!="upgrade");
                assert(arg.find("/persist")==std::string::npos);
            }
        }
        const auto old_umask=umask(0777);
        const auto name=collect("wifi");umask(old_umask);
        assert(name=="uke-diagnostic-wifi.log");
        struct stat st{};assert(stat(name.c_str(),&st)==0&&(st.st_mode&0777)==0600);
        const auto text=read_file(name);
        assert(text.find("COLLECTION COMPLETE")!=std::string::npos);
        assert(text.find("[exit=124")!=std::string::npos);
        assert(text.find("\\x1b[31m\\x00binary")!=std::string::npos);
        assert(text.find("\x1b")==std::string::npos);
        assert(text.find("===== wifi /")!=std::string::npos);
        assert(text.find("===== bluetooth /")==std::string::npos);
        assert(text.find("===== packages /")==std::string::npos);
        const unsigned before=calls;
        bool refused=false;try{collect("wifi");}catch(const std::runtime_error&){refused=true;}
        assert(refused&&calls==before&&read_file(name)==text);
        refused=false;try{collect("../../escape;$(id)");}catch(const std::runtime_error&){refused=true;}
        assert(refused&&calls==before);
        std::ofstream("target")<<"keep";
        std::filesystem::create_symlink("target","uke-diagnostic-bluetooth.log");
        refused=false;try{collect("bluetooth");}catch(const std::runtime_error&){refused=true;}
        assert(refused&&read_file("target")=="keep"&&calls==before);
        std::filesystem::create_directory("uke-diagnostic-sleep.log");
        refused=false;try{collect("sleep");}catch(const std::runtime_error&){refused=true;}
        assert(refused&&calls==before);
        const auto all=read_file(collect("all"));
        for(const auto& topic:topics)assert(all.find("===== "+topic+" /")!=std::string::npos);
        assert(all.find("[exit=127")!=std::string::npos);
        assert(all.find("unavailable: no matching files")!=std::string::npos);
        assert(all.find("Overall limit reached: no")!=std::string::npos);
        std::filesystem::current_path(previous);std::filesystem::remove_all(root);
        std::cout<<"Diagnostic topic coverage, private output, receipts, no-clobber and path refusal passed\n";
    } catch(...) {
        std::filesystem::current_path(previous);std::filesystem::remove_all(root);throw;
    }
}
