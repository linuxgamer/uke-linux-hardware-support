// SPDX-License-Identifier: MIT
// Resolve published packages in an isolated RPM database; never run a transaction.
#include <libdnf5/base/base.hpp>
#include <libdnf5/base/goal.hpp>
#include <libdnf5/repo/repo_sack.hpp>
#include <libdnf5/rpm/package_query.hpp>
#include <libdnf5/transaction/transaction_item_action.hpp>
#include <filesystem>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;
static void need(bool value, const std::string &message) {
    if (!value) throw std::runtime_error(message);
}

int main(int argc, char **argv) try {
    need(argc == 4, "Usage: solver FIXTURE ROOT MODE");
    const fs::path fixture = fs::canonical(argv[1]), root = fs::canonical(argv[2]);
    need(root.string().starts_with(fixture.string() + "/roots/"), "Require an isolated fixture root");
    const std::string mode = argv[3];
    need(mode == "upgrade-noarch" || mode == "upgrade-native" || mode == "fresh", "Unknown NFC check mode");
    const bool upgrading = mode != "fresh";
    const std::string baseline_version = mode == "upgrade-noarch" ? "1.0.0" : "1.1.0";
    const std::string baseline_arch = mode == "upgrade-noarch" ? "noarch" : "aarch64";
    libdnf5::Base base;
    auto &config = base.get_config();
    config.get_installroot_option().set(root.string());
    config.get_use_host_config_option().set(false);
    config.get_plugins_option().set(false);
    config.get_config_file_path_option().set((fixture / "empty.conf").string());
    config.get_reposdir_option().set(std::vector<std::string>{});
    config.get_cachedir_option().set((fixture / "cache").string());
    config.get_install_weak_deps_option().set(true);
    config.get_best_option().set(true);
    config.get_installonly_limit_option().set(0);
    config.get_optional_metadata_types_option().set(std::set<std::string>{"filelists"});
    base.get_vars()->set("releasever", "46");
    base.get_vars()->set("basearch", "aarch64");
    base.setup();
    for (const auto &id : {std::string("uke"), std::string("rawhide")}) {
        auto repo = base.get_repo_sack()->create_repo(id);
        auto &options = repo->get_config();
        if (id == "uke") options.get_baseurl_option().set(std::vector<std::string>{
            "https://download.copr.fedorainfracloud.org/results/mcc45tr/uke-linux-unstable/fedora-rawhide-aarch64/"});
        else options.get_metalink_option().set("https://mirrors.fedoraproject.org/metalink?repo=rawhide&arch=aarch64");
        options.get_skip_if_unavailable_option().set(false);
        options.get_metadata_expire_option().set(0);
        options.get_pkg_gpgcheck_option().set(true);
    }
    base.get_repo_sack()->load_repos();
    libdnf5::rpm::PackageQuery installed(base);
    installed.filter_installed();
    unsigned baseline = 0;
    for (const auto &package : installed) if (package.get_name() == "uke-nfc-ir") {
        need(package.get_version() == baseline_version && package.get_release() == "1.fc46" &&
             package.get_arch() == baseline_arch, "Wrong installed NFC baseline");
        ++baseline;
    }
    need(baseline == (upgrading ? 1U : 0U), "Unexpected NFC baseline count");
    libdnf5::Goal goal(base);
    if (upgrading) goal.add_rpm_upgrade();
    else goal.add_rpm_install("uke-nfc-ir");
    auto transaction = goal.resolve();
    if (transaction.get_problems() != libdnf5::GoalProblem::NO_PROBLEM) {
        for (const auto &message : transaction.get_resolve_logs_as_strings()) std::cerr << message << '\n';
        throw std::runtime_error("Published NFC dependency resolution failed");
    }
    unsigned clients = 0, old_outbound = 0, providers = 0, inbound = 0;
    for (const auto &item : transaction.get_transaction_packages()) {
        const auto package = item.get_package();
        const auto action = item.get_action();
        if (libdnf5::transaction::transaction_item_action_is_outbound(action)) {
            need(package.get_name() == "uke-nfc-ir" && package.get_version() == baseline_version &&
                 package.get_release() == "1.fc46" && package.get_arch() == baseline_arch,
                 "Unexpected outbound package");
            ++old_outbound;
        }
        if (!libdnf5::transaction::transaction_item_action_is_inbound(action)) continue;
        ++inbound;
        if (package.get_name() == "uke-nfc-ir") {
            need(package.get_version() == "1.1.0" && package.get_release() == "2.fc46" &&
                 package.get_arch() == "aarch64" && package.get_repo_id() == "uke",
                 "Wrong NFC client selection");
            ++clients;
        }
        for (const auto &provide : package.get_provides()) {
            if (std::string_view(provide.get_name()) != "uke-kernel(nfc-ir-v2)") continue;
            need(std::string_view(provide.get_version()) == "1" &&
                 package.get_name() == "senemos-uke-linux-kernel-mainline-core" &&
                 package.get_arch() == "aarch64" && package.get_repo_id() == "uke",
                 "Wrong NFC kernel provider");
            ++providers;
        }
    }
    need(clients == 1 && providers == 1, "Missing unique native client or kernel-v2 provider");
    need(old_outbound == baseline, "Installed client was not replaced exactly once");
    std::cout << "UKE_NFC_DELIVERY mode=" << mode << " client=1.1.0-2.fc46.aarch64"
              << " baseline_outbound=" << old_outbound << " kernel_v2_providers=" << providers
              << " inbound=" << inbound << " weak_dependencies=true payload_transaction_executed=false hardware_tested=false\n";
    return 0;
} catch (const std::exception &error) {
    std::cerr << error.what() << '\n'; return 1;
}
