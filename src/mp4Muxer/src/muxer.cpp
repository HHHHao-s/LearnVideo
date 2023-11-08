#include "muxer.h"

Muxer::Muxer()
{
}

Muxer::~Muxer()
{
    DeInit();
}

int Muxer::Init(const char* url)
{
    url_ = url;
    int ret = avformat_alloc_output_context2(&fmt_ctx_, nullptr, nullptr, url);
    if(ret!=0){
        char errStr[64] = { 0 };
        av_strerror(ret, errStr, 64);
        printf("avformat_alloc_output_context2 failed:%s\n", errStr);
        return -1;
    }

    return 0;
}

void Muxer::DeInit(){
    if(de_inited_) return;
    if(fmt_ctx_){
        avformat_close_input(&fmt_ctx_);
    }
    de_inited_ = true;
    AV_MSG("DeInit success");
}

int Muxer::AddStream(AVCodecContext* codecCtx){
    if(!fmt_ctx_){
        printf("fmt_ctx_ is nullptr\n");
        return -1;
    }
    if(!codecCtx){
        printf("codecCtx is nullptr\n");
        return -1;
    }

    AVStream* stream = avformat_new_stream(fmt_ctx_, codecCtx->codec);
    if(!stream){
        
        printf("avformat_new_stream failed\n");
        return -1;
    }
    
    // 复制上下文
    avcodec_parameters_from_context(stream->codecpar, codecCtx);
    av_dump_format(fmt_ctx_, 0, url_.c_str(), 1);

    if(codecCtx->codec_type == AVMEDIA_TYPE_VIDEO){
        video_codec_ctx_ = codecCtx;
        video_stream_ = stream;
        video_index_ = stream->index;
    }else if(codecCtx->codec_type == AVMEDIA_TYPE_AUDIO){
        audio_codec_ctx_ = codecCtx;
        audio_stream_ = stream;
        audio_index_ = stream->index;
    }

    return 0;
}

int Muxer::WriteHeader(){
    if(!fmt_ctx_){
        printf("fmt_ctx_ is nullptr\n");
        return -1;
    }

    int ret = avio_open(&fmt_ctx_->pb, url_.c_str(), AVIO_FLAG_WRITE);
    if(ret!=0){
        char errStr[64] = { 0 };
        av_strerror(ret, errStr, 64);
        printf("avio_open failed:%s\n", errStr);
        return -1;
    }

    ret = avformat_write_header(fmt_ctx_, nullptr);
    if(ret!=0){
        char errStr[64] = { 0 };
        av_strerror(ret, errStr, 64);
        printf("avformat_write_header failed:%s\n", errStr);
        return -1;
    }

    return 0;
}

int Muxer::WritePacket(AVPacket *packet){

    int stream_index = packet->stream_index;
    if(!packet || packet->size<=0 || packet->data==nullptr){
        printf("packet is nullptr\n");
        av_packet_free(&packet);
        return -1;
    }

    AVRational src_time_base; // packet的时间基
    AVRational dst_time_base; // 流的时间基
    if(video_stream_ && video_codec_ctx_ && stream_index == video_index_){
        src_time_base = video_codec_ctx_->time_base;
        dst_time_base = video_stream_->time_base;
        packet->stream_index = video_index_;
    }else if(audio_stream_ && audio_codec_ctx_ && stream_index == audio_index_){
        src_time_base = audio_codec_ctx_->time_base;
        dst_time_base = audio_stream_->time_base;
        packet->stream_index = audio_index_;
    }else{
        printf("stream_index is invalid\n");
        av_packet_free(&packet);
        return -1;
    }
    // 新api
    av_packet_rescale_ts(packet, src_time_base, dst_time_base);

    
    int ret = av_interleaved_write_frame(fmt_ctx_, packet);
    if(ret!=0){
        char errStr[64] = { 0 };
        av_strerror(ret, errStr, 64);   
        printf("av_interleaved_write_frame failed:%s\n", errStr);
        return -1;
    }

    av_packet_free(&packet);
    return 0;
}

int Muxer::WriteTrailer(){
    if(!fmt_ctx_){
        printf("fmt_ctx_ is nullptr\n");
        return -1;
    }

    int ret = av_write_trailer(fmt_ctx_);
    if(ret!=0){
        AV_ERR(ret, "av_write_trailer failed");
        // char errStr[64] = { 0 };
        // av_strerror(ret, errStr, 64); 
        // printf("av_write_trailer failed:%s\n", errStr);
        return -1;
    }
    return 0;
}