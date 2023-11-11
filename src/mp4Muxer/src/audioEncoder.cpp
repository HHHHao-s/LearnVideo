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

    // 有关frame的要设置成编码器的参数
    AVSampleFormat pcm_sample_fmt = (AVSampleFormat)codec_ctx_->sample_fmt;
    frame_ = av_frame_alloc();
    if (!frame_) {
        fprintf(stderr, "Could not allocate audio frame\n");
        exit(1);
    }

    frame_->nb_samples     = GetFrameSize();
    frame_->format         = pcm_sample_fmt;

    ret = av_channel_layout_copy(&frame_->ch_layout, (const AVChannelLayout *)&GetChLayout());
    if (ret < 0)
        exit(1);
    
    /* allocate the data buffers */
    ret = av_frame_get_buffer(frame_, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate audio data buffers\n");
        exit(1);
    }
    

 

    return 0;


}

std::queue<AVPacket*> AudioEncoder::Encode( uint8_t * data, int stream_index, int64_t pts, int64_t time_base){

    if(!data){
        printf("frame is nullptr\n");
        return {};
    }
    if(!codec_ctx_){
        printf("codec_ctx_ is nullptr\n");
        return {};
    }
    
    // av_packet_rescale_ts(pkt, AVRational{1,(int)time_base}, codec_ctx_->time_base);
    av_frame_make_writable(frame_);
    frame_->pts = av_rescale_q(pts, AVRational{1,(int)time_base}, codec_ctx_->time_base);
   
    av_samples_fill_arrays(frame_->data, frame_->linesize, (const uint8_t *)data, codec_ctx_->channels, codec_ctx_->frame_size, codec_ctx_->sample_fmt, 1);

    int ret = avcodec_send_frame(codec_ctx_, frame_);
    if(ret!=0){
        AV_ERR(ret, "avcodec_send_frame");
        return {};
    }
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

    return q;

}