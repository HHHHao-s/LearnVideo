#include <iostream>

// 这里一定要用extern "C"
extern "C"{
#include "libavcodec/avcodec.h"
}

void test1(){
    AVPacket*pkt = av_packet_alloc();
    
    int ret = av_new_packet(pkt, 1024*1024);
    memcpy(pkt->data, "niubi\n", sizeof("niubi\n"));
    printf("%s", pkt->data);
    AVPacket*pkt2 = av_packet_clone(pkt);

    // 这里直接free不需要调用unref
    // av_packet_unref(pkt);
    //Free the packet, if the packet is reference counted, it will be unreferenced first.
    av_packet_free(&pkt);


    av_packet_unref(pkt2);
    printf("%s", pkt2->data);
}

int main(){

    test1();
    
    
    return 0;
}