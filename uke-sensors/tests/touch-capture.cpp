// SPDX-License-Identifier: GPL-3.0-or-later
#include "../src/touch-capture.hpp"
#include <cassert>
#include <iostream>

template<class Call>static void refuses(Call call) {
    bool failed=false;try{call();}catch(const std::exception&){failed=true;}assert(failed);
}
int main() {
    v4l2_capability cap{};
    std::memcpy(cap.driver,"nt36532-uke-thp",15);
    cap.capabilities=V4L2_CAP_DEVICE_CAPS;
    cap.device_caps=V4L2_CAP_META_CAPTURE|V4L2_CAP_STREAMING;
    v4l2_format format{};format.type=V4L2_BUF_TYPE_META_CAPTURE;
    format.fmt.meta.dataformat=touch_capture_fourcc;
    format.fmt.meta.buffersize=format.fmt.meta.width=format.fmt.meta.bytesperline=uke_touch::frame_size;
    format.fmt.meta.height=1;
    touch_capture_format(cap,format);
    for(unsigned field=0;field<8;field++) {
        auto bad_cap=cap;auto bad_format=format;
        if(field==0)bad_cap.driver[0]='x';
        if(field==1)bad_cap.device_caps&=~V4L2_CAP_STREAMING;
        if(field==2)bad_cap.capabilities=0;
        if(field==3)bad_format.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if(field==4)bad_format.fmt.meta.dataformat=V4L2_TCH_FMT_DELTA_TD16;
        if(field==5)bad_format.fmt.meta.buffersize--;
        if(field==6)bad_format.fmt.meta.height=2;
        if(field==7)bad_format.fmt.meta.bytesperline--;
        refuses([&]{touch_capture_format(bad_cap,bad_format);});
    }
    v4l2_buffer buffer{};
    buffer.flags=V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
    buffer.bytesused=uke_touch::frame_size;
    buffer.timestamp.tv_sec=12;buffer.timestamp.tv_usec=500000;
    assert(touch_capture_fresh(buffer,12500000000));
    assert(touch_capture_fresh(buffer,12600000000));
    assert(!touch_capture_fresh(buffer,12600000001));
    assert(!touch_capture_fresh(buffer,12499999999));
    for(unsigned field=0;field<7;field++) {
        auto bad=buffer;
        if(field==0)bad.flags|=V4L2_BUF_FLAG_ERROR;
        if(field==1)bad.flags=V4L2_BUF_FLAG_TIMESTAMP_COPY;
        if(field==2)bad.bytesused--;
        if(field==3)bad.timestamp.tv_sec=-1;
        if(field==4)bad.timestamp.tv_usec=-1;
        if(field==5)bad.timestamp.tv_usec=1000000;
        if(field==6)bad.timestamp.tv_sec=INT64_MAX;
        assert(!touch_capture_fresh(bad,12500000000));
    }
    char directory[]="/tmp/uke-capture-fixture-XXXXXX";
    const char *created=mkdtemp(directory);assert(created);
    const std::filesystem::path root=created;
    const auto add=[&](const std::string &name,const std::string &driver) {
        const auto device=root/name/"device";
        std::filesystem::create_directories(device/"of_node");
        std::filesystem::create_directories(root/"drivers"/driver);
        std::filesystem::create_directory_symlink(root/"drivers"/driver,device/"driver");
        std::ofstream out(device/"of_node/compatible",std::ios::binary);
        constexpr char identity[]="novatek,nt36532-uke-thp";
        out.write(identity,sizeof(identity));
    };
    try {
        assert(touch_capture_provider(root).empty());
        add("video3","nt36532-uke-thp");
        assert(touch_capture_provider(root)==root/"video3");
        refuses([&]{touch_capture_open({});});
        add("video4","foreign");refuses([&]{touch_capture_provider(root);});
        std::filesystem::remove_all(root/"video4");
        add("video4","nt36532-uke-thp");refuses([&]{touch_capture_provider(root);});
        std::filesystem::remove_all(root/"video4");
        std::filesystem::rename(root/"video3",root/"videobad");
        refuses([&]{touch_capture_provider(root);});
        std::filesystem::remove_all(root);
    }catch(...){std::filesystem::remove_all(root);throw;}
    std::cout<<"Kernel THP format, buffer freshness, error/overflow and provider identity refusal: PASS\n";
}
