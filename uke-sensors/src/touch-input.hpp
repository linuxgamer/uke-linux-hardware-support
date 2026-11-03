// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "touch-common.hpp"
#include "touch-thp-geometry.hpp"
#include "touch-thp-packet.hpp"
#include <linux/uinput.h>
#include <sys/ioctl.h>
#include <unistd.h>
static void input_setup(int fd,const nvt::Geometry &geometry) {
    require(ioctl(fd,UI_SET_EVBIT,EV_ABS)==0 && ioctl(fd,UI_SET_EVBIT,EV_KEY)==0 &&
            ioctl(fd,UI_SET_KEYBIT,BTN_TOUCH)==0 && ioctl(fd,UI_SET_PROPBIT,INPUT_PROP_DIRECT)==0,
            "Touch input capabilities failed");
    for(const auto &[code,maximum]:std::array<std::pair<unsigned,int>,4>{{
        {ABS_MT_SLOT,nvt::kFingerSlots-1},{ABS_MT_TRACKING_ID,65535},
        {ABS_MT_POSITION_X,geometry.width-1},{ABS_MT_POSITION_Y,geometry.height-1}}}) {
        require(ioctl(fd,UI_SET_ABSBIT,code)==0,"Touch axis capability failed");
        uinput_abs_setup axis{}; axis.code=code; axis.absinfo.maximum=maximum;
        if(code==ABS_MT_POSITION_X) axis.absinfo.resolution=geometry.width/237;
        if(code==ABS_MT_POSITION_Y) axis.absinfo.resolution=geometry.height/158;
        require(ioctl(fd,UI_ABS_SETUP,&axis)==0,"Touch axis setup failed");
    }
    uinput_setup device{};
    device.id.bustype=BUS_SPI;
    const char name[]="Novatek NT36532 Uke Touchscreen";
    std::copy(std::begin(name),std::end(name),device.name);
    require(ioctl(fd,UI_DEV_SETUP,&device)==0 && ioctl(fd,UI_SET_PHYS,"spi0.0/nt36532-uke-thp")==0 &&
            ioctl(fd,UI_DEV_CREATE)==0,"Touch input registration failed");
}
static void input_report(int fd,const std::vector<nvt::Slot> &slots,std::array<int,nvt::kFingerSlots> &ids,const nvt::Geometry &geometry) {
    if(slots.empty() && std::all_of(ids.begin(),ids.end(),[](int id){return id<0;}))return;
    std::array<bool,nvt::kFingerSlots> seen{};
    for(const auto &slot:slots) {
        require(slot.number>=0 && slot.number<int(ids.size()) && !seen[slot.number],"Invalid or duplicate touch slot");
        seen[slot.number]=true;
        require(slot.tracking_id>=0 && slot.tracking_id<=65535 &&
                slot.contact.x>=0 && slot.contact.x<geometry.width &&
                slot.contact.y>=0 && slot.contact.y<geometry.height,"Touch contact outside admitted range");
        require(std::count_if(slots.begin(),slots.end(),[&](const auto &other){return other.tracking_id==slot.tracking_id;})==1,
                "Duplicate touch tracking ID");
    }
    std::vector<input_event> events;
    const auto event=[&](unsigned type,unsigned code,int value) {
        input_event e{}; e.type=type; e.code=code; e.value=value; events.push_back(e);
    };
    // Legacy mousedev requires BTN_TOUCH before the coordinate events.
    event(EV_KEY,BTN_TOUCH,!slots.empty());
    for(unsigned index=0;index<ids.size();index++) {
        const auto found=std::find_if(slots.begin(),slots.end(),[&](const auto &s){return s.number==int(index);});
        const int id=found==slots.end() ? -1 : found->tracking_id;
        require(id>=-1 && id<=65535,"Touch tracking ID outside admitted range");
        if(id==ids[index] && id<0) continue;
        event(EV_ABS,ABS_MT_SLOT,index);
        if(id!=ids[index]) event(EV_ABS,ABS_MT_TRACKING_ID,id);
        if(found!=slots.end()) {
            event(EV_ABS,ABS_MT_POSITION_X,found->contact.x);
            // The stock framebuffer route is mirrored against the O82 electrode row axis.
            event(EV_ABS,ABS_MT_POSITION_Y,uke_touch::display_y(found->contact.y,geometry.height));
        }
        ids[index]=id;
    }
    event(EV_SYN,SYN_REPORT,0);
    require(::write(fd,events.data(),events.size()*sizeof(input_event))==ssize_t(events.size()*sizeof(input_event)),
            "Touch event publication failed");
}
