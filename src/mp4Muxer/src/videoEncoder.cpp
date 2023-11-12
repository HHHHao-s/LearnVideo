#include "videoEncoder.h"

VideoEncoder::VideoEncoder()
{
    width_ = 0;
    height_ = 0;
    fps_ = 0;
    bitrate_ = 0;
    codec_ = NULL;
    codec_ctx_ = NULL;
    pts_ = 0;
}

VideoEncoder::~VideoEncoder()
{
    Deinit();
}


void VideoEncoder::Deinit()
{
    if(deinited_) return;

    if (codec_ctx_ != NULL)
    {
        avcodec_close(codec_ctx_);
        av_free(codec_ctx_);
        codec_ctx_ = NULL;
    }
    if (codec_ != NULL)
    {
        codec_ = NULL;
    }
    if(frame_){
        av_frame_free(&frame_);
        frame_ = NULL;
    }
    deinited_ = true;
}

int VideoEncoder::InitH264(int width, int height, int fps, int bitrate)
{
    if(codec_ctx_){
        printf("codec_ctx_ is not nullptr\n");
        return -1;
    }
    if(codec_){
        printf("codec_ is not nullptr\n");
        return -1;
    }
    if(width<=0 || height<=0){
        printf("width or height is invalid\n");
        return -1;
    }
    if(fps<=0){
        printf("fps is invalid\n");
        return -1;
    }
    if(bitrate<=0){
        printf("bitrate is invalid\n");
        return -1;
    }
    width_ = width;
    height_ = height;
    fps_ = fps;
    bitrate_ = bitrate;

    codec_ = avcodec_find_encoder(AV_CODEC_ID_H264);
    if(!codec_){
        printf("avcodec_find_encoder failed\n");
        return -1;
    }
    codec_ctx_ = avcodec_alloc_context3(codec_);
    if(!codec_ctx_){
        printf("avcodec_alloc_context3 failed\n");
        return -1;
    }
   
    codec_ctx_->codec_id = AV_CODEC_ID_H264;
    codec_ctx_->codec_type = AVMEDIA_TYPE_VIDEO;
    codec_ctx_->pix_fmt = AV_PIX_FMT_YUV420P;
    codec_ctx_->width = width_;
    codec_ctx_->height = height_;
    codec_ctx_->bit_rate = bitrate_;
    codec_ctx_->gop_size = fps_;
    codec_ctx_->framerate = AVRational{fps_, 1};
    codec_ctx_->time_base = AVRational{1, 1000000}; // 单位为微秒
    codec_ctx_->qmin = 10;
    codec_ctx_->qmax = 51;
    codec_ctx_->max_b_frames = 0;
    codec_ctx_->refs = 3;
    codec_ctx_->thread_count = 1;
    codec_ctx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    AVDictionary *opts=NULL;
    av_dict_set(&opts, "preset", "ultrafast", 0);
    av_dict_set(&opts, "tune", "zerolatency", 0);

    int ret = avcodec_open2(codec_ctx_, codec_, &opts);
    if(ret<0){
        AV_ERR(ret, "avcodec_open2");
        return -1;
    }
    frame_ = av_frame_alloc();
    if(!frame_){
        printf("av_frame_alloc failed\n");
        return -1;
    }
    frame_->format = codec_ctx_->pix_fmt;
    frame_->width = codec_ctx_->width;
    frame_->height = codec_ctx_->height;


    printf("avcodec_open2 success\n");
    deinited_ = false;
    return 0;
}

std::queue<AVPacket*> VideoEncoder::Encode(uint8_t* yuv_data, int yuv_size, int stream_index, int64_t pts, int64_t time_base){

    
    if(!codec_ctx_){
        printf("codec_ctx_ is nullptr\n");
        return {};
    }
    int ret = 0;
    frame_->pts = av_rescale_q(pts, AVRational{1,(int)time_base}, codec_ctx_->time_base);
    if(yuv_data){
        
        int ret_size = av_image_fill_arrays(frame_->data, frame_->linesize, yuv_data, codec_ctx_->pix_fmt, codec_ctx_->width, codec_ctx_->height, 1);
        if(ret_size<0 || ret_size!=yuv_size){
            printf("av_image_fill_arrays failed\n");
            return {};
        }
        printf("video send frame%d\n" ,frame_->pts);
        ret = avcodec_send_frame(codec_ctx_, frame_);
    }else{
        // Flush encoder
        ret = avcodec_send_frame(codec_ctx_, NULL);
    }

    // av_packet_rescale_ts(pkt, AVRational{1,(int)time_base}, codec_ctx_->time_base);
   
    
    if(ret<0){
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