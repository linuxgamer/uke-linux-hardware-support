// SPDX-License-Identifier: MIT
// Resolve actual repository packages. Never execute an RPM transaction.
#include <libdnf5/base/base.hpp>
#include <libdnf5/base/goal.hpp>
#include <libdnf5/repo/repo_sack.hpp>
#include <libdnf5/rpm/package_query.hpp>
#include <libdnf5/transaction/transaction_item_action.hpp>
#include <filesystem>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
namespace fs=std::filesystem;
static void need(bool condition,const std::string& message) {
    if(!condition)throw std::runtime_error(message);
}
int main(int argc,char** argv) try {
    need(argc==4,"Usage: solver FIXTURE ROOT MODE");
    const fs::path fixture=fs::canonical(argv[1]),root=fs::canonical(argv[2]);
    need(root.string().starts_with(fixture.string()+"/roots/"),"Require an isolated fixture root");
    const std::string mode=argv[3];
    need(mode=="upgrade-kde"||mode=="fresh-kde"||mode=="fresh-gnome","Unknown check mode");
    const bool gnome=mode=="fresh-gnome";
    libdnf5::Base base;auto& config=base.get_config();
    config.get_installroot_option().set(root.string());
    config.get_use_host_config_option().set(false);
    config.get_plugins_option().set(false);
    config.get_config_file_path_option().set((fixture/"empty.conf").string());
    config.get_reposdir_option().set(std::vector<std::string>{});
    config.get_cachedir_option().set((fixture/"cache").string());
    config.get_install_weak_deps_option().set(true);
    config.get_best_option().set(true);
    config.get_installonly_limit_option().set(0);
    // Core/boot packages require paths outside primary.xml's limited file set.
    // A library caller must request the same filelists used for those lookups.
    config.get_optional_metadata_types_option().set(std::vector<std::string>{"filelists"});
    base.get_vars()->set("releasever","46");
    base.get_vars()->set("basearch","aarch64");
    base.setup();
    for(const auto& id:{std::string("uke"),std::string("rawhide")}) {
        auto repo=base.get_repo_sack()->create_repo(id);auto& options=repo->get_config();
        if(id=="uke")options.get_baseurl_option().set(std::vector<std::string>{
            "https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/fedora-rawhide-aarch64/"});
        else options.get_metalink_option().set("https://mirrors.fedoraproject.org/metalink?repo=rawhide&arch=aarch64");
        options.get_skip_if_unavailable_option().set(false);
        options.get_metadata_expire_option().set(3600);
        options.get_pkg_gpgcheck_option().set(true);
    }
    base.get_repo_sack()->load_repos();
    libdnf5::rpm::PackageQuery growfs(base);
    growfs.filter_available();
    growfs.filter_file(std::vector<std::string>{"/usr/lib/systemd/systemd-growfs"});
    need(!growfs.empty(),"Repository filelists do not contain systemd-growfs");
    if(mode=="upgrade-kde") {
        libdnf5::rpm::PackageQuery installed(base);installed.filter_installed();
        unsigned metas=0;
        for(const auto& package:installed)if(package.get_name()=="uke-kde-plasma-meta") {
            need(package.get_version()=="1.0.0"&&package.get_release()=="35.1.fc46","Wrong installed KDE baseline");
            ++metas;
        }
        need(metas==1,"Missing installed KDE baseline");
    }
    libdnf5::Goal goal(base);
    if(mode=="upgrade-kde")goal.add_rpm_upgrade();
    else goal.add_rpm_install(gnome?"uke-gnome-meta":"uke-kde-plasma-meta");
    auto transaction=goal.resolve();
    if(transaction.get_problems()!=libdnf5::GoalProblem::NO_PROBLEM) {
        for(const auto& message:transaction.get_resolve_logs_as_strings())std::cerr<<message<<'\n';
        throw std::runtime_error("Published GUI dependency resolution failed");
    }
    std::map<std::string,std::string> inbound;
    unsigned count=0;
    for(const auto& item:transaction.get_transaction_packages()) {
        need(item.get_action()!=libdnf5::transaction::TransactionItemAction::REMOVE,
             "Unexpected package removal: "+item.get_package().get_name());
        if(libdnf5::transaction::transaction_item_action_is_inbound(item.get_action())) {
            const auto package=item.get_package();++count;
            inbound.emplace(package.get_name(),package.get_version()+"-"+package.get_release());
            if(package.get_name().starts_with("uke-control-gui-"))
                need(package.get_repo_id()=="uke","GUI escaped the unstable repository");
        }
    }
    const std::string meta=gnome?"uke-gnome-meta":"uke-kde-plasma-meta";
    const std::string gui=gnome?"uke-control-gui-gtk":"uke-control-gui-qt";
    need(inbound.contains(meta)&&inbound.at(meta)=="1.0.0-37.fc46","Wrong desktop meta transaction");
    need(inbound.contains(gui)&&inbound.at(gui)=="1.0.0-3.fc46","Matching GUI was not selected");
    need(inbound.contains("uke-control")&&inbound.at("uke-control")=="2.8.1-7.fc46","Matched backend was not selected");
    need(!inbound.contains(gnome?"uke-control-gui-qt":"uke-control-gui-gtk"),"Unselected GUI was installed");
    need(!inbound.contains(gnome?"uke-kde-plasma-meta":"uke-gnome-meta"),"Unselected desktop was installed");
    std::cout<<"UKE_GUI_DELIVERY mode="<<mode<<" meta="<<meta<<" gui="<<gui
             <<" inbound="<<count<<" weak_dependencies=true transaction_executed=false hardware_tested=false\n";
    return 0;
} catch(const std::exception& error) {
    std::cerr<<error.what()<<'\n';return 1;
}
