#ifndef MUXER_H
#define MUXER_H
#include <string>

#define AV_ERR(ret, ...) \
    {\
        char errStr[64] = { 0 };\
        av_strerror((ret), errStr, 64);\
        printf("%s %d %s:",__FILE__, __LINE__, errStr);\
        printf(__VA_ARGS__);\
        putchar('\n');\
    }

#define AV_MSG(msg) \
    {\
        printf("%s %d %s\n",__FILE__, __LINE__, msg);\
    }

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

class Muxer{
public:
    Muxer();
    ~Muxer();
    int Init(const char* url);
    void DeInit();
    int AddStream(AVCodecContext* codecCtx);
    
    int WriteHeader();
    int WritePacket(AVPacket* pkt);
    int WriteTrailer();

    int Open();// avio open

    int GetVideoIndex(){
        return video_index_;
    }

    int GetAudioIndex(){
        return audio_index_;
    }

private:
    std::string url_;
    AVFormatContext* fmt_ctx_{nullptr};

    // 编码器
    AVCodecContext* video_codec_ctx_{nullptr};
    AVCodecContext* audio_codec_ctx_{nullptr};

    // 流
    AVStream* video_stream_{nullptr};
    AVStream* audio_stream_{nullptr};

    // index
    int video_index_{-1};
    int audio_index_{-1};
    
    bool de_inited_{false};
};

#endif // MUXER_H