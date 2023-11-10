#include <string>
#include <queue>
#define AV_ERR(ret, msg) \
    {\
        char errStr[64] = { 0 };\
        av_strerror(ret, errStr, 64);\
        printf("%s %d %s:%s\n",__FILE__, __LINE__, msg, errStr);\
    }

#define AV_MSG(msg) \
    {\
        printf("%s %d %s\n",__FILE__, __LINE__, msg);\
    }

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

class AudioEncoder{
public:
    AudioEncoder();
    ~AudioEncoder();
    int InitAAC(int channels, int sample_rate, int bit_rate);
    void DeInit();
    
    std::queue<AVPacket*> Encode(AVFrame* frame, int stream_index, int64_t pts, int64_t time_base);
    int GetFrameSize(){// 一帧数据每个通道的采样数
        if(codec_ctx_==nullptr) return -1;
        return codec_ctx_->frame_size;
    }
    int GetFrameBytes(){// 一帧数据的字节数
        if(codec_ctx_==nullptr) return -1;
        return codec_ctx_->frame_size * codec_ctx_->ch_layout.nb_channels * av_get_bytes_per_sample(codec_ctx_->sample_fmt);
    }
    int GetChannels(){
        if(codec_ctx_==nullptr) return -1;
        return codec_ctx_->ch_layout.nb_channels;
    }
    int GetSampleRate(){
        if(codec_ctx_==nullptr) return -1;
        return codec_ctx_->sample_rate;
    }
    int GetSamepleFormat(){
        if(codec_ctx_==nullptr) return -1;
        return codec_ctx_->sample_fmt;
    
    }

    const AVChannelLayout& GetChLayout(){
        if(codec_ctx_==nullptr) return (AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO;
        return codec_ctx_->ch_layout;
    }

    AVCodecContext* GetCtx(){
        return codec_ctx_;
    }

private:
    std::string url_;
    // AVFormatContext* fmt_ctx_{nullptr};

    // 编码器
    AVCodecContext* codec_ctx_{nullptr};

    // 流
    AVStream* audio_stream_{nullptr};

    // index
    int audio_index_{-1};
    
    bool de_inited_{false};

    int channels_{2};
    int sample_rate_{44100};
    int bit_rate_{128000};


};