#include "audioEncoder.h"

AudioEncoder::AudioEncoder()
{
}

AudioEncoder::~AudioEncoder()
{
    DeInit();
}

void AudioEncoder::DeInit(){
    if(de_inited_) return;
    if(codec_ctx_){
        avcodec_close(codec_ctx_);
        codec_ctx_= nullptr;
    }
    de_inited_ = true;
}

int AudioEncoder::InitAAC(int channels, int sample_rate, int bit_rate ){

    // test_ = fopen("test.aac", "wb");
    // if(!test_){
    //     printf("fopen test.aac failed\n");
    //     return -1;
    // }

    channels_ = channels;
    sample_rate_ = sample_rate;
    bit_rate_ = bit_rate;

    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if(!codec){
        printf("avcodec_find_encoder AV_CODEC_ID_AAC failed\n");
        return -1;
    }
    codec_ctx_ = avcodec_alloc_context3(codec);
    if(!codec_ctx_){
        printf("avcodec_alloc_context3 failed\n");
        return -1;
    }

    codec_ctx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;// 不带adts头
    codec_ctx_->bit_rate = bit_rate_;
    codec_ctx_->sample_rate = sample_rate_;
    codec_ctx_->sample_fmt = AV_SAMPLE_FMT_FLTP;
    // 新api
    codec_ctx_->ch_layout = AV_CHANNEL_LAYOUT_STEREO;
    
    int ret = avcodec_open2(codec_ctx_, codec, nullptr);
    if(ret<0){
        AV_ERR(ret, "avcodec_open2");
        return -1;
    }
    AV_MSG("avcodec_open2 success");


    return 0;


}

static void getAdstHeader(unsigned char* adtsHeader,AVCodecContext *ctx, int len)

{
    int sampleRate = ctx->sample_rate;
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
    int profile = ctx->profile;
    int chanCfg = ctx->ch_layout.nb_channels;
	adtsHeader[0] = (unsigned char) 0xFF;
    adtsHeader[1] = (unsigned char) 0xF9;
    adtsHeader[2] = (unsigned char) (((profile - 1) << 6) + (freqIdx << 2) + (chanCfg >> 2));
    adtsHeader[3] = (unsigned char) (((chanCfg & 3) << 6) + (packetLen >> 11));
    adtsHeader[4] = (unsigned char) ((packetLen & 0x7FF) >> 3);
    adtsHeader[5] = (unsigned char) (((packetLen & 7) << 5) + 0x1F);
    adtsHeader[6] = (unsigned char) 0xFC;
	
	

}


std::queue<AVPacket*> AudioEncoder::Encode( AVFrame*frame,int stream_index, int64_t pts, int64_t time_base){

    if(!frame){
        printf("audio frame is nullptr\n");
        avcodec_send_frame(codec_ctx_, frame);
        return {};
    }
    if(!codec_ctx_){
        printf("codec_ctx_ is nullptr\n");
        return {};
    }
    
    // av_packet_rescale_ts(pkt, AVRational{1,(int)time_base}, codec_ctx_->time_base);
    // av_frame_make_writable(frame_);
    frame->pts = av_rescale_q(pts, AVRational{1,(int)time_base}, codec_ctx_->time_base);
    


    int ret = avcodec_send_frame(codec_ctx_, frame);
    if(ret!=0){
        AV_ERR(ret, "avcodec_send_frame");
        return {};
    }
    av_frame_free(&frame);
    std::queue<AVPacket*> q;
    while(ret>=0){
        AVPacket* pkt = av_packet_alloc();
        if(!pkt){
            printf("av_packet_alloc failed\n");
            return {};
        }
        ret = avcodec_receive_packet(codec_ctx_, pkt);
        if(ret==AVERROR(EAGAIN) || ret==AVERROR_EOF){
            av_packet_free(&pkt);
            break;
        }else if(ret<0){
            AV_ERR(ret, "avcodec_receive_packet");
            av_packet_free(&pkt);
            return {};
        }
        if(ret!=0){
            AV_ERR(ret, "avcodec_receive_packet");
            av_packet_free(&pkt);
            return {};
        }
        pkt->stream_index = stream_index;
        q.push(pkt);
    }
    printf("audio receive packet%d\n" ,q.size());
    // for(int i=0;i<q.size();i++){
    //     AVPacket* pkt = q.front();
    //     q.pop();
    //     unsigned char adtsHeader[7];
    //     getAdstHeader(adtsHeader,codec_ctx_,pkt->size);
    //     fwrite(adtsHeader, 1, 7, test_);
    //     fwrite(pkt->data, 1, pkt->size, test_);
    //     // av_packet_free(&pkt);
    // }


    return q;

}