// SPDX-License-Identifier: MIT
#include "backend.hpp"
#include "charge-policy.hpp"
#include "front-color.hpp"
#include "sensors.hpp"
#include <charconv>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <unistd.h>
#include "charge-notify.hpp"
#include "desktop.hpp"
#include "plasma.hpp"
#include "wake.hpp"
#include "diagnostic.hpp"
#include "lights.hpp"
#include "vrr.hpp"
#include "usb-request.hpp"
#include "link-policy.hpp"
static void help() {
    std::cout << "uke-control 1.10.2 — native device management and development diagnostics\n"
      "  status                      Swap, qualified charge controls and USB state\n"
      "  desktop status              Existing standard desktop interfaces; no activation\n"
      "  desktop plasma              KDE panel control availability and saved policies\n"
      "  desktop rotation on|off     Set original KScreen automatic rotation policy\n"
      "  desktop brightness on|off   Set original KScreen automatic brightness policy\n"
      "  hardware status             Cached chipset/audio/media/input paths; no device access\n"
      "  flashlight status|on|off    Supervised 20 mA torch; shuts off within 60 seconds\n"
      "  led status|off|set R G B    RGB color 0..255 at fixed low PWM duty (root)\n"
      "  wake status                 Driver wake permissions and unsupported event controls\n"
      "  wake set SOURCE on|off      Save and apply one wake permission (root)\n"
      "  wake apply                  Restore saved permissions before suspend (root)\n"
      "  brunch stable|test|unstable  Switch Uke COPR and synchronize its packages (root)\n"
      "  brunch sync|status          Repair selected channel, or show selection\n"
      "  branch ...                  Alias for brunch; --dry-run and --yes supported\n"
      "  update [--check] [--yes]     Sync channel and upgrade distribution packages\n"
      "  sensors [-n5] [--json]      Sensor snapshot; -n repeats every N seconds\n"
      "  sensors list                List exported channels and SensorProxy capabilities\n"
      "  sensors -n5 --count=3       Three snapshots, five seconds apart; Ctrl-C stops\n"
      "  display sensor              Read the existing front-color node; never starts collection\n"
      "  display profiles            Display/USB lane profiles and actual Type-C control availability\n"
      "  display profile NAME        Request usb3-display or display4; unsupported controls fail (root)\n"
      "  display lane-limit 1|2|4    DP training ceiling, disconnected only (root)\n"
      "  usb power DEVICE POLICY     Runtime PM: performance or auto, per USB device (root)\n"
      "  ethernet status IFACE       RTL8153 link, EEE, offload, pause, ring and channel capabilities\n"
      "  ethernet profile IFACE NAME auto, 1000-full, 100-full or 10-full; interface down (root)\n"
      "  display vrr-min [status]    Loaded/saved VRR minimum and restart requirement (root)\n"
      "  display vrr-min set HZ      Save 30 Hz or higher, below the selected mode (root)\n"
      "  inventory [--refresh]       Partition/filesystem inventory; refresh requires root\n"
      "  swap status                 Active swaps and saved policy\n"
      "  swap set half|full|MiB      Save policy and apply (root); prefer linux_swap\n"
      "  swap apply                  Apply saved policy (root)\n"
      "  charge modes                Show requested policies and unavailable controls\n"
      "  charge plan MODE            Preview metadata; performs no hardware write\n"
      "  charge status               Native control metadata, or standard capability\n"
      "  charge capabilities|admission|protocols  Native backend metadata\n"
      "  charge settings             Read three firmware threshold fields (root; no SET)\n"
      "  charge protection           Read firmware moisture fields (root; no SET)\n"
      "  charge protection-status    Read cached moisture and inhibit status\n"
      "  charge moisture-enable      Enable-only protection request (qualified, offline)\n"
      "  charge notifications        Desktop-session moisture notifications\n"
      "  charge enable-cap80         Enable existing 70-80 cap with USB input off (root)\n"
      "  charge apply-cap80          One-shot existing 70-80 request; effect unverified (root)\n"
      "  charge mode MODE            Request qualified charge80, hold80-90, slow or automatic\n"
      "  charge limit PERCENT        Request the qualified 80% cap or a supported standard threshold\n"
      "  usb status                  Cached Type-C, USB topology, NIC and display JSON\n"
      "  firmware status             Cached SCMI discovery and remote-processor JSON\n"
      "  usb watch [SECONDS]         Observe changes at 1 Hz for 1..300 seconds\n"
      "  usb request data host|device Request a Type-C data-role swap (root)\n"
      "  set cdc on|off              One-shot CDC ACM request (root)\n"
      "  set cdc on|off --persistent Save and apply CDC startup policy (root)\n"
      "  test [--help]               Hz, Wayland cadence, touch and sensor diagnostics\n"
      "  diagnostic all|TOPIC        Save uke-diagnostic-TOPIC.log in the current directory\n"
      "  diagnostic --help           Wi-Fi, Bluetooth, sensors, sleep and other topics\n"
      "  resize system [--check]     Check capacity, or confirm offline growth and reboot (root)\n"
      "  report                      Save comprehensive private logs locally (root)\n"
      "  report github DIR OWNER/REPO Submit only the generated anonymous summary via gh\n"
      "  --self-test                 Host refusal and arithmetic checks\n"
      "Settings require root, e.g. sudo uke-control swap set half. No setuid binary.\n";
}
int main(int argc,char** argv) {
    try {
        if(argc<2||std::string_view(argv[1])=="--help"){help();return 0;}
        if(argc==4&&std::string_view(argv[1])=="charge"&&std::string_view(argv[2])=="mode") {
            auto result=uke::charge_control_mode(argv[3]);std::cout<<result.output;return result.status;
        }
        if(const int result=uke::charge::policy_command(argc,argv);result>=0)return result;
        std::string command=argv[1];
        if(command=="flashlight" || command=="led")return uke::lights::command(command=="led",argc-2,argv+2);
        if(command=="resize") {
            if(argc!=3&&argc!=4)throw std::runtime_error("Usage: uke-control resize system [--check]");
            if(std::string_view(argv[2])!="system"||(argc==4&&std::string_view(argv[3])!="--check"))throw std::runtime_error("Usage: uke-control resize system [--check]");
            const char* arguments[]={"/usr/libexec/uke-control/resize-system",argc==4?"--check":"--interactive",nullptr};
            execv(arguments[0],const_cast<char* const*>(arguments));
            throw std::runtime_error("The native resize helper is unavailable");
        }
        if(command=="diagnostic")return uke::diagnostic::command(argc-2,argv+2);
        if(command=="wake") {
            if(argc==3&&std::string_view(argv[2])=="check") return uke::wake::applicable()?0:1;
            if(argc==3&&std::string_view(argv[2])=="status") {std::cout<<uke::wake_status();return 0;}
            if(argc==3&&std::string_view(argv[2])=="apply") {uke::wake_apply();return 0;}
            if(argc==5&&std::string_view(argv[2])=="set") {
                const std::string_view value=argv[4];
                if(value!="on"&&value!="off")throw std::runtime_error("Wake permission must be on or off");
                uke::wake_set(argv[3],value=="on");std::cout<<uke::wake_status();return 0;
            }
        }
        if(command=="desktop"&&argc==3&&std::string_view(argv[2])=="status"){std::cout<<uke::desktop::status();return 0;}
        if(command=="desktop"&&argc==3&&std::string_view(argv[2])=="plasma"){std::cout<<uke::desktop::plasma_status();return 0;}
        if(command=="desktop"&&argc==4){auto result=uke::desktop::plasma_set(argv[2],argv[3]);std::cout<<result.output;return result.status;}
        if(command=="brunch"||command=="branch"||command=="update") {
            execv("/usr/libexec/uke-control/packages",argv+1);
            throw std::runtime_error("The native package manager helper is missing");
        }
        if(command=="sensors")return uke::sensors::command(argc-2,argv+2);
        if(command=="display"&&argc==3&&std::string_view(argv[2])=="profiles") {
            std::cout<<uke::usb::display_profiles("/");return 0;
        }
        if(command=="display"&&argc==4&&std::string_view(argv[2])=="lane-limit") {
            const auto result=uke::display_lane_limit(argv[3]);std::cout<<result.output;return result.status;
        }
        if(command=="ethernet"&&argc==4&&std::string_view(argv[2])=="status") {
            std::cout<<uke::links::ethernet_status(argv[3]);return 0;
        }
        if(command=="ethernet"&&argc==5&&std::string_view(argv[2])=="profile") {
            const auto result=uke::ethernet_profile_set(argv[3],argv[4]);std::cout<<result.output;return result.status;
        }
        if(command=="display"&&argc==4&&std::string_view(argv[2])=="profile") {
            const auto result=uke::display_profile_set(argv[3]);
            std::cout<<result.output;return result.status;
        }
        if(command=="display"&&argc==3&&std::string_view(argv[2])=="sensor") {
            std::cout<<uke::display::front_color_status();return 0;
        }
        if(command=="display"&&argc>=3&&std::string_view(argv[2])=="vrr-min") {
            if(argc==3||(argc==4&&std::string_view(argv[3])=="status")) {
                if(geteuid()!=0)throw std::runtime_error("Use sudo uke-control display vrr-min status to read the current driver floor");
                std::cout<<uke::vrr::status();return 0;
            }
            if(argc==4&&std::string_view(argv[3])=="configured") {
                const auto value=uke::wake::applicable()?uke::vrr::configured():std::nullopt;
                std::cout<<(value?std::to_string(*value):"unset")<<'\n';return 0;
            }
            if(argc==5&&std::string_view(argv[3])=="set") {
                uke::display_vrr_set(uke::vrr::minimum(argv[4]));
                std::cout<<uke::vrr::status();
                std::cerr<<"Saved for the next boot. Restart to apply a changed minimum.\n";return 0;
            }
            throw std::runtime_error("Usage: uke-control display vrr-min [status|set HZ]");
        }
        if(command=="cdc"&&argc==3&&std::string_view(argv[2])=="apply") {
            auto result=uke::cdc_apply();std::cout<<result.output;return result.status;
        }
        if(command=="set"&&(argc==4||argc==5)&&std::string_view(argv[2])=="cdc") {
            const std::string_view setting=argv[3];
            if(setting!="on"&&setting!="off")throw std::runtime_error("CDC setting must be on or off");
            if(argc==5&&std::string_view(argv[4])!="--persistent")throw std::runtime_error("Expected --persistent");
            auto result=argc==5?uke::cdc_persistent(setting=="on"):uke::cdc_set(setting=="on");
            std::cout<<result.output;return result.status;
        }
        if(command=="test"){execv("/usr/libexec/uke-control/tests",argv+1);throw std::runtime_error("Install uke-control-tests from the development repository");}
        if(command=="--self-test"&&argc==2){uke::self_test();return 0;}
        if(command=="status"&&argc==2){std::cout<<uke::status();return 0;}
        if(command=="firmware"&&argc==3&&std::string_view(argv[2])=="status"){std::cout<<uke::firmware_status();return 0;}
        if(command=="hardware"&&argc==3&&std::string_view(argv[2])=="status"){std::cout<<uke::hardware_status();return 0;}
        if(command=="boot"&&argc==2){uke::boot();return 0;}
        if(command=="inventory"&&(argc==2||(argc==3&&std::string_view(argv[2])=="--refresh"))){std::cout<<uke::inventory(argc==3);return 0;}
        if(command=="swap") {
            if(argc==3&&std::string_view(argv[2])=="status"){std::cout<<uke::status();return 0;}
            if(argc==3&&std::string_view(argv[2])=="apply"){uke::swap_apply();return 0;}
            if(argc==4&&std::string_view(argv[2])=="set"){uke::swap_set(argv[3]);return 0;}
        }
        if(command=="charge") {
            if(argc==3&&std::string_view(argv[2])=="notifications") {uke::charge::moisture_notifications();return 0;}
            if(argc==3&&std::string_view(argv[2])=="moisture-enable") {
                auto result=uke::charge_enable_moisture();std::cout<<result.output;return result.status;
            }
            if(argc==3&&std::string_view(argv[2])=="enable-cap80") {
                auto result=uke::charge_enable_preconfigured_cap();std::cout<<result.output;return result.status;
            }
            if(argc==3&&std::string_view(argv[2])=="apply-cap80") {
                auto result=uke::charge_apply_preconfigured_cap();std::cout<<result.output;return result.status;
            }
            if(argc==3&&(std::string_view(argv[2])=="capabilities"||std::string_view(argv[2])=="admission"||std::string_view(argv[2])=="protocols"||std::string_view(argv[2])=="settings"||std::string_view(argv[2])=="protection"||std::string_view(argv[2])=="protection-status")) {
                auto result=uke::charge_control_query(argv[2]);std::cout<<result.output;return result.status;
            }
            if(argc==3&&std::string_view(argv[2])=="status"){std::cout<<uke::charge_status();return 0;}
            if(argc==4&&std::string_view(argv[2])=="limit") {
                unsigned value{};std::string_view arg=argv[3];auto p=std::from_chars(arg.data(),arg.data()+arg.size(),value);
                if(p.ec!=std::errc{}||p.ptr!=arg.data()+arg.size()||value<1||value>100)throw std::runtime_error("Charge limit must be 1..100 percent");
                if(value==80){auto result=uke::charge_control_mode("charge80");std::cout<<result.output;return result.status;}
                uke::charge_set(value);return 0;
            }
        }
        if(command=="usb") {
            if(argc==5&&std::string_view(argv[2])=="power") {
                const auto result=uke::usb_power_set(argv[3],argv[4]);std::cout<<result.output;return result.status;
            }
            if(argc==3&&std::string_view(argv[2])=="status"){std::cout<<uke::usb_status();return 0;}
            if(argc==5&&std::string_view(argv[2])=="request"&&std::string_view(argv[3])=="data") {
                auto result=uke::usb_request_data(argv[4]);std::cout<<result.output;return result.status;
            }
            if((argc==3||argc==4)&&std::string_view(argv[2])=="watch") {
                unsigned seconds=30;
                if(argc==4){std::string_view arg=argv[3];auto p=std::from_chars(arg.data(),arg.data()+arg.size(),seconds);if(p.ec!=std::errc{}||p.ptr!=arg.data()+arg.size())throw std::runtime_error("Invalid USB watch duration");}
                uke::usb_watch(seconds);return 0;
            }
        }
        if(command=="report") {
            if(argc==2){std::cout<<uke::report()<<'\n';return 0;}
            if(argc==5&&std::string_view(argv[2])=="github"){uke::report_github(argv[3],argv[4]);return 0;}
        }
        help();return 64;
    } catch(const std::exception& e){std::cerr<<"uke-control: "<<e.what()<<'\n';return 1;}
}
