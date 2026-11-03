// SPDX-License-Identifier: GPL-3.0-or-later
// Contacts use the existing calibrated C++ core; firmware, SPI and IRQ belong to Linux.
#include "touch-capture.hpp"
#include "touch-panel.hpp"
#include "touch-input.hpp"
#include <csignal>
#include <iostream>
#include <poll.h>
#include <systemd/sd-daemon.h>

static volatile std::sig_atomic_t capture_running=1;
static void stop_capture(int) {capture_running=0;}
static void notify_first_frame(unsigned frames) {
    if(frames==1)require(sd_notify(0,"READY=1\nSTATUS=Calibrated input receiving valid kernel THP frames")>=0,
                        "THP service readiness notification failed");
}

int main(int argc,char **argv) {
    std::optional<TouchCapture> capture;
    int panel=-1,input=-1;
    std::array<int,nvt::kFingerSlots> ids;ids.fill(-1);
    nvt::Geometry geometry;
    try {
        require(argc==2 && std::string_view(argv[1])=="--daemon","Usage: uke-touch-capture --daemon");
        require(geteuid()==0 && compatible("/sys/firmware/devicetree/base/compatible","xiaomi,uke") &&
                compatible("/sys/firmware/devicetree/base/compatible","qcom,sm7675"),"Root on Uke / SM7675 is required");
        geometry=touch_geometry("/var/lib/uke-touch/config.ini");
        std::array<std::uint8_t,uke_touch::frame_size> reference_bytes{};
        std::ifstream reference_file("/var/lib/uke-touch/reference.bin",std::ios::binary);
        require(bool(reference_file.read(reinterpret_cast<char *>(reference_bytes.data()),reference_bytes.size())) &&
                reference_file.peek()==EOF,"Qualified THP reference is unavailable");
        const auto reference=uke_touch::decode(reference_bytes);
        require(bool(reference),"THP reference is corrupt");
        nvt::TouchCore core(geometry);
        const auto reset=[&] {
            if(input>=0)input_report(input,{},ids,geometry);
            core.reset();core.process(reference->matrix,reference->counter,reference->scan_mode);
        };
        reset();
        const auto panel_path=touch_panel_provider();
        require(!panel_path.empty(),"O82 panel readiness provider is unavailable");
        panel=open(panel_path.c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
        require(panel>=0,"Cannot open O82 panel readiness");
        auto admitted=touch_panel_state(panel);
        auto provider=touch_capture_provider();
        require(!provider.empty(),"Kernel THP capture provider is unavailable");
        input=open("/dev/uinput",O_WRONLY|O_CLOEXEC|O_NOFOLLOW);
        require(input>=0,"Kernel uinput is unavailable");
        input_setup(input,geometry);
        std::signal(SIGTERM,stop_capture);std::signal(SIGINT,stop_capture);
        require(sd_notify(0,"STATUS=Calibrated input registered; waiting for kernel THP frames")>=0,
                "THP service readiness notification failed");
        unsigned frames=0,rejected=0;
        while(capture_running) {
            const auto current=touch_panel_state(panel);
            if(current!=admitted) {
                reset();
                // Stop streaming and discard every completed buffer from the old panel.
                capture.reset();
                admitted=current;
            }
            if(admitted.ready) {
                if(!capture)capture.emplace(touch_capture_open(provider));
                std::array<std::uint8_t,uke_touch::frame_size> bytes{};
                const auto received=capture->frame(bytes);
                if(received>0) {
                    if(touch_panel_state(panel)!=admitted)continue;
                    const auto frame=uke_touch::decode(bytes);
                    if(frame) {
                        const auto result=core.process(frame->matrix,frame->counter,frame->scan_mode);
                        input_report(input,result.slots,ids,geometry);
                        ++frames;
                        notify_first_frame(frames);
                    } else {++rejected;reset();}
                    continue;
                }
                if(received<0){++rejected;reset();continue;}
            }
            pollfd ready[]{{capture?capture->fd:-1,POLLIN,0},{panel,POLLPRI|POLLERR,0}};
            const int status=poll(ready,2,admitted.ready?100:-1);
            if(status<0 && errno==EINTR)continue;
            require(status>=0 && !(ready[0].revents&(POLLHUP|POLLNVAL)) &&
                    !(ready[1].revents&POLLNVAL),"THP capture or panel event provider failed");
            if(!status)reset();
        }
        reset();
        require(ioctl(input,UI_DEV_DESTROY)==0,"THP input cleanup failed");
        std::cout<<"Kernel THP valid="<<frames<<" rejected="<<rejected<<'\n';
        close(input);input=-1;
        capture.reset();
        close(panel);
        return 0;
    } catch(const std::exception &error) {
        if(input>=0) {
            try{input_report(input,{},ids,geometry);}catch(...){}
            ioctl(input,UI_DEV_DESTROY);close(input);
        }
        capture.reset();
        if(panel>=0)close(panel);
        std::cerr<<error.what()<<'\n';return 1;
    }
}
