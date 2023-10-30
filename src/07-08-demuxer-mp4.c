#include <stdio.h>

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavcodec/bsf.h"



void getAdstHeader(unsigned char* adtsHeader,int len, int profile, int sampleRate, int chanCfg)
{

    int freqIdx = 0; //默认为0
    switch (sampleRate)
    {   case 96000: freqIdx = 0; break;
        case 88200: freqIdx = 1; break;
        case 64000: freqIdx = 2; break;
        case 48000: freqIdx = 3; break;
        case 44100: freqIdx = 4; break;
        case 32000: freqIdx = 5; break;
        case 24000: freqIdx = 6; break;
        case 22050: freqIdx = 7; break;
        case 16000: freqIdx = 8; break;
        case 12000: freqIdx = 9; break;
        case 11025: freqIdx = 10; break;
        case 8000: freqIdx = 11; break;
        case 7350: freqIdx = 12; break;
        default: freqIdx = 15; break;//如果都不是默认为15

    }

	

	int packetLen=len+7;//ADTS头加AAC数据总长度

	adtsHeader[0] = (unsigned char) 0xFF;
    adtsHeader[1] = (unsigned char) 0xF9;
    adtsHeader[2] = (unsigned char) (((profile - 1) << 6) + (freqIdx << 2) + (chanCfg >> 2));
    adtsHeader[3] = (unsigned char) (((chanCfg & 3) << 6) + (packetLen >> 11));
    adtsHeader[4] = (unsigned char) ((packetLen & 0x7FF) >> 3);
    adtsHeader[5] = (unsigned char) (((packetLen & 7) << 5) + 0x1F);
    adtsHeader[6] = (unsigned char) 0xFC;
	
	

}

int main(int argc, char *argv[] ){

    char * input_filename,*output_video_filename,*output_audio_filename;


    if(argc!=4){
        printf("Usage: %s <input file> <output video file> <output audio file>\n", argv[0]);
        input_filename = "video.mp4";
        output_video_filename = "test.h264";
        output_audio_filename = "test.aac";
        
    }else{
        input_filename = argv[1];
        output_video_filename = argv[2];
        output_audio_filename = argv[3];
    }

    

    FILE * input_file = fopen(input_filename, "rb");

    if(!input_file){
        perror("Error opening input file\n");
        return 1;
    }

    FILE * output_video_file = fopen(output_video_filename, "wb");

    if(!output_video_file){
        perror("Error opening output video file\n");
        return 1;
    }

    FILE * output_audio_file = fopen(output_audio_filename, "wb");

    if(!output_audio_file){
        perror("Error opening output audio file\n");
        return 1;
    }

    AVFormatContext * ifmt_ctx = NULL;
    int ret = 0;
    char errors[1024];
    if((ret=avformat_open_input(&ifmt_ctx, input_filename, NULL, NULL)) < 0){
        av_strerror(ret, &errors, 1024);
        printf("%s\n", errors);
        return 1;
    }
    int video_stream_index = -1;
    int audio_stream_index = -1;

    AVCodec * video_codec = NULL;
    video_stream_index= av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &video_codec, 0);

    if(video_stream_index<0){
        av_strerror(video_stream_index, &errors, 1024);
        printf("%s\n", errors);
        return 1;
    }

    AVCodec * audio_codec = NULL;
    audio_stream_index= av_find_best_stream(ifmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &audio_codec, 0);

    if(audio_stream_index<0){
        av_strerror(audio_stream_index, &errors, 1024);
        printf("%s\n", errors);
        return 1;
    }

    AVPacket *pkt=av_packet_alloc();

    av_init_packet(pkt);

    AVBSFContext * bsf_ctx = NULL;

    AVBitStreamFilter * bsf = av_bsf_get_by_name("h264_mp4toannexb");

    ret = av_bsf_alloc(bsf, &bsf_ctx);
    
    if(ret<0){
        av_strerror(ret, &errors, 1024);
        printf("%s\n", errors);
        return 1;
    }
    avcodec_parameters_copy(bsf_ctx->par_in, ifmt_ctx->streams[video_stream_index]->codecpar);
    av_bsf_init(bsf_ctx);

    while(1){
        // av_read_frame 不会释放pkt的buf
        // 需要手动释放，不然会内存泄漏
        ret = av_read_frame(ifmt_ctx, pkt);
        if(ret<0){
            // 不成功不分配内存
            av_strerror(ret, &errors, 1024);
            printf("%s\n", errors);
            break;
        }
        if(pkt->stream_index == video_stream_index){
            // 从mp4中读取的是AVC格式的数据，需要转换成AnnexB格式
            
            // The bitstream filter will take ownership of the packet and reset the contents of pkt.
            // 内部会移动pkt的buf
            if((ret=av_bsf_send_packet(bsf_ctx, pkt))!=0){ 
                av_strerror(ret, &errors, 1024);
                printf("%s\n", errors);
                av_packet_unref(pkt);
                continue;
            }
            // av_packet_unref(pkt);// 不需要释放
            while((ret=av_bsf_receive_packet(bsf_ctx, pkt)) >=0){
                fwrite(pkt->data, 1, pkt->size, output_video_file);
                av_packet_unref(pkt);
            }

        }else if(pkt->stream_index==audio_stream_index){
            unsigned char adtsHeader[7];
            getAdstHeader(&adtsHeader,
            pkt->size,
            ifmt_ctx->streams[audio_stream_index]->codecpar->profile,
            ifmt_ctx->streams[audio_stream_index]->codecpar->sample_rate,
            ifmt_ctx->streams[audio_stream_index]->codecpar->ch_layout.nb_channels);
            fwrite(adtsHeader, 1, 7, output_audio_file);
            fwrite(pkt->data, 1, pkt->size, output_audio_file);
            av_packet_unref(pkt);//释放buf
        }else{
            av_packet_unref(pkt);//释放buf
        }
        

        
    }

    fclose(input_file);
    fclose(output_video_file);
    fclose(output_audio_file);
    av_bsf_free(&bsf_ctx);
    
    av_packet_free(&pkt);
    avformat_free_context(ifmt_ctx);
    return 0;
}