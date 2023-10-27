#include <iostream>
#include <stdio.h>
#include <fcntl.h>  
// #include <direct.h>
// 这里一定要用extern "C"
extern "C"{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavcodec/bsf.h"
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

void test2(const char * file_name){

    
    char buf[1024];
    // getcwd(buf, 1024);
    // printf("%s", buf);

    AVFormatContext *fmt_ctx = NULL;

    // 这里可以不用avformat_alloc_context，fmt_ctx为NULL也可以
    // 但是需要调用avformat_close_input()释放
    int ret = avformat_open_input(&fmt_ctx, file_name, NULL, NULL);
    if(ret < 0){
        // 错误处理
        av_strerror(ret, (char *)buf, 1024);
        printf("Could not open source file %s, %d(%s)\n", file_name, ret, buf);
        return;
    }
    av_dump_format(fmt_ctx, 0, file_name, 0);
    int video_stream_index = -1;
    int audio_stream_index = -1;
    for(int i=0;i<fmt_ctx->nb_streams;i++){
        AVStream *stream = fmt_ctx->streams[i];
        AVCodecParameters *codecpar = stream->codecpar;
        if(codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            video_stream_index = i; 
        }
        else if(codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
            audio_stream_index = i;
        }

    }
    AVPacket *pkt = av_packet_alloc();
    for(int i=0;i<10;i++){

        
        ret = av_read_frame(fmt_ctx, pkt);
        if(ret < 0){
            // 错误处理
            av_strerror(ret, (char *)buf, 1024);
            printf("Could not read frame %d, %d(%s)\n", i, ret, buf);
            return;
        }
        if(pkt->stream_index == video_stream_index){
            printf("pkt->stream_index == 0\n");
        }else if(pkt->stream_index == audio_stream_index){
            printf("pkt->stream_index == 1\n");
        }else{
            printf("pkt->stream_index == %d\n", pkt->stream_index);
        }
        av_packet_unref(pkt);
    }


    av_packet_free(&pkt);
    avformat_close_input(&fmt_ctx);


}

void test3(const char* file_name,const char* out_file_name){

    char buf[1024];
    // getcwd(buf, 1024);
    // printf("%s", buf);

    AVFormatContext *ifmt_ctx = NULL;

    // 这里可以不用avformat_alloc_context，fmt_ctx为NULL也可以
    // 但是需要调用avformat_close_input()释放
    int ret = avformat_open_input(&ifmt_ctx, file_name, NULL, NULL);
    if(ret < 0){
        // 错误处理
        av_strerror(ret, (char *)buf, 1024);
        printf("Could not open source file %s, %d(%s)\n", file_name, ret, buf);
        return;
    }
    av_dump_format(ifmt_ctx, 0, file_name, 0);
    int video_stream_index = -1;
    int audio_stream_index = -1;
    for(int i=0;i<ifmt_ctx->nb_streams;i++){
        AVStream *stream = ifmt_ctx->streams[i];
        AVCodecParameters *codecpar = stream->codecpar;
        if(codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            video_stream_index = i; 
        }
        else if(codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
            audio_stream_index = i;
        }

    }
    FILE* outfd = fopen(out_file_name, "wb");
    AVPacket *pkt = av_packet_alloc();
    // 1 获取相应的比特流过滤器
    // FLV/MP4/MKV等结构中，h264需要h264_mp4toannexb处理。添加SPS/PPS等信息。
    // FLV封装时，可以把多个NALU放在一个VIDEO TAG中,结构为4B NALU长度+NALU1+4B NALU长度+NALU2+...,
    // 需要做的处理把4B长度换成00000001或者000001
    // annexb模式: startcode 00000001 AVCC模式: 无startcode (mp4 flv mkv)
    if(strcmp(ifmt_ctx->iformat->name, "mov,mp4,m4a,3gp,3g2,mj2")==0){
        const AVBitStreamFilter *bsfilter = av_bsf_get_by_name("h264_mp4toannexb");
        AVBSFContext *bsf_ctx = NULL;
        // 2 初始化过滤器上下文
        av_bsf_alloc(bsfilter, &bsf_ctx); //AVBSFContext;
        // 3 添加解码器属性
        avcodec_parameters_copy(bsf_ctx->par_in, ifmt_ctx->streams[video_stream_index]->codecpar);
        av_bsf_init(bsf_ctx);
        
        int eof=0;
        while(!eof){
            if((ret=av_read_frame(ifmt_ctx, pkt))<0){
                eof = 1;
                printf("eof\n");
            }else{
                if(pkt->stream_index==video_stream_index){
                    // 只对MP4处理
                    
                    if(ifmt_ctx->iformat)
                    int input_size = pkt->size;
                    int out_pkt_count = 0;
                    if(av_bsf_send_packet(bsf_ctx, pkt)!=0){ // bitstream filter 内部会复制pkt
                        printf("av_bsf_send_packet error\n");
                        av_packet_unref(pkt);
                        continue;
                        
                    }

                    while(av_bsf_receive_packet(bsf_ctx, pkt)==0){
                        out_pkt_count++;
                        printf("out_pkt_count=%d\n", out_pkt_count);
                        size_t size = fwrite(pkt->data, 1, pkt->size, outfd);
                        if(size!=pkt->size){
                            perror("write file error\n");
                        }
                    }                
                }
                av_packet_unref(pkt); 
            }
        }
        
        
        av_bsf_free(&bsf_ctx);
    }else{
        int eof = 0;
        while(!eof){
            if((ret=av_read_frame(ifmt_ctx, pkt))<0){
                eof = 1;
                printf("eof\n");
            }else{
                if(pkt->stream_index==video_stream_index){
                    // 只对MP4处理
                    
                    
                    size_t size = fwrite(pkt->data, 1, pkt->size, outfd);
                    if(size!=pkt->size){
                        perror("write file error\n");
                    }

                              
                }
            }
            av_packet_unref(pkt); 
        }
    }
    av_packet_free(&pkt);
    avformat_close_input(&ifmt_ctx);


    
}

int main(int argc, char *argv[]){
    const char* file_name = "video.mp4";
    // test1();
    if(argc==3){
        test3(argv[1],argv[2]);
    }else{
        test3("video.mp4", "out.h264");
    }
    
    
    
}