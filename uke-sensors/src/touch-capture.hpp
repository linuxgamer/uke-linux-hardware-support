// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "touch-common.hpp"
#include "touch-thp-packet.hpp"
#include <filesystem>
#include <linux/videodev2.h>
#include <string_view>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sys/mman.h>
#include <ctime>

// Standard MET8 FourCC; older distribution headers omit its symbolic name.
static constexpr auto touch_capture_fourcc=v4l2_fourcc('M','E','T','8');

static void touch_capture_format(const v4l2_capability &cap,const v4l2_format &format) {
    const std::string_view driver(reinterpret_cast<const char *>(cap.driver),
                                  strnlen(reinterpret_cast<const char *>(cap.driver),sizeof(cap.driver)));
    require(driver=="nt36532-uke-thp" && (cap.capabilities&V4L2_CAP_DEVICE_CAPS) &&
            (cap.device_caps&(V4L2_CAP_META_CAPTURE|V4L2_CAP_STREAMING))==
                (V4L2_CAP_META_CAPTURE|V4L2_CAP_STREAMING),"Foreign or incompatible THP capture driver");
    require(format.type==V4L2_BUF_TYPE_META_CAPTURE &&
            format.fmt.meta.dataformat==touch_capture_fourcc &&
            format.fmt.meta.buffersize==uke_touch::frame_size &&
            format.fmt.meta.width==uke_touch::frame_size && format.fmt.meta.height==1 &&
            format.fmt.meta.bytesperline==uke_touch::frame_size,"Incomplete or incompatible THP metadata format");
}
static std::filesystem::path touch_capture_provider(const std::filesystem::path &root="/sys/class/video4linux") {
    std::filesystem::path selected;
    if(!std::filesystem::exists(root))return selected;
    for(const auto &entry:std::filesystem::directory_iterator(root)) {
        const auto device=entry.path()/"device";
        if(!compatible((device/"of_node/compatible").c_str(),"novatek,nt36532-uke-thp"))continue;
        const auto name=entry.path().filename().string();
        require(name.starts_with("video") && name.size()>5 &&
                name.find_first_not_of("0123456789",5)==std::string::npos,"Invalid THP capture node name");
        require(std::filesystem::canonical(device/"driver").filename()=="nt36532-uke-thp",
                "Unexpected THP transport owner");
        require(selected.empty(),"Ambiguous THP capture provider");
        selected=entry.path();
    }
    return selected;
}
static int touch_capture_open(const std::filesystem::path &provider) {
    require(!provider.empty(),"Uke kernel THP capture is unavailable");
    const auto path=std::filesystem::path("/dev")/provider.filename();
    const int fd=open(path.c_str(),O_RDWR|O_NONBLOCK|O_CLOEXEC|O_NOFOLLOW);
    require(fd>=0,"Cannot open kernel THP capture");
    try {
        unsigned device_major=0,device_minor=0;char separator=0;
        std::ifstream identity(provider/"dev");
        require(bool(identity>>device_major>>separator>>device_minor) && separator==':' &&
                identity.peek()=='\n' && identity.get()=='\n' && identity.peek()==EOF,
                "Invalid THP device identity");
        struct stat st{};
        require(fstat(fd,&st)==0 && S_ISCHR(st.st_mode) && st.st_uid==0 && !(st.st_mode&0002) &&
                major(st.st_rdev)==device_major && minor(st.st_rdev)==device_minor,
                "Untrusted THP device node");
        require(flock(fd,LOCK_EX|LOCK_NB)==0,"THP capture is already owned");
        v4l2_capability cap{};
        require(ioctl(fd,VIDIOC_QUERYCAP,&cap)==0,"THP capture capabilities unavailable");
        v4l2_format format{};format.type=V4L2_BUF_TYPE_META_CAPTURE;
        require(ioctl(fd,VIDIOC_G_FMT,&format)==0,"THP metadata format unavailable");
        touch_capture_format(cap,format);
        return fd;
    } catch(...) {close(fd);throw;}
}
static bool touch_capture_fresh(const v4l2_buffer &buffer,std::uint64_t now_ns) {
    if((buffer.flags&V4L2_BUF_FLAG_ERROR) || buffer.bytesused!=uke_touch::frame_size ||
       (buffer.flags&V4L2_BUF_FLAG_TIMESTAMP_MASK)!=V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC ||
       buffer.timestamp.tv_sec<0 || buffer.timestamp.tv_usec<0 || buffer.timestamp.tv_usec>=1000000 ||
       std::uint64_t(buffer.timestamp.tv_sec)>UINT64_MAX/1000000000)return false;
    const auto seconds=std::uint64_t(buffer.timestamp.tv_sec)*1000000000;
    const auto fraction=std::uint64_t(buffer.timestamp.tv_usec)*1000;
    if(seconds>UINT64_MAX-fraction)return false;
    const auto captured=seconds+fraction;
    return captured<=now_ns && now_ns-captured<=100000000;
}
struct TouchCapture {
    int fd=-1;
    std::array<void *,2> mappings{MAP_FAILED,MAP_FAILED};
    std::array<std::size_t,2> lengths{};
    unsigned count{};
    TouchCapture(const TouchCapture &)=delete;
    TouchCapture &operator=(const TouchCapture &)=delete;
    explicit TouchCapture(int descriptor):fd(descriptor) {
        try {
            require(fd>=0,"Invalid THP capture descriptor");
            v4l2_requestbuffers buffers{};buffers.type=V4L2_BUF_TYPE_META_CAPTURE;
            buffers.memory=V4L2_MEMORY_MMAP;buffers.count=mappings.size();
            require(ioctl(fd,VIDIOC_REQBUFS,&buffers)==0 && buffers.count>=1 &&
                    buffers.count<=mappings.size(),"Cannot allocate bounded THP buffers");
            count=buffers.count;
            for(unsigned i=0;i<count;i++) {
                v4l2_buffer buffer{};buffer.type=V4L2_BUF_TYPE_META_CAPTURE;
                buffer.memory=V4L2_MEMORY_MMAP;buffer.index=i;
                require(ioctl(fd,VIDIOC_QUERYBUF,&buffer)==0 &&
                        buffer.length>=uke_touch::frame_size && buffer.length<=8192,
                        "Invalid THP capture buffer");
                lengths[i]=buffer.length;
                mappings[i]=mmap(nullptr,buffer.length,PROT_READ|PROT_WRITE,MAP_SHARED,fd,buffer.m.offset);
                require(mappings[i]!=MAP_FAILED,"Cannot map THP capture buffer");
                require(ioctl(fd,VIDIOC_QBUF,&buffer)==0,"Cannot queue THP capture buffer");
            }
            auto type=V4L2_BUF_TYPE_META_CAPTURE;
            require(ioctl(fd,VIDIOC_STREAMON,&type)==0,"Cannot start THP capture");
        } catch(...) {release();throw;}
    }
    void release() noexcept {
        if(fd>=0) {
            auto type=V4L2_BUF_TYPE_META_CAPTURE;
            ioctl(fd,VIDIOC_STREAMOFF,&type);
            for(unsigned i=0;i<mappings.size();i++)if(mappings[i]!=MAP_FAILED) {
                munmap(mappings[i],lengths[i]);mappings[i]=MAP_FAILED;
            }
            close(fd);fd=-1;
        }
    }
    ~TouchCapture(){release();}
    int frame(std::array<std::uint8_t,uke_touch::frame_size> &bytes) {
        v4l2_buffer buffer{};buffer.type=V4L2_BUF_TYPE_META_CAPTURE;buffer.memory=V4L2_MEMORY_MMAP;
        if(ioctl(fd,VIDIOC_DQBUF,&buffer)<0) {
            if(errno==EAGAIN || errno==EINTR)return 0;
            throw std::runtime_error("Kernel THP dequeue failed");
        }
        require(buffer.index<count && buffer.bytesused<=lengths[buffer.index],"Invalid dequeued THP buffer");
        timespec now{};require(clock_gettime(CLOCK_MONOTONIC,&now)==0,"THP monotonic clock unavailable");
        const bool valid=touch_capture_fresh(buffer,std::uint64_t(now.tv_sec)*1000000000+now.tv_nsec);
        if(valid)std::memcpy(bytes.data(),mappings[buffer.index],bytes.size());
        require(ioctl(fd,VIDIOC_QBUF,&buffer)==0,"Cannot return THP capture buffer");
        return valid?1:-1;
    }
};
