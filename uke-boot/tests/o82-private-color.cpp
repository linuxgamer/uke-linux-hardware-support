// SPDX-License-Identifier: MIT
// Reuse the original fixture translation unit without rerunning its entry.
// The original package check independently runs all retained updater controls.
#include "kernel-update.cpp"
namespace {
unsigned color_checks = 0;
void color_check(bool value) {
    ++color_checks;
    if (!value) throw std::runtime_error("private-color updater check " + std::to_string(color_checks));
}
void color_contract() {
    const std::string next = "7.3.1-12102-unstable", running = "7.3.1-11003-unstable";
    for (bool marked : {false, true}) for (bool dry : {false, true})
        for (bool fail_prepare : {false, true}) for (bool fail_stage : {false, true}) {
            std::map<std::string, std::string> manifest{{"kernel_release", next}};
            if (marked) manifest["o82_private_color"] = "5";
            std::vector<std::string> calls;
            bool refused = false;
            try {
                uke_color::with_prepared(manifest, running, dry,
                    [&](const std::string &release) {
                        color_check(release == next && !dry && marked);
                        calls.push_back("prepare");
                        if (fail_prepare) throw std::runtime_error("injected private preparation failure");
                    }, [&] {
                        calls.push_back("stage");
                        if (fail_stage) throw std::runtime_error("injected stage failure");
                    });
            } catch (const std::exception &) { refused = true; }
            std::vector<std::string> expected;
            if (marked && !dry) expected.push_back("prepare");
            if (!(marked && !dry && fail_prepare)) expected.push_back("stage");
            color_check(calls == expected);
            color_check(refused == ((marked && !dry && fail_prepare) || fail_stage));
        }
    const auto no_calls = [&](const std::map<std::string, std::string> &manifest, const std::string &current) {
        unsigned called = 0;
        bool refused = false;
        try { uke_color::with_prepared(manifest, current, false, [&](const auto &) { ++called; }, [&] { ++called; }); }
        catch (const std::exception &) { refused = true; }
        color_check(refused && !called);
    };
    for (const char *marker : {"", "4", "6", "05", "5 ", "5\n", "true"})
        no_calls({{"kernel_release", next}, {"o82_private_color", marker}}, running);
    no_calls({{"o82_private_color", "5"}}, running);
    no_calls({{"kernel_release", next}, {"o82_private_color", "5"}}, next);
    for (const char *release : {"7.3.1-12102", "7.2.9-12102-unstable", "7.3.1-0-unstable", "7.3.1-01-unstable",
            "7.3.1-+1-unstable", "7.3.1-4294967296-unstable", "7.3.1-1/2-unstable", "7.3.1--unstable"})
        no_calls({{"kernel_release", release}, {"o82_private_color", "5"}}, running);
    color_check(uke_color::required({{"kernel_release", "7.3.1-1-unstable"}, {"o82_private_color", "5"}}));
    color_check(!uke_color::required({{"kernel_release", "7.2.9-legacy"}}));
    const auto parsed = properties("format=1\nkernel_release=" + next + "\no82_private_color=5\n");
    color_check(uke_color::required(parsed));
    reject([&] { (void)properties("o82_private_color=5\no82_private_color=4\n"); });
}
void capture_contract() {
    color_check(uke_video::capture({"/usr/bin/printf", "ok"}, 2) == Bytes({'o', 'k'}));
    color_check(uke_video::capture({"/usr/bin/printf", "ok"}, 2, std::chrono::seconds(120)) == Bytes({'o', 'k'}));
    reject([&] { (void)uke_video::capture({"/usr/bin/printf", "ok"}, 2, std::chrono::milliseconds(0)); });
    reject([&] { (void)uke_video::capture({"/usr/bin/printf", "ok"}, 2, std::chrono::seconds(121)); });
    reject([&] { (void)uke_video::capture({"/usr/bin/printf", "excess"}, 2); });
    const auto start = std::chrono::steady_clock::now();
    reject([&] { (void)uke_video::capture({"/usr/bin/sleep", "1"}, 2, std::chrono::milliseconds(10)); });
    color_check(std::chrono::steady_clock::now() - start < std::chrono::seconds(2));
    int status = 0;
    color_check(waitpid(-1, &status, WNOHANG) == -1 && errno == ECHILD);
}
} // namespace
extern "C" int __wrap_main() {
    try {
        color_contract();
        capture_contract();
        std::cout << "Actual updater private-color gate: " << color_checks <<
            " checks; legacy/dry paths unchanged, preparation before activation, failure stops staging, exact unstable marker and bounded child timeout/cleanup passed; no device access\n";
        return 0;
    } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
