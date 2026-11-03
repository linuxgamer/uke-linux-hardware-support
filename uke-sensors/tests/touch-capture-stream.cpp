// SPDX-License-Identifier: GPL-3.0-or-later
// Actual queue owner with synthetic ioctls and real memfd-backed mappings.
#include <array>
#include <cassert>
#include <cstdarg>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <ctime>
static unsigned calls=0,fail_at=0,live_maps=0,returned=0,stopped=0;
static bool dequeue=false,fail_return=false;
static v4l2_buffer received{};
static bool fail() {if(++calls!=fail_at)return false;errno=EIO;return true;}
static int capture_ioctl(int,unsigned long request,...) {
    va_list args;va_start(args,request);void *data=va_arg(args,void *);va_end(args);
    if(request==VIDIOC_STREAMOFF){++stopped;return 0;}
    if(fail())return -1;
    if(request==VIDIOC_REQBUFS) {
        auto &buffer=*static_cast<v4l2_requestbuffers *>(data);
        assert(buffer.count==2 && buffer.type==V4L2_BUF_TYPE_META_CAPTURE && buffer.memory==V4L2_MEMORY_MMAP);
        return 0;
    }
    if(request==VIDIOC_QUERYBUF) {
        auto &buffer=*static_cast<v4l2_buffer *>(data);
        assert(buffer.index<2);buffer.length=5432;buffer.m.offset=8192*buffer.index;return 0;
    }
    if(request==VIDIOC_STREAMON)return 0;
    if(request==VIDIOC_QBUF) {
        if(fail_return){errno=EIO;return -1;}
        ++returned;return 0;
    }
    assert(request==VIDIOC_DQBUF);
    if(!dequeue){errno=EAGAIN;return -1;}
    *static_cast<v4l2_buffer *>(data)=received;return 0;
}
static void *capture_mmap(void *address,size_t length,int prot,int flags,int fd,off_t offset) {
    if(fail())return MAP_FAILED;
    void *result=mmap(address,length,prot,flags,fd,offset);
    if(result!=MAP_FAILED)++live_maps;
    return result;
}
static int capture_munmap(void *address,size_t length) {
    const int result=munmap(address,length);assert(result==0 && live_maps);--live_maps;return result;
}
#define ioctl capture_ioctl
#define mmap capture_mmap
#define munmap capture_munmap
#include "../src/touch-capture.hpp"
#undef ioctl
#undef mmap
#undef munmap
#include <iostream>
static int descriptor() {
    const int fd=memfd_create("uke-thp-fixture",MFD_CLOEXEC);assert(fd>=0);
    assert(ftruncate(fd,16384)==0);return fd;
}
int main() {
    // Constructor failure must close the descriptor and every successful mapping.
    for(unsigned step=1;step<=8;step++) {
        calls=returned=stopped=0;fail_at=step;
        const int fd=descriptor();bool refused=false;
        try{TouchCapture capture(fd);}catch(const std::exception&){refused=true;}
        assert(refused && !live_maps && stopped==1 && fcntl(fd,F_GETFD)<0 && errno==EBADF);
    }
    calls=returned=stopped=0;fail_at=0;
    const int fd=descriptor();
    {
        TouchCapture capture(fd);assert(live_maps==2 && returned==2);
        std::array<std::uint8_t,uke_touch::frame_size> bytes{};
        assert(capture.frame(bytes)==0);
        timespec now{};assert(clock_gettime(CLOCK_MONOTONIC,&now)==0);
        received={};received.index=1;received.bytesused=bytes.size();
        received.flags=V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
        received.timestamp.tv_sec=now.tv_sec;received.timestamp.tv_usec=now.tv_nsec/1000;
        dequeue=true;
        std::memset(capture.mappings[1],0x5a,bytes.size());
        assert(capture.frame(bytes)==1 && returned==3);
        for(auto byte:bytes)assert(byte==0x5a);
        received.flags|=V4L2_BUF_FLAG_ERROR;
        assert(capture.frame(bytes)==-1 && returned==4);
        received.flags=V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;received.timestamp={};
        assert(capture.frame(bytes)==-1 && returned==5);
        received.index=2;bool refused=false;
        try{capture.frame(bytes);}catch(const std::exception&){refused=true;}assert(refused);
        received.index=0;fail_return=true;refused=false;
        try{capture.frame(bytes);}catch(const std::exception&){refused=true;}assert(refused);
        fail_return=false;
    }
    assert(!live_maps && stopped==1 && fcntl(fd,F_GETFD)<0 && errno==EBADF);
    // These admission helpers are exercised in the separate discovery/format fixture.
    (void)&touch_capture_provider;(void)&touch_capture_open;
    std::cout<<"Production THP queue: eight constructor failures, real mappings, dequeue/requeue, stale/error frames and cleanup: PASS\n";
}
